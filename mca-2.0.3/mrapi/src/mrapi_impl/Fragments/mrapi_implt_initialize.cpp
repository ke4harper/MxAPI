	// Invalid operations without initialization
	{
		int key = 1;
		int num_locks = 1;
		mrapi_sem_id_t sem_id = 0;

        assert(!mrapi_impl_initialized());
		n_num = mrapi_impl_node_id_get(&status);
        assert((mrapi_node_t)-1 == n_num);
		assert(MRAPI_SUCCESS != status);
		assert(!mrapi_impl_get_domain_num(&d_num));
		assert(!mrapi_impl_whoami(&n_num,&n_index,&d_num,&d_index));
        // System semaphores do not require initialization
		assert(mrapi_impl_create_sys_semaphore(&sem_id,num_locks,key,MRAPI_TRUE));
		assert(sys_sem_delete(sem_id));
    }

	// Runtime initialization
	{

		// One node
		d_num = d_offset + 1;
		n_num = n_offset + 1;
        assert(NULL == mrapi_db);
		assert(mrapi_impl_initialize(d_num,n_num,&status));
		assert(mrapi_impl_whoami(&n_num,&n_index,&d_num,&d_index));
        assert(NULL != mrapi_db);
		assert(d_num == mrapi_db->domains[d_index].domain_id);
        assert(0 == mrapi_db->num_domains); // num_domains not used
		assert(MRAPI_TRUE == mrapi_db->domains[d_index].valid);
		assert(0 < mrapi_db->domains[d_index].num_nodes);
		assert(MRAPI_TRUE == mrapi_db->domains[d_index].nodes[n_index].valid);
		assert(n_num == mrapi_db->domains[d_index].nodes[n_index].node_num);
#if !(__unix__)
		assert(GetCurrentProcessId() == mrapi_db->domains[d_index].nodes[n_index].pid);
		assert((pthread_t)GetCurrentThreadId() == mrapi_db->domains[d_index].nodes[n_index].tid);
#else
		assert(getpid() == mrapi_db->domains[d_index].nodes[n_index].pid);
		assert(pthread_self() == mrapi_db->domains[d_index].nodes[n_index].tid);
#endif  // (__unix__)
		assert(mrapi_impl_initialized());
		assert(!mrapi_impl_initialize(d_num,n_num,&status)); // Error to initialize duplicate node on same thread
		assert(mrapi_impl_finalize());
        assert(NULL == mrapi_db);
		assert(mrapi_impl_initialize(d_num,n_num,&status));
		assert(mrapi_impl_finalize());
	}
