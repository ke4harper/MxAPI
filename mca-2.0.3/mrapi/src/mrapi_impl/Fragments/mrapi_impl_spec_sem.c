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
  Function: mrapi_impl_sem_get
    
  Description: Gets the semaphore for the given semaphore key. 
    
  Parameters: 
    
  Returns:  boolean indicating success or failure 
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_sem_get (mrapi_sem_hndl_t* sem_hndl, 
                                      mrapi_sem_id_t key)
  {
    uint32_t s;
    mrapi_boolean_t rc = MRAPI_FALSE;
    uint32_t d = 0;
    uint32_t n = 0;
    mrapi_node_t node_id;
    mrapi_domain_t domain_id;
    
    /* lock the database (use global lock for get/create sem|rwl|mutex) */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));
    mrapi_dprintf(1,"mrapi_impl_sem_get (&sem,0x%x /*sem key*/);",key);
    
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));
    
    /* now look for the semkey */
    for (s = 0; s < MRAPI_MAX_SEMS; s++) {
      if (mrapi_db->sems[s].key == key) {  
        rc = MRAPI_TRUE;
        if (mrapi_db->domains[d].nodes[n].sems[s] == 0) {   
          mrapi_db->domains[d].nodes[n].sems[s] = 1; /* log this node as a user */
          mrapi_db->sems[s].refs++; /* bump the reference count */
        }
        break;
      }
    }
    
    /* encode the handle */
    /* note: if we didn't find it, the handle will be invalid bc the indices will be max*/
    *sem_hndl = mrapi_impl_encode_hndl(s);
    
    /* unlock the database (use global lock for get/create sem|rwl|mutex) */
    mrapi_assert(mrapi_impl_access_database_post(semid,0));
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_sem_create
    
  Description: Creates the semaphore for the given semaphore key. 
    
  Parameters: key - semaphore key
              members - the number of resources
              sem - the semaphore (to be filled in)                                                                                                                                                                           
  Returns:  boolean indicating success or failure  
              
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_sem_create(mrapi_sem_hndl_t* sem,  
                                        mrapi_sem_id_t key,
                                        const mrapi_sem_attributes_t* attributes, 
                                        mrapi_uint32_t shared_lock_limit,
                                        mrapi_status_t* mrapi_status) 
  {
    uint16_t s;
    mrapi_boolean_t rc = MRAPI_FALSE;
    *mrapi_status = MRAPI_ERR_SEM_LIMIT;

    /* lock the database (use global lock for get/create sem|rwl|mutex)*/
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));

    if (mrapi_impl_create_lock_locked(sem,key,shared_lock_limit,SEM,mrapi_status)) {
      mrapi_assert (mrapi_impl_decode_hndl(*sem,&s));
      mrapi_db->sems[s].type = SEM;
      if (attributes != NULL) {
        /* set the user-specified attributes */
        mrapi_db->sems[s].attributes.ext_error_checking = attributes->ext_error_checking;
        mrapi_db->sems[s].attributes.shared_across_domains = attributes->shared_across_domains;
      } else {
        /* set the default attributes */
        mrapi_db->sems[s].attributes.ext_error_checking = MRAPI_FALSE;
        mrapi_db->sems[s].attributes.shared_across_domains = MRAPI_TRUE;
      }
      rc = MRAPI_TRUE;
    }
    /* unlock the database (use global lock for get/create sem|rwl|mutex)*/
    mrapi_assert(mrapi_impl_access_database_post(semid,0));
    return rc;
  }
  
  /***************************************************************************
  Function: mrapi_impl_sem_init_attributes
  
  Description:
  
  Parameters:
  
  Returns:  boolean 
  
  ***************************************************************************/
  void mrapi_impl_sem_init_attributes(mrapi_sem_attributes_t* attributes) 
  {
    attributes->ext_error_checking = MRAPI_FALSE;
    attributes->shared_across_domains = MRAPI_TRUE;
  }
  
  /***************************************************************************
  Function: mrapi_impl_sem_set_attribute
    
  Description:
    
  Parameters:
    
  Returns:  boolean 
  ***********************************************************************/
  void mrapi_impl_sem_set_attribute (mrapi_sem_attributes_t* attributes, 
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
  Function: mrapi_impl_sem_get_attribute
    
  Description:
                                                                                                                                                                             
  Parameters:
    
  Returns:  boolean 
  ***********************************************************************/
  void mrapi_impl_sem_get_attribute (mrapi_sem_hndl_t sem, 
                                     mrapi_uint_t attribute_num, 
                                     void* attribute,
                                     size_t attr_size, 
                                     mrapi_status_t* status)
  {
    uint16_t m;
    
    mrapi_assert (mrapi_impl_decode_hndl(sem,&m));

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(sems_semid,m,MRAPI_TRUE));
    
    switch(attribute_num) {
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
  Function: mrapi_impl_create_lock_locked
  
  Description: Creates the semaphore for the given semaphore key. 
  
  Parameters: key - semaphore key
              members - the number of resources
              sem - the semaphore (to be filled in)                                                                                                                                                                           
  Returns:  boolean indicating success or failure  
              
  ***************************************************************************/
mrapi_boolean_t mrapi_impl_create_lock_locked(mrapi_sem_hndl_t* sem,  
                                       mrapi_sem_id_t key,
                                       mrapi_uint32_t shared_lock_limit,
                                       lock_type t,
                                       mrapi_status_t* mrapi_status)
  {
    int l = 0;
    int id = 0;
    uint32_t d = 0;
    uint32_t n = 0;
    uint32_t s = 0;
    mrapi_node_t node_id;
    mrapi_domain_t d_id;
    mrapi_boolean_t rc = MRAPI_FALSE;
    
    mrapi_dprintf(1,"mrapi_impl_sem_create (&sem,0x%x /*key*/,attrs,%d /*shared_lock_limit*/,&status);",
                  key,shared_lock_limit);
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&d_id,&d));
    
    /* make sure the semaphore doesn't already exist */
    /* Even though we checked for this at the mrapi layer, we have to check again here because
       the check and the create aren't atomic at the top layer. */
    for (s = 0; s < MRAPI_MAX_SEMS; s++) {
      /* make sure the semaphore doesn't already exist */
      /* Even though we checked for this at the mrapi layer, we have to check again here because
         the check and the create aren't atomic at the top layer. */
      if (mrapi_db->sems[s].valid && mrapi_db->sems[s].key == key) {
        if (t == RWL) { *mrapi_status = MRAPI_ERR_RWL_EXISTS; }
        else if (t == SEM) { *mrapi_status = MRAPI_ERR_SEM_EXISTS; }
        else if (t == MUTEX) { *mrapi_status = MRAPI_ERR_MUTEX_EXISTS; }
       mrapi_dprintf(1,"Unable to create mutex/sem/rwl because this key already exists key=%d",key);
        break;
      }
     }

    /* if we made it through the database without finding a semaphore with this key, then create the new semaphore */   
    if (s == MRAPI_MAX_SEMS) {
    /* update the database */
    for (s = 0; s < MRAPI_MAX_SEMS; s++) {
      if ((mrapi_db->sems[s].valid == MRAPI_FALSE) && (mrapi_db->sems[s].deleted == MRAPI_FALSE))  {
        mrapi_dprintf(1,"mrapi_impl_sem_create: Adding new semaphore set with %d shared_lock_limit (dindex=%d,nindex=%d,semindex=%d id=%d, key=%d",
                      shared_lock_limit,d,n,s,id,key);
        memset(&mrapi_db->sems[s],0,sizeof(mrapi_sem_t));
        mrapi_db->sems[s].id = id; /* not used */
        mrapi_db->sems[s].key = key;
        /* log this node as a user, this way we can decrease the ref count when this node calls finalize */
        mrapi_db->domains[d].nodes[n].sems[s]=1;
        mrapi_db->sems[s].refs++; /* bump the reference count */
        mrapi_db->sems[s].valid = MRAPI_TRUE;
        mrapi_db->sems[s].shared_lock_limit = shared_lock_limit;
        mrapi_db->sems[s].type = t;
        mrapi_db->num_sems++;
#if (__unix__||__MINGW32__)
        for (l=0; l < shared_lock_limit; l++) {
#else
        for (l=0; (mrapi_uint32_t)l < shared_lock_limit; l++) {
#endif  /* !(__unix__||__MINGW32__) */
          mrapi_db->sems[s].locks[l].valid = MRAPI_TRUE;
        }
        *sem = mrapi_impl_encode_hndl(s);
        mrapi_db->sems[s].handle = *sem;  
        rc = MRAPI_TRUE;
        break;
      } 
    } 
    }
 
    if (s == MRAPI_MAX_SEMS) {
      mrapi_dprintf(1,"Unable to create mutex/sem/rwl because there is no more room in the database.");
      mrapi_dprintf(1,"Suggestion, reconfigure with a larger MAX_SEM count or if deleted is a big number, disable extended error checking.\n");
    }
 
    return rc;
  }
  
  
  /***************************************************************************
  Function: mrapi_impl_sem_delete
    
  Description: remove the semaphore for the given semaphore id.
    
  Parameters: sem - semaphore
    
  Returns:  boolean indicating success or failure
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_sem_delete (mrapi_sem_hndl_t sem) 
  {    
    uint16_t s;
    mrapi_boolean_t rc = MRAPI_TRUE;
    uint32_t my_locks = 0;
    uint32_t l = 0;
    uint32_t n = 0;
    uint32_t d = 0;
    mca_node_t node_id;
    mca_domain_t domain_id;
    
    if (!mrapi_impl_decode_hndl(sem,&s)) {
      return MRAPI_FALSE;
    }

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(sems_semid,s,MRAPI_TRUE));
    
    mrapi_dprintf(1,"mrapi_impl_sem_delete (0x%x);",sem);
    
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));
    
    /* check that this node has an exclusive lock on this semaphore */
#if (__unix__||__MINGW32__)
    for (l = 0; l < mrapi_db->sems[s].shared_lock_limit; l++) {
#else
    for (l = 0; (int32_t)l < mrapi_db->sems[s].shared_lock_limit; l++) {
#endif  /* !(__unix__||__MINGW32__) */
      if ((mrapi_db->sems[s].locks[l].valid) &&
          (mrapi_db->sems[s].locks[l].locked == MRAPI_TRUE) &&
          (mrapi_db->sems[s].locks[l].lock_holder_nindex == n) && 
          (mrapi_db->sems[s].locks[l].lock_holder_dindex == d)) {
        my_locks++;
      }
    }
    
    if (my_locks != mrapi_db->sems[s].shared_lock_limit) {
      mrapi_dprintf(1,"sem not exclusively locked by caller shared_lock_limit=%d, my locks=%d",
                    mrapi_db->sems[s].shared_lock_limit,my_locks);
      /* *status=MRAPI_NOT_LOCKED; */
      rc = MRAPI_FALSE;
    }
    
    if ((rc) && (mrapi_db->sems[s].valid)) {
      mrapi_db->sems[s].valid = MRAPI_FALSE;
     
      /* We do not check the reference count, it's up to the user to not delete
         semaphores that other nodes are using.  The reference count is only used
         by finalize to know if the last user of a semaphore has called finalize and
         if so, then it will delete the semaphore. */
 
      /* If extended error checking is enabled, mark it as deleted.  This will prevent
         us from overwriting this entry.  This could be a problem if semaphores are coming and
         going in the user's app because we could run out of entries.*/
      if (mrapi_db->sems[s].attributes.ext_error_checking == MRAPI_TRUE) {
        mrapi_db->sems[s].deleted = MRAPI_TRUE;
      }
      
    }
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(sems_semid,s));
    return rc;
  }
  
  /***************************************************************************
  Function:mrapi_impl_yield_locked

  Description: releases the lock, attempts to yield, re-acquires the lock.

  Parameters: none

  Returns: none
  ***************************************************************************/
  void mrapi_impl_yield_locked (int which_semid,int member)
  {
    /* call this version of sched_yield when you have the lock */
    /* release the lock */
    mrapi_dprintf(4,"mrapi_impl_yield_locked");
    mrapi_assert(mrapi_impl_access_database_post(which_semid,member));
    sys_os_yield();
    /* re-acquire the lock */
    mrapi_assert(mrapi_impl_access_database_pre(which_semid,member,MRAPI_TRUE));
  }
  
  /***************************************************************************
  Function: mrapi_impl_sem_lock (blocking)
                                                                                                                                                                             
  Description: locks the given semaphore
  
  Parameters: sem - the semaphore 
  
  Returns:  boolean indicating success or failure
  
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_sem_lock(mrapi_sem_hndl_t sem, 
                                      int32_t num_locks,
                                      mrapi_timeout_t timeout, 
                                      mrapi_status_t* mrapi_status)
  {
    /* blocking version */
    mrapi_timeout_t time = 0;
    uint16_t s;
    mrapi_boolean_t rc = MRAPI_TRUE;
    
    if (!mrapi_impl_decode_hndl(sem,&s)) {
      return MRAPI_FALSE;
    } 

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(sems_semid,s,MRAPI_TRUE));
    
    mrapi_dprintf(2,"mrapi_impl_sem_lock (0x%x,%d /*num_locks*/,%d /*timeout*/,&status);",
                  sem,num_locks,timeout);
    
    while (1) {
      time++;
      if (mrapi_impl_acquire_lock_locked(sem,num_locks,mrapi_status) == num_locks) {
        *mrapi_status = MRAPI_SUCCESS;
        break;
      } 
      /* try to yield */
      mrapi_dprintf(6,"mrapi_impl_sem_lock: unable to get lock, attemping to yield...");
      /* we have the semaphore, so use this version of yield  */
      mrapi_impl_yield_locked(sems_semid,s);
      
      if ((timeout !=  MRAPI_TIMEOUT_INFINITE) && (time >= timeout)) {
        if (timeout != 1) {
            /* don't overwrite ELOCKED status if this is a trylock */
          *mrapi_status = MRAPI_TIMEOUT;
        }
        rc = MRAPI_FALSE;
        break;
      }
    }
  
    /* unlock the database */
    mrapi_assert (mrapi_impl_access_database_post(sems_semid,s));
    return rc;
  }
  
  
  /***************************************************************************
  Function: mrapi_impl_acquire_lock_locked
    
  Description: This function is used by both the blocking and non-blocking 
       lock functions.
       
  Parameters: 
       
  Returns: boolean indicating success or failure
       
  ***************************************************************************/
  int32_t mrapi_impl_acquire_lock_locked(mrapi_sem_hndl_t sem, 
                                        int32_t num_locks,
                                        mrapi_status_t* status)
  {
    
    /* The database should already  be locked ! */
    
    uint32_t d = 0;
    uint32_t n = 0;
    uint32_t l = 0;
    uint16_t s = 0;
    int32_t num_added = 0;
    mca_node_t node_id;
    mca_domain_t domain_id;
    
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));
    
    if (!mrapi_impl_decode_hndl(sem,&s)) {
      return MRAPI_FALSE;
    }
    mrapi_dprintf(2,"mrapi_impl_acquire_lock_locked sem=%x,dindex=%d,nindex=%d,semindex=%d id=%d num_locks=%d",
                  sem,d,n,s,mrapi_db->sems[s].id,num_locks);
    mrapi_dprintf(2," (mrapi_db->sems[s].shared_lock_limit(%d) mrapi_db->sems[s].num_locks(%d) num_locks(%d))",
                  mrapi_db->sems[s].shared_lock_limit,mrapi_db->sems[s].num_locks, num_locks);
    
    if ((mrapi_db->sems[s].shared_lock_limit - mrapi_db->sems[s].num_locks - num_locks ) >= 0) {
#if (__unix__||__MINGW32__)
      for (l = 0; l < mrapi_db->sems[s].shared_lock_limit; l++) {
#else
      for (l = 0; (int32_t)l < mrapi_db->sems[s].shared_lock_limit; l++) {
#endif  /* !(__unix__||__MINGW32__) */
        if ((mrapi_db->sems[s].locks[l].valid) &&
            (mrapi_db->sems[s].locks[l].locked == MRAPI_FALSE)) {
          mrapi_db->sems[s].locks[l].locked = MRAPI_TRUE;
          mrapi_db->sems[s].locks[l].lock_holder_nindex = n;
          mrapi_db->sems[s].locks[l].lock_holder_dindex = d;
          num_added++;
          mrapi_db->sems[s].num_locks++;
        }
        if (num_added == num_locks) { mrapi_dprintf(3,"got the lock(s)"); *status = MRAPI_SUCCESS; break; }
      }
    } else {
      mrapi_dprintf(3,"unable to get the lock");
      if ( mrapi_db->sems[s].type == RWL) { *status = MRAPI_ERR_RWL_LOCKED; }
      else if ( mrapi_db->sems[s].type == SEM) { *status = MRAPI_ERR_SEM_LOCKED; }
      else if ( mrapi_db->sems[s].type == MUTEX) { *status = MRAPI_ERR_MUTEX_LOCKED; }
    }
    
    return num_added;
  }
  
  /***************************************************************************
  Function: mrapi_sem_unlock
    
  Description: unlocks the given semaphore
    
  Parameters: sem - the semaphore
    
  Returns:   
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_sem_unlock(mrapi_sem_hndl_t sem,
                                        int32_t num_locks,
                                        mrapi_status_t* mrapi_status) 
  { 
    return mrapi_impl_release_lock(sem,num_locks,mrapi_status);
  }
  
  /***************************************************************************
  Function: mrapi_release_lock
    
  Description: unlocks the given semaphore
    
  Parameters: sem - the semaphore
    
  Returns:   
    
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_release_lock(mrapi_sem_hndl_t sem,
                                         int32_t num_locks, 
                                         mrapi_status_t* mrapi_status) 
  { 
    
    //  struct sembuf sem_unlock={ member, 1, 0};
    uint16_t s,l,r;
    uint32_t d = 0;
    uint32_t n = 0;
    uint32_t my_locks = 0;
    uint32_t num_removed = 0;
    mrapi_boolean_t rc = MRAPI_TRUE;
    mca_node_t node_id;
    mca_domain_t domain_id;
    
    if (!mrapi_impl_decode_hndl(sem,&s)) {
      return MRAPI_FALSE;
    }
    
    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(sems_semid,s,MRAPI_TRUE));
    
    mrapi_dprintf(2,"mrapi_impl_sem_unlock(%x);",sem);
    
    
    /* even though these conditions were checked at the mrapi level, we have to check again
       because they could have changed (it's not atomic) */
    mrapi_assert(mrapi_impl_whoami(&node_id,&n,&domain_id,&d));
    
    if (mrapi_db->sems[s].valid == MRAPI_FALSE){
      rc = MRAPI_FALSE;
      if ( mrapi_db->sems[s].type  == RWL) { *mrapi_status = MRAPI_ERR_RWL_INVALID; }
      else if ( mrapi_db->sems[s].type == SEM) { *mrapi_status = MRAPI_ERR_SEM_INVALID; }
      else if ( mrapi_db->sems[s].type == MUTEX) { *mrapi_status = MRAPI_ERR_MUTEX_INVALID; }
    } else {
      /* check that this node has num_locks to unlock */
      for (l = 0; l < mrapi_db->sems[s].shared_lock_limit; l++) {
        if ((mrapi_db->sems[s].locks[l].valid) &&
            (mrapi_db->sems[s].locks[l].locked == MRAPI_TRUE) &&
            (mrapi_db->sems[s].locks[l].lock_holder_nindex == n) && 
            (mrapi_db->sems[s].locks[l].lock_holder_dindex == d)) {
          my_locks++;
        }
      }
#if (__unix__||__MINGW32__)
      if (my_locks < num_locks) {
#else
      if (my_locks < (uint32_t)num_locks) {
#endif  /* !(__unix__||__MINGW32__) */
        rc = MRAPI_FALSE;
        if ( mrapi_db->sems[s].type == RWL) { *mrapi_status = MRAPI_ERR_RWL_NOTLOCKED; }
        else if ( mrapi_db->sems[s].type == SEM) { *mrapi_status = MRAPI_ERR_SEM_NOTLOCKED; }
        else if ( mrapi_db->sems[s].type == MUTEX) {
            *mrapi_status = MRAPI_ERR_MUTEX_NOTLOCKED;
        }
      } else {  
        /* update the lock array in our db */
        mrapi_db->sems[s].num_locks -= num_locks;
		r = num_locks;
        for (l = 0; l < mrapi_db->sems[s].shared_lock_limit; l++) {
          if ((mrapi_db->sems[s].locks[l].valid) &&
              (mrapi_db->sems[s].locks[l].locked == MRAPI_TRUE) &&
              (mrapi_db->sems[s].locks[l].lock_holder_nindex == n) && 
              (mrapi_db->sems[s].locks[l].lock_holder_dindex == d)) {
            mrapi_db->sems[s].locks[l].locked = MRAPI_FALSE;
            num_removed++;
            if(--r <= 0) {
              break;
            }
          }
        }
        mrapi_assert (num_removed == num_locks);
      }
    }
    
    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(sems_semid,s));
    return rc;
  }
