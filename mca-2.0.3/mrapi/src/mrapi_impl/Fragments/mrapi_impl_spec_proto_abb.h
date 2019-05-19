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

/******************************************************************
		   Function declarations (the MRAPI impl API)
******************************************************************/
char* mrapi_impl_display_status(mrapi_status_t status, char* status_message, size_t size);
mrapi_lock_type mrapi_impl_lock_type_get(uint32_t hndl, mrapi_status_t* status);

mrapi_boolean_t mrapi_impl_initialize(mrapi_domain_t domain_id,
	mrapi_node_t node_id,
	mrapi_status_t* status);

mrapi_boolean_t mrapi_impl_initialized();
mrapi_boolean_t mrapi_impl_finalize();

mrapi_boolean_t mrapi_impl_valid_status_param(const mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_valid_info_param(const mrapi_info_t* mrapi_info);
mrapi_boolean_t mrapi_impl_valid_parameters_param(mrapi_parameters_t mrapi_parameters);

mrapi_boolean_t mrapi_impl_valid_request_hndl(const mrapi_request_t* request);
mrapi_boolean_t mrapi_impl_canceled_request(const mrapi_request_t* request);

mrapi_boolean_t mrapi_impl_valid_node();
mrapi_boolean_t mrapi_impl_valid_node_num(mrapi_node_t node_num);
mrapi_boolean_t mrapi_impl_get_node_num(mrapi_node_t* node);
mrapi_node_t mrapi_impl_node_id_get(mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_get_domain_num(mrapi_domain_t* domain);
mrapi_boolean_t mrapi_impl_valid_domain();
mrapi_boolean_t mrapi_impl_valid_domain_num(mrapi_domain_t domain_num);

mrapi_boolean_t mrapi_impl_get_domain_num(mrapi_domain_t* domain);

mrapi_boolean_t mrapi_impl_test(const mrapi_request_t* request, mrapi_status_t* status);




/* MUTEXES */
mrapi_boolean_t mrapi_impl_valid_mutex_hndl(mrapi_mutex_hndl_t mutex, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_mutex_get(mrapi_mutex_hndl_t* mutex, mrapi_mutex_id_t mutex_id);
mrapi_boolean_t mrapi_impl_mutex_create(mrapi_mutex_hndl_t* mutex,
	mrapi_mutex_id_t mutex_id,
	const mrapi_mutex_attributes_t* attributes,
	mrapi_status_t* status);
void mrapi_impl_mutex_init_attributes(mrapi_mutex_attributes_t* attributes);
void mrapi_impl_mutex_set_attribute(mrapi_mutex_attributes_t* attributes,
	mrapi_uint_t attribute_num, const void* attribute, size_t
	attr_size, mrapi_status_t* status);
void mrapi_impl_mutex_get_attribute(mrapi_mutex_hndl_t mutex,
	mrapi_uint_t attribute_num, void* attribute, size_t
	attr_size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_mutex_delete(mrapi_mutex_hndl_t mutex);
mrapi_boolean_t mrapi_impl_mutex_lock(mrapi_mutex_hndl_t mutex,
	mrapi_key_t* lock_key,
	mrapi_timeout_t timeout,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_mutex_unlock(mrapi_mutex_hndl_t mutex,
	const mrapi_key_t* lock_key,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_mutex_validID(mrapi_mutex_id_t mutex);

/* SEMAPHORES */
mrapi_boolean_t mrapi_impl_valid_sem_hndl(mrapi_sem_hndl_t sem, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_sem_get(mrapi_sem_hndl_t* sem,
	mrapi_sem_id_t sem_id);
mrapi_boolean_t mrapi_impl_sem_create(mrapi_sem_hndl_t* sem,
	mrapi_sem_id_t sem_id,
	const mrapi_sem_attributes_t* attributes,
	mrapi_uint32_t num_locks,
	mrapi_uint32_t shared_lock_limit,
	mrapi_status_t* status);
void mrapi_impl_sem_init_attributes(mrapi_sem_attributes_t* attributes);
void mrapi_impl_sem_set_attribute(mrapi_sem_attributes_t* attributes,
	mrapi_uint_t attribute_num, const void* attribute, size_t
	attr_size, mrapi_status_t* status);
void mrapi_impl_sem_get_attribute(mrapi_sem_hndl_t sem,
	mrapi_uint_t attribute_num, void* attribute, size_t
	attr_size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_sem_delete(mrapi_sem_hndl_t sem);
mrapi_boolean_t mrapi_impl_sem_lock(mrapi_sem_hndl_t sem,
	int32_t num_locks,
	mrapi_timeout_t timeout,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_sem_lock_multiple(mrapi_sem_hndl_t* sem,
	int32_t num_locks,
	int32_t count,
	mrapi_boolean_t waitall,
	mrapi_timeout_t timeout,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_sem_unlock(mrapi_sem_hndl_t sem,
	int32_t num_locks,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_sem_unlock_multiple(mrapi_sem_hndl_t* sem,
	int32_t num_locks,
	int count,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_sem_validID(mrapi_sem_id_t sem);

/* READER/WRITER LOCKS */
mrapi_boolean_t mrapi_impl_valid_rwl_hndl(mrapi_rwl_hndl_t shmem, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_rwl_get(mrapi_rwl_hndl_t* rwl, mrapi_rwl_id_t rwl_id);
mrapi_boolean_t mrapi_impl_rwl_create(mrapi_rwl_hndl_t* rwl,
	mrapi_rwl_id_t rwl_id,
	const mrapi_rwl_attributes_t* attributes,
	mrapi_uint32_t reader_lock_limit,
	mrapi_status_t* status);
void mrapi_impl_rwl_init_attributes(mrapi_rwl_attributes_t* attributes);
void mrapi_impl_rwl_set_attribute(mrapi_rwl_attributes_t* attributes,
	mrapi_uint_t attribute_num, const void* attribute, size_t
	attr_size, mrapi_status_t* status);
void mrapi_impl_rwl_get_attribute(mrapi_rwl_hndl_t rwl,
	mrapi_uint_t attribute_num, void* attribute, size_t
	attr_size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_rwl_delete(mrapi_rwl_hndl_t rwl);
mrapi_boolean_t mrapi_impl_rwl_lock(mrapi_rwl_hndl_t rwl,
	mrapi_rwl_mode_t mode,
	mrapi_timeout_t timeout,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_rwl_unlock(mrapi_rwl_hndl_t rwl,
	mrapi_rwl_mode_t mode,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_rwl_validID(mrapi_rwl_id_t rwl);

/* SHARED MEMORY */
void mrapi_impl_shmem_create(mrapi_shmem_hndl_t* shm,
	uint32_t shmkey,
	uint32_t size,
	const mrapi_shmem_attributes_t* attributes,
	mrapi_status_t* status);

void mrapi_impl_shmem_init_attributes(mrapi_shmem_attributes_t* attributes);
void mrapi_impl_shmem_set_attribute(mrapi_shmem_attributes_t* attributes,
	mrapi_uint_t attribute_num, const void* attribute, size_t
	attr_size, mrapi_status_t* status);
void mrapi_impl_shmem_get_attribute(mrapi_shmem_hndl_t shmem,
	mrapi_uint_t attribute_num, void* attribute, size_t
	attr_size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_valid_shmem_hndl(mrapi_shmem_hndl_t shmem);
mrapi_boolean_t mrapi_impl_shmem_get(mrapi_shmem_hndl_t* shm, uint32_t shmkey);
void* mrapi_impl_shmem_attach(mrapi_shmem_hndl_t shm);
mrapi_boolean_t mrapi_impl_shmem_attached(mrapi_shmem_hndl_t shmem);
mrapi_boolean_t mrapi_impl_shmem_exists(uint32_t shmkey);
mrapi_boolean_t mrapi_impl_shmem_delete(mrapi_shmem_hndl_t shm);
mrapi_boolean_t mrapi_impl_shmem_detach(mrapi_shmem_hndl_t shm);
mrapi_boolean_t mrapi_impl_shmem_validID(mrapi_shmem_id_t shmem);

/* REMOTE MEMORY */
mrapi_boolean_t mrapi_impl_valid_rmem_id(mrapi_rmem_id_t rmem_id);
mrapi_boolean_t mrapi_impl_valid_atype(mrapi_rmem_atype_t access_type);
mrapi_boolean_t mrapi_impl_rmem_attached(mrapi_rmem_hndl_t rmem);
mrapi_boolean_t mrapi_impl_rmem_exists(mrapi_rmem_id_t rmem_id);
void mrapi_impl_rmem_create(mrapi_rmem_hndl_t* rmem,
	mrapi_rmem_id_t rmem_id,
	const void* mem,
	mrapi_rmem_atype_t access_type,
	const mrapi_rmem_attributes_t* attributes,
	mrapi_uint_t size, mrapi_status_t* status);
void mrapi_impl_rmem_init_attributes(mrapi_rmem_attributes_t* attributes);
void mrapi_impl_rmem_set_attribute(mrapi_rmem_attributes_t* attributes,
	mrapi_uint_t attribute_num, const void* attribute, size_t
	attr_size, mrapi_status_t* status);
void mrapi_impl_rmem_get_attribute(mrapi_rmem_hndl_t rmem,
	mrapi_uint_t attribute_num, void* attribute, size_t
	attr_size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_valid_rmem_hndl(mrapi_rmem_hndl_t rmem);
mrapi_boolean_t mrapi_impl_rmem_get(mrapi_rmem_hndl_t* rmem_hndl, uint32_t rmem_id);
mrapi_boolean_t mrapi_impl_rmem_attach(mrapi_rmem_hndl_t rmem);
mrapi_boolean_t mrapi_impl_rmem_detach(mrapi_rmem_hndl_t rmem);
mrapi_boolean_t mrapi_impl_rmem_delete(mrapi_rmem_hndl_t rmem);
mrapi_boolean_t mrapi_impl_rmem_read(mrapi_rmem_hndl_t rmem,
	mrapi_uint32_t rmem_offset,
	void* local_buf,
	mrapi_uint32_t local_offset,
	mrapi_uint32_t bytes_per_access,
	mrapi_uint32_t num_strides,
	mrapi_uint32_t rmem_stride,
	mrapi_uint32_t local_stride,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_rmem_write(mrapi_rmem_hndl_t rmem,
	mrapi_uint32_t rmem_offset,
	const void* local_buf,
	mrapi_uint32_t local_offset,
	mrapi_uint32_t bytes_per_access,
	mrapi_uint32_t num_strides,
	mrapi_uint32_t rmem_stride,
	mrapi_uint32_t local_stride,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_rmem_read_i(mrapi_rmem_hndl_t rmem,
	mrapi_uint32_t rmem_offset,
	void* local_buf,
	mrapi_uint32_t local_offset,
	mrapi_uint32_t bytes_per_access,
	mrapi_uint32_t num_strides,
	mrapi_uint32_t rmem_stride,
	mrapi_uint32_t local_stride,
	mrapi_status_t* status,
	mrapi_request_t* request);
mrapi_boolean_t mrapi_impl_rmem_write_i(mrapi_rmem_hndl_t rmem,
	mrapi_uint32_t rmem_offset,
	const void* local_buf,
	mrapi_uint32_t local_offset,
	mrapi_uint32_t bytes_per_access,
	mrapi_uint32_t num_strides,
	mrapi_uint32_t rmem_stride,
	mrapi_uint32_t local_stride,
	mrapi_status_t* status,
	mrapi_request_t* request);
mrapi_boolean_t mrapi_impl_rmem_validID(mrapi_rmem_id_t rmem);

/* RESOURCES */

#ifdef _DEBUG
PUBLIC void mrapi_impl_trigger_rollover(uint16_t index);
PUBLIC void mrapi_impl_increment_cache_hits(mrapi_resource_t *resource,
	int increment);
#endif  /* _DEBUG */

mrapi_resource_t* mrapi_impl_resources_get(
	mrapi_rsrc_filter_t subsystem_filter,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_resource_tree_free(
	mrapi_resource_t* const * root_ptr,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_resource_get_attribute(
	const mrapi_resource_t* resource,
	mrapi_uint_t attribute_number,
	void* attribute_value,
	size_t attr_size,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_dynamic_attribute_reset(
	const mrapi_resource_t *resource,
	mrapi_uint_t attribute_number,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_dynamic_attribute_start(
	const mrapi_resource_t* resource,
	mrapi_uint_t attribute_number,
	void(*rollover_callback) (void),
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_dynamic_attribute_stop(
	const mrapi_resource_t* resource,
	mrapi_uint_t attribute_number,
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_resource_register_callback(
	mrapi_event_t event,
	unsigned int frequency,
	void(*callback_function) (mrapi_event_t event),
	mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_valid_attribute_number(const mrapi_resource_t* resource,
	const mrapi_uint_t attribute_number);

mrapi_boolean_t mrapi_impl_is_static(const mrapi_resource_t* resource,
	const mrapi_uint_t attribute_number);


/* ATOMIC OPERATIONS */
mrapi_boolean_t mrapi_impl_atomic_barrier_init(mrapi_atomic_barrier_t* axb, pid_t dest,
	mrapi_msg_t* buffer, unsigned elems, size_t size, unsigned* pindex, mca_timeout_t timeout);
mrapi_boolean_t mrapi_impl_atomic_exchange_init(mrapi_atomic_barrier_t* axb, pid_t dest,
	mrapi_msg_t* buffer, unsigned elems, size_t size, unsigned* pindex, mca_timeout_t timeout);
mrapi_boolean_t mrapi_impl_atomic_hold(mrapi_atomic_barrier_t* axb, mrapi_boolean_t hold);

mrapi_boolean_t mrapi_impl_atomic_read(void* sync, void* dest, void* value, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_read_ptr(void* sync, uintptr_t* dest, uintptr_t* value, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_add(void* sync, void* dest, void* value, void* previous, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_inc(void* sync, void* dest, void* result, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_dec(void* sync, void* dest, void* result, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_or(void* sync, void* dest, void* value, void* previous, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_and(void* sync, void* dest, void* value, void* previous, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_xor(void* sync, void* dest, void* value, void* previous, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_cas(void* sync, void* dest, void* exchange, void* compare, void* previous, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_xchg(void* sync, void* dest, void* exchange, void* previous, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_cas_ptr(void* sync, uintptr_t* dest, uintptr_t exchange, uintptr_t compare, uintptr_t* previous, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_xchg_ptr(void* sync, uintptr_t* dest, uintptr_t exchange, uintptr_t* previous, mrapi_status_t* status);
void mrapi_impl_atomic_sync(void* sync);
mrapi_boolean_t mrapi_impl_atomic_lock(void* sync, void* dest, void* previous, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_release(void* sync, void* dest, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_set(void* sync, void* dest, int bit, int* previous, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_clear(void* sync, void* dest, int bit, int* previous, size_t size, mrapi_status_t* status);
mrapi_boolean_t mrapi_impl_atomic_change(void* sync, void* dest, int bit, int* previous, size_t size, mrapi_status_t* status);
