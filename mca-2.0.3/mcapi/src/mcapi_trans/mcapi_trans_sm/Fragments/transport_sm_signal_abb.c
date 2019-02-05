  /***************************************************************************
  NAME:mcapi_trans_signal_handler
  DESCRIPTION: The purpose of this function is to catch signals so that we
   can clean up our shared memory and sempaphore resources cleanly.
  PARAMETERS: the signal
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_signal_handler ( int sig )
  {

    uint32_t d,n;
    struct sigaction old_action, new_action;

    mcapi_boolean_t last_man_standing = MCAPI_TRUE;

#if !(__unix__)
    /* Ignore blocked signals */
    if(sigismember(siggetblocked(),sig)) {
      return;
    }
#endif  /* !(__unix__) */

    mca_block_signals();

    /* print info on which signal was caught */
#ifdef __linux
    char sigbuff[128];
    sprintf(sigbuff,"SIGNAL: mcapi received signal[%d] pid=%d tid=%s dindex=%d nindex=%d mcapi_db=%p",
            sig,mcapi_pid,mca_print_tid(mcapi_tid),mcapi_dindex, mcapi_nindex,mcapi_db);
    psignal(sig,sigbuff);
#else
    printf("mcapi received signal: %d\n",sig);
#endif  /* !__linux */

    if (mcapi_db != NULL && mcapi_pid != (pid_t)-1) {
      mcapi_node_state* state = &mcapi_db->domains[mcapi_dindex].nodes[mcapi_nindex].state;
      mcapi_node_state oldstate;
      mcapi_node_state newstate = *state;
      mrapi_status_t status;
      mrapi_atomic_barrier_t axb_nodes;

      mrapi_barrier_init(&axb_nodes,0,(mrapi_msg_t*)mcapi_db->domains[mcapi_dindex].nodes,
        MCA_MAX_NODES,sizeof(mcapi_node_entry),&mcapi_nindex,MCA_INFINITE,&status);
      assert(MRAPI_SUCCESS == status);

      /* mark myself as invalid */
      newstate.data.valid = MCAPI_FALSE;
      mrapi_atomic_xchg(&axb_nodes,state,&newstate,NULL,
          sizeof(mcapi_db->domains[mcapi_dindex].nodes[mcapi_nindex].state),&status);

      /* mark any other nodes that belong to this process as invalid */
      for (d = 0; d < MCA_MAX_DOMAINS; d++) {
        for (n = 0; n < MCA_MAX_NODES; n++) {
          if ( mcapi_db->domains[d].nodes[n].pid == mcapi_pid) {

            mrapi_barrier_init(&axb_nodes,0,(mrapi_msg_t*)mcapi_db->domains[d].nodes,
              MCA_MAX_NODES,sizeof(mcapi_node_entry),&n,MCA_INFINITE,&status);
            assert(MRAPI_SUCCESS == status);

            state = &mcapi_db->domains[d].nodes[n].state;
            oldstate = newstate = *state;
            oldstate.data.valid = MRAPI_TRUE;
            newstate.data.valid = MRAPI_FALSE;
            status = MRAPI_SUCCESS;
            mrapi_atomic_cas(&axb_nodes,state,&newstate,&oldstate,NULL,
                sizeof(mcapi_db->domains[d].nodes[n].state),&status);
            if(MRAPI_SUCCESS != status) {
              last_man_standing = MCAPI_FALSE;
            }
          }
        }
      }

      /* restore the old action */
      old_action = mcapi_db->domains[mcapi_dindex].nodes[mcapi_nindex].signals[sig];
#if (__unix__)
      sigaction (sig, &old_action, NULL);
#else
      (void)signal(sig,old_action.sa_handler);
#endif  /* (__unix__) */
    } else {
      /* Signal handler not running in context of calling
         thread, runs instead on a separate thread, e.g. Win32.
         TLS is not initialized for this thread.
      */
      fprintf(stderr, "MCAPI: Unable to look up node/domain info for this process, thus unable to unwind the signal handlers any further. Restoring the default handler\n");
      new_action.sa_handler = SIG_DFL;
      sigemptyset (&new_action.sa_mask);
      new_action.sa_flags = 0;
#if (__unix__)
      sigaction (sig, &new_action, NULL);
#else
      old_action.sa_handler = signal(sig,new_action.sa_handler);
#endif  /* !(__unix) */
    }

#if !(__unix__)
    if(SIG_DFL == new_action.sa_handler) {
      /* (winsig.c)
       * The current default action for all of the supported
       * signals is to terminate with an exit code of 3.
       */
      _exit(3);
    }
#endif  /* !(__unix__) */

    // unlock the database if we locked it
    if (locked) {
      mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE);
    }

    // clean up mcapi semaphore and shared memory
    transport_sm_finalize(last_man_standing,MCAPI_TRUE,MCAPI_FALSE,global_rwl);
    //mcapi_db = NULL;

    mca_unblock_signals();

    /* Now reraise the signal so that mrapi can do it's cleanup */
    raise (sig);
  }
