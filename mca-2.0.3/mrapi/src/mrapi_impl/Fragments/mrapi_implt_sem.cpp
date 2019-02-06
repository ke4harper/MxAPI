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

// Semaphores
	{
		int key = 0;
        int sys_id = 0;
		int num_locks = 1;
		uint16_t s_index = 0;
		uint32_t shared_lock_limit = 1;
		mrapi_sem_hndl_t sem1 = 0;
		mrapi_sem_hndl_t sem2 = 0;
		mca_boolean_t attribute = MRAPI_FALSE;
		mrapi_sem_attributes_t attributes = { 0 };

		assert(mrapi_impl_whoami(&n_num,&n_index,&d_num,&d_index));

        // System semaphore
        key = 1;
		assert(mrapi_impl_create_sys_semaphore(&sys_id,num_locks,key,MRAPI_TRUE));
		assert(sys_sem_delete(sys_id));

        assert(sys_file_key(NULL,'d',&key));

		// Semaphore create
        status = MRAPI_SUCCESS;
		assert(mrapi_impl_create_lock_locked(&sem1,key,shared_lock_limit,SEM,&status));
		assert(MRAPI_SUCCESS == status);	// status only set if lock_locked returns an error
		assert(mrapi_impl_decode_hndl(sem1,&s_index));
		assert(0 == mrapi_db->sems[s_index].id);	// Not used
		assert(key == mrapi_db->sems[s_index].key);
		assert(1 == mrapi_db->domains[d_index].nodes[n_index].sems[s_index]);
		assert(1 == mrapi_db->sems[s_index].refs);
		assert(mrapi_db->sems[s_index].valid);
		assert((int32_t)shared_lock_limit == mrapi_db->sems[s_index].shared_lock_limit);
		assert(SEM == mrapi_db->sems[s_index].type);
		assert(0 < mrapi_db->num_sems);
		assert(mrapi_db->sems[s_index].locks[0].valid);
		assert(sem1 == mrapi_db->sems[s_index].handle);

        // Semaphore lock, unlock
        assert(mrapi_impl_sem_lock(sem1,1,0,&status));
		assert(MRAPI_FALSE == status);	// status only set if sem_lock returns an error
		assert(!mrapi_impl_sem_lock(sem1,1,10,&status));
		assert(MRAPI_TIMEOUT == status);
		assert(mrapi_impl_sem_unlock(sem1,1,&status));
		// Semaphore must be acquired before it can be deleted.
		assert(mrapi_impl_sem_lock(sem1,1,0,&status));
		assert(mrapi_impl_sem_delete(sem1));
		assert(1 == mrapi_db->sems[s_index].refs);	// Semaphores are marked as deleted but not reused
		assert(MRAPI_FALSE == mrapi_db->sems[s_index].valid);
		assert(1 == mrapi_db->domains[d_index].nodes[n_index].sems[s_index]);

		// Semaphore attributes
		memset(&attributes,0,sizeof(mrapi_sem_attributes_t));
		mrapi_impl_sem_init_attributes(&attributes);
		assert(MRAPI_FALSE == attributes.ext_error_checking);
		assert(MRAPI_TRUE == attributes.shared_across_domains);
		attribute = MRAPI_TRUE;
		mrapi_impl_sem_set_attribute(&attributes,MRAPI_ERROR_EXT,&attribute,sizeof(attribute),&status);
		assert(MRAPI_FALSE == status);	// status only set if sem_set_attribute returns an error
		attribute = MRAPI_FALSE;
		mrapi_impl_sem_set_attribute(&attributes,MRAPI_DOMAIN_SHARED,&attribute,sizeof(attribute),&status);

		// Implementation layer semaphore
		assert(mrapi_impl_sem_create(&sem1,key,&attributes,shared_lock_limit,&status));
		assert(mrapi_impl_decode_hndl(sem1,&s_index));
		assert(attributes.ext_error_checking == mrapi_db->sems[s_index].attributes.ext_error_checking);
		assert(attributes.shared_across_domains == mrapi_db->sems[s_index].attributes.shared_across_domains);
		mrapi_impl_sem_get_attribute(sem1,MRAPI_ERROR_EXT,&attributes,sizeof(attributes.ext_error_checking),&status);
		assert(MRAPI_TRUE == attributes.ext_error_checking);
		mrapi_impl_sem_get_attribute(sem1,MRAPI_DOMAIN_SHARED,&attributes,sizeof(attributes.shared_across_domains),&status);
		assert(MRAPI_FALSE == attributes.ext_error_checking);
        assert(1 == mrapi_db->sems[s_index].refs);
		assert(mrapi_impl_sem_get(&sem2,key));
		assert(sem2 == sem1);
        assert(1 == mrapi_db->sems[s_index].refs); // same node

		// Semaphore rundown
		assert(mrapi_impl_sem_lock(sem1,num_locks,0,&status));
		assert(mrapi_impl_sem_delete(sem1));
		assert(MRAPI_TRUE == mrapi_db->sems[s_index].deleted);
	}
