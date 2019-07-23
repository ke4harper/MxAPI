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

// Semaphores
	{
	int sem_key1 = 0;
	int sem_key2 = 0;
	mrapi_sem_id_t key1 = 0;
	mrapi_sem_id_t key2 = 0;
	int sys_id = 0;
	int sys_id2 = 0;
	int num_locks = 1;
	mrapi_uint8_t num_sems = 0;
	uint16_t s_index1 = 0;
	uint16_t s_index2 = 0;
	uint32_t shared_lock_limit = 0;
	mrapi_sem_hndl_t sem1 = 0;
	mrapi_sem_hndl_t sem2 = 0;
	mrapi_sem_hndl_t sem3 = 0;
	mca_boolean_t attribute = MRAPI_FALSE;
	mrapi_sem_attributes_t attributes = { 0 };

	assert(mrapi_impl_whoami(&n_num, &n_index, &d_num, &d_index));

	// System semaphore
	sem_key1 = 1;
	assert(mrapi_impl_create_sys_semaphore(&sys_id, num_locks, sem_key1, MRAPI_TRUE));
	// Duplicate create gets refreence to the existing semaphore, locks are already taken
	assert(mrapi_impl_create_sys_semaphore(&sys_id2, num_locks, sem_key1, MRAPI_FALSE));
	assert(sys_sem_delete(sys_id2));
	assert(sys_sem_delete(sys_id));

	assert(sys_file_key(NULL, 'd', &key1));
	assert(sys_file_key(NULL, 'e', &key2));

	num_sems = mrapi_db->num_sems;
	status = MRAPI_SUCCESS;

	// Semaphore create
	shared_lock_limit = 1;
	assert(mrapi_impl_create_lock_locked(&sem1, MRAPI_SEM_ID_ANY, 0, shared_lock_limit, MRAPI_SEM, &status));
	assert(MRAPI_SUCCESS == status);	// status only set if lock_locked returns an error
	assert(num_sems + 1 == mrapi_db->num_sems);
	// Semaphore must be acquired before it can be deleted.
	assert(mrapi_impl_sem_lock(sem1, 1, 0, &status));
	assert(mrapi_impl_sem_delete(sem1));
	assert(num_sems + 1 == mrapi_db->num_sems);	// num_sems is never decremented
	assert(mrapi_impl_create_lock_locked(&sem1, key1, 0, shared_lock_limit, MRAPI_SEM, &status));
	assert(MRAPI_SUCCESS == status);
	assert(mrapi_impl_decode_hndl(sem1, &s_index1));
	assert(0 == mrapi_db->sems[s_index1].num_locks);
	assert(0 == mrapi_db->sems[s_index1].spin);
	assert(key1 == mrapi_db->sems[s_index1].key);
	assert(1 == mrapi_db->domains[d_index].nodes[n_index].sems[s_index1]);
	assert(1 == mrapi_db->sems[s_index1].refs);
	assert(mrapi_db->sems[s_index1].valid);
	assert((int32_t)shared_lock_limit == mrapi_db->sems[s_index1].shared_lock_limit);
	assert(0 == mrapi_db->sems[s_index1].num_locks);
	assert(MRAPI_SEM == mrapi_db->sems[s_index1].type);
	assert(num_sems + 2 == mrapi_db->num_sems);
	assert(mrapi_db->sems[s_index1].locks[0].valid);
	assert(0 == mrapi_db->sems[s_index1].locks[0].lock_holder_dindex);
	assert(0 == mrapi_db->sems[s_index1].locks[0].lock_holder_nindex);
	assert(0 == mrapi_db->sems[s_index1].locks[0].id);
	assert(sem1 == mrapi_db->sems[s_index1].handle);

	shared_lock_limit = 2;
	// Semaphore with pre-set lock
	assert(mrapi_impl_create_lock_locked(&sem2, MRAPI_SEM_ID_ANY, 1, shared_lock_limit, MRAPI_SEM, &status));
	assert(MRAPI_SUCCESS == status);	// status only set if lock_locked returns an error
	assert(mrapi_impl_decode_hndl(sem2, &s_index2));
	assert(1 == mrapi_db->sems[s_index2].num_locks);
	assert(num_sems + 3 == mrapi_db->num_sems);
	assert(mrapi_db->sems[s_index2].locks[0].valid);
	assert(mrapi_db->sems[s_index2].locks[0].locked);
	assert(d_index == mrapi_db->sems[s_index2].locks[0].lock_holder_dindex);
	assert((mrapi_uint8_t)-1 == mrapi_db->sems[s_index2].locks[0].lock_holder_nindex);
	assert(mrapi_db->sems[s_index2].locks[1].valid);
	assert(!mrapi_db->sems[s_index2].locks[1].locked);
	assert(0 == mrapi_db->sems[s_index2].locks[1].lock_holder_dindex);
	assert(0 == mrapi_db->sems[s_index2].locks[1].lock_holder_nindex);
	// Semaphore must be acquired before it can be deleted.
	assert(mrapi_impl_sem_lock(sem2, 1, 0, &status));
	assert(mrapi_impl_sem_delete(sem2));
	assert(num_sems + 3 == mrapi_db->num_sems);
	assert(mrapi_impl_create_lock_locked(&sem2, key2, 0, shared_lock_limit, MRAPI_SEM, &status));
	assert(MRAPI_SUCCESS == status);
	assert(key2 == mrapi_db->sems[s_index2].key);
	assert(1 == mrapi_db->domains[d_index].nodes[n_index].sems[s_index2]);
	assert(1 == mrapi_db->sems[s_index2].refs);
	assert(mrapi_db->sems[s_index2].valid);
	assert((int32_t)shared_lock_limit == mrapi_db->sems[s_index2].shared_lock_limit);
	assert(0 == mrapi_db->sems[s_index2].num_locks);
	assert(MRAPI_SEM == mrapi_db->sems[s_index2].type);
	assert(num_sems + 4 == mrapi_db->num_sems);
	assert(mrapi_db->sems[s_index2].locks[0].valid);
	assert(mrapi_db->sems[s_index2].locks[1].valid);
	assert(0 == mrapi_db->sems[s_index2].locks[0].id);
	assert(0 == mrapi_db->sems[s_index2].locks[1].id);
	assert(sem2 == mrapi_db->sems[s_index2].handle);

	// Semaphore lock, unlock, post
	assert(mrapi_impl_sem_lock(sem1, 1, 0, &status));
	assert(MRAPI_SUCCESS == status);
	assert(1 == mrapi_db->sems[s_index1].num_locks);
	assert(MRAPI_TRUE == mrapi_db->sems[s_index1].locks[0].locked);
	assert(d_index == mrapi_db->sems[s_index1].locks[0].lock_holder_dindex);
	assert(n_index == mrapi_db->sems[s_index1].locks[0].lock_holder_nindex);
	assert(0 == mrapi_db->sems[s_index1].locks[0].id);
	assert(!mrapi_impl_sem_lock(sem1, 1, 10, &status));
	assert(MRAPI_TIMEOUT == status);
	assert(mrapi_impl_sem_unlock(sem1, 1, &status));
	assert(MRAPI_SUCCESS == status);
	assert(0 == mrapi_db->sems[s_index1].num_locks);
	assert(MRAPI_FALSE == mrapi_db->sems[s_index1].locks[0].locked);
	assert(0 == mrapi_db->sems[s_index1].locks[0].lock_holder_dindex);
	assert(0 == mrapi_db->sems[s_index1].locks[0].lock_holder_nindex);
	assert(0 == mrapi_db->sems[s_index1].locks[0].id);

	assert(mrapi_impl_sem_lock(sem1, 1, 0, &status));
	assert(MRAPI_SUCCESS == status);
	assert(mrapi_impl_sem_post(sem1, 1, &status));
	assert(MRAPI_SUCCESS == status);

	assert(mrapi_impl_sem_lock(sem2, 1, 0, &status));
	assert(MRAPI_SUCCESS == status);
	assert(1 == mrapi_db->sems[s_index2].num_locks);
	assert(MRAPI_TRUE == mrapi_db->sems[s_index2].locks[0].locked);
	assert(d_index == mrapi_db->sems[s_index2].locks[0].lock_holder_dindex);
	assert(n_index == mrapi_db->sems[s_index2].locks[0].lock_holder_nindex);
	assert(0 == mrapi_db->sems[s_index2].locks[0].id);
	assert(mrapi_impl_sem_lock(sem2, 1, 10, &status));
	assert(MRAPI_SUCCESS == status);
	assert(2 == mrapi_db->sems[s_index2].num_locks);
	assert(MRAPI_TRUE == mrapi_db->sems[s_index2].locks[1].locked);
	assert(d_index == mrapi_db->sems[s_index2].locks[1].lock_holder_dindex);
	assert(n_index == mrapi_db->sems[s_index2].locks[1].lock_holder_nindex);
	assert(0 == mrapi_db->sems[s_index2].locks[1].id);
	assert(!mrapi_impl_sem_lock(sem2, 1, 10, &status));
	assert(MRAPI_TIMEOUT == status);
	assert(mrapi_impl_sem_unlock(sem2, 2, &status));
	assert(MRAPI_SUCCESS == status);
	assert(0 == mrapi_db->sems[s_index2].num_locks);
	assert(MRAPI_FALSE == mrapi_db->sems[s_index2].locks[0].locked);
	assert(0 == mrapi_db->sems[s_index2].locks[0].lock_holder_dindex);
	assert(0 == mrapi_db->sems[s_index2].locks[0].lock_holder_nindex);
	assert(0 == mrapi_db->sems[s_index2].locks[0].id);
	assert(MRAPI_FALSE == mrapi_db->sems[s_index2].locks[1].locked);
	assert(0 == mrapi_db->sems[s_index2].locks[1].lock_holder_dindex);
	assert(0 == mrapi_db->sems[s_index2].locks[1].lock_holder_nindex);
	assert(0 == mrapi_db->sems[s_index2].locks[1].id);

	// Multiple semaphore lock, unlock, post
	mrapi_sem_hndl_t sem[2] = { sem1, sem2 };
	assert(mrapi_impl_sem_lock_multiple(sem, num_locks, 2, TRUE, 0, &status));
	assert(MRAPI_SUCCESS == status);
	assert(1 == mrapi_db->sems[s_index1].num_locks);
	assert(1 == mrapi_db->sems[s_index2].num_locks);
	assert(MRAPI_TRUE == mrapi_db->sems[s_index1].locks[0].locked);
	assert(MRAPI_TRUE == mrapi_db->sems[s_index2].locks[0].locked);
	assert(d_index == mrapi_db->sems[s_index1].locks[0].lock_holder_dindex);
	assert(n_index == mrapi_db->sems[s_index1].locks[0].lock_holder_nindex);
	assert(d_index == mrapi_db->sems[s_index2].locks[0].lock_holder_dindex);
	assert(n_index == mrapi_db->sems[s_index2].locks[0].lock_holder_nindex);
	assert(0 == mrapi_db->sems[s_index1].locks[0].id);
	assert(0 == mrapi_db->sems[s_index2].locks[0].id);
	// Should be able to get the remaining sem2 lock
	assert(mrapi_impl_sem_lock_multiple(sem, num_locks, 2, FALSE, 0, &status));
	assert(MRAPI_SUCCESS == status);
	assert(mrapi_impl_sem_unlock(sem2, 1, &status));
	assert(!mrapi_impl_sem_lock_multiple(sem, num_locks, 2, TRUE, 10, &status));
	assert(MRAPI_TIMEOUT == status);
	assert(mrapi_impl_sem_unlock_multiple(sem, num_locks, 2, &status));
	assert(MRAPI_SUCCESS == status);
	assert(0 == mrapi_db->sems[s_index1].num_locks);

	assert(0 == mrapi_db->sems[s_index2].num_locks);
	assert(MRAPI_FALSE == mrapi_db->sems[s_index1].locks[0].locked);
	assert(MRAPI_FALSE == mrapi_db->sems[s_index2].locks[0].locked);
	assert(0 == mrapi_db->sems[s_index1].locks[0].lock_holder_dindex);
	assert(0 == mrapi_db->sems[s_index1].locks[0].lock_holder_nindex);
	assert(0 == mrapi_db->sems[s_index2].locks[0].lock_holder_dindex);
	assert(0 == mrapi_db->sems[s_index2].locks[0].lock_holder_nindex);
	assert(0 == mrapi_db->sems[s_index1].locks[0].id);
	assert(0 == mrapi_db->sems[s_index2].locks[0].id);

	assert(mrapi_impl_sem_lock_multiple(sem, num_locks, 2, TRUE, 0, &status));
	assert(MRAPI_SUCCESS == status);
	assert(mrapi_impl_sem_post_multiple(sem, num_locks, 2, &status));
	assert(MRAPI_SUCCESS == status);

	assert(mrapi_impl_sem_lock(sem1, 1, 0, &status));
	assert(mrapi_impl_sem_delete(sem1));
	assert(1 == mrapi_db->sems[s_index1].refs);	// Semaphores are marked as deleted but not reused
	assert(MRAPI_FALSE == mrapi_db->sems[s_index1].valid);
	assert(1 == mrapi_db->domains[d_index].nodes[n_index].sems[s_index1]);
	// All the locks must be acquired before deleting
	assert(mrapi_impl_sem_lock(sem2, 2, 0, &status));
	assert(mrapi_impl_sem_delete(sem2));

	// Semaphore attributes
	memset(&attributes, 0, sizeof(mrapi_sem_attributes_t));
	mrapi_impl_sem_init_attributes(&attributes);
	assert(MRAPI_FALSE == attributes.ext_error_checking);
	assert(MRAPI_TRUE == attributes.shared_across_domains);
	attribute = MRAPI_TRUE;
	mrapi_impl_sem_set_attribute(&attributes, MRAPI_ERROR_EXT, &attribute, sizeof(attribute), &status);
	assert(MRAPI_FALSE == status);	// status only set if sem_set_attribute returns an error
	attribute = MRAPI_FALSE;
	mrapi_impl_sem_set_attribute(&attributes, MRAPI_DOMAIN_SHARED, &attribute, sizeof(attribute), &status);

	// Implementation layer semaphore
	shared_lock_limit = 1;
	assert(mrapi_impl_sem_create(&sem1, sem_key1, &attributes, 0, shared_lock_limit, &status));
	assert(mrapi_impl_valid_sem_hndl(sem1, &status));
	assert(MRAPI_LOCK_SEM == mrapi_impl_lock_type_get(sem1, &status));
	assert(mrapi_impl_decode_hndl(sem1, &s_index1));
	assert(attributes.ext_error_checking == mrapi_db->sems[s_index1].attributes.ext_error_checking);
	assert(attributes.shared_across_domains == mrapi_db->sems[s_index1].attributes.shared_across_domains);
	mrapi_impl_sem_get_attribute(sem1, MRAPI_ERROR_EXT, &attributes, sizeof(attributes.ext_error_checking), &status);
	assert(MRAPI_TRUE == attributes.ext_error_checking);
	mrapi_impl_sem_get_attribute(sem1, MRAPI_DOMAIN_SHARED, &attributes, sizeof(attributes.shared_across_domains), &status);
	assert(MRAPI_FALSE == attributes.ext_error_checking);
	assert(1 == mrapi_db->sems[s_index1].refs);
	// Semaphore with pre-set lock
	shared_lock_limit = 2;
	assert(mrapi_impl_sem_create(&sem2, sem_key2, &attributes, 1, shared_lock_limit, &status));
	assert(mrapi_impl_valid_sem_hndl(sem2, &status));
	assert(mrapi_impl_decode_hndl(sem2, &s_index2));

	assert(mrapi_impl_sem_get(&sem3, sem_key1));
	assert(sem3 == sem1);
	assert(1 == mrapi_db->sems[s_index1].refs); // same node

	// Semaphore rundown
	assert(mrapi_impl_sem_lock(sem1, num_locks, 0, &status));
	assert(mrapi_impl_sem_delete(sem1));
	assert(MRAPI_TRUE == mrapi_db->sems[s_index1].deleted);
	}
