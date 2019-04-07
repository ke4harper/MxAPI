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

// Shared memory
	{
		uint16_t m_index = 0;
		mrapi_shmem_id_t key = 0;
		uint32_t size = 0;
		mrapi_shmem_hndl_t shmem1 = 0;
		mrapi_shmem_hndl_t shmem2 = 0;
		mca_boolean_t attribute = 0;
		mrapi_resource_t resource = { 0 };
		void* addr = NULL;
		mrapi_shmem_attributes_t attributes = { 0 };

		assert(mrapi_impl_whoami(&n_num,&n_index,&d_num,&d_index));
        p_index = mrapi_db->domains[d_index].nodes[n_index].proc_num;

        assert(sys_file_key(NULL,'i',&key));

		// Shared memory attributes
		memset(&attributes,0,sizeof(mrapi_shmem_attributes_t));
		mrapi_impl_shmem_init_attributes(&attributes);
		assert(MRAPI_FALSE == attributes.ext_error_checking);
		assert(MRAPI_TRUE == attributes.shared_across_domains);
		attribute = MRAPI_TRUE;
		mrapi_impl_shmem_set_attribute(&attributes,MRAPI_ERROR_EXT,&attribute,sizeof(attribute),&status);
		assert(MRAPI_FALSE == status);	// status only set if shmem_set_attribute returns an error
		attribute = MRAPI_FALSE;
		mrapi_impl_shmem_set_attribute(&attributes,MRAPI_DOMAIN_SHARED,&attribute,sizeof(attribute),&status);
		memset(&resource,0,sizeof(mrapi_resource_t));
		mrapi_impl_shmem_set_attribute(&attributes,MRAPI_SHMEM_RESOURCE,&resource,sizeof(resource),&status);
		addr = &resource;
		mrapi_impl_shmem_set_attribute(&attributes,MRAPI_SHMEM_ADDRESS,addr,sizeof(void*),&status);
		size = sizeof(resource);
		mrapi_impl_shmem_set_attribute(&attributes,MRAPI_SHMEM_SIZE,&size,sizeof(size),&status);

		// Implementation layer shared memory
		mrapi_impl_shmem_create(&shmem1,key,10,&attributes,&status);
		assert(mrapi_impl_valid_shmem_hndl(shmem1));
		assert(MRAPI_FALSE == status);	// status only set if shmem_create returns an error
		assert(mrapi_impl_decode_hndl(shmem1,&m_index));
		assert(key == mrapi_db->shmems[m_index].key);
		assert(NULL != mrapi_db->shmems[m_index].id);	// Internal handle
		assert(MRAPI_TRUE == mrapi_db->shmems[m_index].valid);
		assert(NULL == mrapi_db->shmems[m_index].addr[p_index]);
        assert(10 == mrapi_db->shmems[m_index].size);
		assert(attributes.ext_error_checking == mrapi_db->shmems[m_index].attributes.ext_error_checking);
		assert(attributes.shared_across_domains == mrapi_db->shmems[m_index].attributes.shared_across_domains);
		assert(0 < mrapi_db->num_shmems);
		assert(mrapi_impl_shmem_exists(key));
		assert(mrapi_impl_shmem_get(&shmem2,key));
		assert(shmem2 == shmem1);

		// Shared memory access
		addr = mrapi_impl_shmem_attach(shmem1);
		assert(NULL != addr);
		assert(addr == mrapi_db->shmems[m_index].addr[p_index]);
		assert(1 == mrapi_db->domains[d_index].nodes[n_index].shmems[m_index]);
		assert(1 == mrapi_db->shmems[m_index].refs);
        assert(1 == mrapi_db->shmems[m_index].num_procs);
        assert(1 == mrapi_db->shmems[m_index].processes[p_index]);
		assert(mrapi_impl_shmem_attached(shmem1));
		assert(mrapi_impl_shmem_detach(shmem1));
		assert(0 == mrapi_db->shmems[m_index].refs);
        assert(0 == mrapi_db->shmems[m_index].num_procs);
		assert(0 == mrapi_db->domains[d_index].nodes[n_index].shmems[m_index]);
        assert(0 == mrapi_db->shmems[m_index].processes[p_index]);
		assert(!mrapi_impl_shmem_attached(shmem1));

		// Shared memory rundown
		assert(mrapi_impl_shmem_delete(shmem1));
        assert(!mrapi_impl_shmem_exists(shmem1));
	}
