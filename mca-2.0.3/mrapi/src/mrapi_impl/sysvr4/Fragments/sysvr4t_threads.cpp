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

static int gseq = -1;
static shmem_proc_t gpdb = { { 0 } };

void* first_sem(void* args)
{
	sem_args_t* sa = (sem_args_t*) args;
    int affinity = (sa->multicore) ? sa->cpu : 0;
	int lsem = 0;

#if (__unix__)
    cpu_set_t mask = { { 0 } };
    CPU_ZERO(&mask);
    CPU_SET(affinity,&mask);
    assert(0 == sched_setaffinity(0,sizeof(cpu_set_t),&mask));
#endif  // (__unix__)

    gseq = 0;
	assert(gseq == 0);
	assert(sys_sem_get(sa->key,1,&lsem));
#if !(__unix__)
	InterlockedIncrement((unsigned int*)&gseq); // gseq = 1
#else
    __sync_add_and_fetch(&gseq,1);
#endif  // (__unix__)
	assert(gseq == 1);
    while(gseq < 2); // wait for get
	sys_sem_lock(lsem, 0);
#if !(__unix__)
	InterlockedIncrement((unsigned int*)&gseq); // gseq = 3
#else
    __sync_add_and_fetch(&gseq,1);
#endif  // (__unix__)
	assert(gseq == 3);
	sys_sem_unlock(lsem, 0);
    while(gseq < 4); // wait for unlock
#if !(__unix__)
    sys_sem_delete(lsem);
#endif  // !(__unix__)
	assert(gseq == 4);
#if !(__unix__)
	InterlockedIncrement((unsigned int*)&gseq); // gseq = 5
#else
    __sync_add_and_fetch(&gseq,1);
#endif  // (__unix__)
	return 0;
}

void* second_sem(void* args)
{
	sem_args_t* sa = (sem_args_t*) args;
    int affinity = (sa->multicore) ? sa->cpu+1 : 0;
	int lsem = 0;

#if (__unix__)
    cpu_set_t mask = { { 0 } };
    CPU_ZERO(&mask);
    CPU_SET(affinity,&mask);
    assert(0 == sched_setaffinity(0,sizeof(cpu_set_t),&mask));
#endif  // (__unix__)

	while(gseq < 0); // wait for first thread startup
	while(gseq < 1); // wait for create
	assert(gseq == 1);
	sys_sem_get(sa->key,1,&lsem);
#if !(__unix__)
	InterlockedIncrement((unsigned int*)&gseq); // gseq = 2
#else
    __sync_add_and_fetch(&gseq,1);
#endif  // (__unix__)
	assert(gseq == 2);
	while(gseq < 3); // wait for lock
	sys_sem_lock(lsem,0);
	assert(gseq == 3);
	sys_sem_unlock(lsem,0);
#if !(__unix__)
	InterlockedIncrement((unsigned int*)&gseq); // gseq = 4
#else
    __sync_add_and_fetch(&gseq,1);
#endif  // (__unix__)
	assert(gseq == 4);
#if !(__unix__)
    sys_sem_delete(lsem);
#endif  // !(__unix__)
	while(gseq < 5); // wait for delete
	gseq = -1;
	return 0;
}

void* first_shmem(void* args)
{
	shmem_args_t* sa = (shmem_args_t*) args;
    char msg[512] = "";
    int affinity = (sa->multicore) ? sa->cpu : 0;
	int i = 0;
    int j = 0;
    int k = 0;
	int p = 0;
	int t = 0;
	int rp = 0;
	int at = 0;
	int rcount = 0;
	double elapsed = 0.0;
	double split_elapsed = 0.0;
	double util = 0.0;
	shmem_db_t* db = NULL;
    shmem_proc_t* pdb = NULL;
    shmem_sync_t* sync = NULL;
    shmem_sync_t* rsync = NULL;
    mca_timestamp_t ts = { 0 };
    mca_cpu_t cpu = { 0 };

#if (__unix__)
    cpu_set_t mask = { { 0 } };
    CPU_ZERO(&mask);
    CPU_SET(affinity,&mask);
    assert(0 == sched_setaffinity(0,sizeof(cpu_set_t),&mask));
#endif  // (__unix__)

	// Get and attach shared memory
    db = (shmem_db_t*)sa->db;
    assert(NULL != db);

	// Register process
#if !(__unix__)
	p = InterlockedIncrement((unsigned int*)&db->nprocess)-1;
    db->process[p].pid = (pid_t)GetCurrentProcessId();
#else
    p = __sync_add_and_fetch(&db->nprocess,1)-1;
    db->process[p].pid = getpid();
#endif  // (__unix__)

	// Register thread
#if !(__unix__)
	t = InterlockedIncrement((unsigned int*)&db->process[p].nthread)-1;
	db->process[p].thread[t].tid = (pthread_t)GetCurrentThreadId();
#else
    t = __sync_add_and_fetch(&db->process[p].nthread,1)-1;
    db->process[p].thread[t].tid = pthread_self();
#endif  // (__unix__)

	// Get remote process slot
	rp = (0 == p) ? 1 : 0;

	// Get alternate thread slot
	at = (0 == t) ? 1 : 0;

    for(j = 0; j < 3; j++) {
        static char threadmode_rtpshared[] = "rtp-rtp shared";
        static char threadmode_localshared[] = "task-task shared";
        static char threadmode_global[] = "task-task";
        char* threadmode = NULL;

        switch(j) {
        case 0: // Shared memory between processes
            pdb = &db->process[p];
            sync = &db->process[p].sync;
            rsync = &db->process[rp].sync;
            threadmode = threadmode_rtpshared;
            break;
        case 1: // Shared memory between threads
            pdb = &db->process[p];
            sync = &db->process[p].thread[t].sync;
            rsync = &db->process[p].thread[at].sync;
            threadmode = threadmode_localshared;
            break;
        case 2: // Global memory between threads
            pdb = &gpdb;
            pdb->thread[t].tid = db->process[p].thread[t].tid;
            sync = &pdb->thread[t].sync;
            rsync = &pdb->thread[at].sync;
            threadmode = threadmode_global;
            break;
        }

        /* Increment mode count */
#if !(__unix__)
        InterlockedIncrement((unsigned int*)&pdb->nmode[j]);
#else
        __sync_add_and_fetch(&pdb->nmode[j],1);
#endif  // (__unix__)

        if(0 == j && !sa->bproc) {
            // no remote process
            continue;
        }
        if(0 == j && 0 < t) { // Only one thread for rtp-rtp shared
            // Wait until all threads are started
            while(2 > pdb->nmode[j]) {
                sys_os_yield();
            }

            // Wait until primary thread runs down
            while(1 < pdb->nmode[j]) {
                sys_os_yield();
            }

            // Decrement mode count
#if !(__unix__)
            InterlockedDecrement((unsigned int*)&pdb->nmode[j]);
#else
            __sync_add_and_fetch(&pdb->nmode[j],-1);
#endif  // (__unix__)

            continue;
        }

	    // Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

	    // Save remote counter, enable self counter
	    rcount = rsync->ncount;
	    sync->benable = 1;

	    // Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

        // Spin waiting for remote counter enable
        while(!rsync->benable) {
            sys_os_yield();
        }

        // Save beginning time
        mca_begin_cpu(&cpu); /* causes a delay, call before timing start */
        mca_begin_ts(&ts);

        for(i = 0; i < 1000; i++) {

            // Increment self counter
#if !(__unix__)
            InterlockedIncrement((unsigned int*)&sync->ncount);
#else
            __sync_add_and_fetch(&sync->ncount,1);
#endif  // (__unix__)

	        // Force memory synchronization
#if !(__unix__)
            MemoryBarrier();
#else
            __sync_synchronize();
#endif  // (__unix__)

            // Spin waiting for remote counter increment
            while(rcount >= rsync->ncount) {
                sys_os_yield();
            }
            rcount = rsync->ncount;

            // Save beginning split time
            mca_begin_split_ts(&ts);

            // Increment self counter
#if !(__unix__)
            InterlockedIncrement((unsigned int*)&sync->ncount);
#else
            __sync_add_and_fetch(&sync->ncount,1);
#endif  // (__unix__)

	        // Force memory synchronization
#if !(__unix__)
            MemoryBarrier();
#else
            __sync_synchronize();
#endif  // (__unix__)

            // Spin waiting for remote counter increment
            while(rcount >= rsync->ncount) {
                sys_os_yield();
            }
            rcount = rsync->ncount;

            // Compute and save elapsed split time
            (void)mca_end_split_ts(&ts);
        }

        // Increment self counter to allow remote task rundown
#if !(__unix__)
        InterlockedIncrement((unsigned int*)&sync->ncount);
#else
        __sync_add_and_fetch(&sync->ncount,1);
#endif  // (__unix__)

        // Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

        // Compute and print elapsed times
        elapsed = mca_end_ts(&ts);
        util = mca_end_cpu(&cpu); /* causes delay, call after timing measurement */
        split_elapsed = ts.split_sum/ts.split_samples;
#if !(__unix__)
        sprintf_s(msg,sizeof(msg),"pid %d, cpu %d, %d: %7.2f %7.2f %7.2f  ",db->process[p].pid,affinity,i,elapsed,split_elapsed,util);
        for(k = 0; k < (int)cpu.processors; k++) {
            int msglen = strlen(msg);
            sprintf_s(&msg[msglen],sizeof(msg)-msglen," %5.2f",cpu.split_sum[k+1]/cpu.split_samples);
        }
        sprintf_s(&msg[strlen(msg)],sizeof(msg)-strlen(msg)," (%s)\n",threadmode);
#else
        sprintf(msg,"pid %d, cpu %d, %d: %7.2f %7.2f %7.2f  ",db->process[p].pid,affinity,i,elapsed,split_elapsed,util);
        for(k = 0; k < (int)cpu.processors; k++) {
            sprintf(&msg[strlen(msg)]," %5.2f",cpu.split_sum[k+1]/cpu.split_samples[k+1]);
        }
        sprintf(&msg[strlen(msg)]," (%s)\n",threadmode);
#endif  // (__unix__)
        printf(msg);

        // Enable thread rundown
	    sync->benable = 0;

	    // Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

	    // Spin waiting for remote thread rundown
        while(rsync->benable) {
            sys_os_yield();
        }

        // Decrement mode count
#if !(__unix__)
    	InterlockedDecrement((unsigned int*)&pdb->nmode[j]);
#else
        __sync_add_and_fetch(&pdb->nmode[j],-1);
#endif  // (__unix__)

        /* Spin waiting for rest of threads to finish mode */
        while(0 < pdb->nmode[j]) {
            sys_os_yield();
        }
    }

    // Enable thread rundown
#if !(__unix__)
	InterlockedDecrement((unsigned int*)&db->process[p].nthread);
#else
    __sync_add_and_fetch(&db->process[p].nthread,-1);
#endif  // (__unix__)

    // Force memory synchronization
#if !(__unix__)
    MemoryBarrier();
#else
    __sync_synchronize();
#endif  // (__unix__)


	// Spin waiting for alternate thread rundown
    while(0 < db->process[p].nthread) {
        sys_os_yield();
    }

	// Spin waiting for remote thread rundown
    while(0 < db->process[rp].nthread) {
        sys_os_yield();
    }

    // Clean up process resources
#if !(__unix__)
	InterlockedDecrement((unsigned int*)&db->nprocess);
#else
    __sync_add_and_fetch(&db->nprocess,-1);
#endif  // (__unix__)
    memset(&db->process[p],0,sizeof(shmem_proc_t));

    // Force memory synchronization
#if !(__unix__)
    MemoryBarrier();
#else
    __sync_synchronize();
#endif  // (__unix__)

    if(sa->bproc) {
        // Spin waiting for remote process rundown
        while(0 < db->nprocess) {
            sys_os_yield();
        }
    }

	return 0;
}

void* second_shmem(void* args)
{
	shmem_args_t* sa = (shmem_args_t*) args;
    char msg[512] = "";
    int affinity = (sa->multicore) ? sa->cpu+1 : 0;
	int i = 0;
    int j = 0;
    int k = 0;
	int p = 0;
	int t = 0;
	int at = 0;
	int tcount = 0;
	int registered = 0;
	double elapsed = 0.0;
	double split_elapsed = 0.0;
    double util = 0.0;
	pid_t pid = 0;
	shmem_db_t* db = NULL;
    shmem_proc_t* pdb = NULL;
    shmem_sync_t* sync = NULL;
    shmem_sync_t* rsync = NULL;
    mca_timestamp_t ts = { 0 };
    mca_cpu_t cpu = { 0 };

#if (__unix__)
    cpu_set_t mask = { { 0 } };
    CPU_ZERO(&mask);
    CPU_SET(affinity,&mask);
    assert(0 == sched_setaffinity(0,sizeof(cpu_set_t),&mask));
#endif  // (__unix__)

    // Get shared memory
    db = (shmem_db_t*)sa->db;
    assert(NULL != db);

    // Get self process slot
#if !(__unix__)
    pid = (pid_t)GetCurrentProcessId();
#else
    pid = getpid();
#endif  // (__unix__)

    // Wait for self process to be registered
    while(!registered) {
        sys_os_yield();
        for(i = 0; i < 2; i++) {
            if(db->process[i].pid == pid) {
                p = i;
                registered = 1;
                break;
            }
        }
    }

	// Register thread
#if !(__unix__)
	t = InterlockedIncrement((unsigned int*)&db->process[p].nthread)-1;
	db->process[p].thread[t].tid = (pthread_t)GetCurrentThreadId();
#else
    t = __sync_add_and_fetch(&db->process[p].nthread,1)-1;
	db->process[p].thread[t].tid = pthread_self();
#endif  // (__unix__)

	// Get alternate thread slot
	at = (0 == t) ? 1 : 0;

    for(j = 1; j < 3; j++) {
        static char threadmode_local[] = "task-task shared";
        static char threadmode_global[] = "task-task";
        char* threadmode = NULL;
        switch(j) {
        case 1: // shared memory between threads
            pdb = &db->process[p];
            threadmode = threadmode_local;
            break;
        case 2: // global memory between threads
            pdb = &gpdb;
            threadmode = threadmode_global;
            pdb->thread[t].tid = db->process[p].thread[t].tid;
            break;
        }
        sync = &pdb->thread[t].sync;
        rsync = &pdb->thread[at].sync;

        // Increment mode count
#if !(__unix__)
        InterlockedIncrement((unsigned int*)&pdb->nmode[j]);
#else
        __sync_add_and_fetch(&pdb->nmode[j],1);
#endif  // (__unix__)


	    // Save remote counter, enable self counter
	    tcount = rsync->ncount;
	    sync->benable = 1;

	    // Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

	    // Spin waiting for remote counter enable
	    while(!rsync->benable) {
            sys_os_yield();
        }

        // Save beginning time
        mca_begin_cpu(&cpu); /* causes a delay, call before timing start */
        mca_begin_ts(&ts);

        for(i = 0; i < 1000; i++) {
            // Increment self counter
#if !(__unix__)
            InterlockedIncrement((unsigned int*)&sync->ncount);
#else
            __sync_add_and_fetch(&sync->ncount,1);
#endif  // (__unix__)

	        // Force memory synchronization
#if !(__unix__)
            MemoryBarrier();
#else
            __sync_synchronize();
#endif  // (__unix__)

            // Spin waiting for remote counter increment
            while(tcount >= rsync->ncount) {
                sys_os_yield();
            }
            tcount = rsync->ncount;

            // Save beginning split time
            mca_begin_split_ts(&ts);

            // Increment self counter
#if !(__unix__)
            InterlockedIncrement((unsigned int*)&sync->ncount);
#else
            __sync_add_and_fetch(&sync->ncount,1);
#endif  // (__unix__)

	        // Force memory synchronization
#if !(__unix__)
            MemoryBarrier();
#else
            __sync_synchronize();
#endif  // (__unix__)

            // Spin waiting for remote counter increment
            while(tcount >= rsync->ncount) {
                sys_os_yield();
            }
            tcount = rsync->ncount;

            // Compute and save elapsed split time
            (void)mca_end_split_ts(&ts);
        }

        // Increment self counter to allow remote rundown
#if !(__unix__)
        InterlockedIncrement((unsigned int*)&sync->ncount);
#else
        __sync_add_and_fetch(&sync->ncount,1);
#endif  // (__unix__)

    	// Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

        // Compute and print elapsed times
        elapsed = mca_end_ts(&ts);
        util = mca_end_cpu(&cpu); /* causes delay, call after timing measurement */
        split_elapsed = ts.split_sum/ts.split_samples;
#if !(__unix__)
        sprintf_s(msg,sizeof(msg),"pid %d, cpu %d, %d: %7.2f %7.2f %7.2f  ",db->process[p].pid,affinity,i,elapsed,split_elapsed,util);
        for(k = 0; k < (int)cpu.processors; k++) {
            int msglen = strlen(msg);
            sprintf_s(&msg[msglen],sizeof(msg)-msglen," %5.2f",cpu.split_sum[k+1]/cpu.split_samples);
        }
        sprintf_s(&msg[strlen(msg)],sizeof(msg)-strlen(msg)," (%s)\n",threadmode);
#else
        sprintf(msg,"pid %d, cpu %d, %d: %7.2f %7.2f %7.2f  ",db->process[p].pid,affinity,i,elapsed,split_elapsed,util);
        for(k = 0; k < (int)cpu.processors; k++) {
            sprintf(&msg[strlen(msg)]," %5.2f",cpu.split_sum[k+1]/cpu.split_samples[k+1]);
        }
        sprintf(&msg[strlen(msg)]," (%s)\n",threadmode);
#endif  // (__unix__)
        printf(msg);

        // Enable thread rundown
#if !(__unix__)
    	InterlockedDecrement((unsigned int*)&pdb->nthread);
#else
        __sync_add_and_fetch(&pdb->nthread,-1);
#endif  // (__unix__)
	    pdb->thread[t].sync.benable = 0;

	    // Force memory synchronization
#if !(__unix__)
        MemoryBarrier();
#else
        __sync_synchronize();
#endif  // (__unix__)

	    // Spin waiting for remote thread rundown
        while(pdb->thread[at].sync.benable) {
            sys_os_yield();
        }

        // Decrement mode count
#if !(__unix__)
    	InterlockedDecrement((unsigned int*)&pdb->nmode[j]);
#else
        __sync_add_and_fetch(&pdb->nmode[j],-1);
#endif  // (__unix__)

        // Spin waiting for rest of threads to finish mode
        while(0 < pdb->nmode[j]) {
            sys_os_yield();
        }
    }

	return 0;
}
