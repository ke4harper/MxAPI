  /***************************************************************************
  NAME:mrapi_impl_signal_hndlr
  DESCRIPTION: The purpose of this function is to catch signals so that we
   can clean up our shared memory and semaphore resources cleanly.
  PARAMETERS: the signal
  RETURN VALUE: none
  ***************************************************************************/
  void mrapi_impl_signal_hndlr ( int sig )
  {
    struct sigaction old_action,new_action;

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
    sprintf(sigbuff,"SIGNAL: mrapi received signal[%d] pid=%d tid=%s dindex=%d nindex=%d mrapi_db=%p",
            sig,mrapi_pid,mca_print_tid(mrapi_tid),mrapi_dindex, mrapi_nindex,mrapi_db);
    psignal(sig,sigbuff);

#else
    printf("mrapi received signal: %d\n",sig);
#endif  /* !__linux */

    switch(sig) {
    case SIGSEGV:
    case SIGABRT:
      /* Release global semaphore if possible */
      mrapi_impl_access_database_post(semid,0);
    }

    /* restore the old action */
    if (mrapi_db != NULL && mrapi_pid != (pid_t)-1) {
      mrapi_impl_access_database_pre(semid,0,MRAPI_FALSE);
      old_action = mrapi_db->domains[mrapi_dindex].nodes[mrapi_nindex].signals[sig];
      mrapi_impl_access_database_post(semid,0);
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
      mrapi_dprintf(1, "MRAPI: Unable to look up node/domain info for this process, thus unable to unwind the signal handlers any further. Restoring the default handler\n");
      new_action.sa_handler = SIG_DFL;
      sigemptyset (&new_action.sa_mask);
      new_action.sa_flags = 0;
#if (__unix__)
      sigaction (sig, &new_action, NULL);
#else
      old_action.sa_handler = signal(sig,new_action.sa_handler);
#endif  /* !(__unix__) */
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

    /* clean up ipc resources */
    mrapi_impl_free_resources(MRAPI_TRUE);

    mca_unblock_signals();

    /* Now reraise the signal. */
    raise (sig);
  }
