  /***************************************************************************
  NAME: mcapi_trans_start
  DESCRIPTION: mark the node as running.
  PARAMETERS: none
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_start () {
    mcapi_node_state state;
    mrapi_status_t status;
    mrapi_atomic_barrier_t axb_nodes;
    mrapi_barrier_init(&axb_nodes,0,(mrapi_msg_t*)mcapi_db->domains[mcapi_dindex].nodes,
      MCA_MAX_NODES,sizeof(mcapi_node_entry),&mcapi_nindex,MCA_INFINITE,&status);
    /* set node as valid */
    mrapi_atomic_read(&axb_nodes,&mcapi_db->domains[mcapi_dindex].nodes[mcapi_nindex].state,&state,
        sizeof(mcapi_db->domains[mcapi_dindex].nodes[mcapi_nindex].state),&status);
    assert(MRAPI_SUCCESS == status);
    state.data.valid = MRAPI_TRUE;
    mrapi_atomic_xchg(&axb_nodes,&mcapi_db->domains[mcapi_dindex].nodes[mcapi_nindex].state,&state,NULL,
        sizeof(mcapi_db->domains[mcapi_dindex].nodes[mcapi_nindex].state),&status);
    assert(MRAPI_SUCCESS == status);
  }

  /***************************************************************************
  NAME: mcapi_trans_initialize
  DESCRIPTION: sets up the semaphore and shared memory if necessary
  PARAMETERS: none
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_initialize (mcapi_domain_t domain_id,
                                          mcapi_node_t node_id,
                                          const mcapi_node_attributes_t* node_attrs,
                                          mcapi_boolean_t start)
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
    mcapi_domain_state* dstate = NULL;
    mcapi_domain_state olddstate;
    mcapi_domain_state newdstate;
    mcapi_node_state* nstate = NULL;
    mcapi_node_state oldnstate;
    mcapi_node_state newnstate;
    mrapi_status_t mrapi_status;
    int key = 0;

    mrapi_atomic_barrier_t axb_domains;
    mrapi_atomic_barrier_t axb_nodes;

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

    if (!rc) {
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
      strcat_s(buff,sizeof(buff),"_mcapi");
#endif  /* (_WIN32||_WIN64) */
      key = 0;
      key =  mca_Crc32_ComputeBuf( key,buff,sizeof(buff));
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
        mrapi_atomic_xchg_ptr(NULL,(uintptr_t*)&mcapi_db,(uintptr_t)shm_addr,NULL,&mrapi_status);
        assert(MRAPI_SUCCESS == mrapi_status);
      }
      else if (transport_sm_create_shared_mem(&shm_addr,key,sizeof(mcapi_database))) {
        mcapi_dprintf(1," Requesting %lu bytes of shared memory using key=%x\n",sizeof(mcapi_database),key);
        mcapi_dprintf(1," using defaults: MCAPI_MAX_MSG_SIZE=%u, MCAPI_MAX_PKT_SIZE=%u, MCAPI_MAX_ENDPOINTS=%u, MCAPI_MAX_ATTRIBUTES=%u, MCAPI_MAX_CHANNELS=%u, MCA_MAX_NODES=%u, MCAPI_MAX_BUFFERS=%u, MCAPI_MAX_QUEUE_ELEMENTS=%u",
                      MCAPI_MAX_MSG_SIZE, MCAPI_MAX_PKT_SIZE, MCAPI_MAX_ENDPOINTS, MCAPI_MAX_ATTRIBUTES,
                      MCAPI_MAX_CHANNELS, MCA_MAX_NODES, MCAPI_MAX_BUFFERS,MCAPI_MAX_QUEUE_ELEMENTS);
        /* setup the database pointer */
        mrapi_status = MRAPI_SUCCESS;
        mrapi_atomic_cas_ptr(NULL,(uintptr_t*)&mcapi_db,(uintptr_t)shm_addr,(uintptr_t)NULL,NULL,&mrapi_status);
        if(MRAPI_SUCCESS != mrapi_status) {
          mrapi_shmem_detach(key,&status);
        }
        /* the memory is not zeroed for us, go through and set all nodes/domains to invalid */
        for (d=0; d<MCA_MAX_DOMAINS; d++) {
          dstate = &mcapi_db->domains[d].state;
          memset(&newdstate,0,sizeof(mcapi_db->domains[d].state));
          mrapi_atomic_xchg(NULL,dstate,&newdstate,NULL,sizeof(mcapi_db->domains[d].state),&mrapi_status);
          assert(MRAPI_SUCCESS == mrapi_status);
          for (n=0; n<MCA_MAX_NODES; n++) {
            nstate = &mcapi_db->domains[d].nodes[n].state;
            memset(&newnstate,0,sizeof(mcapi_node_state));
            mrapi_atomic_xchg(NULL,nstate,&newnstate,NULL,sizeof(mcapi_db->domains[d].nodes[n].state),&mrapi_status);
            assert(MRAPI_SUCCESS == mrapi_status);
          }
        }
      } else {
        mcapi_dprintf(1,"FAILED: Couldn't get and attach to shared memory (is the database too large?)");
        rc = MCAPI_FALSE;
      }

      /* Request database not in shared memory */
      mcapi_trans_init_indexed_array();	/* by etem */

      mcapi_dprintf(1," using MCAPI database in shared memory at %p\n",mcapi_db);
    }

    /*  add the node/domain to the database */
    if (rc) {
      /* first see if this domain already exists */
      mrapi_barrier_init(&axb_domains,0,(mrapi_msg_t*)mcapi_db->domains,
          MCA_MAX_DOMAINS,sizeof(mcapi_domain_entry),(unsigned*)&d,MCA_INFINITE,&mrapi_status);
      assert(MRAPI_SUCCESS == mrapi_status);
      for (d = 0; d < MCA_MAX_DOMAINS; d++) {
        mrapi_atomic_read(&axb_domains,&mcapi_db->domains[d].state,&olddstate,
            sizeof(mcapi_db->domains[d].state),&mrapi_status);
        assert(MRAPI_SUCCESS == mrapi_status);
        if (olddstate.data.domain_id == domain_id) {
          break;
        }
      }
      if (d == MCA_MAX_DOMAINS) {
        /* it didn't exist so find the first available entry */
        for (d = 0; d < MCA_MAX_DOMAINS; d++) {
          dstate = &mcapi_db->domains[d].state;
          mrapi_atomic_read(&axb_domains,dstate,&olddstate,sizeof(mcapi_db->domains[d].state),&mrapi_status);
          assert(MRAPI_SUCCESS == mrapi_status);
          newdstate = olddstate;
          olddstate.data.allocated = MCAPI_FALSE;
          newdstate.data.domain_id = domain_id;
          newdstate.data.allocated = MCAPI_TRUE;
          mrapi_status = MRAPI_SUCCESS;
          mrapi_atomic_cas(&axb_domains,dstate,&newdstate,&olddstate,NULL,
              sizeof(mcapi_db->domains[d].state),&mrapi_status);
          if(MRAPI_SUCCESS == mrapi_status) {
            break;
          }
        }
      }

      mrapi_barrier_init(&axb_nodes,0,(mrapi_msg_t*)mcapi_db->domains[d].nodes,
          MCA_MAX_NODES,sizeof(mcapi_node_entry),(unsigned*)&n,MCA_INFINITE,&mrapi_status);
      assert(MRAPI_SUCCESS == mrapi_status);

      if (d != MCA_MAX_DOMAINS) {
        /* now find an available node index...*/
        for (n = 0; n < MCA_MAX_NODES; n++) {
          mrapi_atomic_read(&axb_nodes,&mcapi_db->domains[d].nodes[n].state,&oldnstate,
              sizeof(mcapi_db->domains[d].nodes[n].state),&mrapi_status);
          assert(MRAPI_SUCCESS == mrapi_status);
          /* Even though initialized() is checked by mcapi, we have to check again here because
             initialized() and initalize() are  not atomic at the top layer */
          if (oldnstate.data.allocated &&
              oldnstate.data.node_num == node_id) {
            /* this node already exists for this domain */
            rc = MCAPI_FALSE;
            mcapi_dprintf(1,"This node (%d) already exists for this domain(%d)",node_id,domain_id);
            break;
          }
        }
        if (n == MCA_MAX_NODES) {
          /* it didn't exist so find the first available entry */
          for (n = 0; n < MCA_MAX_NODES; n++) {
            nstate = &mcapi_db->domains[d].nodes[n].state;
            mrapi_atomic_read(&axb_nodes,nstate,&oldnstate,sizeof(mcapi_db->domains[d].nodes[n].state),&mrapi_status);
            assert(MRAPI_SUCCESS == mrapi_status);
            newnstate = oldnstate;
            oldnstate.data.allocated = MCAPI_FALSE;
            newnstate.data.node_num = node_id;
            newnstate.data.allocated = MCAPI_TRUE;
            mrapi_status = MRAPI_SUCCESS;
            mrapi_atomic_cas(&axb_nodes,nstate,&newnstate,&oldnstate,NULL,
                sizeof(mcapi_db->domains[d].nodes[n].state),&mrapi_status);
            if(MRAPI_SUCCESS == mrapi_status) {
              break;
            }
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
        dstate = &mcapi_db->domains[d].state;
        mrapi_atomic_read(&axb_domains,dstate,&newdstate,sizeof(mcapi_db->domains[d].state),&mrapi_status);
        assert(MRAPI_SUCCESS == mrapi_status);
        newdstate.data.valid = MCAPI_TRUE;
        mrapi_atomic_xchg(&axb_domains,dstate,&newdstate,NULL,sizeof(mcapi_db->domains[d].state),&mrapi_status);
        assert(MRAPI_SUCCESS == mrapi_status);
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
        mcapi_db->domains[d].nodes[n].pid = mcapi_pid;
        mcapi_db->domains[d].nodes[n].tid = mcapi_tid;
        nstate = &mcapi_db->domains[d].nodes[n].state;
        mrapi_atomic_read(&axb_nodes,nstate,&newnstate,sizeof(mcapi_db->domains[d].nodes[n].state),&mrapi_status);
        assert(MRAPI_SUCCESS == mrapi_status);
        newnstate.data.valid = MCAPI_FALSE;
        mrapi_atomic_xchg(&axb_nodes,nstate,&newnstate,NULL,sizeof(mcapi_db->domains[d].nodes[n].state),&mrapi_status);
        assert(MRAPI_SUCCESS == mrapi_status);
        mrapi_atomic_inc(&axb_domains,&mcapi_db->domains[d].num_nodes,NULL,sizeof(mcapi_db->domains[d].num_nodes),&mrapi_status);
        /* set the node attributes */
        if (node_attrs != NULL) {
          memcpy(&mcapi_db->domains[d].nodes[n].attributes,
                 node_attrs,
                 sizeof(mcapi_node_attributes_t));
        }
        /* initialize the attribute size for the only attribute we support */
        mcapi_db->domains[d].nodes[n].attributes.entries[MCAPI_NODE_ATTR_TYPE_REGULAR].bytes =
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

    /* clear rundown flag */
    if(rc) {
      mcapi_node_state state;
      mrapi_atomic_read(&axb_nodes,&mcapi_db->domains[d].nodes[n].state,&state,
          sizeof(mcapi_db->domains[d].nodes[n].state),&status);
      assert(MRAPI_SUCCESS == status);
      state.data.rundown = MCAPI_FALSE;
      mrapi_atomic_xchg(&axb_nodes,&mcapi_db->domains[d].nodes[n].state,&state,NULL,
          sizeof(mcapi_db->domains[d].nodes[n].state),&status);
      assert(MRAPI_SUCCESS == status);
    }

    mcapi_dprintf(1, "mcapi_trans_initialize complete.  domain=%u, node=%u added\n",
                  domain_id,node_id);

    if(rc &&
       start) {
      mcapi_trans_start();
    }

    return rc;
  }
