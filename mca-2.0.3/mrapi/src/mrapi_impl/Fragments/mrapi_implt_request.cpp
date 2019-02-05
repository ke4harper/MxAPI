	// Requests
	{
		const int max = 10;
		int i = 0;
		uint32_t r[max] = { 0 };
		mrapi_request_t request[max] = { 0 };

		assert(mrapi_impl_get_domain_num(&d_num));
		assert(mrapi_impl_get_node_num(&n_num));
		for(i = 0; i < max; i++)
		{
			r[i] = mrapi_impl_setup_request();
			assert(MRAPI_TRUE == mrapi_db->requests[r[i]].valid);
			assert(d_num == mrapi_db->requests[r[i]].domain_id);
			assert(n_num == mrapi_db->requests[r[i]].node_num);
			assert(MRAPI_FALSE == mrapi_db->requests[r[i]].cancelled);
			assert(MRAPI_FALSE == mrapi_db->requests[r[i]].completed);
			mrapi_db->requests[r[i]].completed = MRAPI_TRUE;	// Non-blocking MRAPI operations complete immediately?
			request[i] = mrapi_impl_encode_hndl(r[i]);
			assert(0 != request[i]);
			assert(mrapi_impl_valid_request_hndl(&request[i]));
			assert(!mrapi_impl_canceled_request(&request[i]));
			assert(mrapi_impl_test(&request[i],&status));	// Releases request regardless of state
			assert(MRAPI_FALSE == mrapi_db->requests[r[i]].valid);
		}
	}
