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
	* Neither the name of ABB, Inc nor the names of its contributors may be
	  used to endorse or promote products derived from this software without
	  specific prior written permission.

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
  Function: mrapi_impl_rwl_create
  
  Description:
  
  Parameters:
  
  Returns:  boolean indicating success or failure
  
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rwl_create(mrapi_rwl_hndl_t* rwl,  
                                        mrapi_rwl_id_t rwl_id,
                                        const mrapi_rwl_attributes_t* attributes, 
                                        mrapi_uint32_t reader_lock_limit,
                                        mrapi_status_t* mrapi_status)
  {
    uint16_t r;
    mrapi_boolean_t rc = MRAPI_FALSE;
    *mrapi_status = MRAPI_ERR_RWL_LIMIT;

    /* lock the database (use global lock for get/create sem|rwl|mutex) */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));

    if ( mrapi_impl_create_lock_locked(rwl,rwl_id,reader_lock_limit,RWL,mrapi_status)) {
      mrapi_assert (mrapi_impl_decode_hndl(*rwl,&r));
      mrapi_db->sems[r].type = RWL;
      if (attributes != NULL) {
        /* set the user-specified attributes */
        mrapi_db->sems[r].attributes.ext_error_checking = attributes->ext_error_checking;
        mrapi_db->sems[r].attributes.shared_across_domains = attributes->shared_across_domains;
      } else {
        /* set the default attributes */
        mrapi_db->sems[r].attributes.ext_error_checking = MRAPI_FALSE;
        mrapi_db->sems[r].attributes.shared_across_domains = MRAPI_TRUE;
      }
      rc = MRAPI_TRUE;
    }
    /* unlock the database (use global lock for get/create sem|rwl|mutex)*/
    mrapi_assert(mrapi_impl_access_database_post(semid,0));
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_rwl_init_attributes
    
  Description:
    
  Parameters:
    
  Returns:  boolean 
    
  ***************************************************************************/
  void mrapi_impl_rwl_init_attributes(mrapi_rwl_attributes_t* attributes) 
  {
    attributes->ext_error_checking = MRAPI_FALSE;
    attributes->shared_across_domains = MRAPI_TRUE;
  }
  
  /***************************************************************************
  Function: mrapi_impl_rwl_set_attribute
    
  Description:
    
  Parameters:
    
  Returns:  boolean 
  ***********************************************************************/
  void mrapi_impl_rwl_set_attribute (mrapi_rwl_attributes_t* attributes, 
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
  Function: mrapi_impl_rwl_get_attribute
    
  Description:
    
  Parameters:
    
  Returns:  boolean 
  ***********************************************************************/
  void mrapi_impl_rwl_get_attribute (mrapi_rwl_hndl_t rwl, 
                                     mrapi_uint_t attribute_num, 
                                     void* attribute,
                                     size_t attr_size, 
                                     mrapi_status_t* status)
  {
    uint16_t r;
    
    mrapi_assert (mrapi_impl_decode_hndl(rwl,&r));
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(sems_semid,r,MRAPI_TRUE));

    switch(attribute_num) {
    case (MRAPI_ERROR_EXT): 
      if (attr_size != sizeof(mrapi_db->sems[r].attributes.ext_error_checking)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->sems[r].attributes.ext_error_checking,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case (MRAPI_DOMAIN_SHARED): 
      if (attr_size != sizeof(mrapi_db->sems[r].attributes.shared_across_domains)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->sems[r].attributes.shared_across_domains,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    default:
      *status = MRAPI_ERR_ATTR_NUM;
    };
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(sems_semid,r));
  }
  
  /***************************************************************************
  Function:  mrapi_impl_rwl_get
    
  Description:
    
  Parameters:
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rwl_get (mrapi_rwl_hndl_t* rwl, mrapi_sem_id_t key) 
  {
    return mrapi_impl_sem_get(rwl,key);
  }
  
  /***************************************************************************
  Function: mrapi_impl_rwl_delete
    
  Description:
    
  Parameters:
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rwl_delete (mrapi_rwl_hndl_t rwl) 
  { 
    return mrapi_impl_sem_delete(rwl);
  }
  
  /***************************************************************************
  Function: mrapi_impl_rwl_lock
    
  Description:
    
  Parameters:
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rwl_lock(mrapi_rwl_hndl_t rwl,
                                      mrapi_rwl_mode_t mode, 
                                      mrapi_timeout_t timeout, 
                                      mrapi_status_t* mrapi_status)
  {
    uint16_t s;
    int n;
    mrapi_boolean_t rc = MRAPI_FALSE;    
    mrapi_dprintf(1,"mrapi_impl_rwl_lock (0x%x,%d /*mode*/,%d /*timeout*/,&status);",
                  rwl,mode,timeout);
    
    if (!mrapi_impl_decode_hndl(rwl,&s)) {
      return MRAPI_FALSE;
    }
    
    if (mode == MRAPI_RWL_READER) {
      /* a reader (shared) lock is being requested... */
      rc= mrapi_impl_sem_lock(rwl,1,timeout,mrapi_status);
    } else {
      /* a writer (exclusive) lock is being requested...*/
      
      /* lock the database */
      mrapi_assert(mrapi_impl_access_database_pre(sems_semid,s,MRAPI_TRUE));
      
      n = mrapi_db->sems[s].shared_lock_limit;
      
      /* unlock the database */
      mrapi_assert(mrapi_impl_access_database_post(sems_semid,s));
      /* request all the locks */
      rc = mrapi_impl_sem_lock(rwl,n,timeout,mrapi_status);
    }
  return rc; 
  }
  
  /***************************************************************************
  Function: mrapi_impl_rwl_unlock
    
  Description:
    
  Parameters:
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_rwl_unlock(mrapi_rwl_hndl_t rwl,
                                        mrapi_rwl_mode_t mode,
                                        mrapi_status_t* mrapi_status) 
  { 
    uint16_t s;
    int n;
    
    mrapi_dprintf(1,"mrapi_impl_rwl_unlock (0x%x,%d /*mode*/,&status);",
                  rwl,mode);
    
    if (!mrapi_impl_decode_hndl(rwl,&s)) {
      return MRAPI_FALSE;
    }
    
    if (mode == MRAPI_RWL_READER) {
      /* a reader (shared) unlock is being requested...*/
      return mrapi_impl_sem_unlock(rwl,1,mrapi_status);
    } else {
      /* a writer (exclusive) unlock is being requested...*/
      /* lock the database */
      mrapi_assert(mrapi_impl_access_database_pre(sems_semid,s,MRAPI_TRUE));
      
      n = mrapi_db->sems[s].shared_lock_limit;
      
      /* unlock the database */
      mrapi_assert(mrapi_impl_access_database_post(sems_semid,s));
      
      /* release all the locks */
      return mrapi_impl_sem_unlock(rwl,n,mrapi_status);
    }
  }     
