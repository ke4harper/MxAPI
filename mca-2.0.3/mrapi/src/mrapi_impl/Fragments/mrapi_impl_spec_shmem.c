 /***************************************************************************
  Function: mrapi_impl_shmem_attached
    
  Description:  
    
  Parameters: 
    
  Returns: boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_shmem_attached (mrapi_shmem_hndl_t shmem) {
    uint16_t s;
    uint32_t d = 0;
    uint32_t n = 0;
    mrapi_boolean_t rc = MRAPI_FALSE;
    mrapi_node_t node_id;
    mrapi_domain_t domain_id;

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(shmems_semid,0,MRAPI_TRUE));

    /* make sure we recognize the caller */
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));

    if (mrapi_impl_decode_hndl(shmem,&s) && mrapi_db->shmems[s].valid && 
        ( mrapi_db->domains[d].nodes[n].shmems[s]==1) ) {
      rc = MRAPI_TRUE;
    }
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(shmems_semid,0));

    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_shmem_exists
    
  Description:
    
  Parameters: shmemkey - the shared key for all users of this memory segment
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_shmem_exists(uint32_t shmemkey) 
  {
    
    mrapi_boolean_t rc = MRAPI_FALSE;
    unsigned s;
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(shmems_semid,0,MRAPI_TRUE));
    
    /* look for a valid shmem w/ the same key */
    for (s = 0; s < MRAPI_MAX_SHMEMS; s++) {
      if ((mrapi_db->shmems[s].valid == MRAPI_TRUE) &&
          (mrapi_db->shmems[s].key == shmemkey)) {
        rc = MRAPI_TRUE;
        break;
      }
    }
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(shmems_semid,0));
    
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_shmem_get
    
  Description:
    
  Parameters: shmemkey - the shared key for all users of this memory segment
              size - the desired size (in bytes)
              
  Returns: the address of the shared memory segment
  
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_shmem_get(mrapi_shmem_hndl_t* shmem_hndl,uint32_t key) 
  {
    uint32_t s;
    mrapi_boolean_t rc = MRAPI_FALSE;
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(shmems_semid,0,MRAPI_TRUE));
    mrapi_dprintf(1,"mrapi_impl_shmem_get (&shmem,0x%x /*shmem key*/);",key);
    
    /* now look for the shmemkey */
    for (s = 0; s < MRAPI_MAX_SHMEMS; s++) {
        if (MRAPI_TRUE == mrapi_db->shmems[s].valid &&
            mrapi_db->shmems[s].key == key) {
        rc = MRAPI_TRUE;
        break;
      }
    }
    
    /* encode the handle */
    /* note: if we didn't find it, the handle will be invalid bc the indices will be max*/
    *shmem_hndl = mrapi_impl_encode_hndl(s);
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(shmems_semid,0));
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_shmem_create
    
  Description:
    
  Parameters: shmemkey - the shared key for all users of this memory segment
              size - the desired size (in bytes)
              
  Returns: the address of the shared memory segment
              
  ***************************************************************************/
  void mrapi_impl_shmem_create(mrapi_shmem_hndl_t* shmem,
                                          uint32_t key,
                                          uint32_t size,
                                          const mrapi_shmem_attributes_t* attributes,
                                          mrapi_status_t* mrapi_status) 
  {    
    /* create shared memory */
    uint32_t n = 0;
    uint32_t d = 0;
    uint32_t s = 0;
    int id;
    mca_node_t node_id;
    mca_domain_t domain_id;
    mrapi_boolean_t rc = MRAPI_TRUE;
     
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(shmems_semid,0,MRAPI_TRUE));
    mrapi_dprintf(1,"mrapi_impl_shmem_create(&shmem,0x%x,%d,&attrs);",key,size);
   
    *mrapi_status = MRAPI_SUCCESS;
 
    /* make sure we recognize the caller */
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));
    
    /* make sure we have room in the database */
    if  (mrapi_db->num_shmems == MRAPI_MAX_SHMEMS){
      mrapi_dprintf(2,"mrapi_impl_shmem_create: error: num_shmems == MRAPI_MAX_SHMEMS");
      *mrapi_status = MRAPI_ERR_MEM_LIMIT; 
      rc = MRAPI_FALSE;
    } else {
      /* make sure the shared memory doesn't already exist */
      for (s=0; s < MRAPI_MAX_SHMEMS; s++) {
        if (mrapi_db->shmems[s].valid && mrapi_db->shmems[s].key == key) {
          mrapi_dprintf(2,"mrapi_impl_shmem_create: error: the shmem now exists");
          *mrapi_status = MRAPI_ERR_SHM_EXISTS; 
          rc = MRAPI_FALSE;
          break;          
        }
      }
    }
    
    if (rc) {
      /* sysvr4: try to create the shared memory */
      id = sys_shmem_create(key,size); 
      if (id == -1) {
        mrapi_dprintf(2,"mrapi_impl_shmem_create: error: OS failed to create shared memory");
        *mrapi_status = MRAPI_ERR_MEM_LIMIT;
        rc = MRAPI_FALSE;
      } 
    }
    
    if (rc) {
      /* update our database */
      for (s = 0; s < MRAPI_MAX_SHMEMS; s++) {
        if (mrapi_db->shmems[s].valid == MRAPI_FALSE) {
          memset(&mrapi_db->shmems[s],0,sizeof(mrapi_shmem_t));
          mrapi_dprintf(1,"adding shmem: id=%u to dindex=%u nindex=%u sindex=%u \n",id,d,n,s);
          mrapi_db->shmems[s].key = key;
          mrapi_db->shmems[s].id = id;
          mrapi_db->shmems[s].valid = MRAPI_TRUE;
          mrapi_db->shmems[s].addr = NULL; /* we'll fill this in when we attach */
          /* fill in the attributes */
          if (attributes != NULL) {
            /* set the user-specified attributes */
            mrapi_db->shmems[s].attributes.ext_error_checking = attributes->ext_error_checking;
            mrapi_db->shmems[s].attributes.shared_across_domains = attributes->shared_across_domains;
          } else {
            /* set the default attributes */
            mrapi_db->shmems[s].attributes.ext_error_checking = MRAPI_FALSE;
            mrapi_db->shmems[s].attributes.shared_across_domains = MRAPI_TRUE;
          }
          mrapi_db->num_shmems++;
          break;
        }
      }
    }

    if (!rc) {
      s = MRAPI_MAX_SHMEMS;
    }
    
    *shmem = mrapi_impl_encode_hndl(s);
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(shmems_semid,0));
  }
  
  /***************************************************************************
  Function: mrapi_impl_shmem_init_attributes
    
  Description:
    
  Parameters:
    
  Returns:  boolean 
    
  ***************************************************************************/
  void mrapi_impl_shmem_init_attributes(mrapi_shmem_attributes_t* attributes) 
  {
    attributes->ext_error_checking = MRAPI_FALSE;
    attributes->shared_across_domains = MRAPI_TRUE;
  }
  
  /***************************************************************************
  Function: mrapi_impl_shmem_set_attribute
    
  Description:
    
  Parameters:
    
  Returns:  boolean 
  ***********************************************************************/
  void mrapi_impl_shmem_set_attribute (mrapi_shmem_attributes_t* attributes, 
                                       mrapi_uint_t attribute_num, 
                                       const void* attribute,
                                       size_t attr_size, 
                                       mrapi_status_t* status)
  {  

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(shmems_semid,0,MRAPI_TRUE))

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

    case(MRAPI_SHMEM_RESOURCE): 
      if (attr_size != sizeof(attributes->resource)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy(&attributes->resource,attribute,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case(MRAPI_SHMEM_ADDRESS): 
      if (attr_size != sizeof(attributes->mem_addr)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy(&attributes->mem_addr,attribute,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case(MRAPI_SHMEM_SIZE): 
      if (attr_size != sizeof(attributes->mem_size)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy(&attributes->mem_size,attribute,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;

    default:
      *status = MRAPI_ERR_ATTR_NUM;
    };
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(shmems_semid,0))
  }
  
  /***************************************************************************
  Function: mrapi_impl_shmem_get_attribute
  
  Description:
  
  Parameters:
  
  Returns:  boolean 
  ***********************************************************************/
  void mrapi_impl_shmem_get_attribute (mrapi_shmem_hndl_t shmem, 
                                       mrapi_uint_t attribute_num, 
                                       void* attribute,
                                       size_t attr_size, 
                                       mrapi_status_t* status)
  {
    uint16_t s;
    
    mrapi_assert (mrapi_impl_decode_hndl(shmem,&s));
    
    switch(attribute_num) {
    case (MRAPI_ERROR_EXT): 
      if (attr_size != sizeof(mrapi_db->shmems[s].attributes.ext_error_checking)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->shmems[s].attributes.ext_error_checking,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case (MRAPI_DOMAIN_SHARED): 
      if (attr_size != sizeof(mrapi_db->shmems[s].attributes.shared_across_domains)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->shmems[s].attributes.shared_across_domains,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case(MRAPI_SHMEM_RESOURCE): 
      if (attr_size != sizeof(mrapi_db->shmems[s].attributes.resource)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->shmems[s].attributes.resource,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
  case(MRAPI_SHMEM_ADDRESS): 
      if (attr_size != sizeof(mrapi_db->shmems[s].attributes.mem_addr)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->shmems[s].attributes.mem_addr,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
  case(MRAPI_SHMEM_SIZE): 
      if (attr_size != sizeof(mrapi_db->shmems[s].attributes.mem_size)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->shmems[s].attributes.mem_size,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;

    default:
      *status = MRAPI_ERR_ATTR_NUM;
    };
  }
  
  /***************************************************************************
  Function: mrapi_impl_shmem_attach
    
  Description: attach to the shared memory segment
    
  Parameters: 
    
  Returns: The address of the shared memory segment or NULL if it fails.
    
  ***************************************************************************/
  void* mrapi_impl_shmem_attach (mrapi_shmem_hndl_t shmem) 
  {
    uint16_t s;
    uint32_t n = 0;
    uint32_t d = 0;
    mca_node_t node_id;
    mca_domain_t d_id;
    
    /* lock the database */
   mrapi_assert(mrapi_impl_access_database_pre(shmems_semid,0,MRAPI_TRUE));
    
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&d_id,&d)); 
    
    mrapi_dprintf(1,"mrapi_impl_shmem_attach(0x%x);",shmem);
    mrapi_assert (mrapi_impl_decode_hndl(shmem,&s));
    /* fill in the addr */
    mrapi_db->shmems[s].addr = 
      sys_shmem_attach(mrapi_db->shmems[s].id);

    if (mrapi_db->domains[d].nodes[n].shmems[s] == 0) {   
      mrapi_db->domains[d].nodes[n].shmems[s] = 1; /* log this node as a user */
      mrapi_db->shmems[s].refs++; /* bump the reference count */
    }
    
   /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(shmems_semid,0));
    
    return mrapi_db->shmems[s].addr;
  }
  
  /***************************************************************************
  Function: mrapi_delete_sharedMem
    
  Description: 
    
  Parameters: shmem_address - the address of the shared memory segment
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_shmem_delete(mrapi_shmem_hndl_t shmem) 
  {
    
    uint16_t s;
    mrapi_boolean_t rc = MRAPI_TRUE;
    
    /* lock the database */
   mrapi_assert(mrapi_impl_access_database_pre(shmems_semid,0,MRAPI_TRUE));
    
    mrapi_dprintf(1,"mrapi_impl_shmem_delete(0x%x);",shmem);
    
    mrapi_assert (mrapi_impl_decode_hndl(shmem,&s));
   
    /* We do not check the reference count, it's up to the user to not delete
    shared mem that other nodes are using.  The reference count is only used
    by finalize to know if the last user of the shared mem has called finalize and
    if so, then it will delete it. */
 
    /* note: we can't actually free it unless no-one is attached to it */ 
    rc = sys_shmem_delete(mrapi_db->shmems[s].id);
    if (rc) {
      mrapi_db->shmems[s].valid = MRAPI_FALSE;
      mrapi_db->num_shmems--;
    }
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(shmems_semid,0));
    
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_shmem_detach
   
  Description: 
   
  Parameters: shmem_address - the address of the shared memory segment
   
  Returns:  boolean indicating success or failure
   
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_shmem_detach(mrapi_shmem_hndl_t shmem) 
  { 
    uint16_t s;
    uint32_t d = 0;
    uint32_t n = 0;
    uint32_t i = 0, j = 0;
    mrapi_domain_t domain_id;
    mrapi_node_t node_id;
    mrapi_boolean_t last_man_standing_for_this_process = MRAPI_TRUE;
#if (__unix__||__MINGW32__)
    pid_t pid = getpid();
#else
    pid_t pid = (pid_t)GetCurrentProcessId();
#endif  /* !(__unix__||__MINGW32__) */
    
    mrapi_boolean_t rc = MRAPI_TRUE;
    /* lock the database */
   mrapi_assert(mrapi_impl_access_database_pre(shmems_semid,0,MRAPI_TRUE));
    
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));
    
    mrapi_dprintf(1,"mrapi_impl_shmem_detach(handle=0x%x);",shmem);
    
    mrapi_assert (mrapi_impl_decode_hndl(shmem,&s));
    
    mrapi_db->domains[d].nodes[n].shmems[s]=0; /* remove this node as a user */
    mrapi_db->shmems[s].refs--; /* decrement the reference count */

    /* detach system shared memory if this is the last node attached
     * for this process */
    for (i = 0; i < MRAPI_MAX_DOMAINS; i++) {
      for (j = 0; j < MRAPI_MAX_NODES; j++) {
        if(d == i && n == j) {
          continue;
        }
        if ((mrapi_db->domains[i].nodes[j].valid == MRAPI_TRUE) &&
            (mrapi_db->domains[i].nodes[j].pid == pid)) {
          last_man_standing_for_this_process = MRAPI_FALSE;
          break;
        }
      }
    }

    if (last_man_standing_for_this_process) {
      /* look up the id that corresponds to this address */
      rc = sys_shmem_detach(mrapi_db->shmems[s].addr);
    }
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(shmems_semid,0));
    return rc;
  }
