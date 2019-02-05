	// Reader / writer locks
	{
		mrapi_rwl_id_t key = 0;
		uint16_t r_index = 0;
		mrapi_rwl_hndl_t rwl1 = 0;
		mrapi_rwl_hndl_t rwl2 = 0;
		mca_boolean_t attribute = MRAPI_FALSE;
		mrapi_rwl_attributes_t attributes = { 0 };
		mrapi_uint32_t reader_lock_limit = 0;
		mrapi_rwl_mode_t mode = MRAPI_RWL_READER;

		assert(mrapi_impl_whoami(&n_num,&n_index,&d_num,&d_index));

        assert(sys_file_key(NULL,'f',&key));

		// Mutex attributes
		memset(&attributes,0,sizeof(attributes));
		mrapi_impl_rwl_init_attributes(&attributes);
		assert(MRAPI_FALSE == attributes.ext_error_checking);
		assert(MRAPI_TRUE == attributes.shared_across_domains);
		attribute = MRAPI_TRUE;
		mrapi_impl_rwl_set_attribute(&attributes,MRAPI_ERROR_EXT,&attribute,sizeof(attribute),&status);
		assert(MRAPI_FALSE == status);	// status only set if rwl_set_attribute returns an error
		attribute = MRAPI_FALSE;
		mrapi_impl_rwl_set_attribute(&attributes,MRAPI_DOMAIN_SHARED,&attribute,sizeof(attribute),&status);

		// Implementation layer reader / writer lock
		reader_lock_limit = 2;
		assert(mrapi_impl_rwl_create(&rwl1,key,&attributes,reader_lock_limit,&status));
		assert(mrapi_impl_decode_hndl(rwl1,&r_index));
		assert(attributes.ext_error_checking == mrapi_db->sems[r_index].attributes.ext_error_checking);
		assert(attributes.shared_across_domains == mrapi_db->sems[r_index].attributes.shared_across_domains);
		mrapi_impl_rwl_get_attribute(rwl1,MRAPI_ERROR_EXT,&attributes,sizeof(attributes.ext_error_checking),&status);
		assert(MRAPI_TRUE == attributes.ext_error_checking);
		mrapi_impl_rwl_get_attribute(rwl1,MRAPI_DOMAIN_SHARED,&attributes,sizeof(attributes.shared_across_domains),&status);
		assert(MRAPI_FALSE == attributes.ext_error_checking);
		assert(mrapi_impl_rwl_get(&rwl2,key));
		assert(rwl2 == rwl1);
		mode = MRAPI_RWL_READER;
		assert(mrapi_impl_rwl_lock(rwl1,mode,0,&status));
		assert(MRAPI_FALSE == status);	// status only set if rwl_lock returns an error
		assert(mrapi_impl_rwl_lock(rwl1,mode,0,&status));
		assert(!mrapi_impl_rwl_lock(rwl1,mode,0,&status));
		assert(MRAPI_TIMEOUT == status);
		assert(mrapi_impl_rwl_unlock(rwl1,mode,&status));
		assert(mrapi_impl_rwl_unlock(rwl1,mode,&status));
		mode = MRAPI_RWL_WRITER;
		assert(mrapi_impl_rwl_lock(rwl1,mode,0,&status));
		assert(!mrapi_impl_rwl_lock(rwl1,mode,0,&status));
		assert(MRAPI_TIMEOUT == status);
		assert(!mrapi_impl_rwl_lock(rwl1,MRAPI_RWL_READER,0,&status));
		assert(mrapi_impl_rwl_unlock(rwl1,mode,&status));

		// Reader / writer lock rundown
		assert(mrapi_impl_rwl_lock(rwl1,mode,0,&status));
		assert(mrapi_impl_rwl_delete(rwl1));
	}
