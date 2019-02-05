	mca_set_debug_level(0);

    // Runtime stress tests
    {
        int rc = 0;
#if !(__unix__||__MINGW32__)
		DWORD* tid = (DWORD*)malloc(num_nodes*sizeof(DWORD));
#endif  /* !(__unix__||__MINGW32__) */
		pthread_t* threads = (pthread_t*)malloc(num_nodes*sizeof(pthread_t));
        mrapi_test_args_t* mta = (mrapi_test_args_t*)malloc(num_nodes*sizeof(mrapi_test_args_t));

        int mutex_key = 0;
        mrapi_mutex_hndl_t mutex_id = 0;
        mrapi_mutex_attributes_t mutex_attributes = { 0 };
        mrapi_key_t lock_key = 0;
        mrapi_shmem_id_t shmem_key = 0;
        mrapi_shmem_hndl_t shmem_id = 0;
        mrapi_shmem_attributes_t shmem_attributes = { 0 };
        mrapi_test_db_t* db = NULL;

        if(0 == d_offset) {
            assert(sys_file_key(NULL,'h',&mutex_key));
            mrapi_impl_mutex_init_attributes(&mutex_attributes);
            assert(mrapi_impl_mutex_create(&mutex_id,mutex_key,&mutex_attributes,&status));
        }
        else {
		    // mutex created by parent process
            assert(sys_file_key(NULL,'a',&mutex_key));
        }

        if(0 == d_offset) {
            assert(sys_file_key(NULL,'i',&shmem_key));
            mrapi_impl_shmem_init_attributes(&shmem_attributes);
            status = MRAPI_SUCCESS;
            mrapi_impl_shmem_create(&shmem_id,shmem_key,sizeof(mrapi_test_db_t),&shmem_attributes,&status);
            assert(MRAPI_SUCCESS == status);
        }
        else {
		    // shared memory created by parent process
            assert(sys_file_key(NULL,'b',&shmem_key));
            assert(mrapi_impl_shmem_get(&shmem_id,shmem_key));
        }

        db = (mrapi_test_db_t*)mrapi_impl_shmem_attach(shmem_id); // Hold a reference count
        assert(NULL != db);

        // Allow other process to initialize
        sys_os_yield();

        for(int i = 0; i < 1; i++) { // one overall test run
        for(int j = 0; j < 2; j++)    // one set on single core, one set with each node on a separate core
        for(int k = 0; k < 5; k++)   // five iterations for selected configuration
	    {
            printf("domain %d node %d stress %d %d %d iterations: /sec; elapsed(usec) rundown(usec) /iteration; util(%%), cpus(%%)\n",
                d_offset + 1, n_offset + 1, i, j, k);

#if !(__unix__||__MINGW32__)
            memset(tid,0,num_nodes*sizeof(DWORD));
#endif  /* !(__unix__||__MINGW32__) */
            memset(threads,0,num_nodes*sizeof(pthread_t));
            memset(mta,0,num_nodes*sizeof(mrapi_test_args_t));

            barrier = 0;

            for(int ii = 0; ii < num_nodes; ii++) {
                mta[ii].bproc = (1 <= d_offset);
                mta[ii].affinity = (0 == j) ? 0 : ii;
                mta[ii].num_nodes = num_nodes;
                mta[ii].domain = d_offset+1;
                mta[ii].node = n_offset+ii+2; // main thread is node 1
                mta[ii].mutex_key = mutex_key;
                mta[ii].shmem_id = shmem_id;
                mta[ii].db = db;
            }

#if !(__unix__||__MINGW32__)
            for(int ii = 0; ii < num_nodes; ii++) {
		        threads[ii] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)client, &mta[ii], CREATE_SUSPENDED, &tid[ii]);
#ifdef NOTUSED
		        SetThreadAffinityMask(threads[ii], 1<<mta[ii].affinity);
#endif  // NOTUSED
            }
            for(int ii = 0; ii < num_nodes; ii++) {
		        ResumeThread(threads[ii]);
            }
		    WaitForMultipleObjects(num_nodes, threads, TRUE, INFINITE);
#else
            for(int ii = 0; ii < num_nodes; ii++) {
		        rc += pthread_create(&threads[ii], NULL, client, (void*)&mta[ii]);
            }
            for(int ii = 0; ii < num_nodes; ii++) {
		        pthread_join(threads[ii], NULL);
            }
#endif  /* (__unix__||__MINGW32__) */
        }
	    }

        assert(mrapi_impl_shmem_detach(shmem_id)); // release reference count
        if(0 == d_offset) {
            assert(mrapi_impl_mutex_lock(mutex_id,&lock_key,MRAPI_TIMEOUT_INFINITE,&status));
            assert(mrapi_impl_mutex_delete(mutex_id));
            assert(mrapi_impl_shmem_delete(shmem_id));
        }

#if !(__unix__||__MINGW32__)
        free((void*)tid);
#endif  /* !(__unix__||__MINGW32__) */
        free((void*)threads);
        free((void*)mta);
    }
