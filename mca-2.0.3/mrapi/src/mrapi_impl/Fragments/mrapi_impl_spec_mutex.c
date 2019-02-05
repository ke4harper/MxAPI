  /***************************************************************************
  Function: mrapi_impl_mutex_create
  
  Description:
  
  Parameters:
  
  Returns:  boolean indicating success or failure
  
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_mutex_create(mrapi_mutex_hndl_t* mutex,  
                                          mrapi_mutex_id_t mutex_id,
                                          const mrapi_mutex_attributes_t* attributes, 
                                          mrapi_status_t* mrapi_status)
  {
    uint16_t m;
    mrapi_boolean_t rc = MRAPI_FALSE;
    *mrapi_status = MRAPI_ERR_MUTEX_LIMIT; 
    
    /* lock the database (use global lock for get/create sem|rwl|mutex) */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));
    
    if (mrapi_impl_create_lock_locked(mutex,mutex_id,1,MUTEX,mrapi_status)) {
      mrapi_assert (mrapi_impl_decode_hndl(*mutex,&m));
      mrapi_db->sems[m].type = MUTEX;
      if (attributes != NULL) {
        /* set the user-specified attributes */
        mrapi_db->sems[m].attributes.recursive = attributes->recursive;
        mrapi_db->sems[m].attributes.ext_error_checking = attributes->ext_error_checking;
        mrapi_db->sems[m].attributes.shared_across_domains = attributes->shared_across_domains;
      } else {
        /* set the default attributes */
        mrapi_db->sems[m].attributes.recursive = MRAPI_FALSE;
        mrapi_db->sems[m].attributes.ext_error_checking = MRAPI_FALSE;
        mrapi_db->sems[m].attributes.shared_across_domains = MRAPI_TRUE;
      }
      rc = MRAPI_TRUE;
    }
    /* unlock the database (use global lock for get/create sem|rwl|mutex) */
    mrapi_assert(mrapi_impl_access_database_post(semid,0));
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_mutex_init_attributes

  Description:

  Parameters:

  Returns:  boolean

  ***************************************************************************/
  void mrapi_impl_mutex_init_attributes(mrapi_mutex_attributes_t* attributes) 
  {
    attributes->recursive = MRAPI_FALSE;
    attributes->ext_error_checking = MRAPI_FALSE;
    attributes->shared_across_domains = MRAPI_TRUE;
  }
  
  /***************************************************************************
  Function: mrapi_impl_mutex_set_attribute

  Description:

  Parameters:

  Returns:  boolean
  ***********************************************************************/
  void mrapi_impl_mutex_set_attribute (mrapi_mutex_attributes_t* attributes,
                                       mrapi_uint_t attribute_num, 
                                       const void* attribute,
                                       size_t attr_size, 
                                       mrapi_status_t* status)
  {
    
    switch(attribute_num) {
    case (MRAPI_MUTEX_RECURSIVE):
      if (attr_size != sizeof(attributes->recursive)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy(&attributes->recursive,(mrapi_boolean_t*)attribute,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case (MRAPI_ERROR_EXT):
      if (attr_size != sizeof(attributes->ext_error_checking)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy(&attributes->ext_error_checking,(mrapi_boolean_t*)attribute,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case (MRAPI_DOMAIN_SHARED):
      if (attr_size != sizeof(attributes->shared_across_domains)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy(&attributes->shared_across_domains,(mrapi_boolean_t*)attribute,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    default:
      *status = MRAPI_ERR_ATTR_NUM;
    };
  }

  /***************************************************************************
  Function: mrapi_impl_mutex_get_attribute

  Description:

  Parameters:

  Returns:  boolean
  ***********************************************************************/
  void mrapi_impl_mutex_get_attribute (mrapi_mutex_hndl_t mutex,
                                       mrapi_uint_t attribute_num, 
                                       void* attribute,
                                       size_t attr_size, 
                                       mrapi_status_t* status)
  {
    uint16_t m;
    mrapi_assert (mrapi_impl_decode_hndl(mutex,&m));
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(sems_semid,m,MRAPI_TRUE));
    
    switch(attribute_num) {
    case (MRAPI_MUTEX_RECURSIVE):
      if (attr_size != sizeof(mrapi_db->sems[m].attributes.recursive)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->sems[m].attributes.recursive,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case (MRAPI_ERROR_EXT):
      if (attr_size != sizeof(mrapi_db->sems[m].attributes.ext_error_checking)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->sems[m].attributes.ext_error_checking,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    case (MRAPI_DOMAIN_SHARED):
      if (attr_size != sizeof(mrapi_db->sems[m].attributes.shared_across_domains)) {
        *status = MRAPI_ERR_ATTR_SIZE;
      } else {
        memcpy((mrapi_boolean_t*)attribute,&mrapi_db->sems[m].attributes.shared_across_domains,attr_size);
        *status = MRAPI_SUCCESS;
      }
      break;
    default:
      *status = MRAPI_ERR_ATTR_NUM;
    };
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(sems_semid,m));
  }
  
  /***************************************************************************
  Function:  mrapi_impl_mutex_get
    
  Description:
    
  Parameters:
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_mutex_get (mrapi_mutex_hndl_t* mutex, 
                                        mrapi_sem_id_t key) 
  {
    return mrapi_impl_sem_get(mutex,key);
  }
  
  /***************************************************************************
  Function: mrapi_impl_mutex_delete
    
  Description:
    
  Parameters:
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_mutex_delete (mrapi_mutex_hndl_t mutex) 
  { 
    return mrapi_impl_sem_delete(mutex);
  }
  
  /***************************************************************************
  Function: mrapi_impl_mutex_lock
    
  Description:
    
  Parameters:
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_mutex_lock(mrapi_mutex_hndl_t mutex,
                                        mrapi_key_t* lock_key, 
                                        mrapi_timeout_t timeout, 
                                        mrapi_status_t* mrapi_status)
  {
    uint32_t n = 0;
    uint32_t d = 0;
    uint16_t m = 0;
    mrapi_boolean_t rc = MRAPI_FALSE;
    mca_node_t node_id;
    mca_domain_t domain_id;
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));
    
    mrapi_assert (mrapi_impl_decode_hndl(mutex,&m));
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(sems_semid,m,MRAPI_TRUE));
        
    if ( (mrapi_db->sems[m].locks[0].locked == MRAPI_TRUE) &&
         (mrapi_db->sems[m].locks[0].lock_holder_nindex == n) &&
         (mrapi_db->sems[m].locks[0].lock_holder_dindex == d)) {
      
      if (mrapi_db->sems[m].attributes.recursive == MRAPI_TRUE) {
        mrapi_db->sems[m].locks[0].lock_key++;
        *lock_key = mrapi_db->sems[m].locks[0].lock_key;
        rc=MRAPI_TRUE;
        mrapi_dprintf(1,"mrapi_impl_mutex_lock rc=TRUE (recursive) (0x%x,%d /*lock_key*/,%d /*timeout*/,&status);",
                      mutex,*lock_key,timeout);
      } else {
        mrapi_dprintf(1,"WARNING: attempting to lock a non-recursive mutex when you already have the lock!");
        /* unlock the database */
        mrapi_assert(mrapi_impl_access_database_post(sems_semid,m));
        return MRAPI_FALSE;
      }
    }
    
    /* unlock the database */
   mrapi_assert(mrapi_impl_access_database_post(sems_semid,m));
    
   if (!rc) {
     rc = mrapi_impl_sem_lock(mutex,1,timeout,mrapi_status);
     if (rc) {
       *lock_key = 0;
       mrapi_dprintf(1,"mrapi_impl_mutex_lock rc=TRUE (not recursive/first lock) (0x%x,%d /*lock_key*/,%d /*timeout*/,&status);",mutex,*lock_key,timeout);
     } else {
       mrapi_dprintf(1,"mrapi_impl_mutex_lock rc=FALSE (0x%x,%d /*lock_key*/,%d /*timeout*/,&status);",
                     mutex,*lock_key,timeout); 
     }
   }
    
   return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_mutex_unlock
                             
  Description:
              
  Parameters:
             
  Returns:  boolean indicating success or failure
             
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_mutex_unlock(mrapi_mutex_hndl_t mutex,
                                          const mrapi_key_t* lock_key,
                                          mrapi_status_t* mrapi_status) 
  { 
    uint32_t n = 0;
    uint32_t d = 0;
    uint16_t m;
    mrapi_boolean_t rc = MRAPI_FALSE;
    mca_node_t node_id;
    mca_domain_t domain_id;
    mrapi_boolean_t recursively_locked;
    
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));
    
    mrapi_assert (mrapi_impl_decode_hndl(mutex,&m));

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(sems_semid,m,MRAPI_TRUE));
        
    mrapi_dprintf(1,"mrapi_impl_mutex_unlock (0x%x,%d /*lock_key*/,&status);",mutex,*lock_key);
    
    if ((mrapi_db->sems[m].locks[0].lock_holder_nindex == n) &&
        (mrapi_db->sems[m].locks[0].lock_holder_dindex == d) && 
        (*lock_key == mrapi_db->sems[m].locks[0].lock_key) &&
        (*lock_key != 0) ) {
      
      mrapi_db->sems[m].locks[0].lock_key--;
      rc=MRAPI_TRUE;
    } 
    
    recursively_locked = mrapi_db->sems[m].locks[0].lock_key;
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(sems_semid,m));
    
    if ((!rc) && (!recursively_locked)) {
      rc =  mrapi_impl_release_lock(mutex,1,mrapi_status);
    }
    
    if ((!rc) && (recursively_locked)) { 
      *mrapi_status = MRAPI_ERR_MUTEX_LOCKORDER;
    }
    
    return rc;
  }
