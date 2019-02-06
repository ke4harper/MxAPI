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
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

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

// Remote memory
	{
		mrapi_rmem_id_t key = 0;
		uint16_t m_index = 0;
		mrapi_rmem_hndl_t rmem1 = 0;
		mrapi_rmem_hndl_t rmem2 = 0;
		mca_boolean_t attribute = MRAPI_FALSE;
		mrapi_rmem_attributes_t attributes = { 0 };
		const int mem_length = 100;
		char mem[mem_length] = "";
		char buf[mem_length] = "";
		mrapi_rmem_atype_t access_type = MRAPI_RMEM_SWCACHE;

		assert(mrapi_impl_whoami(&n_num,&n_index,&d_num,&d_index));

        assert(sys_file_key(NULL,'h',&key));

		// Remote memory attributes
		memset(&attributes,0,sizeof(attributes));
		mrapi_impl_rmem_init_attributes(&attributes);
		assert(MRAPI_FALSE == attributes.ext_error_checking);
		assert(MRAPI_TRUE == attributes.shared_across_domains);
		attribute = MRAPI_TRUE;
		mrapi_impl_rmem_set_attribute(&attributes,MRAPI_ERROR_EXT,&attribute,sizeof(attribute),&status);
		assert(MRAPI_FALSE == status);	// status only set if rmem_set_attribute returns an error
		attribute = MRAPI_FALSE;
		mrapi_impl_rmem_set_attribute(&attributes,MRAPI_DOMAIN_SHARED,&attribute,sizeof(attribute),&status);

		// Implementation remote memory
		mrapi_impl_rmem_create(&rmem1,key,mem,access_type,&attributes,mem_length,&status);
		assert(MRAPI_SUCCESS == status);
		assert(mrapi_impl_rmem_exists(key));
		assert(mrapi_impl_decode_hndl(rmem1,&m_index));
		assert(key == mrapi_db->rmems[m_index].key);
		assert(MRAPI_TRUE == mrapi_db->rmems[m_index].valid);
		assert(mem == mrapi_db->rmems[m_index].addr);
		assert((size_t)mem_length == mrapi_db->rmems[m_index].size);
		assert(access_type == mrapi_db->rmems[m_index].access_type);
		assert(attributes.ext_error_checking == mrapi_db->rmems[m_index].attributes.ext_error_checking);
		assert(attributes.shared_across_domains == mrapi_db->rmems[m_index].attributes.shared_across_domains);
		mrapi_impl_rwl_get_attribute(rmem1,MRAPI_ERROR_EXT,&attribute,sizeof(attributes.ext_error_checking),&status);
		assert(MRAPI_TRUE == attributes.ext_error_checking);
		mrapi_impl_rwl_get_attribute(rmem1,MRAPI_DOMAIN_SHARED,&attribute,sizeof(attributes.shared_across_domains),&status);
		assert(MRAPI_FALSE == attributes.shared_across_domains);
		assert(0 < mrapi_db->num_rmems);
		assert(mrapi_impl_rmem_get(&rmem2,key));
		assert(rmem2 == rmem1);

		// Remote memory access
		assert(mrapi_impl_rmem_attach(rmem1));
		assert(1 == mrapi_db->rmems[m_index].nodes[n_index]);
		assert(1 == mrapi_db->rmems[m_index].refs);
		assert(mrapi_impl_rmem_attached(rmem1));
		assert(mrapi_impl_rmem_write(rmem1,0,"Hello, world!",0,16,1,1,1,&status));
		assert(0 == strcmp("Hello, world!",mem));
		assert(mrapi_impl_rmem_read(rmem1,0,buf,0,16,1,1,1,&status));
		assert(0 == strcmp("Hello, world!",buf));
		assert(mrapi_impl_rmem_write(rmem1,20,"Hello, MRAPI!",0,4,4,4,4,&status));
		assert(0 == strcmp("Hello, MRAPI!",&mem[20]));
		assert(mrapi_impl_rmem_read(rmem1,20,buf,0,4,4,4,4,&status));
		assert(0 == strcmp("Hello, MRAPI!",buf));
		assert(mrapi_impl_rmem_detach(rmem1));
		assert(0 == mrapi_db->rmems[m_index].nodes[n_index]);
		assert(0 == mrapi_db->rmems[m_index].refs);
		assert(!mrapi_impl_rmem_attached(rmem1));

		// Remote memory rundown
		assert(mrapi_impl_rmem_delete(rmem1));
		assert(MRAPI_FALSE == mrapi_db->rmems[m_index].valid);	// Possible race condition with other clients
	}
