  /***************************************************************************
   Function: mrapi_impl_initialize

   Description:initializeZs the mrapi_impl layer (sets up the database and semaphore)

   Parameters:

   Returns: boolean indicating success or failure

***************************************************************************/
  mrapi_boolean_t mrapi_impl_initialize (mrapi_domain_t domain_id,
                                         mrapi_node_t node_id,
                                         mrapi_status_t* status)
  {
    /* associate this node w/ a pid,tid pair so that we can recognize the
       caller on later calls */

    int d = 0;
    int n = 0;
    mrapi_boolean_t rc = MRAPI_TRUE;
    int semid_local;
    int shmemid_local;
    int key;
	int db_key;
    int sems_key;
    int shmems_key;
    int rmems_key;
    int requests_key;
	mrapi_database* mrapi_db_local = NULL;
#if !(__MINGW32__||__unix__)
    mrapi_boolean_t created_shmem = MRAPI_FALSE;
#endif  /* !(__MINGW32__) */
#if (__unix__)
    register struct passwd *pw;
    register uid_t uid;
#endif  /* (__unix__) */
    char buff[128];
    mrapi_boolean_t use_uid = MRAPI_TRUE;

    mrapi_dprintf(1,"mrapi_impl_initialize (%d,%d);",domain_id,node_id);

	// Use atomic counter to prevent concurrent initialization or rundown by another thread
	// Counter is also tested in mrapi_impl_free_resources
    while (
#if !(__unix__||__MINGW32__)
           0 != InterlockedCompareExchange(&mrapi_initialize_ctr,1,0)) {
#else
           0 != __sync_lock_test_and_set(&mrapi_initialize_ctr,1,0)) {
#endif  /* (__unix__||__MINGW32__) */
      sys_os_usleep(10000);
    }

	// Do not initialize resources during rundown
    while(0 < mrapi_finalize_ctr) {
      sys_os_usleep(10000);
	}

    if (use_uid) {
#if (__unix__)
      uid = geteuid ();
      pw = getpwuid (uid);
      if (pw) {
        //printf ("pw=%s\n",pw->pw_name);
      } else {
        fprintf (stderr," cannot find username for UID %u\n", (unsigned) uid);
      }
      memset(buff,0,sizeof(buff));
      strcat(buff,pw->pw_name);
      strcat(buff,"_mrapi");
#elif (__MINGW32__)
      // Get username and convert to single byte string
      char szName[sizeof(buff)] = "";
      DWORD lpnSize = sizeof(szName);
      GetUserName(szName,&lpnSize);
      strcat(szName,"_mrapi");
#else
      // Get username and convert to single byte string
      wchar_t szwName[sizeof(buff)] = L"";
	  size_t returnValue = 0;
      DWORD lpnSize = sizeof(szwName);
      GetUserName(szwName,&lpnSize);
	  wcstombs_s(&returnValue,buff,sizeof(buff),szwName,lpnSize);
      strcat_s(buff,sizeof(buff),"_mrapi");
#endif  /* !(__unix__||__MINGW32__) */
      /* global key */
      key = 0;
      key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
#if (__unix__||__MINGW32__)
      /* db key */
      strcat(buff,"_db");
      db_key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
      /* sems key */
      strcat(buff,"_sems");
      sems_key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
      /* shmems key */
      strcat(buff,"_shmems");
      shmems_key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
      /* rmems key */
      strcat(buff,"_rmems");
      rmems_key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
      /* requests key */
      strcat(buff,"_requests");
      requests_key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
#else
      /* db key */
      strcat_s(buff,sizeof(buff),"_db");
      db_key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
      /* sems key */
      strcat_s(buff,sizeof(buff),"_sems");
      sems_key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
      /* shmems key */
      strcat_s(buff,sizeof(buff),"_shmems");
      shmems_key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
      /* rmems key */
      strcat_s(buff,sizeof(buff),"_rmems");
      rmems_key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
      /* requests key */
      strcat_s(buff,sizeof(buff),"_requests");
      requests_key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
#endif  /* !(__unix__||__MINGW32__) */
    } else {
      (void)sys_file_key(NULL,'c',&key);
      db_key = key + 10;
      sems_key = key + 20;
      shmems_key = key + 30;
      rmems_key = key + 40;
      requests_key = key + 50;
    }

    /* 1) setup the global database */
    /* get/create the shared memory database */
    shmemid_local = sys_shmem_get(db_key,sizeof(mrapi_database));
    if (shmemid_local == -1) {
#if !(__MINGW32__||__unix__)
      created_shmem = MRAPI_TRUE;
#endif  /* !(__MINGW32__) */
      shmemid_local = sys_shmem_create(db_key,sizeof(mrapi_database));
    }
    /* attach to the shared memory */
    if (shmemid_local != -1) {
      /* setup the global mrapi_db pointer and global shmemid */
      /* FIXME: IS IT SAFE TO WRITE THIS GLOBAL W/O A LOCK ??? */
      /* FIXME resolution: do not replace existing address if already attached */
      mrapi_db_local = (mrapi_database*)sys_shmem_attach(shmemid_local);
#if !(__unix__)
      if(NULL != InterlockedCompareExchangePointer(&mrapi_db,mrapi_db_local,NULL)) {
#else
      if((uintptr_t)NULL != __sync_val_compare_and_swap((uintptr_t*)&mrapi_db,(uintptr_t)NULL,(uintptr_t)mrapi_db_local)) {
#endif  /* (__unix__) */
        sys_shmem_detach(mrapi_db_local);
      }
    }
    if (mrapi_db == NULL) {
#if (__unix__||__MINGW32__)
      fprintf(stderr,"MRAPI_ERROR: Unable to attached to shared memory key= %x, errno=%s",
              key,strerror(errno));
#else
      char buf[80];
	  strerror_s(buf,80,errno);
      fprintf(stderr,"MRAPI_ERROR: Unable to attached to shared memory key= %x, errno=%s",
              key,buf);
#endif  /* !(__unix__||__MINGW32__) */
      rc = MRAPI_FALSE;
    }

    /* 2) create or get the semaphore and lock it */
    /* we loop here because of the following race condition:
       initialize                  finalize
       1) create/get sem           1) lock sem
       2) lock sem                 2) check db: any valid nodes?
       3) setup db & add self      3a)  no -> delete db & delete sem
       4) unlock sem               3b)  yes-> unlock sem

       finalize-1 can occur between initialize-1 and initialize-2 which will cause initilize-2
       to fail because the semaphore no longer exists.
    */
    if (!mrapi_impl_create_sys_semaphore(&semid_local,1/*num_locks*/,key,MRAPI_TRUE)) {
#if (__unix__||__MINGW32__)
      fprintf(stderr,"MRAPI ERROR: Unable to get the semaphore key= %x, errno=%s\n",
              key,strerror(errno));
#else
      char buf[80];
	  strerror_s(buf,80,errno);
      fprintf(stderr,"MRAPI ERROR: Unable to get the semaphore key= %x, errno=%s\n",
              key,buf);
#endif  /* !(__unix__||__MINGW32__) */
      rc = MRAPI_FALSE;
    }

    if (rc) {
      mrapi_dprintf(1,"mrapi_impl_initialize lock acquired, now attaching to shared memory and adding node to database");
      shmemid = shmemid_local;
      /* At this point we've managed to acquire and lock the semaphore ...*/
      /* NOTE: it's important to write to the globals only while
         we have the semaphore otherwise we introduce race conditions.  This
         is why we are using the local variable id until everything is set up.*/

      /* set the global semid */
      semid = semid_local;

      /* get or create our finer grained locks */
      /* in addition to a lock on the sems array, every lock (rwl,sem,mutex) has it's own
         database semaphore, this allows us to access different locks in parallel */
      if (use_global_only ||
          !mrapi_impl_create_sys_semaphore(&sems_semid,MRAPI_MAX_SEMS+1/*num_locks*/,sems_key,MRAPI_FALSE)) {
        sems_semid = semid;
      }
      if (use_global_only ||
          !mrapi_impl_create_sys_semaphore(&shmems_semid,1/*num_locks*/,shmems_key,MRAPI_FALSE)) {
        shmems_semid = semid;
      }
      if (use_global_only ||
          !mrapi_impl_create_sys_semaphore(&rmems_semid,1/*num_locks*/,rmems_key,MRAPI_FALSE)) {
        rmems_semid = semid;
      }
      if (use_global_only ||
          !mrapi_impl_create_sys_semaphore(&requests_semid,1/*num_locks*/,requests_key,MRAPI_FALSE)) {
        requests_semid = semid;
      }

      //printf("**** semid:%x sems:%x shmems:%x rmems:%x requests:%x\n",
             //key,sems_key,shmems_key,rmems_key,requests_key);

      /* 3) add the node/domain to the database */
      if (rc) {
        /* first see if this domain already exists */
        for (d = 0; d < MRAPI_MAX_DOMAINS; d++) {
          if (mrapi_db->domains[d].domain_id == domain_id) {
            break;
          }
        }
        if (d == MRAPI_MAX_DOMAINS) {
          /* it didn't exist so find the first available entry */
          for (d = 0; d < MRAPI_MAX_DOMAINS; d++) {
            if (mrapi_db->domains[d].valid == MRAPI_FALSE) {
              break;
            }
          }
        }
        if (d != MRAPI_MAX_DOMAINS) {
          /* now find an available node index...*/
          for (n = 0; n < MRAPI_MAX_NODES; n++) {
            /* Even though initialized() is checked by mrapi, we have to check again here because
               initialized() and initalize() are  not atomic at the top layer */
            if ((mrapi_db->domains[d].nodes[n].valid )&&
                (mrapi_db->domains[d].nodes[n].node_num == node_id)) {
              /* this node already exists for this domain */
              rc = MRAPI_FALSE;
              *status = MRAPI_ERR_NODE_INITFAILED;
              mrapi_dprintf(1,"This node (%d) already exists for this domain(%d)",node_id,domain_id);
              break;
            }
          }
          if (n == MRAPI_MAX_NODES) {
            /* it didn't exist so find the first available entry */
            for (n = 0; n < MRAPI_MAX_NODES; n++) {
            if (mrapi_db->domains[d].nodes[n].valid == MRAPI_FALSE)
              break;
            }
          }
        } else {
          /* we didn't find an available domain index */
          mrapi_dprintf(1,"You have hit MRAPI_MAX_DOMAINS, either use less domains or reconfigure with more domains");
          rc = MRAPI_FALSE;
        }
        if (n == MRAPI_MAX_NODES) {
          /* we didn't find an available node index */
          mrapi_dprintf(1,"You have hit MRAPI_MAX_NODES, either use less nodes or reconfigure with more nodes.");
          rc = MRAPI_FALSE;
        }
      }

      if (rc) {
        struct sigaction new_action, old_action;

        mrapi_dprintf(1,"adding domain_id:%x node_id:%x to d:%d n:%d",
                      domain_id,node_id,d,n);
        mrapi_assert (mrapi_db->domains[d].nodes[n].valid == MRAPI_FALSE);
        mrapi_db->domains[d].domain_id = domain_id;
        mrapi_db->domains[d].valid = MRAPI_TRUE;
        mrapi_db->domains[d].num_nodes++;
        mrapi_db->domains[d].nodes[n].valid = MRAPI_TRUE;
        mrapi_db->domains[d].nodes[n].node_num = node_id;

        /* Initialize metadata callback rollover */
        mrapi_db->rollover_index = 0;

        /* set our cached (thread-local-storage) identity */
#if !(__unix__||__MINGW32__)
        mrapi_pid = (pid_t)GetCurrentProcessId();
        mrapi_tid = (pthread_t)GetCurrentThreadId();
#else
        mrapi_pid = getpid();
        mrapi_tid = pthread_self();
#endif  /* (__unix__||__MINGW32__) */
        mrapi_nindex = n;
        mrapi_dindex = d;
        mrapi_domain_id = domain_id;
        mrapi_node_id = node_id;
        mrapi_db->domains[d].nodes[n].pid = mrapi_pid;
        mrapi_db->domains[d].nodes[n].tid = mrapi_tid;

        /* Set the resource tree pointer */
        resource_root = &chip;

        mrapi_dprintf(1,"registering signal handlers");
        /* register signal handlers so that we can still clean up resources
           if an interrupt occurs
           http://www.gnu.org/software/libtool/manual/libc/Sigaction-Function-Example.html
        */

        /* Register signal handler */

        /* Set up the structure to specify the new action. */
        new_action.sa_handler = mrapi_impl_signal_hndlr;
        sigemptyset (&new_action.sa_mask);
        new_action.sa_flags = 0;

#if (__unix__)
        sigaction (SIGINT, NULL, &old_action);
#else
        old_action.sa_handler = signal(SIGINT,SIG_GET);
#endif  /* !(__unix__) */
        mrapi_db->domains[d].nodes[n].signals[SIGINT] = old_action;
        if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
          sigaction (SIGINT, &new_action, NULL);
#else
          (void)signal(SIGINT,new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
        sigaction (SIGHUP, NULL, &old_action);
        mrapi_db->domains[d].nodes[n].signals[SIGHUP] = old_action;
        if (old_action.sa_handler != SIG_IGN)
          sigaction (SIGHUP, &new_action, NULL);
#endif  /* (__unix__) */

#if (__unix__)
        sigaction (SIGILL, NULL, &old_action);
#else
        old_action.sa_handler = signal(SIGILL,SIG_GET);
#endif  /* !(__unix__) */
        mrapi_db->domains[d].nodes[n].signals[SIGILL] = old_action;
        if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
          sigaction (SIGILL, &new_action, NULL);
#else
          (void)signal(SIGILL,new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
        sigaction (SIGSEGV, NULL, &old_action);
#else
        old_action.sa_handler = signal(SIGSEGV,SIG_GET);
#endif  /* !(__unix__) */
        mrapi_db->domains[d].nodes[n].signals[SIGSEGV] = old_action;
        if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
          sigaction (SIGSEGV, &new_action, NULL);
#else
          (void)signal(SIGSEGV,new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
        sigaction (SIGTERM, NULL, &old_action);
#else
        old_action.sa_handler = signal(SIGTERM,SIG_GET);
#endif  /* !(__unix__) */
        mrapi_db->domains[d].nodes[n].signals[SIGTERM] = old_action;
        if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
          sigaction (SIGTERM, &new_action, NULL);
#else
          (void)signal(SIGTERM,new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
        sigaction (SIGFPE, NULL, &old_action);
#else
        old_action.sa_handler = signal(SIGFPE,SIG_GET);
#endif  /* !(__unix__) */
        mrapi_db->domains[d].nodes[n].signals[SIGFPE] = old_action;
        if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
          sigaction (SIGFPE, &new_action, NULL);
#else
          (void)signal(SIGFPE,new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
        sigaction (SIGABRT, NULL, &old_action);
        mrapi_db->domains[d].nodes[n].signals[SIGABRT] = old_action;
        if (old_action.sa_handler != SIG_IGN)
          sigaction (SIGABRT, &new_action, NULL);
#endif  /* (__unix__) */
      }

      /* release the lock */
      mrapi_impl_access_database_post(semid_local,0);
    }

    // Initialization complete, decrement atomic counter
#if !(__unix__||__MINGW32__)
    InterlockedDecrement(&mrapi_initialize_ctr);
#else
    __sync_lock_release(&mrapi_initialize_ctr);
#endif  /* (__unix__||__MINGW32__) */

    return rc;
  }
