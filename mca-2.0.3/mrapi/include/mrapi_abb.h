/*
Copyright (c) 2010, The Multicore Association
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

(1) Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
 
(2) Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution. 

(3) Neither the name of the Multicore Association nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Port to Windows: #if !(__unix__||__MINGW32__), etc.

*/
 

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MRAPI_H
#define MRAPI_H

#include "Fragments/mrapi_inc1_abb.h"

/* the mca data types */
typedef mca_domain_t mrapi_domain_t;
typedef mca_node_t mrapi_node_t;
typedef mca_status_t mrapi_status_t;
typedef mca_timeout_t mrapi_timeout_t;
typedef mca_int_t  mrapi_int_t;
typedef mca_uint_t mrapi_uint_t;
typedef mca_uint8_t mrapi_uint8_t;
typedef mca_uint16_t mrapi_uint16_t;
typedef mca_uint32_t mrapi_uint32_t;
typedef mca_uint64_t mrapi_uint64_t;
typedef mca_boolean_t mrapi_boolean_t;

/* lock type for semaphores */
typedef enum {
	MRAPI_LOCK_UNKNOWN,
	MRAPI_LOCK_RWL,
	MRAPI_LOCK_SEM,
	MRAPI_LOCK_MUTEX,
} mrapi_lock_type;

/* lock type for reader/writer locks */
typedef enum {
  MRAPI_RWL_READER,
  MRAPI_RWL_WRITER
} mrapi_rwl_mode_t;

/* access type for remote memory */
typedef enum {
  MRAPI_RMEM_DMA,
  MRAPI_RMEM_SWCACHE,
  MRAPI_RMEM_DUMMY
} mrapi_rmem_atype_t;

/* atomic operation mode */
typedef enum {
  MRAPI_ATOMIC_WRITE = 0,
  MRAPI_ATOMIC_READ,
  MRAPI_ATOMIC_NONE
} mrapi_atomic_mode_t;

/* base message object;
   messages include this as first member of their struct */
typedef struct {
  mrapi_boolean_t valid; /* MRAPI_FALSE when MRAPI_ATOMIC_WRITE, MRAPI_TRUE otherwise */
  unsigned txn; /* transaction ID */
  uint16_t counter; /* non-blocking buffer synchronization */
  uint16_t tindex; /* reference to mca_timestamp_t (FUTURE) */
} mrapi_msg_t;

/* atomic sync object */
typedef struct {
  unsigned* pindex; /* buffer index reference */
  unsigned last_counter; /* non-blocking buffer previous state */
  mrapi_boolean_t hold; /* MRAPI_TRUE if valid msg flag should be retained
                           across atomic calls; last call must release */
  mrapi_boolean_t override; /* one-time override to allow specified mode */
  mrapi_boolean_t active; /* MRAPI_TRUE if caller holds sync */
} mrapi_atomic_sync_t;

/* sync descriptor for non-Windows cross-process atomic operations */
typedef struct {
  mrapi_atomic_mode_t mode;
  mrapi_boolean_t xchg; /* read only when valid, write only when invalid;
                           flip flag state on completion */
  mca_timeout_t timeout;
  pid_t src; /* local */
  pid_t dest; /* remote; local proc == remote proc, not sync required */
  unsigned elems; /* number of buffer elements */
  size_t size; /* element size */
  mrapi_atomic_sync_t sync;
  mrapi_msg_t* buffer; /* messages */
} mrapi_atomic_barrier_t;

typedef int mrapi_parameters_t;

#define MRAPI_VERSION "FSL 0.9.3.4"
typedef struct {
  char mrapi_version[64];
} mrapi_info_t;

typedef mca_request_t mrapi_request_t;

/* The following keys are either agreed upon apriori among nodes or
   passed via messages.  They are usually created by tokenizing a
   string, for example using ftok or the posix IPC_PRIVATE macro.  */
typedef int mrapi_shmem_id_t; /* the shared key */
typedef int mrapi_mutex_id_t; /*the shared key */
typedef int mrapi_sem_id_t; /* the shared key */
typedef int mrapi_rwl_id_t; /* the shared key */
typedef int mrapi_rmem_id_t; /* the shared key */

typedef enum {
  MRAPI_RSRC_MEM,
  MRAPI_RSRC_CPU,
  MRAPI_RSRC_CACHE,
  MRAPI_RSRC_DMA,
  MRAPI_RSRC_CROSSBAR,
} mrapi_rsrc_filter_t;

#define MRAPI_TRUE MCA_TRUE
#define MRAPI_FALSE MCA_FALSE
#define MRAPI_NULL MCA_NULL

#define MRAPI_IN const
#define MRAPI_OUT
#define MRAPI_FUNCTION_PTR

#define MRAPI_TIMEOUT_INFINITE          (~0)            /* Wait forever, no timeout */
#define MRAPI_TIMEOUT_IMMEDIATE           0                     /* Return immediately, with success or failure */

#define MRAPI_NODE_INVALID MCA_NODE_INVALID
#define MRAPI_DOMAIN_INVALID MCA_DOMAIN_INVALID
#define MRAPI_RETURN_VALUE_INVALID MCA_RETURN_VALUE_INVALID

#define MRAPI_NONE 0xffffffff

#define MRAPI_MUTEX_ID_ANY 0xffffffff
#define MRAPI_SEM_ID_ANY 0xffffffff
#define MRAPI_RWL_ID_ANY 0xffffffff
#define MRAPI_SHMEM_ID_ANY 0xffffffff
#define MRAPI_RMEM_ID_ANY 0xffffffff

#include "Fragments/mrapi_inc2_abb.h"
   
   /* The default remote memory access type for this implementation */   
#define MRAPI_RMEM_DEFAULT MRAPI_RMEM_DUMMY
   
 

#define MRAPI_MAX_STATUS_SIZE 32
 
   /* error codes */
typedef enum {
  MRAPI_SUCCESS,
  MRAPI_TIMEOUT,
  MRAPI_INCOMPLETE,
  MRAPI_ERR_ATTR_NUM,
  MRAPI_ERR_ATTR_READONLY,
  MRAPI_ERR_ATTR_SIZE,
  MRAPI_ERR_DOMAIN_INVALID,
  MRAPI_ERR_DOMAIN_NOTSHARED,
  MRAPI_ERR_MEM_LIMIT,
  MRAPI_ERR_MUTEX_DELETED,
  MRAPI_ERR_MUTEX_EXISTS,
  MRAPI_ERR_MUTEX_ID_INVALID,
  MRAPI_ERR_MUTEX_INVALID,
  MRAPI_ERR_MUTEX_KEY,
  MRAPI_ERR_MUTEX_LIMIT,
  MRAPI_ERR_MUTEX_LOCKED,
  MRAPI_ERR_MUTEX_LOCKORDER,
  MRAPI_ERR_MUTEX_NOTLOCKED,
  MRAPI_ERR_MUTEX_NOTVALID,
  MRAPI_ERR_NODE_INITFAILED,
  MRAPI_ERR_NODE_FINALFAILED,
  MRAPI_ERR_NODE_INITIALIZED,
  MRAPI_ERR_NODE_INVALID,
  MRAPI_ERR_NODE_NOTINIT,
  MRAPI_ERR_NOT_SUPPORTED,
  MRAPI_ERR_PARAMETER,
  MRAPI_ERR_REQUEST_CANCELED,
  MRAPI_ERR_REQUEST_INVALID,
  MRAPI_ERR_REQUEST_LIMIT,
  MRAPI_ERR_RMEM_ID_INVALID,
  MRAPI_ERR_RMEM_ATTACH,
  MRAPI_ERR_RMEM_ATTACHED,
  MRAPI_ERR_RMEM_ATYPE,
  MRAPI_ERR_RMEM_ATYPE_NOTVALID,
  MRAPI_ERR_RMEM_BLOCKED,
  MRAPI_ERR_RMEM_BUFF_OVERRUN,
  MRAPI_ERR_RMEM_CONFLICT,
  MRAPI_ERR_RMEM_EXISTS,
  MRAPI_ERR_RMEM_INVALID,
  MRAPI_ERR_RMEM_NOTATTACHED,
  MRAPI_ERR_RMEM_NOTOWNER,
  MRAPI_ERR_RMEM_STRIDE,
  MRAPI_ERR_RMEM_TYPENOTVALID,
  MRAPI_ERR_RSRC_COUNTER_INUSE,
  MRAPI_ERR_RSRC_INVALID,
  MRAPI_ERR_RSRC_INVALID_CALLBACK,
  MRAPI_ERR_RSRC_INVALID_EVENT,
  MRAPI_ERR_RSRC_INVALID_SUBSYSTEM,
  MRAPI_ERR_RSRC_INVALID_TREE,
  MRAPI_ERR_RSRC_NOTDYNAMIC,
  MRAPI_ERR_RSRC_NOTOWNER,
  MRAPI_ERR_RSRC_NOTSTARTED,
  MRAPI_ERR_RSRC_STARTED,
  MRAPI_ERR_RWL_DELETED,
  MRAPI_ERR_RWL_EXISTS,
  MRAPI_ERR_RWL_ID_INVALID,
  MRAPI_ERR_RWL_INVALID,
  MRAPI_ERR_RWL_LIMIT,
  MRAPI_ERR_RWL_LOCKED,
  MRAPI_ERR_RWL_NOTLOCKED,
  MRAPI_ERR_SEM_DELETED,
  MRAPI_ERR_SEM_EXISTS,
  MRAPI_ERR_SEM_ID_INVALID,
  MRAPI_ERR_SEM_INVALID,
  MRAPI_ERR_SEM_LIMIT,
  MRAPI_ERR_SEM_LOCKED,
  MRAPI_ERR_SEM_LOCKLIMIT,
  MRAPI_ERR_SEM_NOTLOCKED,
  MRAPI_ERR_SHM_ATTACHED,
  MRAPI_ERR_SHM_ATTCH,
  MRAPI_ERR_SHM_EXISTS,
  MRAPI_ERR_SHM_ID_INVALID,
  MRAPI_ERR_SHM_INVALID,
  MRAPI_ERR_SHM_NODES_INCOMPAT,
  MRAPI_ERR_SHM_NODE_NOTSHARED,
  MRAPI_ERR_SHM_NOTATTACHED,
  MRAPI_ERR_ATOM_INVALID_ARG,
  MRAPI_ERR_ATOM_OP_FAILED,
  MRAPI_ERR_ATOM_OP_NOFORWARD,
} mrapi_status_flags;


typedef enum {
  MRAPI_MUTEX_RECURSIVE,
  MRAPI_ERROR_EXT,
  MRAPI_DOMAIN_SHARED,
  MRAPI_SHMEM_RESOURCE,
  MRAPI_SHMEM_ADDRESS,
  MRAPI_SHMEM_SIZE,
} attributes;

typedef enum {
  MRAPI_RSRC_MEM_BASEADDR,
  MRAPI_RSRC_MEM_NUMWORDS,
  MRAPI_RSRC_MEM_WORDSIZE,
} mrapi_rsrc_mem_attrs;

typedef enum {
  MRAPI_RSRC_CACHE_SIZE,
  MRAPI_RSRC_CACHE_LINE_SIZE,
  MRAPI_RSRC_CACHE_ASSOCIATIVITY,
  MRAPI_RSRC_CACHE_LEVEL,
} mrapi_rsrc_cache_attrs;

typedef enum {
  MRAPI_RSRC_CPU_FREQUENCY,
  MRAPI_RSRC_CPU_TYPE,
  MRAPI_RSRC_CPU_ID,
} mrapi_rsrc_cpu_attrs;


/*-------------------------------------------------------------------
  Function declarations: misc
  -------------------------------------------------------------------*/
PUBLIC
char* mrapi_display_status(mrapi_status_t status,char* status_message,size_t size);
PUBLIC
void mrapi_set_debug_level(int d);
PUBLIC
mrapi_lock_type mrapi_lock_type_get(
	MRAPI_IN uint32_t hndl,
	MRAPI_OUT mrapi_status_t* status
);

/*-------------------------------------------------------------------
  MRAPI
  -------------------------------------------------------------------*/
PUBLIC
void mrapi_initialize(
                      MRAPI_IN mrapi_domain_t domain_id,
                      MRAPI_IN mrapi_node_t node_id,
                      MRAPI_IN mrapi_parameters_t init_parameters,
                      MRAPI_OUT mrapi_info_t* mrapi_info,
                      MRAPI_OUT mrapi_status_t* status
                      );

PUBLIC
void mrapi_finalize(
                    MRAPI_OUT mrapi_status_t* status
                    );

PUBLIC
mrapi_domain_t mrapi_domain_id_get(
                                   MRAPI_OUT mrapi_status_t* status
                                   );

PUBLIC
mrapi_node_t mrapi_node_id_get(
                               MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_mutex_hndl_t mrapi_mutex_create(
 	MRAPI_IN mrapi_mutex_id_t mutex_id,
 	MRAPI_IN mrapi_mutex_attributes_t* attributes,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_mutex_init_attributes(
 	MRAPI_OUT mrapi_mutex_attributes_t* attributes,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_mutex_set_attribute (
 	MRAPI_OUT mrapi_mutex_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_IN void* attribute,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_mutex_get_attribute (
 	MRAPI_IN mrapi_mutex_hndl_t mutex,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_OUT void* attribute,
 	MRAPI_IN size_t attribute_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_mutex_hndl_t mrapi_mutex_get(
 	MRAPI_IN mrapi_mutex_id_t mutex_id,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_mutex_delete(
 	MRAPI_IN mrapi_mutex_hndl_t mutex,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_mutex_lock (
 MRAPI_IN mrapi_mutex_hndl_t mutex,
 MRAPI_OUT mrapi_key_t* lock_key,
 MRAPI_IN mrapi_timeout_t timeout,
 MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_boolean_t mrapi_mutex_trylock(
 MRAPI_IN mrapi_mutex_hndl_t mutex,
 MRAPI_OUT mrapi_key_t* lock_key,
 MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_mutex_unlock(
 MRAPI_IN mrapi_mutex_hndl_t mutex,
 MRAPI_IN mrapi_key_t* lock_key,
 MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_sem_hndl_t mrapi_sem_create(
 	MRAPI_IN mrapi_sem_id_t sem_id,
 	MRAPI_IN mrapi_sem_attributes_t* attributes,
	MRAPI_IN mrapi_uint_t num_locks,
	MRAPI_IN mrapi_uint_t shared_lock_limit,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_sem_init_attributes(
 	MRAPI_OUT mrapi_sem_attributes_t* attributes,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_sem_set_attribute(
 	MRAPI_OUT mrapi_sem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_IN void* attribute,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_sem_get_attribute (
 	MRAPI_IN mrapi_sem_hndl_t sem,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_OUT void* attribute,
 	MRAPI_IN size_t attribute_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_sem_hndl_t mrapi_sem_get(
 	MRAPI_IN mrapi_sem_id_t sem_id,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_sem_delete(
 	MRAPI_IN mrapi_sem_hndl_t sem,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_sem_lock(
 MRAPI_IN mrapi_sem_hndl_t sem,
 MRAPI_IN mrapi_timeout_t timeout,
 MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_sem_lock_multiple(
	MRAPI_OUT mrapi_sem_hndl_t* sem,
	MRAPI_IN int count,
	MRAPI_IN mrapi_boolean_t waitall,
	MRAPI_IN mrapi_timeout_t timeout,
	MRAPI_OUT mrapi_status_t* status
);

PUBLIC
mrapi_boolean_t mrapi_sem_trylock(
 MRAPI_IN mrapi_sem_hndl_t sem,
 MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_boolean_t mrapi_sem_trylock_multiple(
	MRAPI_OUT mrapi_sem_hndl_t* sem,
	MRAPI_IN int count,
	MRAPI_IN mrapi_boolean_t waitall,
	MRAPI_OUT mrapi_status_t* status
);

PUBLIC
void mrapi_sem_unlock (
 MRAPI_IN mrapi_sem_hndl_t sem,
 MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_sem_post(
	MRAPI_IN mrapi_sem_hndl_t sem,
	MRAPI_OUT mrapi_status_t* status
);

PUBLIC
mrapi_rwl_hndl_t mrapi_rwl_create(
 	MRAPI_IN mrapi_rwl_id_t rwl_id,
 	MRAPI_IN mrapi_rwl_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t reader_lock_limit,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rwl_init_attributes(
 	MRAPI_OUT mrapi_rwl_attributes_t* attributes,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rwl_set_attribute(
 	MRAPI_OUT mrapi_rwl_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_IN void* attribute,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rwl_get_attribute (
 	MRAPI_IN mrapi_rwl_hndl_t rwl,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_OUT void* attribute,
 	MRAPI_IN size_t attribute_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_rwl_hndl_t mrapi_rwl_get(
 	MRAPI_IN mrapi_rwl_id_t rwl_id,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rwl_delete(
 	MRAPI_IN mrapi_rwl_hndl_t rwl,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rwl_lock(
 MRAPI_IN mrapi_rwl_hndl_t rwl,
 MRAPI_IN mrapi_rwl_mode_t mode,
 MRAPI_IN mrapi_timeout_t timeout,
 MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_boolean_t mrapi_rwl_trylock(
 MRAPI_IN mrapi_rwl_hndl_t rwl,
 MRAPI_IN mrapi_rwl_mode_t mode,
 MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rwl_unlock (
 MRAPI_IN mrapi_rwl_hndl_t rwl,
 MRAPI_IN mrapi_rwl_mode_t mode,
 MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_shmem_hndl_t mrapi_shmem_create(
 	MRAPI_IN mrapi_shmem_id_t shmem_id,
 	MRAPI_IN mrapi_uint_t size,
 	MRAPI_IN mrapi_node_t* nodes,
 	MRAPI_IN mrapi_uint_t nodes_size,
 	MRAPI_IN mrapi_shmem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attr_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_shmem_init_attributes(
 	MRAPI_OUT mrapi_shmem_attributes_t* attributes,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_shmem_set_attribute(
 	MRAPI_OUT mrapi_shmem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_IN void* attribute,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_shmem_get_attribute(
 	MRAPI_IN mrapi_shmem_hndl_t shmem,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_OUT void* attribute,
 	MRAPI_IN size_t attribute_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_shmem_hndl_t mrapi_shmem_get(
 	MRAPI_IN mrapi_shmem_id_t shmem_id,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void* mrapi_shmem_attach(
 	MRAPI_IN mrapi_shmem_hndl_t shmem,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_shmem_detach(
 	MRAPI_IN mrapi_shmem_hndl_t shmem,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_shmem_delete(
 	MRAPI_IN mrapi_shmem_hndl_t shmem,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_rmem_hndl_t mrapi_rmem_create(
 	MRAPI_IN mrapi_rmem_id_t rmem_id,
 	MRAPI_IN void* mem,
 	MRAPI_IN mrapi_rmem_atype_t access_type,
 	MRAPI_IN mrapi_rmem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rmem_init_attributes(
 	MRAPI_OUT mrapi_rmem_attributes_t* attributes,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rmem_set_attribute(
 	MRAPI_OUT mrapi_rmem_attributes_t* attributes,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_IN void* attribute,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rmem_get_attribute(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_IN mrapi_uint_t attribute_num,
 	MRAPI_OUT void* attribute,
 	MRAPI_IN size_t attribute_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
mrapi_rmem_hndl_t mrapi_rmem_get(
	 MRAPI_IN mrapi_rmem_id_t rmem_id,
	 MRAPI_IN mrapi_rmem_atype_t access_type,
	 MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rmem_attach(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_IN mrapi_rmem_atype_t access_type,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rmem_detach(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rmem_delete(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rmem_read(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_IN mrapi_uint32_t rmem_offset,
        MRAPI_OUT void* local_buf,
 	MRAPI_IN mrapi_uint32_t local_offset,
 	MRAPI_IN mrapi_uint32_t bytes_per_access,
	MRAPI_IN mrapi_uint32_t num_strides,
 	MRAPI_IN mrapi_uint32_t rmem_stride,
 	MRAPI_IN mrapi_uint32_t local_stride,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rmem_read_i(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_IN mrapi_uint32_t rmem_offset,
 	MRAPI_OUT void* local_buf,
 	MRAPI_IN mrapi_uint32_t local_offset,
 	MRAPI_IN mrapi_uint32_t bytes_per_access,
	MRAPI_IN mrapi_uint32_t num_strides,
 	MRAPI_IN mrapi_uint32_t rmem_stride,
 	MRAPI_IN mrapi_uint32_t local_stride,
 	MRAPI_OUT mrapi_request_t* mrapi_request,
 	MRAPI_OUT mrapi_status_t* status
     );

PUBLIC
void mrapi_rmem_write(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_IN mrapi_uint32_t rmem_offset,
 	MRAPI_IN void* local_buf,
 	MRAPI_IN mrapi_uint32_t local_offset,
 	MRAPI_IN mrapi_uint32_t bytes_per_access,
	MRAPI_IN mrapi_uint32_t num_strides,
 	MRAPI_IN mrapi_uint32_t rmem_stride,
 	MRAPI_IN mrapi_uint32_t local_stride,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_rmem_write_i(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_IN mrapi_uint32_t rmem_offset,
 	MRAPI_IN void* local_buf,
 	MRAPI_IN mrapi_uint32_t local_offset,
 	MRAPI_IN mrapi_uint32_t bytes_per_access,
	MRAPI_IN mrapi_uint32_t num_strides,
 	MRAPI_IN mrapi_uint32_t rmem_stride,
 	MRAPI_IN mrapi_uint32_t local_stride,
 	MRAPI_OUT mrapi_request_t* mrapi_request,
 	MRAPI_OUT mrapi_status_t* status
     );

PUBLIC
void mrapi_rmem_flush(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_OUT mrapi_status_t* status
 	);

PUBLIC
void mrapi_rmem_synch(
 	MRAPI_IN mrapi_rmem_hndl_t rmem,
 	MRAPI_OUT mrapi_status_t* status
 	);

PUBLIC
mrapi_boolean_t mrapi_test(
        MRAPI_IN mrapi_request_t* request,
        MRAPI_OUT size_t* size,
        MRAPI_OUT mrapi_status_t* mrapi_status);

PUBLIC
mrapi_boolean_t mrapi_wait(
        MRAPI_IN mrapi_request_t* request,
        MRAPI_OUT size_t* size,
        MRAPI_IN mrapi_timeout_t timeout,
        MRAPI_OUT mrapi_status_t* mrapi_status
);

PUBLIC
mrapi_uint_t mrapi_wait_any(
        MRAPI_IN size_t number,
        MRAPI_IN mrapi_request_t* requests,
        MRAPI_OUT size_t* size,
        MRAPI_IN mrapi_timeout_t timeout,
        MRAPI_OUT mrapi_status_t* mrapi_status
);

PUBLIC
void mrapi_cancel(
        MRAPI_IN mrapi_request_t* request,
        MRAPI_OUT mrapi_status_t* mrapi_status
);


PUBLIC
mrapi_resource_t* mrapi_resources_get(
 	MRAPI_IN mrapi_rsrc_filter_t subsystem_filter,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_resource_get_attribute(
 	MRAPI_IN mrapi_resource_t* resource,
 	MRAPI_IN mrapi_uint_t attribute_number,
 	MRAPI_OUT void* attribute_value,
 	MRAPI_IN size_t attr_size,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_dynamic_attribute_start(
 	MRAPI_IN mrapi_resource_t* resource,
 	MRAPI_IN mrapi_uint_t attribute_number,
	MRAPI_FUNCTION_PTR void (*rollover_callback) (void),
	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_dynamic_attribute_reset(
 	MRAPI_IN mrapi_resource_t *resource,
 	MRAPI_IN mrapi_uint_t attribute_number,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_dynamic_attribute_stop(
 	MRAPI_IN mrapi_resource_t* resource,
 	MRAPI_IN mrapi_uint_t attribute_number,
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_resource_register_callback(
 	MRAPI_IN mrapi_event_t event,
 	MRAPI_IN unsigned int frequency,
 	MRAPI_FUNCTION_PTR void (*callback_function) (mrapi_event_t event),
 	MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_resource_tree_free(
 	mrapi_resource_t* MRAPI_IN * root_ptr,
 	MRAPI_OUT mrapi_status_t* status
 );


PUBLIC
void mrapi_barrier_init(
    MRAPI_OUT mrapi_atomic_barrier_t* axb,
    MRAPI_IN pid_t dest,
    MRAPI_OUT mrapi_msg_t* buffer,
    MRAPI_IN unsigned elems,
    MRAPI_IN size_t size,
    MRAPI_OUT unsigned* pindex,
    MRAPI_IN mca_timeout_t timeout,
    MRAPI_OUT mrapi_status_t* status
  );

PUBLIC
void mrapi_exchange_init(
    MRAPI_OUT mrapi_atomic_barrier_t* axb,
    MRAPI_IN pid_t dest,
    MRAPI_OUT mrapi_msg_t* buffer,
    MRAPI_IN unsigned elems,
    MRAPI_IN size_t size,
    MRAPI_OUT unsigned* pindex,
    MRAPI_IN mca_timeout_t timeout,
    MRAPI_OUT mrapi_status_t* status
  );

PUBLIC
mrapi_boolean_t mrapi_atomic_hold(
    MRAPI_OUT mrapi_atomic_barrier_t* axb,
    MRAPI_IN mrapi_boolean_t hold,
    MRAPI_OUT mrapi_status_t* status
  );

PUBLIC
void mrapi_atomic_read(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* value,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_read_ptr(
    MRAPI_OUT void* sync,
    MRAPI_IN uintptr_t* dest,
    MRAPI_OUT uintptr_t* value,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_add(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* value,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_inc(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_OUT void* result,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_dec(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_OUT void* result,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_or(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* value,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_and(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* value,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_xor(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* value,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_cas(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* exchange,
    MRAPI_IN void* compare,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_xchg(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* exchange,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_cas_ptr(
    MRAPI_OUT void* sync,
    MRAPI_IN uintptr_t* dest,
    MRAPI_IN uintptr_t exchange,
    MRAPI_IN uintptr_t compare,
    MRAPI_OUT uintptr_t* previous,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_xchg_ptr(
    MRAPI_OUT void* sync,
    MRAPI_IN uintptr_t* dest,
    MRAPI_IN uintptr_t exchange,
    MRAPI_OUT uintptr_t* previous,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_sync(
    MRAPI_OUT void* sync,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_lock(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_release(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_set(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN int bit,
    MRAPI_OUT int* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_clear(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN int bit,
    MRAPI_OUT int* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

PUBLIC
void mrapi_atomic_change(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN int bit,
    MRAPI_OUT int* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status
 );

#endif

#ifdef __cplusplus
} 
#endif /* __cplusplus */
