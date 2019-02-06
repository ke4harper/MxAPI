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
