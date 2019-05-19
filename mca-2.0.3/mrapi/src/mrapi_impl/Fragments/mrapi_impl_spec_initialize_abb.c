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
   Function: mrapi_impl_initialize

   Description: initializes the mrapi_impl layer (sets up the database and semaphore)

   Parameters:
	domain_id - collection of nodes that share resources
	node_id - task that synchronizes with other nodes in a domain
	status - initialization error:
		MRAPI_ERR_NODE_INITFAILED
		MRAPI_ERR_ATOM_OP_NOFORWARD

   Returns: boolean indicating success or failure

***************************************************************************/
mrapi_boolean_t mrapi_impl_initialize(mrapi_domain_t domain_id,
	mrapi_node_t node_id,
	mrapi_status_t* status)
{
	/* associate this node w/ a pid,tid pair so that we can recognize the
	   caller on later calls */

	int d = 0; // domain index
	int n = 0; // node index
	int p = 0; // process index
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

	mrapi_dprintf(1, "mrapi_impl_initialize (%d,%d);", domain_id, node_id);

	if (mrapi_impl_initialized())
	{
		*status = MRAPI_ERR_NODE_INITFAILED;
		return MRAPI_FALSE;
	}

	if (use_uid) {
#if (__unix__)
		uid = geteuid();
		pw = getpwuid(uid);
		if (pw) {
			//printf ("pw=%s\n",pw->pw_name);
		}
		else {
			fprintf(stderr, " cannot find username for UID %u\n", (unsigned)uid);
		}
		memset(buff, 0, sizeof(buff));
		strcat(buff, pw->pw_name);
		strcat(buff, "_mrapi");
#elif (__MINGW32__)
		// Get username and convert to single byte string
		char szName[sizeof(buff)] = "";
		DWORD lpnSize = sizeof(szName);
		GetUserName(szName, &lpnSize);
		strcat(buff, "_");
		strcat(buff, PACKAGE_NAME);
		strcat(szName, "_mrapi");
#else
		// Get username and convert to single byte string
		wchar_t szwName[sizeof(buff)] = L"";
		size_t returnValue = 0;
		DWORD lpnSize = sizeof(szwName);
		GetUserName(szwName, &lpnSize);
		wcstombs_s(&returnValue, buff, sizeof(buff), szwName, lpnSize);
		strcat_s(buff, sizeof(buff), "_");
		strcat_s(buff, sizeof(buff), PACKAGE_NAME);
		strcat_s(buff, sizeof(buff), "_mrapi");
#endif  /* !(__unix__||__MINGW32__) */
		/* global key */
		key = 0;
		key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
#if (__unix__||__MINGW32__)
		/* db key */
		strcat(buff, "_db");
		db_key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
		/* sems key */
		strcat(buff, "_sems");
		sems_key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
		/* shmems key */
		strcat(buff, "_shmems");
		shmems_key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
		/* rmems key */
		strcat(buff, "_rmems");
		rmems_key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
		/* requests key */
		strcat(buff, "_requests");
		requests_key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
#else
		/* db key */
		strcat_s(buff, sizeof(buff), "_db");
		db_key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
		/* sems key */
		strcat_s(buff, sizeof(buff), "_sems");
		sems_key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
		/* shmems key */
		strcat_s(buff, sizeof(buff), "_shmems");
		shmems_key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
		/* rmems key */
		strcat_s(buff, sizeof(buff), "_rmems");
		rmems_key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
		/* requests key */
		strcat_s(buff, sizeof(buff), "_requests");
		requests_key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
#endif  /* !(__unix__||__MINGW32__) */
	}
	else {
		(void)sys_file_key(NULL, 'z', &key);
		db_key = key + 10;
		sems_key = key + 20;
		shmems_key = key + 30;
		rmems_key = key + 40;
		requests_key = key + 50;
	}

	/* 1) setup the global database */
	/* get/create the shared memory database */
	shmemid_local = sys_shmem_get(db_key, sizeof(mrapi_database));
	if (shmemid_local == -1) {
#if !(__MINGW32__||__unix__)
		created_shmem = MRAPI_TRUE;
#endif  /* !(__MINGW32__) */
		shmemid_local = sys_shmem_create(db_key, sizeof(mrapi_database));
	}
	/* attach to the shared memory */
	if (shmemid_local != -1) {
		uintptr_t previous = 0;
		/* setup the global mrapi_db pointer and global shmemid */
		/* FIXME: IS IT SAFE TO WRITE THIS GLOBAL W/O A LOCK ??? */
		/* FIXME resolution: do not replace existing address if already attached */
		mrapi_db_local = (mrapi_database*)sys_shmem_attach(shmemid_local);
		if (sys_atomic_cas_ptr(NULL, (uintptr_t*)&mrapi_db, (uintptr_t)mrapi_db_local, (uintptr_t)NULL, &previous) && previous) {
			sys_shmem_detach(mrapi_db_local);
		}
	}
	if (mrapi_db == NULL) {
#if (__unix__||__MINGW32__)
		fprintf(stderr, "MRAPI_ERROR: Unable to attached to shared memory key= %x, errno=%s",
			key, strerror(errno));
#else
		char buf[80];
		strerror_s(buf, 80, errno);
		fprintf(stderr, "MRAPI_ERROR: Unable to attached to shared memory key= %x, errno=%s",
			key, buf);
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
	if (!mrapi_impl_create_sys_semaphore(&semid_local, 1/*num_locks*/, key, !use_spin_lock)) {
#if (__unix__||__MINGW32__)
		fprintf(stderr, "MRAPI ERROR: Unable to get the semaphore key= %x, errno=%s\n",
			key, strerror(errno));
#else
		char buf[80];
		strerror_s(buf, 80, errno);
		fprintf(stderr, "MRAPI ERROR: Unable to get the semaphore key= %x, errno=%s\n",
			key, buf);
#endif  /* !(__unix__||__MINGW32__) */
		rc = MRAPI_FALSE;
	}

	if (rc) {
		mrapi_dprintf(1, "mrapi_impl_initialize lock acquired, now attaching to shared memory and adding node to database");
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
			!mrapi_impl_create_sys_semaphore(&sems_semid, MRAPI_MAX_SEMS + 1/*num_locks*/, sems_key, MRAPI_FALSE)) {
			sems_semid = sems_global = semid;
		}
		if (!mrapi_impl_create_sys_semaphore(&shmems_semid, 1/*num_locks*/, shmems_key, MRAPI_FALSE)) {
			shmems_semid = shmems_global = semid;
		}
		if (!mrapi_impl_create_sys_semaphore(&rmems_semid, 1/*num_locks*/, rmems_key, MRAPI_FALSE)) {
			rmems_semid = rmems_global = semid;
		}
		if (!mrapi_impl_create_sys_semaphore(&requests_semid, 1/*num_locks*/, requests_key, MRAPI_FALSE)) {
			requests_semid = requests_global = semid;
		}

		//printf("**** semid:%x sems:%x shmems:%x rmems:%x requests:%x\n",
			   //key,sems_key,shmems_key,rmems_key,requests_key);

		/* get our identity */
#if !(__unix__||__MINGW32__)
		mrapi_pid = (pid_t)GetCurrentProcessId();
		mrapi_proc = (int)GetProcessId(GetCurrentProcess());
		mrapi_tid = (pthread_t)GetCurrentThreadId();
#else
		mrapi_pid = getpid();
		mrapi_proc = mrapi_pid;
		mrapi_tid = pthread_self();
#endif  /* (__unix__||__MINGW32__) */

		if (use_spin_lock)
		{
			/* lock the internal database */
			int32_t unlocked = 0;
			int32_t locked = (int32_t)mrapi_tid;
			int32_t prev;
			mrapi_status_t status;

			while (1)
			{
				if (mrapi_impl_atomic_cas(NULL, &mrapi_db->sems[0].spin, &locked, &unlocked, &prev, sizeof(int32_t), &status))
				{
					break;
				}
				sys_os_yield();
			}
		}

		/* seed random number generator */
		sys_os_srand((unsigned int)mrapi_tid);

		/* 3) add the process/node/domain to the database */
		if (rc) {
			/* first see if this domain already exists */
			for (d = 0; d < MRAPI_MAX_DOMAINS; d++) {
				mrapi_domain_state dstate;
				mrapi_assert(sys_atomic_read(NULL, &mrapi_db->domains[d].state, &dstate, sizeof(mrapi_db->domains[d].state)));
				if (dstate.data.domain_id == domain_id) {
					break;
				}
			}
			if (d == MRAPI_MAX_DOMAINS) {
				/* it didn't exist so find the first available entry */
				for (d = 0; d < MRAPI_MAX_DOMAINS; d++) {
					mrapi_domain_state* dstate = &mrapi_db->domains[d].state;
					mrapi_domain_state oldstate;
					mrapi_domain_state newstate;
					mrapi_assert(sys_atomic_read(NULL, dstate, &oldstate, sizeof(mrapi_db->domains[d].state)));
					newstate = oldstate;
					oldstate.data.allocated = MRAPI_FALSE;
					newstate.data.domain_id = domain_id;
					newstate.data.allocated = MRAPI_TRUE;
					if (sys_atomic_cas(NULL, dstate, &newstate, &oldstate, NULL,
						sizeof(mrapi_db->domains[d].state)))
						break;
				}
			}
		}
		if (d != MRAPI_MAX_DOMAINS) {
			/* now find an available node index...*/
			for (n = 0; n < MRAPI_MAX_NODES; n++) {
				mrapi_node_state state;
				mrapi_assert(sys_atomic_read(NULL, &mrapi_db->domains[d].nodes[n].state, &state, sizeof(mrapi_db->domains[d].nodes[n].state)));
				/* Even though initialized() is checked by mrapi, we have to check again here because
				   initialized() and initalize() are  not atomic at the top layer */
				if (state.data.allocated &&
					state.data.node_num == node_id) {
					/* this node already exists for this domain */
					rc = MRAPI_FALSE;
					*status = MRAPI_ERR_NODE_INITFAILED;
					mrapi_dprintf(1, "This node (%d) already exists for this domain(%d)", node_id, domain_id);
					break;
				}
			}
			if (n == MRAPI_MAX_NODES) {
				/* it didn't exist so find the first available entry */
				for (n = 0; n < MRAPI_MAX_NODES; n++) {
					mrapi_node_state* nstate = &mrapi_db->domains[d].nodes[n].state;
					mrapi_node_state oldstate;
					mrapi_node_state newstate;
					mrapi_assert(sys_atomic_read(NULL, nstate, &oldstate, sizeof(mrapi_db->domains[d].nodes[n].state)));
					newstate = oldstate;
					oldstate.data.allocated = MRAPI_FALSE;
					newstate.data.node_num = node_id;
					newstate.data.allocated = MRAPI_TRUE;
					if (sys_atomic_cas(NULL, nstate, &newstate, &oldstate, NULL,
						sizeof(mrapi_db->domains[d].nodes[n].state))) {
						break;
					}
				}
				if (n != MRAPI_MAX_NODES) {
					/* see if this process exists */
					for (p = 0; p < MRAPI_MAX_PROCESSES; p++) {
						mrapi_process_state pstate;
						mrapi_assert(sys_atomic_read(NULL, &mrapi_db->processes[p].state, &pstate, sizeof(mrapi_db->processes[p].state)));
						if (pstate.data.pid == mrapi_pid) {
							break;
						}
					}
					if (p == MRAPI_MAX_PROCESSES) {
						/* it didn't exist so find the first available entry */
						for (p = 0; p < MRAPI_MAX_PROCESSES; p++) {
							mrapi_process_state* pstate = &mrapi_db->processes[p].state;
							mrapi_process_state oldstate;
							mrapi_process_state newstate;
							mrapi_assert(sys_atomic_read(NULL, pstate, &oldstate, sizeof(mrapi_db->processes[p].state)));
							newstate = oldstate;
							oldstate.data.allocated = MRAPI_FALSE;
							newstate.data.pid = mrapi_pid;
							newstate.data.allocated = MRAPI_TRUE;
							if (sys_atomic_cas(NULL, pstate, &newstate, &oldstate, NULL,
								sizeof(mrapi_db->processes[p].state))) {
								break;
							}
						}
					}
				}
			}
			else {
				/* we didn't find an available domain index */
				mrapi_dprintf(1, "You have hit MRAPI_MAX_DOMAINS, either use less domains or reconfigure with more domains");
				rc = MRAPI_FALSE;
			}
			if (n == MRAPI_MAX_NODES) {
				/* we didn't find an available node index */
				mrapi_dprintf(1, "You have hit MRAPI_MAX_NODES, either use less nodes or reconfigure with more nodes.");
				rc = MRAPI_FALSE;
			}
			if (p == MRAPI_MAX_PROCESSES) {
				/* we didn't find an available process index */
				mrapi_dprintf(1, "You have hit MRAPI_MAX_PROCESSES, either use less processes or reconfigure with more processes");
				rc = MRAPI_FALSE;
			}
			else {
				mrapi_pindex = p;
			}
		}

		if (rc) {
			struct sigaction new_action, old_action;
#if !(__unix)
			wchar_t wszPidx[40] = L"";
			wchar_t wszAtomicEvt[128] = L"";
#endif  /* !(__unix__) */
			mrapi_domain_state dstate;
			mrapi_node_state nstate;
			mrapi_process_state pstate;

			mrapi_dprintf(1, "adding domain_id:%x node_id:%x to d:%d n:%d",
				domain_id, node_id, d, n);
			mrapi_assert(mrapi_db->domains[d].nodes[n].state.data.valid == MRAPI_FALSE);
			mrapi_assert(sys_atomic_read(NULL, &mrapi_db->domains[d].state, &dstate, sizeof(mrapi_db->domains[d].state)));
			dstate.data.domain_id = domain_id;
			dstate.data.valid = MRAPI_TRUE;
			sys_atomic_xchg(NULL, &mrapi_db->domains[d].state, &dstate, NULL, sizeof(mrapi_db->domains[d].state));
			mrapi_assert(sys_atomic_inc(NULL, &mrapi_db->domains[d].num_nodes, NULL, sizeof(mrapi_db->domains[d].num_nodes)));
			mrapi_db->domains[d].nodes[n].tid = mrapi_tid;
			mrapi_db->domains[d].nodes[n].proc_num = p;
			mrapi_assert(sys_atomic_read(NULL, &mrapi_db->domains[d].nodes[n].state, &nstate, sizeof(mrapi_db->domains[d].nodes[n].state)));
			nstate.data.node_num = node_id;
			nstate.data.valid = MRAPI_TRUE;
			sys_atomic_xchg(NULL, &mrapi_db->domains[d].nodes[n].state, &nstate, NULL, sizeof(mrapi_db->domains[d].nodes[n].state));

			/* set our cached (thread-local-storage) identity */
			mrapi_nindex = n;
			mrapi_dindex = d;
			mrapi_domain_id = domain_id;
			mrapi_node_id = node_id;

			mrapi_assert(sys_atomic_read(NULL, &mrapi_db->processes[p].state, &pstate, sizeof(mrapi_db->processes[p].state)));
			if (!pstate.data.valid) {
#if !(__unix__)
				int i = 0;
#endif  /* !(__unix__) */
				mrapi_atomic_op op = { MRAPI_ATOM_OPENPROC, 0 };
				mrapi_db->processes[p].proc = mrapi_proc;
				pstate.data.pid = mrapi_pid;
				pstate.data.valid = MRAPI_TRUE;
				sys_atomic_xchg(NULL, &mrapi_db->processes[p].state, &pstate, NULL, sizeof(mrapi_db->processes[p].state));

#if !(__unix__)
				/* create / open atomic event handles for active processes */
				for (i = 0; i < MRAPI_MAX_PROCESSES; i++) {
					mrapi_process_state pstate;
					mrapi_assert(sys_atomic_read(NULL, &mrapi_db->processes[i].state, &pstate, sizeof(mrapi_db->processes[i].state)));
					if (i == p || pstate.data.valid) {
						swprintf_s(wszPidx, sizeof(wszPidx) / sizeof(wchar_t), L"%d", i);
						wcscpy_s(wszAtomicEvt, sizeof(wszAtomicEvt) / sizeof(wchar_t), wszLocal);
						wcscat_s(wszAtomicEvt, sizeof(wszAtomicEvt) / sizeof(wchar_t), MRAPI_XWSTRING(PACKAGE));
						wcscat_s(wszAtomicEvt, sizeof(wszAtomicEvt) / sizeof(wchar_t), wszAtomic);
						wcscat_s(wszAtomicEvt, sizeof(wszAtomicEvt) / sizeof(wchar_t), wszPidx);
						if (i == p) {
							/* auto reset, not signaled, name "Local\mrapi_atomic_<process_index>" */
							mrapi_db->processes[p].hAtomicEvt = CreateEvent(NULL, FALSE, FALSE, wszAtomicEvt);
							mrapi_atomic_evt[i] = OpenEvent(SYNCHRONIZE, FALSE, wszAtomicEvt);
						}
						else if (pstate.data.valid) {
							mrapi_atomic_evt[i] = OpenEvent(EVENT_MODIFY_STATE, FALSE, wszAtomicEvt);
						}
					}
				}
#endif  /* !(__unix__) */

				/* Signal remote processes to add handling for this process */
				mrapi_impl_atomic_forward(0, &op, status);
			}
			mrapi_assert(sys_atomic_inc(NULL, &mrapi_db->processes[p].num_nodes, NULL, sizeof(mrapi_db->processes[p].num_nodes)));

			/* Force memory synchronization */
			sys_atomic_sync(NULL);

			/* Initialize metadata callback rollover */
			mrapi_db->rollover_index = 0;

			/* Set the resource tree pointer */
			resource_root = &chip;

			mrapi_dprintf(1, "registering signal handlers");
			/* register signal handlers so that we can still clean up resources
			   if an interrupt occurs
			   http://www.gnu.org/software/libtool/manual/libc/Sigaction-Function-Example.html
			*/

			/* Register atomic handler */
#if !(__unix__)
		/* Create atomic event listener thread for process if not started */
			if (NULL == hAtomicListener) {
				DWORD tid = 0;
				hAtomicListener = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mrapi_impl_atomic_listener,
					&mrapi_pindex, 0, &tid);
			}
#else
		/* Set up the structure to specify atomic action. */
			memset(&new_action, 0, sizeof(struct sigaction));
			new_action.sa_handler = mrapi_impl_atomic_hndlr;
			sigemptyset(&new_action.sa_mask);
			sigaddset(&new_action.sa_mask, SIGUSR1); /* block concurrent atomic signals */
			new_action.sa_flags = 0;
			sigaction(SIGUSR1, NULL, &old_action);
			mrapi_db->processes[mrapi_pindex].atomic = old_action;
			if (old_action.sa_handler != SIG_IGN) {
				sigaction(SIGUSR1, &new_action, NULL);
			}
#endif  /* (__unix__) */

			/* Register signal handler */

			/* Set up the structure to specify the signal action. */
			memset(&new_action, 0, sizeof(struct sigaction));
			new_action.sa_handler = mrapi_impl_signal_hndlr;
			sigemptyset(&new_action.sa_mask);
			new_action.sa_flags = 0;

#if (__unix__)
			sigaction(SIGINT, NULL, &old_action);
#else
			old_action.sa_handler = signal(SIGINT, SIG_GET);
#endif  /* !(__unix__) */
			mrapi_db->domains[d].nodes[n].signals[SIGINT] = old_action;
			if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
				sigaction(SIGINT, &new_action, NULL);
#else
				(void)signal(SIGINT, new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
			sigaction(SIGHUP, NULL, &old_action);
			mrapi_db->domains[d].nodes[n].signals[SIGHUP] = old_action;
			if (old_action.sa_handler != SIG_IGN)
				sigaction(SIGHUP, &new_action, NULL);
#endif  /* (__unix__) */

#if (__unix__)
			sigaction(SIGILL, NULL, &old_action);
#else
			old_action.sa_handler = signal(SIGILL, SIG_GET);
#endif  /* !(__unix__) */
			mrapi_db->domains[d].nodes[n].signals[SIGILL] = old_action;
			if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
				sigaction(SIGILL, &new_action, NULL);
#else
				(void)signal(SIGILL, new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
			sigaction(SIGSEGV, NULL, &old_action);
#else
			old_action.sa_handler = signal(SIGSEGV, SIG_GET);
#endif  /* !(__unix__) */
			mrapi_db->domains[d].nodes[n].signals[SIGSEGV] = old_action;
			if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
				sigaction(SIGSEGV, &new_action, NULL);
#else
				(void)signal(SIGSEGV, new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
			sigaction(SIGTERM, NULL, &old_action);
#else
			old_action.sa_handler = signal(SIGTERM, SIG_GET);
#endif  /* !(__unix__) */
			mrapi_db->domains[d].nodes[n].signals[SIGTERM] = old_action;
			if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
				sigaction(SIGTERM, &new_action, NULL);
#else
				(void)signal(SIGTERM, new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
			sigaction(SIGFPE, NULL, &old_action);
#else
			old_action.sa_handler = signal(SIGFPE, SIG_GET);
#endif  /* !(__unix__) */
			mrapi_db->domains[d].nodes[n].signals[SIGFPE] = old_action;
			if (old_action.sa_handler != SIG_IGN)
#if (__unix__)
				sigaction(SIGFPE, &new_action, NULL);
#else
				(void)signal(SIGFPE, new_action.sa_handler);
#endif  /* !(__unix__) */

#if (__unix__)
			sigaction(SIGABRT, NULL, &old_action);
			mrapi_db->domains[d].nodes[n].signals[SIGABRT] = old_action;
			if (old_action.sa_handler != SIG_IGN)
				sigaction(SIGABRT, &new_action, NULL);
#endif  /* (__unix__) */
		}

		/* release the lock */
		mrapi_impl_sem_ref_t ref = { semid_local, 0 };
		mrapi_impl_access_database_post(ref);
	}

	return rc;
}
