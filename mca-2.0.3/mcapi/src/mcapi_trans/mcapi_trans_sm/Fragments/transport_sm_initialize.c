  /***************************************************************************
  NAME: mcapi_trans_initialize
  DESCRIPTION: sets up the semaphore and shared memory if necessary
  PARAMETERS: none
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_initialize (mcapi_domain_t domain_id,
                                          mcapi_node_t node_id,
                                          const mcapi_node_attributes_t* node_attrs)
  {

    /* I've re-written the initialize and finalize routines a few times.  This seems to work
       most of the time.  However, we do still have a race condition.  mtapi_test3 in the
       regression is currently turned off since it runs into the race condition sometimes.
       The race occurs when one thread is initializing while another is finalizing.  A prize
       to whomever can find the problem and get that test to reliably pass (the test does nothing
       - each thread/node just initializes and then finalizes).
    */

    mcapi_boolean_t rc = MCAPI_TRUE;
    uint32_t global_rwl_local = 0;
    void* shm_addr;
    int d = 0;
    int n = 0;
#if (__unix__)
    register struct passwd *pw;
    register uid_t uid;
#endif  /* (__unix__) */
    char buff[128];
    int i;
    mcapi_status_t status = MCAPI_TRUE;
    mcapi_boolean_t use_uid = MCAPI_TRUE;
    int key = 0;

	/* Use atomic counter to prevent concurrent initialization or rundown by another thread
	 * Counter is also tested in mcapi_trans_finalize
	 */
    while (
#if !(__unix__||__MINGW32__)
           0 != InterlockedCompareExchange(&mcapi_initialize_ctr,1,0)) {
      SleepEx(10,0);
#else
           0 != __sync_lock_test_and_set(&mcapi_initialize_ctr,1,0)) {
      usleep(10000);
#endif  /* (__unix__||__MINGW32__) */
    }

	/* Do not initialize resources during rundown
	 */
    while(0 < mcapi_finalize_ctr) {
#if (__unix__||__MINGW32__)
      usleep(10000);
#else
      SleepEx(10,0);
#endif  /* !(__unix__||__MINGW32__) */
	}

    /* initialize this node in the mrapi layer so that this pid/tid will be recognized */
    /* this will also create a semaphore for us to use */

    /* fixme: the mrapi transport should check and use uid if requested for creating the semaphore */
    if (transport_sm_initialize(domain_id,node_id,&global_rwl_local)) {
      global_rwl = global_rwl_local;
    }
    else {
      mcapi_dprintf(1,"ERROR: mcapi_transport_sm_initialize FAILED\n");
      rc = MCAPI_FALSE;
    }

    /* lock the database */
    if (rc && !mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE)) {
      rc = MCAPI_FALSE;
    }

    if (!rc) {
	  /* Initialization failed, decrement atomic counter
	   */
 #if !(__unix__||__MINGW32__)
	  InterlockedDecrement(&mcapi_initialize_ctr);
#else
      __sync_lock_release(&mcapi_initialize_ctr);
#endif  /* (__unix__||__MINGW32__) */
      return rc;
    }

    if (use_uid) {
#if (__unix__)
      uid = geteuid ();
      pw = getpwuid (uid);
      if (pw) {
        /*printf ("pw=%s\n",pw->pw_name);*/
      } else {
        fprintf (stderr," cannot find username for UID %u\n", (unsigned) uid);
      }
      memset(buff,0,sizeof(buff));
      strcat(buff,pw->pw_name);
      strcat(buff,"_mcapi");
#elif (__MINGW32__)
      /* Get username and convert to single byte string
       */
      char szName[sizeof(buff)] = "";
      DWORD lpnSize = sizeof(szName);
      GetUserName(szName,&lpnSize);
      strcat(szName,"_mcapi");
#else
      /* Get username and convert to single byte string
       */
      wchar_t szwName[sizeof(buff)] = L"";
	  size_t returnValue = 0;
      DWORD lpnSize = sizeof(szwName);
      GetUserName(szwName,&lpnSize);
	  wcstombs_s(&returnValue,buff,sizeof(buff),szwName,lpnSize);
      key = 0;
      key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
      strcat_s(buff,sizeof(buff),"_mcapi");
#endif  /* (_WIN32||_WIN64) */
      mcapi_dprintf(1,"using shared memory %s, key = 0x%x\n",buff,key);
    } else {
      (void)sys_file_key(NULL,'c',&key);
    }

    if (rc) {
      /* get / create the shared memory (it may already exist) */
      // mcapi_cached_domain = domain_id;

      if (transport_sm_get_shared_mem(&shm_addr,key,sizeof(mcapi_database))) {
        mcapi_dprintf(1,"Attaching to shared memory (it already existed)\n");
        /* setup the database pointer */
#if !(__unix__||__MINGW32__)
        InterlockedExchangePointer(&mcapi_db,shm_addr);
#else
        (void)__sync_val_compare_and_swap(&mcapi_db,mcapi_db,shm_addr);
#endif  /* (__unix__||__MINGW32__) */
      }
      else if (transport_sm_create_shared_mem(&shm_addr,key,sizeof(mcapi_database))) {
        mcapi_dprintf(1," Requesting %lu bytes of shared memory using key=%x\n",sizeof(mcapi_database),key);
        mcapi_dprintf(1," using defaults: MCAPI_MAX_MSG_SIZE=%u, MCAPI_MAX_PKT_SIZE=%u, MCAPI_MAX_ENDPOINTS=%u, MCAPI_MAX_ATTRIBUTES=%u, MCAPI_MAX_CHANNELS=%u, MCA_MAX_NODES=%u, MCAPI_MAX_BUFFERS=%u, MCAPI_MAX_QUEUE_ELEMENTS=%u",
                      MCAPI_MAX_MSG_SIZE, MCAPI_MAX_PKT_SIZE, MCAPI_MAX_ENDPOINTS, MCAPI_MAX_ATTRIBUTES,
                      MCAPI_MAX_CHANNELS, MCA_MAX_NODES, MCAPI_MAX_BUFFERS,MCAPI_MAX_QUEUE_ELEMENTS);
        /* setup the database pointer */
#if !(__unix__)
        if(NULL != InterlockedCompareExchangePointer(&mcapi_db,shm_addr,NULL)) {
#else
        if(NULL != __sync_val_compare_and_swap(&mcapi_db,mcapi_db,shm_addr)) {
#endif  /* (__unix__) */
          mrapi_shmem_detach(key,&status);
        }
        /* the memory is not zeroed for us, go through and set all nodes/domains to invalid */
        for (d=0; d<MCA_MAX_DOMAINS; d++) {
          mcapi_db->domains[d].valid = MCAPI_FALSE;
          for (n=0; n<MCA_MAX_NODES; n++) {
            mcapi_db->domains[d].nodes[n].valid = MCAPI_FALSE;
          }
        }
        mcapi_trans_init_indexed_array_have_lock();	/* by etem */
      } else {
        mcapi_dprintf(1,"FAILED: Couldn't get and attach to shared memory (is the database too large?)");
        rc = MCAPI_FALSE;
      }


      mcapi_dprintf(1," using MCAPI database in shared memory at %p\n",mcapi_db);
    }

    /*  add the node/domain to the database */
    if (rc) {
      /* first see if this domain already exists */
      for (d = 0; d < MCA_MAX_DOMAINS; d++) {
        if (mcapi_db->domains[d].domain_id == domain_id) {
          break;
        }
      }
      if (d == MCA_MAX_DOMAINS) {
        /* it didn't exist so find the first available entry */
        for (d = 0; d < MCA_MAX_DOMAINS; d++) {
          if (mcapi_db->domains[d].valid == MCAPI_FALSE) {
            break;
          }
        }
      }
      if (d != MCA_MAX_DOMAINS) {
        /* now find an available node index...*/
        for (n = 0; n < MCA_MAX_NODES; n++) {
          /* Even though initialized() is checked by mcapi, we have to check again here because
             initialized() and initalize() are  not atomic at the top layer */
          if ((mcapi_db->domains[d].nodes[n].valid )&&
              (mcapi_db->domains[d].nodes[n].node_num == node_id)) {
            /* this node already exists for this domain */
            rc = MCAPI_FALSE;
            mcapi_dprintf(1,"This node (%d) already exists for this domain(%d)",node_id,domain_id);
            break;
          }
        }
        if (n == MCA_MAX_NODES) {
          /* it didn't exist so find the first available entry */
          for (n = 0; n < MCA_MAX_NODES; n++) {
            if (mcapi_db->domains[d].nodes[n].valid == MCAPI_FALSE)
              break;
          }
        }
      } else {
        /* we didn't find an available domain index */
        mcapi_dprintf(1,"You have hit MCA_MAX_DOMAINS, either use less domains or reconfigure with more domains");
        rc = MCAPI_FALSE;
      }
      if (n == MCA_MAX_NODES) {
        /* we didn't find an available node index */
        mcapi_dprintf(1,"You have hit MCA_MAX_NODES, either use less nodes or reconfigure with more nodes.");
        rc = MCAPI_FALSE;
      }
    }

    if (rc) {
      struct sigaction new_action, old_action;

      if (n < MCA_MAX_NODES) {
        /* add the caller to the database*/
        /* set the domain */
        mcapi_db->domains[d].domain_id = domain_id;
        mcapi_db->domains[d].valid = MCAPI_TRUE;
        /* set the node */
#if !(__unix__||__MINGW32__)
        mcapi_pid = (pid_t)GetCurrentProcessId();
        mcapi_tid = (pthread_t)GetCurrentThreadId();
#else
        mcapi_pid = getpid();
        mcapi_tid = pthread_self();
#endif  /* (__unix__||__MINGW32__) */
        mcapi_nindex = n;
        mcapi_node_num = node_id;
        mcapi_domain_id = domain_id;
        mcapi_dindex = d;
        mcapi_db->domains[d].nodes[n].valid = MCAPI_TRUE;
        mcapi_db->domains[d].nodes[n].node_num = node_id;
        mcapi_db->domains[d].nodes[n].pid = mcapi_pid;
        mcapi_db->domains[d].nodes[n].tid = mcapi_tid;
        mcapi_db->domains[d].num_nodes++;
        /* set the node attributes */
        if (node_attrs != NULL) {
          memcpy(&mcapi_db->domains[d].nodes[n].attributes,
                 node_attrs,
                 sizeof(mcapi_node_attributes_t));
        }
        /* initialize the attribute size for the only attribute we support */
        mcapi_db->domains[d].nodes[n].attributes.entries[MCAPI_NODE_ATTR_TYPE_REGULAR].bytes=
          sizeof(mcapi_node_attr_type_t);

        for (i = 0; i < MCAPI_MAX_ENDPOINTS; i++) {
          /* zero out all the endpoints */
          memset (&mcapi_db->domains[d].nodes[n].node_d.endpoints[i],0,sizeof(endpoint_entry));
        }
      }


      mcapi_dprintf(1,"registering mcapi signal handlers\n");
      /* register signal handlers so that we can still clean up resources
         if an interrupt occurs
         http://www.gnu.org/software/libtool/manual/libc/Sigaction-Function-Example.html
      */

      /* Set up the structure to specify the new action. */
      new_action.sa_handler = mcapi_trans_signal_handler;
      sigemptyset (&new_action.sa_mask);
      new_action.sa_flags = 0;

#if (__unix__)
      sigaction (SIGINT, NULL, &old_action);
#else
      old_action.sa_handler = signal(SIGINT,SIG_GET);
#endif  /* !(__unix__) */
      mcapi_db->domains[d].nodes[n].signals[SIGINT] = old_action;
      if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
        sigaction (SIGINT, &new_action, NULL);
#else
        (void)signal(SIGINT,new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
      sigaction (SIGHUP, NULL, &old_action);
      mcapi_db->domains[d].nodes[n].signals[SIGHUP] = old_action;
      if (old_action.sa_handler != SIG_IGN)
        sigaction (SIGHUP, &new_action, NULL);
#endif  /* (__unix__) */

#if (__unix__)
      sigaction (SIGILL, NULL, &old_action);
#else
      old_action.sa_handler = signal(SIGILL,SIG_GET);
#endif  /* !(__unix__) */
      mcapi_db->domains[d].nodes[n].signals[SIGILL] = old_action;
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
      mcapi_db->domains[d].nodes[n].signals[SIGSEGV] = old_action;
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
      mcapi_db->domains[d].nodes[n].signals[SIGTERM] = old_action;
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
      mcapi_db->domains[d].nodes[n].signals[SIGFPE] = old_action;
      if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
        sigaction (SIGFPE, &new_action, NULL);
#else
        (void)signal(SIGFPE,new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
      sigaction (SIGABRT, NULL, &old_action);
      mcapi_db->domains[d].nodes[n].signals[SIGABRT] = old_action;
      if (old_action.sa_handler != SIG_IGN)
        sigaction (SIGABRT, &new_action, NULL);
#endif  /* (__unix__) */
    }

    mcapi_dprintf(1, "mcapi_trans_initialize complete.  domain=%u, node=%u added\n",
                  domain_id,node_id);

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));

	// Initialization complete, decrement atomic counter
#if !(__unix__||__MINGW32__)
	InterlockedDecrement(&mcapi_initialize_ctr);
#else
    __sync_lock_release(&mcapi_initialize_ctr);
#endif  /* (__unix__||__MINGW32__) */

    return rc;
  }
