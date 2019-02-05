    int bproc = (1 <= d_offset);
    int d = d_offset+1;
    int n = n_offset+1;  // Main thread
#if (__unix__)
    int run;
    pthread_cond_t cv_init = PTHREAD_COND_INITIALIZER;
    pthread_cond_t cv;
    pthread_mutex_t sync_init = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t sync;
#endif  // (__unix__)
	mcapi_test_args_t mta[3] = {
#if !(__unix__)
        { bproc, mode, NULL, d, n+1, 0, (n < 10)?0x1:0x8 },
        { bproc, mode, NULL, d, n+2, 0, (n < 10)?0x2:0x4 },
        { bproc, mode, NULL, d, n+3, 0, (n < 10)?0x4:0x2 }
#else  // (__unix__)
        { bproc, &cv, &sync, &run, mode, NULL, d, n+1, 0, (n < 10)?0x1:0x8 },
        { bproc, &cv, &sync, &run, mode, NULL, d, n+2, 0, (n < 10)?0x2:0x4 },
        { bproc, &cv, &sync, &run, mode, NULL, d, n+3, 0, (n < 10)?0x4:0x2 }
#endif  // (__unix__)
    };

    // Create configuration copies for threads
    for(int i = 0; i < 3; i++) {
        FILE* fp = NULL;
        mxml_node_t* root = NULL;
#if (__unix__||__MINGW32__)
        if((fp = fopen(xml, "rb")) == NULL) {
            perror(xml);
#else
        if(0 != _wfopen_s(&fp,xml, L"rb")) {
            _wperror(xml);
#endif  // !(__unix__||__MINGW32__)
            return 0;
        }
        else
        {
            root = mxmlLoadFile(NULL, fp, NULL);
            fclose(fp);
        }
        mta[i].root = root;
    }

    // Runtime stress tests
    for(mta[0].iteration = mta[1].iteration = mta[2].iteration = 1;
        mta[0].iteration <= 10;
        mta[0].iteration = mta[1].iteration = ++mta[2].iteration) {
    mta[0].affinity = mta[1].affinity = mta[2].affinity = 0;
    mta[0].multicore = mta[1].multicore = mta[2].multicore = 0;
//#ifdef NOTUSED
    for(mta[0].sample = mta[1].sample = mta[2].sample = 1;
        mta[0].sample <= 3;
        mta[0].sample = mta[1].sample = ++mta[2].sample)
	// Concurrent initialization and messaging with nodes from different threads on one CPU
	{
        /*
        typedef struct
        {
            mcapi_uint_t mode;
            mxml_node_t* root;
            mcapi_domain_t domain;
            mcapi_node_t node;
            int affinity;
            int multicore;
            int iteration;
            int sample;
        } mcapi_test_args_t;
        */
#if !(__unix__||__MINGW32__)
		DWORD tid[3] = { 0 };
#endif  // !(__unix__||__MINGW32__)
		pthread_t threads[3] = { 0 };

#if !(__unix__||__MINGW32__)
		threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)node1_init, &mta[0], CREATE_SUSPENDED, &tid[0]);
		threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)node2_init, &mta[1], CREATE_SUSPENDED, &tid[1]);
		threads[2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)node3_init, &mta[2], CREATE_SUSPENDED, &tid[2]);
        for(int ii = 0; ii < 3; ii++) {
            SetThreadAffinityMask(threads[ii], 0x1);
        }
        for(int ii = 0; ii < 3; ii++) {
            ResumeThread(threads[ii]);
        }
		WaitForMultipleObjects(3, threads, TRUE, INFINITE);
#else
        cv = cv_init;
        sync = sync_init;
        run = 0;

        rc = rc;
        rc = pthread_create(&threads[0], NULL, node1_init, (void*)&mta[0]);
        rc = pthread_create(&threads[1], NULL, node2_init, (void*)&mta[1]);
        rc = pthread_create(&threads[2], NULL, node3_init, (void*)&mta[2]);

        usleep(100000);
        rc = pthread_mutex_lock(&sync);
        run = 1;
        rc = pthread_cond_broadcast(&cv);
        rc = pthread_mutex_unlock(&sync);

        // Wait for thread rundown
        for(int ii = 0; ii < 3; ii++) {
            pthread_join(threads[ii], NULL);
        }

        pthread_cond_destroy(&cv);
        pthread_mutex_destroy(&sync);
#endif  // (__unix__||__MINGW32__)
	}
//#endif  // NOTUSED
//#ifdef NOTUSED
	for(int i = 0; i < 3; i++) {
	    if(10 > n) {
	        mta[i].affinity = i;
	    }
	    else {
            mta[i].affinity = 2-i;
	    }
	}
    mta[0].multicore = mta[1].multicore = mta[2].multicore = 1;
    for(mta[0].sample = mta[1].sample = mta[2].sample = 1;
        mta[0].sample <= 3;
        mta[0].sample = mta[1].sample = ++mta[2].sample)
	// Concurrent initialization and messaging with nodes from different threads on multiple CPUs
	{
#if !(__unix__||__MINGW32__)
		DWORD tid[3] = { 0 };
#endif  // !(__unix__||__MINGW32__)
		pthread_t threads[3] = { 0 };

#if !(__unix__||__MINGW32__)
		threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)node1_init, &mta[0], CREATE_SUSPENDED, &tid[0]);
		threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)node2_init, &mta[1], CREATE_SUSPENDED, &tid[1]);
		threads[2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)node3_init, &mta[2], CREATE_SUSPENDED, &tid[2]);
#ifdef NOTUSED
        for(int ii = 0; ii < 3; ii++) {
            SetThreadAffinityMask(threads[ii], 1<<mta[ii].affinity);
        }
#endif  // NOTUSED
        for(int ii = 0; ii < 3; ii++) {
            ResumeThread(threads[ii]);
        }
		WaitForMultipleObjects(3, threads, TRUE, INFINITE);
#else
        cv = cv_init;
        sync = sync_init;
        run = 0;

        rc = rc;
        rc = pthread_create(&threads[0], NULL, node1_init, (void*)&mta[0]);
        rc = pthread_create(&threads[1], NULL, node2_init, (void*)&mta[1]);
        rc = pthread_create(&threads[2], NULL, node3_init, (void*)&mta[2]);

        usleep(100000);
        rc = pthread_mutex_lock(&sync);
        run = 1;
        rc = pthread_cond_broadcast(&cv);
        rc = pthread_mutex_unlock(&sync);

        // Wait for thread rundown
        for(int ii = 0; ii < 3; ii++) {
            pthread_join(threads[ii], NULL);
        }

        pthread_cond_destroy(&cv);
        pthread_mutex_destroy(&sync);
#endif  // (__unix__||__MINGW32__)
	}
//#endif  // NOTUSED
    }

    // Release configuration copies
    for(int i = 0; i < 3; i++) {
        mxmlDelete(mta[i].root);
    }
