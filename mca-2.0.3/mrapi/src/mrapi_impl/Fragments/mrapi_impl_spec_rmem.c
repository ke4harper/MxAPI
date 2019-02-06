/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/***************************************************************************
  Function: mrapi_impl_valid_rmem_id
  
  Description:  
  
  Parameters: 
  
  Returns: boolean indicating success or failure
  
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_valid_rmem_id (mrapi_rmem_id_t rmem_id) 
  {
    return MRAPI_TRUE;
  }
  
  /***************************************************************************
  Function: mrapi_impl_valid_atype
    
  Description:  
    
  Parameters: 
    
  Returns: boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_valid_atype (mrapi_rmem_atype_t access_type) 
  {
    if (access_type == MRAPI_RMEM_DUMMY) {
      return MRAPI_TRUE;
    } else {
      return MRAPI_FALSE;
    }
  }

  /***************************************************************************
  Function: mrapi_impl_rmem_attached
    
  Description:  
    
  Parameters: 
    
  Returns: boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rmem_attached (mrapi_rmem_hndl_t rmem) 
  {
    uint16_t r;
    uint32_t n = 0;
    uint32_t d = 0;
    mrapi_boolean_t rc = MRAPI_FALSE;
    mrapi_node_t node_id;
    mrapi_domain_t domain_id;

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));

    /* make sure we recognize the caller */
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));

    if (mrapi_impl_decode_hndl(rmem,&r) && 
        mrapi_db->rmems[r].valid && 
        ( mrapi_db->rmems[r].nodes[n]==1) ) {
      rc = MRAPI_TRUE;
    }
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));

    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_rmem_exists
    
  Description:  
    
  Parameters: 
    
  Returns: boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rmem_exists (mrapi_rmem_id_t rmem_id) 
  {
    
    mrapi_boolean_t rc = MRAPI_FALSE;
    int r;
    
    /* lock the database */
   mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
    mrapi_dprintf(1,"mrapi_impl_rmem_exists(%d);",rmem_id);
    
    for (r=0; r < MRAPI_MAX_RMEMS; r++) {
      if (mrapi_db->rmems[r].valid && mrapi_db->rmems[r].key == rmem_id) {
        /* *mrapi_status = MRAPI_EXISTS; */
        rc = MRAPI_TRUE;
        break;          
      }
    }
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));
    
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_rmem_create
    
  Description:  
    
  Parameters: 
    
  Returns: boolean indicating success or failure
    
  ***************************************************************************/
  void mrapi_impl_rmem_create(mrapi_rmem_hndl_t* rmem,
                                         mrapi_rmem_id_t rmem_id,
                                         const void* mem,
                                         mrapi_rmem_atype_t access_type,
                                         const mrapi_rmem_attributes_t* attributes,
                                         mrapi_uint_t size,
                                         mrapi_status_t* status) 
  {
    
    uint32_t n = 0;
    uint32_t d = 0;
    uint32_t r = 0;
    mrapi_boolean_t rc = MRAPI_TRUE;
    mca_node_t node_id;
    mca_domain_t domain_id;
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
    mrapi_dprintf(1,"mrapi_impl_rmem_create(&rmem,0x%x,%d,&attrs);",rmem_id,size);
   
    *status = MRAPI_SUCCESS;
 
    /* make sure we recognize the caller */
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));
    
    /* make sure we have room in the database */
    if  (mrapi_db->num_rmems == MRAPI_MAX_RMEMS){
      *status = MRAPI_ERR_MEM_LIMIT;
      rc = MRAPI_FALSE; 
    } else {
      /* make sure the remote memory doesn't already exist */
      for (r=0; r < MRAPI_MAX_RMEMS; r++) {
        if (mrapi_db->rmems[r].valid && mrapi_db->rmems[r].key == rmem_id) {
          *status = MRAPI_ERR_RMEM_EXISTS; 
          rc = MRAPI_FALSE;
          break;          
        }
      }
    }
    
    if (rc) {
      /* update our database */
      for (r = 0; r < MRAPI_MAX_RMEMS; r++) {
        if (mrapi_db->rmems[r].valid == MRAPI_FALSE) {
          mrapi_dprintf(1,"adding rmem: id=%u to dindex=%u nindex=%u sindex=%u \n",
                        rmem_id,d,n,r);
          mrapi_db->rmems[r].key = rmem_id; /* the shared key passed in on get/create */
          mrapi_db->rmems[r].valid = MRAPI_TRUE;
          mrapi_db->rmems[r].addr = mem;
          mrapi_db->rmems[r].size = size;
          mrapi_db->rmems[r].access_type = access_type;
          /* fill in the attributes */
          if (attributes != NULL) {
            /* set the user-specified attributes */
            mrapi_db->rmems[r].attributes.ext_error_checking = attributes->ext_error_checking;
            mrapi_db->rmems[r].attributes.shared_across_domains = attributes->shared_across_domains;
          } else {
            /* set the default attributes */
            mrapi_db->rmems[r].attributes.ext_error_checking = MRAPI_FALSE;
            mrapi_db->rmems[r].attributes.shared_across_domains = MRAPI_TRUE;
          }
          mrapi_db->num_rmems++;
          break;
        }
      }
    }
    
    if (!rc) {
      r = MRAPI_MAX_RMEMS;
    }
    
    *rmem = mrapi_impl_encode_hndl(r);
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));
    
  }
  
  /***************************************************************************
  Function: mrapi_impl_rmem_init_attributes
    
  Description:
    
  Parameters:
    
  Returns:  boolean 
                                                                                                                                                                             
  ***************************************************************************/
  void mrapi_impl_rmem_init_attributes(mrapi_rmem_attributes_t* attributes) 
  {
    attributes->ext_error_checking = MRAPI_FALSE;
    attributes->shared_across_domains = MRAPI_TRUE;
  }
  
  /***************************************************************************
  Function: mrapi_impl_rmem_set_attribute
    
  Description:
    
  Parameters:
    
  Returns:  boolean 
  ***********************************************************************/
  void mrapi_impl_rmem_set_attribute (mrapi_rmem_attributes_t* attributes, 
                                      mrapi_uint_t attribute_num, 
                                      const void* attribute,
                                      size_t attr_size, 
                                      mrapi_status_t* status)
  {  

    switch(attribute_num) {
    case (MRAPI_ERROR_EXT): 
      if (attr_size != sizeof(attributes->ext_error_checking)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy(&attributes->ext_error_checking,attribute,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case (MRAPI_DOMAIN_SHARED): 
      if (attr_size != sizeof(attributes->shared_across_domains)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy(&attributes->shared_across_domains,attribute,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    default:
      *status = MRAPI_ERR_ATTR_NUM;
    };

  }
  
  /***************************************************************************
  Function: mrapi_impl_rmem_get_attribute
    
  Description:
    
  Parameters:
                                                                                                                                                                           
  Returns:  boolean 
  ***********************************************************************/
  void mrapi_impl_rmem_get_attribute (mrapi_rmem_hndl_t rmem, 
                                      mrapi_uint_t attribute_num, 
                                      void* attribute,
                                      size_t attr_size, 
                                      mrapi_status_t* status)
  {
    uint16_t r;
    
    mrapi_assert (mrapi_impl_decode_hndl(rmem,&r));
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));

    switch(attribute_num) {
    case (MRAPI_ERROR_EXT): 
      if (attr_size != sizeof(mrapi_db->rmems[r].attributes.ext_error_checking)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->rmems[r].attributes.ext_error_checking,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case (MRAPI_DOMAIN_SHARED): 
      if (attr_size != sizeof(mrapi_db->rmems[r].attributes.shared_across_domains)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->rmems[r].attributes.shared_across_domains,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    default:
      *status = MRAPI_ERR_ATTR_NUM;
    };
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));
  }
  
  /***************************************************************************
  Function: mrapi_impl_rmem_get
    
  Description:
    
  Parameters: 
    
  Returns: 
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rmem_get(mrapi_rmem_hndl_t* rmem_hndl,uint32_t rmem_id) 
  {
    mrapi_boolean_t rc = MRAPI_FALSE;
    int r;
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
    mrapi_dprintf(1,"mrapi_impl_rmem_get (&rmem,0x%x /*rmem_id*/);",rmem_id);
    
    /* now look for the shared key (aka rmem_id) */
    for (r = 0; r < MRAPI_MAX_RMEMS; r++) {
      if (mrapi_db->rmems[r].key == rmem_id) {  
        rc = MRAPI_TRUE;
        break;
      }
    }
    
    /* encode the handle */
    /* note: if we didn't find it, the handle will be invalid bc the indices will be max*/
    *rmem_hndl = mrapi_impl_encode_hndl(r);
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_valid_rmem_hndl

  Description: Checks if the sem handle refers to a valid rmem segment

  Parameters:
  
  Returns: true/false indicating success or failure
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_valid_rmem_hndl(mrapi_rmem_hndl_t rmem) 
  {
    uint16_t r;
    mrapi_boolean_t rc = MRAPI_FALSE;
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
    if (mrapi_impl_decode_hndl(rmem,&r) && 
        (r < MRAPI_MAX_RMEMS) && 
        mrapi_db->rmems[r].valid) {
      rc = MRAPI_TRUE;
    }
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_rmem_attach
    
  Description: 
                                                                                                                                                                             
  Parameters: 
    
  Returns: 
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rmem_attach (mrapi_rmem_hndl_t rmem) 
  {
    uint16_t r;
    uint32_t n = 0;
    uint32_t d = 0;
    mrapi_node_t node_id;
    mrapi_domain_t d_id;

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
    
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&d_id,&d));
    mrapi_dprintf(1,"mrapi_impl_rmem_attach(0x%x);",rmem);
    mrapi_assert (mrapi_impl_decode_hndl(rmem,&r));
    
    if ( mrapi_db->rmems[r].nodes[n]==0) {
      mrapi_db->rmems[r].nodes[n]=1; /* log this node as a user */
      mrapi_db->rmems[r].refs++; /* bump the reference count */
    }
    
   /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));

    
    return MRAPI_TRUE;
  }
  
  /***************************************************************************
  Function: mrapi_impl_rmem_detach
    
  Description: 
    
  Parameters: 
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rmem_detach(mrapi_rmem_hndl_t rmem) 
  {  
    uint16_t r;
    uint32_t d = 0;
    uint32_t n = 0;
    mrapi_domain_t domain_id;
    mrapi_node_t node_id;
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
    
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));
    
    mrapi_dprintf(1,"mrapi_impl_rmem_detach(handle=0x%x);",rmem);
    
    mrapi_assert (mrapi_impl_decode_hndl(rmem,&r));
    
    mrapi_db->rmems[r].nodes[n]=0; /* remove this node as a user */
    mrapi_db->rmems[r].refs--; /* decrement the reference count */
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));

    return MRAPI_TRUE;  
  }
  
  /***************************************************************************
  Function: mrapi_delete_sharedMem
    
  Description: 
    
  Parameters: shmem_address - the address of the shared memory segment
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rmem_delete(mrapi_rmem_hndl_t rmem) 
  {  
    uint16_t r;
    mrapi_boolean_t rc = MRAPI_TRUE;
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
    
    mrapi_dprintf(1,"mrapi_impl_rmem_delete(0x%x);",rmem);
    
    mrapi_assert (mrapi_impl_decode_hndl(rmem,&r));
    
    mrapi_db->rmems[r].valid = MRAPI_FALSE;
    
    /* FIXME: WHAT IF OTHERS ARE STILL ATTACHED?*/
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));
    
    return rc;
  }
  
    /***************************************************************************
  Function: mrapi_impl_rmem_read_i
    
  Description: 
    
  Parameters: 
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rmem_read_i( mrapi_rmem_hndl_t rmem,
                                          mrapi_uint32_t rmem_offset,
                                          void* local_buf,
                                          mrapi_uint32_t local_offset,
                                          mrapi_uint32_t bytes_per_access,
                                          mrapi_uint32_t num_strides,
                                          mrapi_uint32_t rmem_stride,
                                          mrapi_uint32_t local_stride,
                                          mrapi_status_t* status,
                                          mrapi_request_t* request)
  {
    
    mrapi_boolean_t rc = MRAPI_FALSE;
    
    uint32_t r = mrapi_impl_setup_request();
    char status_buff[MRAPI_MAX_STATUS_SIZE];
    if (r < MRAPI_MAX_REQUESTS) {
    rc |= mrapi_impl_rmem_read(rmem,rmem_offset,local_buf,local_offset,bytes_per_access,
                               num_strides,rmem_stride,local_stride,status);
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
    // update the request
    mrapi_db->requests[r].completed = MRAPI_TRUE;
    mrapi_db->requests[r].status = *status;
    *status = MRAPI_SUCCESS; // we were successful setting up the request
    *request = mrapi_impl_encode_hndl(r);
    mrapi_dprintf(3,"mrapi_impl_rmem_read_i r=%d request=0x%x completed=%d status=%s",
                  r,
                  *request,
                  mrapi_db->requests[r].completed,
                  mrapi_impl_display_status(mrapi_db->requests[r].status,status_buff,sizeof(status_buff))); 
    /* unlock the database */
   mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));

  } else {
     *request = 0xffffffff;
     *status = MRAPI_ERR_REQUEST_LIMIT; 
      mrapi_dprintf(3,"mrapi_impl_rmem_read_i returning MRAPI_ERR_REQUEST_LIMIT");
  }
    
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_rmem_read
    
  Description: 
    
  Parameters: 
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rmem_read( mrapi_rmem_hndl_t rmem,
                                        mrapi_uint32_t rmem_offset,
                                        void* local_buf,
                                        mrapi_uint32_t local_offset,
                                        mrapi_uint32_t bytes_per_access,
                                        mrapi_uint32_t num_strides,
                                        mrapi_uint32_t rmem_stride,
                                        mrapi_uint32_t local_stride,
                                        mrapi_status_t* status)
  {
    uint16_t r,i;
    mrapi_boolean_t rc = MRAPI_TRUE;
    char* rmem_buf;
    const char* last_remote_access;
    const char* upper_bound;
    
    mrapi_dprintf(1,"mrapi_impl_rmem_read handle=(0x%x) offset=(0x%x)",
                  rmem,rmem_offset); 
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
    
    mrapi_assert (mrapi_impl_decode_hndl(rmem,&r));
    
#if (__unix__||__MINGW32__)
    last_remote_access = mrapi_db->rmems[r].addr + rmem_offset + (rmem_stride * num_strides );
    upper_bound = mrapi_db->rmems[r].addr +  mrapi_db->rmems[r].size - 1;
#else
    last_remote_access = (const char*)mrapi_db->rmems[r].addr + rmem_offset + (rmem_stride * num_strides );
    upper_bound = (const char*)mrapi_db->rmems[r].addr +  mrapi_db->rmems[r].size - 1;
#endif  /* !(__unix__||__MINGW32__) */
    
    if (mrapi_db->rmems[r].valid == MRAPI_FALSE) {
      rc = MRAPI_FALSE;
    } else if ( last_remote_access >  upper_bound) {
      // buffer overrun
      rc = MRAPI_FALSE;
    } else {
#if (__unix__||__MINGW32__)
      local_buf += local_offset;
      rmem_buf = (void*)mrapi_db->rmems[r].addr + rmem_offset;
#else
      (char*)local_buf += local_offset;
      rmem_buf = (char*)mrapi_db->rmems[r].addr + rmem_offset;
#endif  /* !(__unix__||__MINGW32__) */
      for (i = 0; i < num_strides; i++) {
        memcpy (local_buf,rmem_buf,bytes_per_access);
        rmem_buf += rmem_stride;
#if (__unix__||__MINGW32__)
        local_buf += local_stride;
#else
        (char*)local_buf += local_stride;
#endif  /* !(__unix__||__MINGW32__) */
      }
    }
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));
    
    return rc;                             
  }
  
  /***************************************************************************
  Function: mrapi_impl_rmem_write_i

  Description: 
  
  Parameters:
  
  Returns:  boolean indicating success or failure
  
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rmem_write_i( mrapi_rmem_hndl_t rmem,
                                           mrapi_uint32_t rmem_offset,
                                           const void* local_buf,
                                           mrapi_uint32_t local_offset,
                                           mrapi_uint32_t bytes_per_access,
                                           mrapi_uint32_t num_strides,
                                           mrapi_uint32_t rmem_stride,
                                           mrapi_uint32_t local_stride,
                                           mrapi_status_t* status,
                                           mrapi_request_t* request)
  {
    
    
    mrapi_boolean_t rc = MRAPI_FALSE;
    char status_buff [MRAPI_MAX_STATUS_SIZE];
    uint32_t r = mrapi_impl_setup_request();
    
    if (r < MRAPI_MAX_REQUESTS) {
      rc |= mrapi_impl_rmem_write(rmem,rmem_offset,local_buf,local_offset,bytes_per_access,
                                  num_strides,rmem_stride,local_stride,status);
      /* lock the database */
      mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
      // update the request
      mrapi_db->requests[r].completed = MRAPI_TRUE;
      mrapi_db->requests[r].status = *status;
      *status = MRAPI_SUCCESS; // we were successful setting up the request
      *request = mrapi_impl_encode_hndl(r);
      mrapi_dprintf(3,"mrapi_impl_rmem_write_i r=%d request=0x%x completed=%d status=%s",
                    r,
                    *request,
                    mrapi_db->requests[r].completed,
                    mrapi_impl_display_status(mrapi_db->requests[r].status,status_buff,sizeof(status_buff)));
      /* unlock the database */
      mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));
      
    } else {
      *request = 0xffffffff;
      *status = MRAPI_ERR_REQUEST_LIMIT;
    }
    
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_rmem_write

  Description: 
  
  Parameters:
  
  Returns:  boolean indicating success or failure
  
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rmem_write( mrapi_rmem_hndl_t rmem,
                                         mrapi_uint32_t rmem_offset,
                                         const void* local_buf,
                                         mrapi_uint32_t local_offset,
                                         mrapi_uint32_t bytes_per_access,
                                         mrapi_uint32_t num_strides,
                                         mrapi_uint32_t rmem_stride,
                                         mrapi_uint32_t local_stride,
                                         mrapi_status_t* status)
  {
    
    uint16_t r,i;
    mrapi_boolean_t rc = MRAPI_TRUE;
    char* rmem_buf;
    const char* last_remote_access;
    const char* upper_bound;
    
    mrapi_dprintf(1,"mrapi_impl_rmem_write handle=(0x%x) offset=(0x%x)",
                  rmem,rmem_offset);  
    
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(rmems_semid,0,MRAPI_TRUE));
    
    mrapi_assert (mrapi_impl_decode_hndl(rmem,&r));
#if (__unix__||__MINGW32__)
    last_remote_access = mrapi_db->rmems[r].addr + rmem_offset + (rmem_stride * num_strides ) + bytes_per_access;
    upper_bound = mrapi_db->rmems[r].addr +  mrapi_db->rmems[r].size;
#else
    last_remote_access = (const char*)mrapi_db->rmems[r].addr + rmem_offset + (rmem_stride * num_strides ) + bytes_per_access;
    upper_bound = (const char*)mrapi_db->rmems[r].addr +  mrapi_db->rmems[r].size;
#endif  /* !(__unix__||__MINGW32__) */
    
    if (mrapi_db->rmems[r].valid == MRAPI_FALSE) {
      rc = MRAPI_FALSE;
    } else if ( last_remote_access >  upper_bound) {
      // buffer overrun
      rc = MRAPI_FALSE;
    } else {
#if (__unix__||__MINGW32__)
      local_buf += local_offset;
      rmem_buf = (void*)mrapi_db->rmems[r].addr + rmem_offset;
#else
      (char*)local_buf += local_offset;
      rmem_buf = (char*)mrapi_db->rmems[r].addr + rmem_offset;
#endif  /* !(__unix__||__MINGW32__) */
      for (i = 0; i < num_strides; i++) {
        memcpy (rmem_buf,local_buf,bytes_per_access);
        rmem_buf += rmem_stride;
#if (__unix__||__MINGW32__)
        local_buf += local_stride;
#else
        (char*)local_buf += local_stride;
#endif  /* !(__unix__||__MINGW32__) */
      }
    }
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(rmems_semid,0));
    
    return rc;  
  }
