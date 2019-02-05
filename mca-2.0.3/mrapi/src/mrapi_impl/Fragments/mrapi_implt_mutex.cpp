	// Mutexes
	{
		mrapi_mutex_id_t key = 0;
		uint16_t m_index = 0;
		mrapi_mutex_hndl_t mutex1 = 0;
		mrapi_mutex_hndl_t mutex2 = 0;
		mca_boolean_t attribute = MRAPI_FALSE;
		mrapi_mutex_attributes_t attributes = { 0 };
		mrapi_key_t lock_key = 0;

		assert(mrapi_impl_whoami(&n_num,&n_index,&d_num,&d_index));

        assert(sys_file_key(NULL,'e',&key));

		// Mutex attributes
		memset(&attributes,0,sizeof(attributes));
		mrapi_impl_mutex_init_attributes(&attributes);
		assert(MRAPI_FALSE == attributes.recursive);
		assert(MRAPI_FALSE == attributes.ext_error_checking);
		assert(MRAPI_TRUE == attributes.shared_across_domains);
		attribute = MRAPI_TRUE;
		mrapi_impl_mutex_set_attribute(&attributes,MRAPI_MUTEX_RECURSIVE,&attribute,sizeof(attribute),&status);
		assert(MRAPI_FALSE == status);	// status only set if mutex_set_attribute returns an error
		mrapi_impl_mutex_set_attribute(&attributes,MRAPI_ERROR_EXT,&attribute,sizeof(attribute),&status);
		attribute = MRAPI_FALSE;
		mrapi_impl_mutex_set_attribute(&attributes,MRAPI_DOMAIN_SHARED,&attribute,sizeof(attribute),&status);

		// Implementation layer mutex
		assert(mrapi_impl_mutex_create(&mutex1,key,&attributes,&status));
		assert(mrapi_impl_decode_hndl(mutex1,&m_index));
		assert(attributes.recursive == mrapi_db->sems[m_index].attributes.recursive);
		assert(attributes.ext_error_checking == mrapi_db->sems[m_index].attributes.ext_error_checking);
		assert(attributes.shared_across_domains == mrapi_db->sems[m_index].attributes.shared_across_domains);
		mrapi_impl_mutex_get_attribute(mutex1,MRAPI_MUTEX_RECURSIVE,&attributes,sizeof(attributes.recursive),&status);
		assert(MRAPI_TRUE == attributes.recursive);
		mrapi_impl_mutex_get_attribute(mutex1,MRAPI_ERROR_EXT,&attributes,sizeof(attributes.ext_error_checking),&status);
		assert(MRAPI_TRUE == attributes.ext_error_checking);
		mrapi_impl_sem_get_attribute(mutex1,MRAPI_DOMAIN_SHARED,&attributes,sizeof(attributes.shared_across_domains),&status);
		assert(MRAPI_FALSE == attributes.ext_error_checking);
		assert(mrapi_impl_mutex_get(&mutex2,key));
		assert(mutex2 == mutex1);
		assert(mrapi_impl_mutex_lock(mutex1,&lock_key,0,&status));
		assert(0 == lock_key);
		assert(mrapi_impl_mutex_lock(mutex1,&lock_key,0,&status));
		assert(1 == lock_key);
		lock_key = 0;
		assert(!mrapi_impl_mutex_unlock(mutex1,&lock_key,&status));
		assert(MRAPI_ERR_MUTEX_LOCKORDER == status);
		lock_key = 1;
		assert(mrapi_impl_mutex_unlock(mutex1,&lock_key,&status));

		// Mutex rundown
		assert(mrapi_impl_mutex_lock(mutex1,&lock_key,0,&status));
		assert(mrapi_impl_mutex_delete(mutex1));
	}
