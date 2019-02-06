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

// Semaphore synchronization from different threads on one CPU
	{
        int rc = 0;
        int i = 0;
        int lsem = 0;
        pthread_t threads[2];
#if !(__unix||__MINGW32__)
		DWORD tid[2] = { 0 };
#endif  /* !(__unix||__MINGW32__) */

		sem_args_t sa = { 0 };
		if(!bproc) {
            assert(sys_file_key(NULL,'k',&sa.key));
            assert(sys_sem_create(sa.key,1,&lsem));
		}
		else {
		    // semaphore created by parent process
            assert(sys_file_key(NULL,'a',&sa.key));
		}
		sa.multicore = 0;
        sa.cpu = 0;

#if (__unix||__MINGW32__)
        rc += pthread_create(&threads[0], NULL, first_sem,(void*)&sa);
        rc += pthread_create(&threads[1], NULL, second_sem,(void*)&sa);

        for (i = 0; i < 2; i++) {
            pthread_join(threads[i],NULL);
        }
#else
		threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)first_sem, &sa, CREATE_SUSPENDED, &tid[0]);
		threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)second_sem, &sa, CREATE_SUSPENDED, &tid[1]);
		SetThreadAffinityMask(threads[0], 0x1);
		SetThreadAffinityMask(threads[1], 0x1);
		ResumeThread(threads[0]);
		ResumeThread(threads[1]);
		WaitForMultipleObjects(2, threads, TRUE, INFINITE);
#endif  /* !(__unix||__MINGW32__) */

        if(!bproc) {
            assert(sys_sem_delete(lsem));
        }
	}

	// Semaphore synchronization from different threads on two CPUs
	{
        int rc = 0;
        int i = 0;
        int lsem = 0;
        int cpu = 0;
        pthread_t threads[2];
#if !(__unix||__MINGW32__)
		DWORD tid[2] = { 0 };
#endif  /* !(__unix||__MINGW32__) */

        sem_args_t sa = { 0 };
		if(!bproc) {
            assert(sys_file_key(NULL,'l',&sa.key));
            assert(sys_sem_create(sa.key,1,&lsem));
		}
		else {
		    // semaphore created by parent process
            assert(sys_file_key(NULL,'a',&sa.key));
            cpu = (nproc/2)*2;
		}
		sa.multicore = 1;
        sa.cpu = cpu;

#if (__unix||__MINGW32__)
        rc += pthread_create(&threads[0], NULL, first_sem,(void*)&sa);
        rc += pthread_create(&threads[1], NULL, second_sem,(void*)&sa);

        for (i = 0; i < 2; i++) {
            pthread_join(threads[i],NULL);
        }
#else
		threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)first_sem, &sa, CREATE_SUSPENDED, &tid[0]);
		threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)second_sem, &sa, CREATE_SUSPENDED, &tid[1]);
		SetThreadAffinityMask(threads[0], 1<<cpu);
		SetThreadAffinityMask(threads[1], 1<<(cpu+1));
		ResumeThread(threads[0]);
		ResumeThread(threads[1]);
		WaitForMultipleObjects(2, threads, TRUE, INFINITE);
#endif  /* !(__unix||__MINGW32__) */

        if(!bproc) {
            assert(sys_sem_delete(lsem));
        }
	}

	// Shared memory synchronization from different threads on one CPU,
    // once with shared memory and then again with duplicated shared memory
    for(int i = 0; i < 5; i++) {
    for(int j = 0; j < 2; j++)
	{
        int rc = 0;
        int shmem_key = 0;
        uint32_t id = 0;
        uint32_t id_dup = 0;
        shmem_db_t* db = NULL;
        shmem_xchg_t* xchg = NULL;
        pthread_t threads[2];
#if !(__unix||__MINGW32__)
		DWORD tid[2] = { 0 };
#else
        int t = 0;
#endif  /* (__unix||__MINGW32__) */

        printf("nproc %d iter %d single CPU start: total(usec), split(usec), util(%%), cpus(%%)\n",nproc,i);

		shmem_args_t sa = { 0 };
		sa.bproc = bproc;
		sa.nproc = nproc;
		if(!bproc) {
            assert(sys_file_key(NULL,'m',&shmem_key));
            id = sys_shmem_create(shmem_key,sizeof(shmem_db_t));
            assert(0 < (int)id);
            db = (shmem_db_t*)sys_shmem_attach(id);
            assert(NULL != db);
            memset(db,0,sizeof(shmem_db_t));
		}
		else {
            switch(j) {
            case 0: // Shared memory created by parent process
                assert(sys_file_key(NULL,'b',&shmem_key));
                id = sys_shmem_get(shmem_key,sizeof(shmem_db_t));
                assert(0 < (int)id);
                db = (shmem_db_t*)sys_shmem_attach(id);
                assert(NULL != db);
                break;
            case 1: // Duplicated shared memory created by parent process
                assert(sys_file_key(NULL,'c',&shmem_key));
                id = sys_shmem_get(shmem_key,sizeof(shmem_xchg_t));
                assert(0 < (int)id);
                xchg = (shmem_xchg_t*)sys_shmem_attach(id);
                assert(NULL != xchg);

                // Get shared memory from parent
#if !(__unix__)
                xchg->proc[nproc-1] = (int)GetProcessId(GetCurrentProcess());
#else
                xchg->proc[nproc-1] = getpid();
#endif  // (__unix__)
                while(0 == xchg->shmid[nproc-1]) {
                    sys_os_yield();
                }

                // Duplicate shared memory for use in this process
                assert(sys_shmem_duplicate(xchg->shmid[nproc-1],xchg->proc[nproc-1],&id_dup));
                db = (shmem_db_t*)sys_shmem_attach(id_dup);
                assert(NULL != db);
                assert(sys_shmem_detach(xchg));
                assert(sys_shmem_release(shmem_key));
                break;
            }
		}
		sa.multicore = 0;
		sa.cpu = 0;
		sa.db = db;

#if (__unix||__MINGW32__)
        rc += pthread_create(&threads[0], NULL, first_shmem,(void*)&sa);
        rc += pthread_create(&threads[1], NULL, second_shmem,(void*)&sa);

        for (t = 0; t < 2; t++) {
            pthread_join(threads[t],NULL);
        }
#else
		threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)first_shmem, &sa, CREATE_SUSPENDED, &tid[0]);
		threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)second_shmem, &sa, CREATE_SUSPENDED, &tid[1]);
		SetThreadAffinityMask(threads[0], 0x1);
		SetThreadAffinityMask(threads[1], 0x1);
		ResumeThread(threads[0]);
		ResumeThread(threads[1]);
		WaitForMultipleObjects(2, threads, TRUE, INFINITE);
#endif  /* !(__unix||__MINGW32__) */

        printf("nproc %d iter %d single CPU end\n",nproc,j);

        assert(sys_shmem_detach(db));
        if(!bproc) {
            assert(sys_shmem_delete(id));
        }
        else {
            assert(sys_shmem_release(id));
            switch(j) {
            case 1:
                assert(sys_shmem_release(id_dup));
                break;
            }
        }
	}
    }

	// Shared memory synchronization from different threads on two CPUs,
    // once with shared memory and then again with duplicated shared memory
    for(int i = 0; i < 5; i++) {
    for(int j = 0; j < 2; j++)
	{
        int rc = 0;
        int cpu = 0;
        int shmem_key = 0;
        uint32_t id = 0;
        uint32_t id_dup = 0;
        shmem_db_t* db = NULL;
        shmem_xchg_t* xchg = NULL;
        pthread_t threads[2];
#if !(__unix||__MINGW32__)
		DWORD tid[2] = { 0 };
#else
        int t = 0;
#endif  /* (__unix||__MINGW32__) */

        printf("nproc %d iter %d multiple CPU start: total(usec), split(usec), util(%%), cpus(%%)\n",nproc,i);

        shmem_args_t sa = { 0 };
		sa.bproc = bproc;
		sa.nproc = nproc;
		if(!bproc) {
            assert(sys_file_key(NULL,'n',&shmem_key));
            id = sys_shmem_create(shmem_key,sizeof(shmem_db_t));
            assert(0 < (int)id);
            db = (shmem_db_t*)sys_shmem_attach(id);
            assert(NULL != db);
            memset(db,0,sizeof(shmem_db_t));
		}
		else {
            switch(j) {
            case 0: // Shared memory created by parent process
                assert(sys_file_key(NULL,'b',&shmem_key));
                id = sys_shmem_get(shmem_key,sizeof(shmem_db_t));
                assert(0 < (int)id);
                db = (shmem_db_t*)sys_shmem_attach(id);
                assert(NULL != db);
                break;
            case 1: // Duplicated shared memory created by parent process
                assert(sys_file_key(NULL,'c',&shmem_key));
                id = sys_shmem_get(shmem_key,sizeof(shmem_xchg_t));
                assert(0 < (int)id);
                xchg = (shmem_xchg_t*)sys_shmem_attach(id);
                assert(NULL != xchg);

                // Get shared memory from parent
#if !(__unix__)
                xchg->proc[nproc-1] = (int)GetProcessId(GetCurrentProcess());
#else
                xchg->proc[nproc-1] = getpid();
#endif  // (__unix__)
                while(0 == xchg->shmid[nproc-1]) {
                    sys_os_yield();
                }

                // Duplicate shared memory for use in this process
                assert(sys_shmem_duplicate(xchg->shmid[nproc-1],xchg->proc[nproc-1],&id_dup));
                db = (shmem_db_t*)sys_shmem_attach(id_dup);
                assert(NULL != db);
                assert(sys_shmem_detach(xchg));
                assert(sys_shmem_release(shmem_key));
                break;
            }
            cpu = (nproc/2)*2;
		}
		sa.multicore = 1;
		sa.cpu = cpu;
		sa.db = db;

#if (__unix||__MINGW32__)
        rc += pthread_create(&threads[0], NULL, first_shmem,(void*)&sa);
        rc += pthread_create(&threads[1], NULL, second_shmem,(void*)&sa);

        for (t = 0; t < 2; t++) {
            pthread_join(threads[t],NULL);
        }
#else
		threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)first_shmem, &sa, CREATE_SUSPENDED, &tid[0]);
		threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)second_shmem, &sa, CREATE_SUSPENDED, &tid[1]);
#ifdef NOTUSED
		SetThreadAffinityMask(threads[0], 1<<cpu);
		SetThreadAffinityMask(threads[1], 1<<(cpu+1));
#endif  // NOTUSED
		ResumeThread(threads[0]);
		ResumeThread(threads[1]);
		WaitForMultipleObjects(2, threads, TRUE, INFINITE);
#endif  /* !(__unix||__MINGW32__) */

        printf("nproc %d iter %d multiple CPU end\n",nproc,j);

        assert(sys_shmem_detach(db));
        if(!bproc) {
            assert(sys_shmem_delete(id));
        }
        else {
            assert(sys_shmem_release(id));
            switch(j) {
            case 1:
                assert(sys_shmem_release(id_dup));
                break;
            }
        }
    }
    }
