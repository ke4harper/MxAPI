	// Create semaphore
	{
		int id = 0;
#if !(__unix__)
		sem_set_t* ss = NULL;
#endif  // !(__unix__)
		int key = 0;
        int created = 0;
		assert(sys_file_key(NULL,'d',&key));
		// Empty set
		assert(!sys_sem_create(key,0,&id));
		// Single semaphore
        created = 0;
		if(!sys_sem_get(key,1,&id)) { // race with other process?
            created = 1;
            assert(sys_sem_create(key,1,&id));
        }
		assert(0 != id);
#if !(__unix__)
		ss = (sem_set_t*)id;
		assert(1 == ss->num_locks);
		assert(NULL != ss->sem);
		assert(NULL != ss->sem[0]);
#endif  // !(__unix__)
        if(created) {
            assert(sys_sem_delete(id)); // clean up Windows resources,
        }                               // possible Unix error is ignored
        else {
            assert(sys_sem_release(id));
        }
		// Semaphore set
        created = 0;
		if(!sys_sem_get(key+2,2,&id)) { // race with other process?
            created = 1;
            assert(sys_sem_create(key+1,2,&id));
        }
		assert(0 != id);
#if !(__unix__)
		ss = (sem_set_t*)id;
		assert(2 == ss->num_locks);
		assert(NULL != ss->sem);
		assert(NULL != ss->sem[0]);
		assert(NULL != ss->sem[1]);
#endif  // !(__unix__)
        if(created) {
            assert(sys_sem_delete(id)); // clean up Windows resources,
        }                               // possible Unix error is ignored
        else {
            assert(sys_sem_release(id));
        }
	}

	// Get semaphore
	{
		const int num_locks = 2;
		int id1 = 0;
		int id2 = 0;
		int key = 0;
        int created  = 0;
		assert(sys_file_key(NULL,'e',&key));
#if !(__unix__)
		int i = 0;
		sem_set_t* ss1 = NULL;
		sem_set_t* ss2 = NULL;
#endif  // !(__unix__)
        created = 0;
        if(!sys_sem_get(key,num_locks,&id1)) { // race with other process?
            created = 1;
            assert(sys_sem_create(key,num_locks,&id1));
        }
#if !(__unix__)
		ss1 = (sem_set_t*)id1;
#endif  // !(__unix__)
		assert(sys_sem_get(key,num_locks,&id2));
#if !(__unix__)
		ss2 = (sem_set_t*)id2;
		assert(ss1 != ss2);
		for(i = 0; i < num_locks; i++)
		{
			assert(NULL != ss2->sem[i]);
		}
#else
        assert(id2 == id1);
#endif  // (__unix__)
        if(created) {
            assert(sys_sem_delete(id1)); // clean up Windows resources,
        }
        else {
            assert(sys_sem_release(id1));
        }
		assert(sys_sem_release(id2));
	}

	// Duplicate semaphore
	{
		const int num_locks = 2;
		int id1 = 0;
		int id2 = 0;
		int key = 0;
        int created  = 0;
        int pproc = 0;
		assert(sys_file_key(NULL,'f',&key));
#if !(__unix__)
		int i = 0;
		sem_set_t* ss1 = NULL;
		sem_set_t* ss2 = NULL;
        pproc = (int)GetCurrentProcess();
#endif  // !(__unix__)
        created = 0;
        if(!sys_sem_get(key,num_locks,&id1)) { // race with other process?
            created = 1;
            assert(sys_sem_create(key,num_locks,&id1));
        }
#if !(__unix__)
		ss1 = (sem_set_t*)id1;
#endif  // !(__unix__)
		assert(sys_sem_duplicate(pproc,id1,&id2));
#if !(__unix__)
		ss2 = (sem_set_t*)id2;
		assert(ss1 != ss2);
		for(i = 0; i < num_locks; i++)
		{
			assert(NULL != ss2->sem[i]);
		}
#else
        assert(id2 == id1);
#endif  // (__unix__)
        if(created) {
            assert(sys_sem_delete(id1)); // clean up Windows resources,
        }
        else {
            assert(sys_sem_release(id1));
        }
		assert(sys_sem_release(id2));
	}

	// Lock and unlock semaphore
	{
		const int num_locks = 1;
		int id = 0;
		int key = 0;
        int created = 0;
		assert(sys_file_key(NULL,'g',&key));
        created = 0;
		if(!sys_sem_get(key,num_locks,&id)) { // race with other process?
            created = 1;
            assert(sys_sem_create(key,num_locks,&id));
        }
		assert(sys_sem_trylock(id,0));
		assert(!sys_sem_trylock(id,0));
		assert(sys_sem_unlock(id,0));
		assert(sys_sem_lock(id,0));
		assert(sys_sem_unlock(id,0));
        if(created) {
            assert(sys_sem_delete(id)); // clean up Windows resources,
        }                               // Unix error is ignored
        else {
            assert(sys_sem_release(id));
        }
	}
