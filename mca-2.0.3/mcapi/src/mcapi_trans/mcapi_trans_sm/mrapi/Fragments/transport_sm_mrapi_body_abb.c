  mrapi_boolean_t sys_file_key(const char* pathname,int proj_id,int* key);

  /* the shared memory address */
  mrapi_shmem_hndl_t shm = 0;
  void* shm_addr;

  /* These functions serve as a wrapper for the resource API (in this case MRAPI). The
     function prototypes are defined in transport_sm.h */

  /* To better understand the layers of the software architecture, take a look at
     the design document in the docs directory. */


  /***************************************************************************
  NAME: transport_sm_initialize
  DESCRIPTION:
  PARAMETERS: none
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t transport_sm_initialize(mcapi_domain_t domain_id,
                                          mcapi_node_t node_id,
                                          uint32_t* lock_handle)
  {
    mcapi_status_t status;
    mrapi_info_t mrapi_version;
    mrapi_parameters_t parms = 0;
    int key = 0;
    mcapi_boolean_t rc = MCAPI_FALSE;
    char status_buff[MRAPI_MAX_STATUS_SIZE];
    /* We are using mrapi reader/writer locks */
    /* Eventually move to one lock per endpoint, for now we are just using one
       global lock */
    uint32_t num_readers = 1;
    mrapi_initialize(domain_id,node_id,parms,&mrapi_version,&status);
    if ((status == MRAPI_SUCCESS) || (status == MRAPI_ERR_NODE_INITIALIZED))  {
      /* create  */
      (void)sys_file_key(NULL,'c',&key);
      /* create it (it may already exist) */
      if (transport_sm_create_rwl(key,lock_handle,num_readers)) {
        rc = MCAPI_TRUE;
      }
    } else {
      fprintf(stderr,"mrapi_initialize failed status=%s\n",
              mrapi_display_status(status,status_buff,sizeof(status_buff)));
    }
    return rc;
  }

  /***************************************************************************
  NAME: transport_sm_finalize
  DESCRIPTION:
  PARAMETERS: none
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t transport_sm_finalize(mcapi_boolean_t last_man_standing,
                                        mcapi_boolean_t last_man_standing_for_this_process,
                                        mcapi_boolean_t finalize_mrapi,
                                        uint32_t handle)
  {
    mcapi_status_t status;
    mcapi_boolean_t rc = MCAPI_TRUE;
    char status_buff[MRAPI_MAX_STATUS_SIZE];
    mrapi_dprintf(1,"mcapi transport_sm_finalize: detaching from shared memory\n");
    mrapi_shmem_detach(shm,&status);
    if (status != MRAPI_SUCCESS) {
      fprintf(stderr,"ERROR: transport_sm_finalize mrapi_shmem_detach(shm handle=%08lx, status=%s) failed\n",
              (unsigned long)shm,mrapi_display_status(status,status_buff,sizeof(status_buff)));
      rc = MCAPI_FALSE;
    }

    if (last_man_standing) {
      mrapi_dprintf(1,"mcapi transport_sm_finalize: freeing lock and shared memory\n");
      mrapi_shmem_delete(shm,&status);
      if (status != MRAPI_SUCCESS) {
        fprintf(stderr,"ERROR: transport_sm_finalize mrapi_shmem_free(shm handle=%x) failed %s\n",
                (unsigned)shm,mrapi_display_status(status,status_buff,sizeof(status_buff)));
        rc = MCAPI_FALSE;
      }
      mrapi_rwl_lock (handle,MCAPI_TRUE/*exclusive*/,MRAPI_TIMEOUT_INFINITE /*timeout*/,&status);
      if (status != MRAPI_SUCCESS) {
        fprintf(stderr,"ERROR: transport_sm_finalize mrapi_rwl_lock failed  %s\n",
                mrapi_display_status(status,status_buff,sizeof(status_buff)));
        rc = MCAPI_FALSE;
      } else {
        mrapi_rwl_delete(handle,&status);
        if (status != MRAPI_SUCCESS) {
          fprintf(stderr,"ERROR: transport_sm_finalize mrapi_rwl_free(handle=%x ) failed %s\n",
                  handle,mrapi_display_status(status,status_buff,sizeof(status_buff)));
          rc = MCAPI_FALSE;
        }
      }
    }
    if (finalize_mrapi) {
      /* tell mrapi to finalize this node */
      mrapi_finalize (&status);
      if (status != MRAPI_SUCCESS) {
        fprintf(stderr,"ERROR: unable to finalize mrapi %s\n",
                mrapi_display_status(status,status_buff,sizeof(status_buff)));
        rc = MCAPI_FALSE;
      }
    }
    return rc;
  }

  /***************************************************************************
  NAME: transport_sm_get_shared_mem
  DESCRIPTION:
  PARAMETERS: none
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t  transport_sm_get_shared_mem(void** addr,
                                               uint32_t shmkey,
                                               uint32_t size)
  {
    mcapi_status_t status;
    mrapi_boolean_t rc = MCAPI_FALSE;
    char status_buff[MRAPI_MAX_STATUS_SIZE];
    mrapi_shmem_hndl_t shm_local = mrapi_shmem_get (shmkey,&status);
    if (status == MRAPI_SUCCESS) {
      shm = shm_local;
      /* attach to the shared memory */
      *addr = mrapi_shmem_attach(shm,&status);
      if (status == MRAPI_SUCCESS) {
        rc = MCAPI_TRUE;
      }
    }
    if (status != MRAPI_SUCCESS) {
      mcapi_dprintf(1,"transport_sm_get_shared_mem failed.  mrapi_status=%s",
              mrapi_display_status(status,status_buff,sizeof(status_buff)));
    }
    return rc;
  }


  /***************************************************************************
  NAME: transport_sm_create_shared_mem
  DESCRIPTION:
  PARAMETERS: none
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t  transport_sm_create_shared_mem(void** addr,
                                                  uint32_t shmkey
                                                  ,uint32_t size)
  {
    mcapi_status_t status;
    mrapi_shmem_hndl_t shm_local = 0;
    mrapi_boolean_t rc = MCAPI_FALSE;
    char status_buff[MRAPI_MAX_STATUS_SIZE];
    /* try to create the shared memory */
    shm_local = mrapi_shmem_create(shmkey,size,
                             NULL /* nodes list */,0 /* nodes list size */,
                             NULL /*attrs*/,0 /*attrs size*/,&status);
    if (status == MRAPI_SUCCESS) {
      /* attach to the shared memory */
      shm = shm_local;
      *addr = mrapi_shmem_attach(shm,&status);
      if (status != MRAPI_SUCCESS) {
        fprintf(stderr,"transport_sm_create_shared_mem failed.  mrapi_status=%s",
                mrapi_display_status(status,status_buff,sizeof(status_buff)));
      }
      else {
        rc = MCAPI_TRUE;
      }
    }
    return rc;
  }

  /***************************************************************************
  NAME: transport_sm_create_rwl
  DESCRIPTION:
  PARAMETERS: none
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t transport_sm_create_rwl(uint32_t key,
                                          uint32_t* handle,
                                          uint32_t num_readers)
  {
    mcapi_status_t status;
    mcapi_boolean_t rc = MCAPI_FALSE;
    mrapi_rwl_hndl_t s;
    char status_buff [MRAPI_MAX_STATUS_SIZE];
    s = mrapi_rwl_create(key,NULL/*attrs*/,num_readers /*shared_lock_limit*/,&status);
    if (status == MRAPI_ERR_RWL_EXISTS) {
      s = mrapi_rwl_get(key,&status);
    }
    if (status == MRAPI_SUCCESS) {
      *handle = (uint32_t)s;
      rc = MCAPI_TRUE;
    } else {
      fprintf(stderr,"transport_sm_create_rwl failed status=%s\n",
              mrapi_display_status(status,status_buff,sizeof(status_buff)));
    }
    return rc;
  }

  /***************************************************************************
  NAME: transport_sm_lock_rwl
  DESCRIPTION:
  PARAMETERS: none
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t transport_sm_lock_rwl(uint32_t handle,
                                        mcapi_boolean_t write)
  {
    mcapi_status_t status;
    //char status_buff[MRAPI_MAX_STATUS_SIZE];
    mrapi_rwl_mode_t mode = (write) ? MRAPI_RWL_WRITER : MRAPI_RWL_READER;
    mrapi_rwl_lock (handle,mode,MRAPI_TIMEOUT_INFINITE /*timeout*/,&status);
    if (status == MRAPI_SUCCESS) {
      return MCAPI_TRUE;
    } else {
      //fprintf(stderr,"transport_sm_lock mrapi_status=%s handle=%x\n",
      // mrapi_display_status(status,status_buff,sizeof(status_buff)),handle);
      fflush(stdout);
      return MCAPI_FALSE;
    }
  }


  /***************************************************************************
  NAME: transport_sm_unlock_rwl
  DESCRIPTION:
  PARAMETERS: none
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t transport_sm_unlock_rwl(uint32_t handle,
                                          mcapi_boolean_t write)
  {
    mcapi_status_t status;
    char status_buff[MRAPI_MAX_STATUS_SIZE];
    mrapi_rwl_mode_t mode = (write) ? MRAPI_RWL_WRITER : MRAPI_RWL_READER;
    mrapi_rwl_unlock(handle,mode,&status);
    if (status == MRAPI_SUCCESS) {
      return MCAPI_TRUE;
    } else {
      fprintf(stderr,"transport_sm_unlock mrapi_status=%s handle=%x\n",
              mrapi_display_status(status,status_buff,sizeof(status_buff)),
              (unsigned)handle);
      fflush(stdout);
      return MCAPI_FALSE;
    }
  }
