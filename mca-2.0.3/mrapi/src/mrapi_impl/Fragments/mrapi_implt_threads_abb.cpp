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
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

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
static mrapi_proc_t gpdb = { { 0 } };

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
    mrapi_key_t lock_key = 0;
    volatile int who_is_here = 0;

	mrapi_status_t status = MRAPI_SUCCESS;
	mrapi_test_args_t* mta = (mrapi_test_args_t*)args;

    int i = 0;
    int j = 0;
	int p = 0;
	int t = 0;
	int rp = 0;
	int at = 0;
    int zero = 0;
    int one = 1;
    int txn_id = 0;
    int benable = 0;
	int nthread = 0;
	int nprocess = 0;
    int counter = 0;
    unsigned idx = 0;
#if !(__unix__||__atomic_barrier_test__)
    unsigned int valid = 0;
#endif  /* !(__unix__||__atomic_barrier_test__) */
#ifdef NOTUSED
    char* threadmode = NULL;
#endif  // NOTUSED
    pid_t pid = 0;
    pid_t* prpid = NULL;
    pid_t empty = 0;
    pid_t prev = 0;
    pthread_t tid = 0;
    pthread_t* prtid = NULL;
    mrapi_test_role_t role;
    mrapi_proc_t* pdb = NULL;
    mrapi_proc_t* rpdb = NULL;
    mrapi_sync_t* sync = NULL;
    mrapi_sync_t* rsync = NULL;
	mrapi_test_db_t* db = NULL;

    mrapi_atomic_barrier_t paxb;
    mrapi_atomic_barrier_t rpaxb;
    mrapi_atomic_barrier_t saxb;
    mrapi_atomic_barrier_t rsaxb;
    mrapi_atomic_barrier_t sbaxb;
    mrapi_atomic_barrier_t rsbaxb;
    mca_timeout_t timeout = MCA_INFINITE;

#ifdef NOTUSED
#if (__unix__)
    cpu_set_t mask = { { 0 } };
    CPU_ZERO(&mask);
    CPU_SET(mta->affinity,&mask);
    assert(0 == sched_setaffinity(0,sizeof(cpu_set_t),&mask));
#endif  // (__unix__)
#endif  // NOTUSED

    assert(mrapi_impl_initialize(mta->domain,mta->node,&status));
    assert(MRAPI_SUCCESS == status);
	assert(mrapi_impl_initialized());
    assert(mrapi_impl_whoami(&node,&n_index,&domain,&d_index));

	assert(mrapi_impl_mutex_get(&mutex_id,mta->mutex_key));
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

    p = -1;
    for(i = 0; i < SYNC_PROCESSES; i++) {
        //printf("pid %d tid 0x%x: CAS db->process[%d].pid %d\n",pid,tid,i,db->process[i].pid);
        status = MRAPI_SUCCESS;
        if(mrapi_impl_atomic_cas(NULL,&db->process[i].pid,&pid,&empty,&prev,
                                     sizeof(db->process[i].pid),&status)) {
            assert(MRAPI_SUCCESS == status);
            assert(mrapi_impl_atomic_inc(NULL,&db->nprocess,&nprocess,sizeof(db->nprocess),&status));
            assert(MRAPI_SUCCESS == status);
            p = nprocess-1;
            //printf("pid %d tid 0x%x: INC db->nprocess %d, new process idx %d\n",pid,tid,db->nprocess,p);
            break;
        }
        else if(pid == db->process[i].pid) {
            p = i;
            //printf("pid %d tid 0x%x: MATCH db->nprocess %d, existing process idx %d\n",pid,tid,db->nprocess,p);
            break;
        }
    }
    assert(0 <= p);
    assert(SYNC_PROCESSES > p);
    db->process[p].pid = pid;

	// Register thread
    status = MRAPI_SUCCESS;
    assert(mrapi_impl_atomic_inc(NULL,&db->process[p].nthread,&nthread,sizeof(db->process[p].nthread),&status));
    assert(MRAPI_SUCCESS == status);
    t = nthread-1;
    assert(0 <= t);
    assert(SYNC_THREADS > t);

    db->process[p].thread[t].tid = tid;

    /* Spin waiting for all threads to start */
    while(mta->num_nodes > db->process[p].nthread) {
        sys_os_yield();
    }

    //printf("pid %d tid %u: nprocess %d nthread %d start\n",pid,tid,nprocess,nthread);

	// Get remote process slot
    rp = (0 == p % 2) ? p + 1 : p - 1;

	// Get alternate thread slot
	at = (0 == t % 2) ? t + 1 : t - 1;

    for(j = 0; j < 3; j++) {

        int txn[SPLIT_ITERATIONS] = { 0 };

        switch(j) {
        case 0: // Shared memory between processes
            pdb = &db->process[p];
            rpdb = &db->process[rp];
            prpid = &rpdb->pid;
            sync = &pdb->thread[t].sync[j];
            rsync = &rpdb->thread[at].sync[j];
            prtid = &rpdb->thread[at].tid;
            role = (mrapi_test_role_t)(p % 2);
#ifdef NOTUSED
            threadmode = threadmode_rtpshared;
#endif  // NOTUSED
            break;
        case 1: // Shared memory between threads
            pdb = &db->process[p];
            rpdb = pdb;
            prpid = &rpdb->pid;
            sync = &pdb->thread[t].sync[j];
            rsync = &pdb->thread[at].sync[j];
            prtid = &pdb->thread[at].tid;
            role = (mrapi_test_role_t)(t % 2);
#ifdef NOTUSED
            threadmode = threadmode_localshared;
#endif  // NOTUSED
            break;
        case 2: // Global memory between threads
            pdb = &gpdb;
            pdb->pid = pid;
            rpdb = pdb;
            prpid = &rpdb->pid;
            sync = &pdb->thread[t].sync[j];
            rsync = &pdb->thread[at].sync[j];
            pdb->thread[at].tid = tid;
            prtid = &pdb->thread[at].tid;
            pdb->thread[t].tid = db->process[p].thread[t].tid;
            role = (mrapi_test_role_t)(t % 2);
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

        // Initialize sync exchange barriers
        while(mta->bproc &&
            0 == *prpid) {
            sys_os_yield();
        }
        while(0 == *prtid) {
            sys_os_yield();
        }
        mrapi_impl_atomic_barrier_init(&paxb,*prpid,(mrapi_msg_t*)pdb,1,sizeof(mrapi_proc_t),NULL,timeout);
        mrapi_impl_atomic_barrier_init(&rpaxb,*prpid,(mrapi_msg_t*)rpdb,1,sizeof(mrapi_proc_t),NULL,timeout);
        mrapi_impl_atomic_barrier_init(&saxb,*prpid,(mrapi_msg_t*)sync,1,sizeof(mrapi_sync_t),NULL,timeout);
        mrapi_impl_atomic_barrier_init(&rsaxb,*prpid,(mrapi_msg_t*)rsync,1,sizeof(mrapi_sync_t),NULL,timeout);
        mrapi_impl_atomic_exchange_init(&sbaxb,*prpid,(mrapi_msg_t*)sync->buffer,SYNC_BUFFERS,sizeof(sync->buffer[0]),&idx,timeout);
        mrapi_impl_atomic_exchange_init(&rsbaxb,*prpid,(mrapi_msg_t*)rsync->buffer,SYNC_BUFFERS,sizeof(rsync->buffer[0]),&idx,timeout);

        /* Increment mode count */
        assert(mrapi_impl_atomic_inc(&paxb,&pdb->nmode[j],NULL,sizeof(pdb->nmode[j]),&status));
        assert(MRAPI_SUCCESS == status);

    	// Force memory synchronization
	    mrapi_impl_atomic_sync(&paxb);

        // Reset read and write counters
        sync->nread = sync->nwrite = counter = 0;

        memset(txn,0,SPLIT_ITERATIONS*sizeof(int));
        memset(&sync->elapsed,0,sizeof(mrapi_elapsed_t));
        memset(&sync->cpu,0,sizeof(mca_cpu_t));

	    // Enable self
        assert(mrapi_impl_atomic_xchg(&saxb,&sync->benable,&one,NULL,sizeof(sync->benable),&status));
        assert(MRAPI_SUCCESS == status);

    	// Force memory synchronization
	    mrapi_impl_atomic_sync(&paxb);

        // Spin waiting for remote enable
        while(1) {
            assert(mrapi_impl_atomic_read(&rsaxb,&rsync->benable,&benable,sizeof(rsync->benable),&status));
            assert(MRAPI_SUCCESS == status);
            if(benable) {
                break;
            }
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

            idx = counter % SYNC_BUFFERS;

            switch(role) {
            case MRAPI_TEST_WRITE: // Write to remote

#if !(__unix__||__atomic_barrier_test__)
                // Spin waiting for available write slot
                while(1) {
                    assert(mrapi_impl_atomic_read(NULL,&sync->buffer[idx].valid,&valid,sizeof(sync->buffer[idx].valid),&status));
                    assert(MRAPI_SUCCESS == status);
                    if(!valid) {
                        break;
                    }
                    sys_os_yield();
                }
#endif  // !(__unix__||__atomic_barrier_test__)

                // Set message with transaction ID
                txn_id = i;
                assert(mrapi_impl_atomic_xchg(&sbaxb,&sync->buffer[idx].txn,&txn_id,NULL,sizeof(sync->buffer[idx].txn),&status));
                assert(MRAPI_SUCCESS == status);
                txn[i]++;

#if !(__unix__||__atomic_barrier_test__)
                // Enable remote read
                valid = 1;
                assert(mrapi_impl_atomic_xchg(NULL,&sync->buffer[idx].valid,&valid,NULL,sizeof(sync->buffer[idx].valid),&status));
                assert(MRAPI_SUCCESS == status);
#endif  // !(__unix__||__atomic_barrier_test__)

#ifdef NOTUSED
                // Save beginning transfer time
                mca_begin_split_ts(&sync->buffer[idx].start_xfr);
#endif  // NOTUSED

                // Increment self write counter
                assert(mrapi_impl_atomic_inc(&saxb,&sync->nwrite,&counter,sizeof(sync->nwrite),&status));
                assert(MRAPI_SUCCESS == status);

                // Force memory synchronization
 	            mrapi_impl_atomic_sync(&paxb);

                break;

            case MRAPI_TEST_READ: // Read from remote

#if !(__unix__||__atomic_barrier_test__)
                // Spin waiting for available read transfer
                while(1) {
                    assert(mrapi_impl_atomic_read(NULL,&rsync->buffer[idx].valid,&valid,sizeof(rsync->buffer[idx].valid),&status));
                    assert(MRAPI_SUCCESS == status);
                    if(valid) {
                        break;
                    }
                    sys_os_yield();
                }
#endif  // !(__unix__||__atomic_barrier_test__)

                // Collect transaction ID
                assert(mrapi_impl_atomic_read(&rsbaxb,&rsync->buffer[idx].txn,&txn_id,sizeof(rsync->buffer[idx].txn),&status));
                assert(MRAPI_SUCCESS == status);
                txn[txn_id]++;

#ifdef NOTUSED
                // Compute and save elapsed transfer time
                (void)mca_end_split_ts(&rsync->buffer[idx].start_xfr);
#endif  // NOTUSED

#if !(__unix__||__atomic_barrier_test__)
                // Enable remote write
                valid = 0;
                assert(mrapi_impl_atomic_xchg(NULL,&rsync->buffer[idx].valid,&valid,NULL,sizeof(rsync->buffer[idx].valid),&status));
                assert(MRAPI_SUCCESS == status);
#endif  // !(__unix__||__atomic_barrier_test__)

                // Increment self read counter
                assert(mrapi_impl_atomic_inc(&saxb,&sync->nread,&counter,sizeof(sync->nread),&status));
                assert(MRAPI_SUCCESS == status);

                break;
            }
        }

        // Save beginning rundown time
        mca_begin_ts(&sync->start_rundown);

        if(0 == t && 0 == role) {
            // Spin waiting for final remote read
            while(rsync->nread < SPLIT_ITERATIONS) {
                sys_os_yield();
            }
        }

        // Force memory synchronization
    	mrapi_impl_atomic_sync(&paxb);

        // Compute and save elapsed rundown time/iteration
        sync->elapsed.rundown = mca_end_ts(&sync->start_rundown)/sync->elapsed.iterations;

        // Compute and save elapsed total time/iteration
        sync->elapsed.run = mca_end_ts(&sync->start)/sync->elapsed.iterations;
        sync->elapsed.util = mca_end_cpu(&sync->cpu); /* causes delay, call after timing measurement */

        // Verify all transactions complete
        for(i = 0; i < SPLIT_ITERATIONS; i++) {
            assert(1 == txn[i]);
        }

        if(mta->bproc) {
            // Wait for remote process to finish
            sys_os_usleep(100000);
        }

        // Disable self
        assert(mrapi_impl_atomic_xchg(&saxb,&sync->benable,&zero,NULL,sizeof(sync->benable),&status));
        assert(MRAPI_SUCCESS == status);

        // Force memory synchronization
    	mrapi_impl_atomic_sync(&paxb);

	    // Spin waiting for remote disable
        while(1) {
            assert(mrapi_impl_atomic_read(&rsaxb,&rsync->benable,&benable,sizeof(rsync->benable),&status));
            assert(MRAPI_SUCCESS == status);
            if(!benable) {
                break;
            }
            sys_os_yield();
        }

        /* Decrement mode count */
        assert(mrapi_impl_atomic_dec(&paxb,&pdb->nmode[j],NULL,sizeof(pdb->nmode[j]),&status));
        assert(MRAPI_SUCCESS == status);

        /* Spin waiting for rest of local threads to finish mode */
        while(0 < pdb->nmode[j]) {
            sys_os_yield();
        }

        //printf("pid %d tid 0x%x: nmode %d, %s end\n",
        //    db->process[p].pid,db->process[p].thread[t].tid,pdb->nmode[j],threadmode);
    }

    // Enable thread rundown
    assert(mrapi_impl_atomic_dec(NULL,&db->process[p].nthread,&nthread,sizeof(db->process[p].nthread),&status));
    assert(0 <= nthread);
    if(0 >= nthread) {
        assert(mrapi_impl_atomic_dec(NULL,&db->nprocess,&nprocess,sizeof(db->nprocess),&status));
        //printf("pid %d tid 0x%x: DEC db->nprocess %d, rundown process idx %d\n",pid,tid,db->nprocess,p);
        assert(0 <= nprocess);
    }

    //printf("pid %d tid 0x%x: nprocess %d nthread %d end\n",pid,tid,nprocess,nthread);

    // Force memory synchronization
	mrapi_impl_atomic_sync(NULL);

	// Spin waiting for thread rundown
    while(0 < db->process[p].nthread) {
        sys_os_yield();
    }

    if(0 == t) {
        // Spin waiting for alternate thread rundown
        while(0 < db->process[p].nthread) {
            sys_os_yield();
        }

        // Spin waiting for remote thread rundown
        while(0 < db->process[rp].nthread) {
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
            pdb = &db->process[p];
            sync = &pdb->thread[t].sync[j];
            threadmode = threadmode_rtpshared;
            break;
        case 1: // Shared memory between threads
            pdb = &db->process[p];
            sync = &pdb->thread[t].sync[j];
            threadmode = threadmode_localshared;
            break;
        case 2: // Global memory between threads
            pdb = &gpdb;
            sync = &pdb->thread[t].sync[j];
            pdb->thread[t].tid = db->process[p].thread[t].tid;
            threadmode = threadmode_global;
            break;
        }

        split_run = (0.0 == sync->elapsed.run) ? -1.0 : sync->elapsed.run;
        split_rundown = (0 == sync->elapsed.rundown) ? -1.0 : sync->elapsed.rundown;
        split_util = sync->elapsed.util;

        i = sync->elapsed.iterations;
        assert(0 < i);
#if !(__unix__)
        sprintf_s(msg,sizeof(msg),"pid %6d t %d cpu %d, %d: %12.2f %7.2f %7.2f %7.2f  ",
            pid,t,mta->affinity,i,1.0E6/split_run,split_run,split_rundown,split_util);
        for(k = 0; k < (int)sync->cpu.processors; k++) {
            int msglen = strlen(msg);
            sprintf_s(&msg[msglen],sizeof(msg)-msglen," %6.2f",sync->cpu.split_sum[k+1]/sync->cpu.split_samples);
        }
        sprintf_s(&msg[strlen(msg)],sizeof(msg)-strlen(msg)," (%s)\n",threadmode);
#else
        sprintf(msg,"pid %6d t %d cpu %d, %d: %12.2f %7.2f %7.2f %7.2f  ",
            pid,t,mta->affinity,i,1.0E6/split_run,split_run,split_rundown,split_util);
        for(k = 0; k < (int)sync->cpu.processors; k++) {
            int msglen = strlen(msg);
            sprintf(&msg[msglen]," %6.2f",sync->cpu.split_sum[k+1]/sync->cpu.split_samples[k]);
        }
        sprintf(&msg[strlen(msg)]," (%s)\n",threadmode);
#endif  // (__unix__)
        printf(msg);
    }

    db->process[p].pid = 0;

    // Force memory synchronization
	mrapi_impl_atomic_sync(NULL);

    if(mta->bproc) {
        // Spin waiting for remote process rundown
        while(0 < db->nprocess &&
            0 < db->process[rp].nthread) {
            sys_os_yield();
        }
    }

    assert(mrapi_impl_finalize());
    return 0;
}
