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

    mcapi_boolean_t locked = MCAPI_FALSE;
    mcapi_boolean_t last_man_standing = MCAPI_TRUE;

#if !(__unix__)
    /* Ignore blocked signals */
    if(sigismember(siggetblocked(),sig)) {
      return;
    }
#endif  /* !(__unix__) */

    mca_block_signals();

    /* try to lock the database */
    locked = mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE);

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
      /* mark myself as invalid */
      mcapi_db->domains[mcapi_dindex].nodes[mcapi_nindex].valid = MCAPI_FALSE;

      /* mark any other nodes that belong to this process as invalid */
      for (d = 0; d < MCA_MAX_DOMAINS; d++) {
        for (n = 0; n < MCA_MAX_NODES; n++) {
          if ( mcapi_db->domains[d].nodes[n].valid) {
            if  ( mcapi_db->domains[d].nodes[n].pid == mcapi_pid) {
              mcapi_db->domains[d].nodes[n].valid = MCAPI_FALSE;
            } else {
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
