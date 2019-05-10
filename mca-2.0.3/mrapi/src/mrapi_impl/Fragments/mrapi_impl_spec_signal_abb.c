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
void mrapi_impl_signal_hndlr(int sig)
{
	struct sigaction old_action, new_action;

#if !(__unix__)
	/* Ignore blocked signals */
	if (sigismember(siggetblocked(), sig)) {
		return;
	}
#endif  /* !(__unix__) */

	mca_block_signals();

	/* print info on which signal was caught */
#if (__linux__)
	char sigbuff[128];
	sprintf(sigbuff, "SIGNAL: mrapi received signal[%d] pid=%d tid=%s dindex=%d nindex=%d mrapi_db=%p",
		sig, mrapi_pid, mca_print_tid(mrapi_tid), mrapi_dindex, mrapi_nindex, mrapi_db);
	psignal(sig, sigbuff);
#else
	printf("mrapi received signal: %d\n", sig);
#endif  /* (__linux__) */

	switch (sig) {
	case SIGSEGV:
	case SIGABRT:
	{
		/* Release global semaphore if possible */
		mrapi_impl_sem_ref_t ref = { semid, 0 };
		mrapi_impl_access_database_post(ref);
	}
	}

	/* restore the old action */
	if (mrapi_db != NULL && mrapi_pid != (pid_t)-1) {
		mrapi_impl_sem_ref_t ref = { semid, 0 };
		mrapi_impl_access_database_pre(ref, MRAPI_FALSE);
		old_action = mrapi_db->domains[mrapi_dindex].nodes[mrapi_nindex].signals[sig];
		mrapi_impl_access_database_post(ref);
#if (__unix__)
		sigaction(sig, &old_action, NULL);
#else
		(void)signal(sig, old_action.sa_handler);
#endif  /* (__unix__) */
	}
	else {
		/* Signal handler not running in context of calling
		   thread, runs instead on a separate thread, e.g. Win32.
		   TLS is not initialized for this thread.
		*/
		mrapi_dprintf(1, "MRAPI: Unable to look up node/domain info for this process, thus unable to unwind the signal handlers any further. Restoring the default handler\n");
		new_action.sa_handler = SIG_DFL;
		sigemptyset(&new_action.sa_mask);
		new_action.sa_flags = 0;
#if (__unix__)
		sigaction(sig, &new_action, NULL);
#else
		old_action.sa_handler = signal(sig, new_action.sa_handler);
#endif  /* !(__unix__) */
	}

#if !(__unix__)
	if (SIG_DFL == new_action.sa_handler) {
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
	raise(sig);
}

/***************************************************************************
NAME:mrapi_impl_atomic_exec
DESCRIPTION: Execute atomic operation
PARAMETERS: process index
RETURN VALUE: none
***************************************************************************/
void mrapi_impl_atomic_exec(int pindex, mrapi_atomic_op* op)
{
#if !(__unix__)
	wchar_t wszPidx[40] = L"";
	wchar_t wszAtomicEvt[40] = L"";
#endif  /* !(__unix__) */

	mrapi_dprintf(2, "mrapi_impl_atomic_exec %d /*op->type*/; %d /*source*/",
		op->type, mrapi_db->processes[op->spindex].state.data.pid);

	switch (op->type) {
	case MRAPI_ATOM_OPENPROC:
#if !(__unix__)
		/* open atomic event handle to signal remote process */
		swprintf_s(wszPidx, sizeof(wszPidx) / sizeof(wchar_t), L"%d", op->spindex);
		wcscpy_s(wszAtomicEvt, sizeof(wszAtomicEvt) / sizeof(wchar_t), wszLocal);
		wcscat_s(wszAtomicEvt, sizeof(wszAtomicEvt) / sizeof(wchar_t), MRAPI_XWSTRING(PACKAGE));
		wcscat_s(wszAtomicEvt, sizeof(wszAtomicEvt) / sizeof(wchar_t), wszAtomic);
		wcscat_s(wszAtomicEvt, sizeof(wszAtomicEvt) / sizeof(wchar_t), wszPidx);
		mrapi_atomic_evt[op->spindex] = OpenEvent(EVENT_MODIFY_STATE, FALSE, wszAtomicEvt);
		mrapi_db->processes[pindex].link[op->spindex] = 1;
#endif  /* !(__unix__) */
		break;
	case MRAPI_ATOM_CLOSEPROC:
#if !(__unix)
		/* close atomic event handle */
		if (NULL != mrapi_atomic_evt[op->spindex]) {
			mrapi_db->processes[pindex].link[op->spindex] = 0;
			CloseHandle(mrapi_atomic_evt[op->spindex]);
			mrapi_atomic_evt[op->spindex] = NULL;
		}
#endif  /* !(__unix) */
		break;
	case MRAPI_ATOM_SHMDUP:
		mrapi_assert(sys_shmem_duplicate(mrapi_db->shmems[op->u.dup.s].id[pindex],
			mrapi_db->processes[op->spindex].proc, &mrapi_db->shmems[op->u.dup.s].id[op->spindex]));
		sys_atomic_sync(NULL);
		break;
	default:
		return;
	}
}

/***************************************************************************
NAME:mrapi_impl_atomic_hndlr
DESCRIPTION: The purpose of this function is to catch signals so that we
 can process atomic operations between address spaces. This is only used
 for Unix, the Windows implementation uses event objects instead.
PARAMETERS: the signal (SIGUSR1)
RETURN VALUE: none
***************************************************************************/
void mrapi_impl_atomic_hndlr(int sig)
{
#if !(__unix__)
	/* signal handler not used for Windows and minGW */
	return;
#else
	struct sigaction new_action;
	/* Ignore signals that are not SIGUSR1 */
	if (SIGUSR1 == sig) {
#ifdef NOTUSED
		/* print info on which signal was caught */
#if (__linux__)
		char sigbuff[128];
		sprintf(sigbuff, "SIGNAL: mrapi received atomic signal[%d] pid=%d tid=%s dindex=%d nindex=%d mrapi_db=%p",
			sig, mrapi_pid, mca_print_tid(mrapi_tid), mrapi_dindex, mrapi_nindex, mrapi_db);
		psignal(sig, sigbuff);
#endif  /* (__linux__) */

		printf("mrapi received atomic signal: %d\n", sig);
#endif  // NOTUSED

		if (mrapi_db == NULL || mrapi_pid == (pid_t)-1) {
			/* Signal handler not running in context of calling
			   thread, runs instead on a separate thread, e.g. Win32.
			   TLS is not initialized for this thread.
			*/
			mrapi_dprintf(1,
				"MRAPI: Unable to look up info for this process, thus unable to unwind the atomic handler any further. Restoring the default handler\n");
			new_action.sa_handler = SIG_DFL;
			sigemptyset(&new_action.sa_mask);
			new_action.sa_flags = 0;
			sigaction(sig, &new_action, NULL);
		}
		else {
			mrapi_atomic_op op = { 0 };

			/* spin until operation is valid */
			while (!mrapi_db->processes[mrapi_pindex].op.valid) {
				sys_os_yield();
				mrapi_impl_atomic_sync(NULL);
			}

			/* local copy to allow other threads to process atomic signals */
			memcpy(&op, &mrapi_db->processes[mrapi_pindex].op, sizeof(mrapi_atomic_op));

			mrapi_dprintf(2, "mrapi_impl_atomic_hndlr %d /*op->type*/;", op.type);

			/* mark operation as available */
			mrapi_db->processes[mrapi_pindex].op.valid = 0;

			/* Force shared memory synchronization */
			mrapi_impl_atomic_sync(NULL);

			mrapi_impl_atomic_exec(mrapi_pindex, &op);
		}
	}
#endif  /* (__unix__) */
}

#if !(__unix)
/***************************************************************************
NAME:mrapi_impl_atomic_listener
DESCRIPTION: The purpose of this function is to process events related
 to remote atomic operations.
PARAMETERS: process index
RETURN VALUE: none
***************************************************************************/
void mrapi_impl_atomic_listener(void* arg)
{
	int pindex = *(int*)arg;
	do {
		DWORD dwEvent = WaitForSingleObject(mrapi_atomic_evt[pindex], 100);
		if (WAIT_FAILED == dwEvent) {
			DWORD err = GetLastError();
		}
		else if (WAIT_TIMEOUT != dwEvent) {
			/* spin until operation is valid */
			while (!mrapi_db->processes[pindex].op.valid);

			mrapi_impl_atomic_exec(pindex, &mrapi_db->processes[pindex].op);

			/* mark operation as available */
			mrapi_db->processes[pindex].op.valid = 0;

			/* Force memory synchronization */
			sys_atomic_sync(NULL);
		}
	} while ((NULL != mrapi_db) && mrapi_db->processes[pindex].state.data.valid);
	hAtomicListener = NULL;
}
#endif  /* !(__unix) */
