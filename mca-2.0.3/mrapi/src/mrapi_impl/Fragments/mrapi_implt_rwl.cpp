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
