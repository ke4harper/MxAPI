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
	* Neither the name of ABB, Inc nor the names of its contributors may be
	  used to endorse or promote products derived from this software without
	  specific prior written permission.

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
