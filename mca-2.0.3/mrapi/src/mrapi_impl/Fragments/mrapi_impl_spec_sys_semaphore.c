  /***************************************************************************
   Function: mrapi_impl_create_sys_semaphore
   
   Description: Create or get the semaphore corresponding to the key
   
   Parameters: 
   
   Returns: boolean indicating success or failure
   
***************************************************************************/
  mrapi_boolean_t mrapi_impl_create_sys_semaphore (int* id, 
                                                   int num_locks, 
                                                   int key, 
                                                   mrapi_boolean_t lock) 
  {
    unsigned max_tries = 0xffffffff;
    unsigned trycount = 0;
   
    while (trycount < max_tries) {
      trycount++;
      if ((sys_sem_create(key,num_locks,id) || sys_sem_get(key,num_locks,id))) {  
        if (!lock) { 
          return MRAPI_TRUE;
        }
        else {
          while (trycount < max_tries) {
            if (sys_sem_trylock(*id,0)) {
              return MRAPI_TRUE;
            }
            else {
              sys_os_yield();
            }
          }
        }
      }
    }
    return MRAPI_FALSE;
  }
