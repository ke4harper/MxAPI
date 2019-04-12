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

volatile int barrier = 0;
static mrapi_proc_t gpdb = { 0 };

void* client(void* args)
{
    static char threadmode_rtpshared[] = "rtp-rtp shared";
    static char threadmode_localshared[] = "task-task shared";
    static char threadmode_global[] = "task-task";

    uint32_t n_index = 0;
    uint32_t d_index = 0;
    mrapi_node_t node = 0;
    mrapi_domain_t domain = 0;
    mrapi_mutex_hndl_t mutex_id = 0;
	mrapi_sem_hndl_t sem_id = 0;
	mrapi_key_t lock_key = 0;
    volatile int who_is_here = 0;

	mrapi_status_t status = MRAPI_SUCCESS;
	mrapi_test_args_t* mta = (mrapi_test_args_t*)args;

    int i = 0;
    int j = 0;
	int p_index = 0;
	int t_index = 0;
	int rp_index = 0;
	int at_index = 0;
    int zero = 0;
    int one = 1;
    int idx = 0;
    int txn_id = 0;
    int role = 0;
	int nthread = 0;
	int nprocess = 0;
    int counter = 0;
#ifdef NOTUSED
    char* threadmode = NULL;
#endif  // NOTUSED
    pid_t pid = 0;
    pid_t empty = 0;
    pid_t prev = 0;
    pthread_t tid = 0;
    mrapi_proc_t* pdb = NULL;
    mrapi_sync_t* sync = NULL;
    mrapi_sync_t* rsync = NULL;
	mrapi_test_db_t* db = NULL;

	mca_cpu_t sem_cpu = { 0 };
	mca_timestamp_t sem_start = { 0 };
	mrapi_elapsed_t sem_elapsed = { 0 };
	mca_cpu_t mutex_cpu = { 0 };
	mca_timestamp_t mutex_start = { 0 };
	mrapi_elapsed_t mutex_elapsed = { 0 };

#if (__unix__)
    cpu_set_t mask = { { 0 } };
    CPU_ZERO(&mask);
    CPU_SET(mta->affinity,&mask);
    assert(0 == sched_setaffinity(0,sizeof(cpu_set_t),&mask));
#endif  // (__unix__)

	mca_set_debug_level(0);
	
	assert(mrapi_impl_initialize(mta->domain,mta->node,&status));
    assert(MRAPI_SUCCESS == status);
	assert(mrapi_impl_initialized());
    assert(mrapi_impl_whoami(&node,&n_index,&domain,&d_index));

	assert(mrapi_impl_mutex_get(&mutex_id,mta->mutex_key));
	assert(mrapi_impl_sem_get(&sem_id, mta->sem_key));
    assert(mrapi_impl_mutex_lock(mutex_id,&lock_key,MRAPI_TIMEOUT_INFINITE /*timeout*/,&status));
    assert(MRAPI_SUCCESS == status);
    barrier++;
    assert(mrapi_impl_mutex_unlock(mutex_id,&lock_key,&status));
    assert(MRAPI_SUCCESS == status);

    while(who_is_here < mta->num_nodes) {
        assert(mrapi_impl_mutex_lock(mutex_id,&lock_key,MRAPI_TIMEOUT_INFINITE /*timeout*/,&status));
        assert(MRAPI_SUCCESS == status);
        who_is_here = barrier;
        assert(mrapi_impl_mutex_unlock(mutex_id,&lock_key,&status));
        assert(MRAPI_SUCCESS == status);
        // give other threads a chance to run
        sys_os_yield();
    }

	// Get shared memory
    db = (mrapi_test_db_t*)mta->db;
    assert(NULL != db);

	// Register process
#if !(__unix__)
    pid = (pid_t)GetCurrentProcessId();
	tid = (pthread_t)GetCurrentThreadId();
#else
    pid = getpid();
    tid = pthread_self();
#endif  // (__unix__)

    p_index = -1;
    for(i = 0; i < SYNC_PROCESSES; i++) {
#if !(__unix__)
        prev = InterlockedCompareExchange((uint32_t*)&db->process[i].pid,pid,empty);
#else
        prev = __sync_val_compare_and_swap((pid_t*)&db->process[i].pid,empty,pid);
#endif  /* (__unix__) */
        //printf("pid %d tid 0x%x: CAS db->process[%d].pid %d, prev %d\n",pid,tid,i,db->process[i].pid,prev);
        if(empty == prev) {
#if !(__unix__)
            nprocess = InterlockedIncrement((uint32_t*)&db->nprocess);
#else
            nprocess = __sync_add_and_fetch(&db->nprocess,1);
#endif  // (__unix__)
            p_index = nprocess-1;
            //printf("pid %d tid 0x%x: INC db->nprocess %d, new process idx %d\n",pid,tid,db->nprocess,p);
            break;
        }
        else if(pid == db->process[i].pid) {
            p_index = i;
            //printf("pid %d tid 0x%x: MATCH db->nprocess %d, existing process idx %d\n",pid,tid,db->nprocess,p);
            break;
        }
    }
    assert(0 <= p_index);
    assert(SYNC_PROCESSES > p_index);
    db->process[p_index].pid = pid;

	// Register thread
#if !(__unix__)
    nthread = InterlockedIncrement((uint32_t*)&db->process[p_index].nthread);
#else
    nthread = __sync_add_and_fetch(&db->process[p].nthread,1);
#endif  // (__unix__)
    t_index = nthread-1;
    assert(0 <= t_index);
    assert(SYNC_THREADS > t_index);

    db->process[p_index].thread[t_index].tid = tid;

    /* Spin waiting for all threads to start */
    while(mta->num_nodes > db->process[p_index].nthread) {
        sys_os_yield();
    }

    //printf("pid %d tid %u: nprocess %d nthread %d start\n",pid,tid,nprocess,nthread);

	// Get remote process slot
    rp_index = (0 == p_index % 2) ? p_index + 1 : p_index - 1;

	// Get alternate thread slot
	at_index = (0 == t_index % 2) ? t_index + 1 : t_index - 1;

    for(j = 0; j < 3; j++) {

        int txn[SPLIT_ITERATIONS] = { 0 };

        switch(j) {
        case 0: // Shared memory between processes
            pdb = &db->process[p_index];
            sync = &pdb->thread[t_index].sync[j];
            rsync = &db->process[rp_index].thread[at_index].sync[j];
            role = p_index % 2;
#ifdef NOTUSED
            threadmode = threadmode_rtpshared;
#endif  // NOTUSED
            break;
        case 1: // Shared memory between threads
            pdb = &db->process[p_index];
            sync = &pdb->thread[t_index].sync[j];
            rsync = &pdb->thread[at_index].sync[j];
            role = t_index % 2;
#ifdef NOTUSED
            threadmode = threadmode_localshared;
#endif  // NOTUSED
            break;
        case 2: // Global memory between threads
            pdb = &gpdb;
            sync = &pdb->thread[t_index].sync[j];
            rsync = &pdb->thread[at_index].sync[j];
            pdb->thread[t_index].tid = db->process[p_index].thread[t_index].tid;
            role = t_index % 2;
#ifdef NOTUSED
            threadmode = threadmode_global;
#endif  // NOTUSED
            break;
        }
        sync->mode = j;

        if(0 == j && !mta->bproc) {
          // no remote process
          continue;
        }

        /* Increment mode count */
#if !(__unix__)
        (void)InterlockedIncrement((uint32_t*)&pdb->nmode[j]);
#else
        (void)__sync_add_and_fetch(&pdb->nmode[j],1);
#endif  // (__unix__)

    	// Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

        // Reset read and write counters
        sync->nread = sync->nwrite = counter = 0;

        memset(txn,0,SPLIT_ITERATIONS*sizeof(int));
        memset(&sync->elapsed,0,sizeof(mrapi_elapsed_t));
        memset(&sync->cpu,0,sizeof(mca_cpu_t));

	    // Enable self
#if !(__unix__)
        (void)InterlockedCompareExchange((uint32_t*)&sync->benable,one,sync->benable);
#else
        (void)__sync_val_compare_and_swap(&sync->benable,sync->benable,one);
#endif  /* (__unix__) */

    	// Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

        // Spin waiting for remote enable
        while(!rsync->benable) {
            sys_os_yield();
        }

        //printf("pid %d tid 0x%x: nmode %d, %s start\n",
        //    db->process[p].pid,db->process[p].thread[t].tid,pdb->nmode[j],threadmode);

        // Save beginning time
        mca_begin_cpu(&sync->cpu); /* causes a delay, call before timing start */
        mca_begin_ts(&sync->start);

        for(i = 0; i < SPLIT_ITERATIONS; i++) {

            // Increment number of iterations
            sync->elapsed.iterations++;
            assert(SPLIT_ITERATIONS >= sync->elapsed.iterations);

            switch(role) {
            case 0: // Write to remote

#ifdef NOTUSED
                // Save beginning write delay time
                mca_begin_split_ts(&sync->start_write);
#endif  // NOTUSED

                // Spin waiting for available write slot
                idx = counter % SYNC_BUFFERS;
                while(sync->buffer[idx].valid) {
                    sys_os_yield();
                }

#ifdef NOTUSED
                // Compute and save elapsed write delay time
                (void)mca_end_split_ts(&sync->start_write);
#endif  // NOTUSED

                // Set message with transaction ID
                sync->buffer[idx].txn = i;
                txn[i]++;

                // Enable remote read
                sync->buffer[idx].valid = 1;

#ifdef NOTUSED
                // Save beginning transfer time
                (void)mca_begin_split_ts(&sync->buffer[idx].start_xfr);
#endif  // NOTUSED

                // Increment self write counter
#if !(__unix__)
                counter = InterlockedIncrement((uint32_t*)&sync->nwrite);
#else
                counter = __sync_add_and_fetch(&sync->nwrite,1);
#endif  // (__unix__)

#ifdef NOTUSED
                // Save beginning sync delay time
                mca_begin_split_ts(&sync->start_sync);
#endif  // NOTUSED

                // Force memory synchronization
#if !(__unix__)
                MemoryBarrier();
#else
                __sync_synchronize();
#endif  // (__unix__)

#ifdef NOTUSED
                // Compute and save elapsed sync delay time
                (void)mca_end_split_ts(&sync->start_sync);
#endif  // NOTUSED

                break;

            case 1: // Read from remote
#ifdef NOTUSED
                // Save beginning read delay time
                mca_begin_split_ts(&sync->start_read);
#endif  // NOTUSED

                // Spin waiting for available read transfer
                idx = counter % SYNC_BUFFERS;
                while(!rsync->buffer[idx].valid) {
                    sys_os_yield();
                }

#ifdef NOTUSED
                // Compute and save elapsed read delay time
                (void)mca_end_split_ts(&sync->start_read);
#endif  // NOTUSED

                // Collect transaction ID
                txn_id = rsync->buffer[idx].txn;
                txn[txn_id]++;

#ifdef NOTUSED
                // Compute and save elapsed transfer time
                (void)mca_end_split_ts(&rsync->buffer[idx].start_xfr);
#endif  // NOTUSED

                // Enable remote write
                rsync->buffer[idx].valid = 0;

                // Increment self read counter
#if !(__unix__)
                counter = InterlockedIncrement((uint32_t*)&sync->nread);
#else
                counter = __sync_add_and_fetch(&sync->nread,1);
#endif  // (__unix__)

                break;
            }
        }

		// Compute and save elapsed total time/iteration
		sync->elapsed.run = mca_end_ts(&sync->start) / sync->elapsed.iterations;
		sync->elapsed.util = mca_end_cpu(&sync->cpu); /* causes delay, call after timing measurement */

		// Save beginning time
		mca_begin_cpu(&sem_cpu); /* causes a delay, call before timing start */
		mca_begin_ts(&sem_start);
		memset(&sem_elapsed, 0, sizeof(mrapi_elapsed_t));

		for (i = 0; i < SPLIT_ITERATIONS; i++) {

			// Increment number of iterations
			sem_elapsed.iterations++;
			assert(SPLIT_ITERATIONS >= sem_elapsed.iterations);

			while (1)
			{
				if (mrapi_impl_sem_lock(sem_id, 1, MRAPI_TIMEOUT_INFINITE, &status))
				{
					break;
				}
				sys_os_yield();
			}
			while (1)
			{
				if (mrapi_impl_sem_unlock(sem_id, 1, &status))
				{
					break;
				}
				sys_os_yield();
			}
		}

		// Compute and save elapsed total time/iteration
		sem_elapsed.run = mca_end_ts(&sem_start) / sem_elapsed.iterations;
		sem_elapsed.util = mca_end_cpu(&sem_cpu); /* causes delay, call after timing measurement */

		// Save beginning rundown time
        mca_begin_ts(&sync->start_rundown);

        if(0 == t_index && 0 == role) {
            // Spin waiting for final remote read
            while(rsync->nread < SPLIT_ITERATIONS) {
                sys_os_yield();
            }
        }

        // Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

        // Compute and save elapsed rundown time/iteration
        sync->elapsed.rundown = mca_end_ts(&sync->start_rundown)/sync->elapsed.iterations;

        // Verify all transactions complete
        for(i = 0; i < SPLIT_ITERATIONS; i++) {
          assert(1 == txn[i]);
        }

        // Disable self
#if !(__unix__)
        (void)InterlockedCompareExchange((uint32_t*)&sync->benable,zero,sync->benable);
#else
        (void)__sync_val_compare_and_swap(&sync->benable,sync->benable,zero);
#endif  /* (__unix__) */

        // Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

	    // Spin waiting for remote disable
        while(rsync->benable) {
            sys_os_yield();
        }

        /* Decrement mode count */
#if !(__unix__)
        (void)InterlockedDecrement((uint32_t*)&pdb->nmode[j]);
#else
        (void)__sync_add_and_fetch(&pdb->nmode[j],-1);
#endif  // (__unix__)

        /* Spin waiting for rest of threads to finish mode */
        while(0 < pdb->nmode[j]) {
            sys_os_yield();
        }

        //printf("pid %d tid 0x%x: nmode %d, %s end\n",
        //    db->process[p].pid,db->process[p].thread[t].tid,pdb->nmode[j],threadmode);
    }

    // Enable thread rundown
#if !(__unix__)
    nthread = InterlockedDecrement((uint32_t*)&db->process[p_index].nthread);
#else
    nthread = __sync_add_and_fetch(&db->process[p].nthread,-1);
#endif  // (__unix__)
    assert(0 <= nthread);
    if(0 >= nthread) {
#if !(__unix__)
        nprocess = InterlockedDecrement((uint32_t*)&db->nprocess);
#else
        nprocess = __sync_add_and_fetch(&db->nprocess,-1);
#endif  // (__unix__)
        //printf("pid %d tid 0x%x: DEC db->nprocess %d, rundown process idx %d\n",pid,tid,db->nprocess,p);
        assert(0 <= nprocess);
    }

    //printf("pid %d tid 0x%x: nprocess %d nthread %d end\n",pid,tid,nprocess,nthread);

    // Force memory synchronization
#if !(__unix__)
    MemoryBarrier();
#else
    __sync_synchronize();
#endif  // (__unix__)

	// Spin waiting for thread rundown
    while(0 < db->process[p_index].nthread) {
        sys_os_yield();
    }

    if(0 == t_index) {
        // Spin waiting for alternate thread rundown
        while(0 < db->process[p_index].nthread) {
            sys_os_yield();
        }

        // Spin waiting for remote thread rundown
        while(0 < db->process[rp_index].nthread) {
            sys_os_yield();
        }
    }

    for(j = 0; j < 3; j++) {
        int k = 0;
        char msg[512] = "";
        char* threadmode = NULL;
        double split_run = 0;
        double split_rundown = 0;
        double split_util = 0.0;

        if(0 == j && !mta->bproc) {
            continue; // no remote process
        }

        switch(j) {
        case 0: // Shared memory between processes
            pdb = &db->process[p_index];
            sync = &pdb->thread[t_index].sync[j];
            threadmode = threadmode_rtpshared;
            break;
        case 1: // Shared memory between threads
            pdb = &db->process[p_index];
            sync = &pdb->thread[t_index].sync[j];
            threadmode = threadmode_localshared;
            break;
        case 2: // Global memory between threads
            pdb = &gpdb;
            sync = &pdb->thread[t_index].sync[j];
            pdb->thread[t_index].tid = db->process[p_index].thread[t_index].tid;
            threadmode = threadmode_global;
            break;
        }

        split_run = (0.0 == sync->elapsed.run) ? -1.0 : sync->elapsed.run;
        split_rundown = (0 == sync->elapsed.rundown) ? -1.0 : sync->elapsed.rundown;
        split_util = sync->elapsed.util;

        i = sync->elapsed.iterations;
        assert(0 < i);
#if !(__unix__)
        sprintf_s(msg,sizeof(msg),"(SM)  pid %6d t %d cpu %d, %d: %12.2f %7.2f %7.2f %7.2f  ",
            pid,t_index,mta->affinity,i,1.0E6/split_run,split_run,split_rundown,split_util);
        for(k = 0; k < (int)sync->cpu.processors; k++) {
            int msglen = strlen(msg);
            sprintf_s(&msg[msglen],sizeof(msg)-msglen," %6.2f",sync->cpu.split_sum[k+1]/sync->cpu.split_samples);
        }
        sprintf_s(&msg[strlen(msg)],sizeof(msg)-strlen(msg)," (%s)\n",threadmode);
#else
        sprintf(msg,"(SM)  pid %6d t %d cpu %d, %d: %12.2f %7.2f %7.2f %7.2f  ",
            pid,t,mta->affinity,i,1.0E6/split_run,split_run,split_rundown,split_util);
        for(k = 0; k < (int)sync->cpu.processors; k++) {
            int msglen = strlen(msg);
            sprintf(&msg[msglen]," %6.2f",sync->cpu.split_sum[k+1]/sync->cpu.split_samples[k]);
        }
        sprintf(&msg[strlen(msg)]," (%s)\n",threadmode);
#endif  // (__unix__)
        printf(msg);

		split_run = (0.0 == sem_elapsed.run) ? -1.0 : sem_elapsed.run;
		split_util = sem_elapsed.util;

#if !(__unix__)
		sprintf_s(msg, sizeof(msg), "(SEM) pid %6d t %d cpu %d, %d: %12.2f %7.2f %7.2f  ",
			pid, t_index, mta->affinity, i, 1.0E6 / split_run, split_run, split_util);
		for (k = 0; k < (int)sync->cpu.processors; k++) {
			int msglen = strlen(msg);
			sprintf_s(&msg[msglen], sizeof(msg) - msglen, " %6.2f", sem_cpu.split_sum[k + 1] / sem_cpu.split_samples);
		}
		sprintf_s(&msg[strlen(msg)], sizeof(msg) - strlen(msg), " (%s)\n", threadmode);
#else
		sprintf(msg, "(SEM) pid %6d t %d cpu %d, %d: %12.2f %7.2f %7.2f  ",
			pid, t, mta->affinity, i, 1.0E6 / split_run, split_run, split_util);
		for (k = 0; k < (int)sync->cpu.processors; k++) {
			int msglen = strlen(msg);
			sprintf(&msg[msglen], " %6.2f", sem_cpu.split_sum[k + 1] / sem_cpu.split_samples[k]);
		}
		sprintf(&msg[strlen(msg)], " (%s)\n", threadmode);
#endif  // (__unix__)
		printf(msg);
	}

	db->process[p_index].pid = 0;

    // Force memory synchronization
#if !(__unix__)
    MemoryBarrier();
#else
    __sync_synchronize();
#endif  // (__unix__)

    if(mta->bproc) {
        // Spin waiting for remote process rundown
        while(0 < db->nprocess &&
            0 < db->process[rp_index].nthread) {
                sys_os_yield();
        }
    }

    assert(mrapi_impl_finalize());
    return 0;
}
