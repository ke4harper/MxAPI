	// Get, create, duplicate shared memory
	{
		uint32_t id1 = 0;
		uint32_t id2 = 0;
		uint32_t id3 = 0;
		int key = 0;
        int created = 0;
        int proc = 0;
		assert(sys_file_key(NULL,'h',&key));
        created = 0;
		if((uint32_t)-1 == (id1 = sys_shmem_get(key,10))) { // race with other process?
            created = 1;
            id1 = sys_shmem_create(key,10);
		}
		assert(0 < id1);
		id2 = sys_shmem_get(key,10);
		assert(0 < id2);
#if !(__unix__)
		assert(id1 != id2);
#else
		assert(id1 == id2);
#endif  // (__unix__)
#if !(__unix__)
        proc = (int)GetProcessId(GetCurrentProcess());
#endif  // !(__unix__)
		assert(sys_shmem_duplicate(id2,proc,&id3));
		assert(0 < id3);
#if !(__unix__)
		assert(id2 != id3);
#else
		assert(id2 == id3);
#endif  // (__unix__)
        if(created) {
            assert(sys_shmem_delete(id1));
        }
        else {
            assert(sys_shmem_release(id1)); 
        }
		assert(sys_shmem_release(id2));
		assert(sys_shmem_release(id3));
	}

	// Attach, detach shared memory
	{
		uint32_t id = 0;
		void* addr1 = NULL;
		void* addr2 = NULL;
		int key = 0;
		assert(sys_file_key(NULL,'i',&key));
		if((uint32_t)-1 == (id = sys_shmem_get(key,10))) { // race with other process?
            id = sys_shmem_create(key,10);
		}
		assert(0 < id);
		addr1 = sys_shmem_attach(id);
		assert(NULL != addr1);
		addr2 = sys_shmem_attach(id);
		assert(NULL != addr2);
		assert(addr1 != addr2);
		assert(sys_shmem_detach(addr1));
		assert(sys_shmem_detach(addr2));
        assert(sys_shmem_delete(id)); // clean up Windows resources,
                                      // possible Unix error is ignored
	}
