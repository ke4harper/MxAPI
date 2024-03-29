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
           datatypes
******************************************************************/

typedef enum {
  MRAPI_RWL,
  MRAPI_SEM,
  MRAPI_MUTEX,
} lock_type;

/* Metadata resource related structs */
typedef enum {
  MRAPI_CPU,
  MRAPI_CACHE,
  MRAPI_MEM,
  MRAPI_CORE_COMPLEX,
  MRAPI_CROSSBAR,
  MRAPI_SYSTEM,
} mrapi_resource_type;


typedef enum {
  MRAPI_ATTR_STATIC,
  MRAPI_ATTR_DYNAMIC,
} mrapi_attribute_static;


typedef enum {
  MRAPI_EVENT_POWER_MANAGEMENT,
  MRAPI_EVENT_CROSSBAR_BUFFER_UNDER_20PERCENT,
  MRAPI_EVENT_CROSSBAR_BUFFER_OVER_80PERCENT,
} mrapi_event_t;

typedef enum {
  MRAPI_ATOM_NOOP,
  MRAPI_ATOM_OPENPROC,
  MRAPI_ATOM_CLOSEPROC,
  MRAPI_ATOM_SHMDUP,
} mrapi_atomic_t;


/* Part of this struct is opaque */
typedef struct mrapi_resource {
  char*                         name;
  mrapi_resource_type           resource_type;
  uint32_t                      number_of_children;
  struct mrapi_resource       **children;
  uint32_t                      number_of_attributes;
  void                        **attribute_types;
  void                        **attribute_values;
  mrapi_attribute_static      **attribute_static;
  mca_boolean_t               **attribute_started;
} mrapi_resource_t;


typedef struct {
  mca_boolean_t ext_error_checking;
  mca_boolean_t shared_across_domains;
  mca_boolean_t recursive; /* only applies to mutexes */
  mca_boolean_t spinlock_guard;
  void* mem_addr;
  uint32_t mem_size;
  mrapi_resource_t resource;
} mrapi_impl_attributes_t;

#define mrapi_impl_sem_ref_t sem_ref_t

typedef mrapi_impl_attributes_t mrapi_mutex_attributes_t;
typedef mrapi_impl_attributes_t mrapi_sem_attributes_t;
typedef mrapi_impl_attributes_t mrapi_rwl_attributes_t;
typedef mrapi_impl_attributes_t mrapi_shmem_attributes_t;
typedef mrapi_impl_attributes_t mrapi_rmem_attributes_t;

typedef uint32_t mrapi_mutex_hndl_t;
typedef uint32_t mrapi_sem_hndl_t;
typedef uint32_t mrapi_rwl_hndl_t;
typedef uint32_t mrapi_shmem_hndl_t;
typedef uint32_t mrapi_rmem_hndl_t;

typedef int mrapi_key_t;  /* system created key used for locking/unlocking for recursive mutexes */


#define MRAPI_WSTRING(text) L ## text
#define MRAPI_XWSTRING(text) MRAPI_WSTRING(text)

#define MRAPI_MAX_NODES MCA_MAX_NODES
#define MRAPI_MAX_PROCESSES MCA_MAX_PROCESSES
#define MRAPI_MAX_DOMAINS MCA_MAX_DOMAINS
#define MRAPI_MAX_CALLBACKS 10
#define MRAPI_ATOMIC_NULL (-1)


/*-------------------------------------------------------------------
  the mrapi_impl database structure
  -------------------------------------------------------------------*/
/* resource structure */
typedef struct {
  void (*callback_func) (mrapi_event_t the_event);
  mrapi_event_t callback_event;
  unsigned int callback_frequency;
  unsigned int callback_count;
  mrapi_node_t node_id;
} mrapi_callback_t;

/* lock state is atomic */
typedef struct {
  mrapi_uint32_t lock_key;
  mrapi_uint8_t lock_holder_nindex;
  mrapi_uint8_t lock_holder_dindex;
  mrapi_uint8_t id;
  struct {
	uint8_t valid : 1;
	uint8_t locked : 1;
	uint8_t : 6;
  };
} mrapi_lock_t;

/* mutexes, semaphores and reader-writer locks share this data structure */
typedef struct {
  uint32_t handle; /* used for reverse look-up when ext error checking is enabled */
  int32_t num_locks;
  mrapi_lock_t locks [MRAPI_MAX_SHARED_LOCKS];
  int32_t key; /* the shared key passed in on get/create */
  int32_t spin;
  int32_t shared_lock_limit;
  lock_type   type;
  mrapi_sem_attributes_t attributes;
  mrapi_boolean_t valid;
  /* only used when ext error checking is enabled.  Basically protects the
     entry from ever being overwritten. */
  mrapi_boolean_t deleted;
  uint8_t refs; /* the number of nodes using the sem (for reference counting) */
} mrapi_sem_t;

/* shared memory */
typedef struct {
  mrapi_boolean_t valid;
  int32_t key; /* the shared key passed in on get/create */
  uint32_t id[MRAPI_MAX_PROCESSES]; /* the handle returned by the os or whoever creates it */
  void* addr[MRAPI_MAX_PROCESSES];
  uint32_t size;
  mrapi_shmem_attributes_t attributes;
  uint8_t refs; /* the number of nodes currently attached (for reference counting) */
  mrapi_uint8_t num_procs;
  uint8_t processes[MRAPI_MAX_PROCESSES]; /* the list of processes currently attached */
} mrapi_shmem_t;

/* remote memory */
typedef struct {
  mrapi_boolean_t valid;
  mrapi_rmem_atype_t access_type;
  int32_t key; /* the shared key passed in on get/create */
  const void* addr;
  size_t size;
  mrapi_rmem_attributes_t attributes;
  uint8_t refs; /* the number of nodes currently attached (for reference counting) */
  uint8_t nodes[MRAPI_MAX_NODES]; /* the list of nodes currently attached */
} mrapi_rmem_t;

typedef union {
  struct {
    mrapi_uint_t node_num;
    mrapi_boolean_t allocated;
    mrapi_boolean_t valid;
  } data;
  uint64_t buf;
} mrapi_node_state;

typedef struct {
  struct sigaction signals[MCA_MAX_SIGNALS];
#if !(__unix__)
  HANDLE hAlarm;
  LARGE_INTEGER liDueTime;
#endif  /* !(__unix__) */
  mrapi_node_state state;
  pthread_t tid;
  mrapi_uint_t proc_num;
  uint8_t sems [MRAPI_MAX_SEMS]; // list of sems this node is referencing
  uint8_t shmems [MRAPI_MAX_SHMEMS]; // list of shmems this node is referencing
} mrapi_node_data;

typedef union {
  struct {
    mca_domain_t domain_id;
    mrapi_boolean_t allocated;
    mrapi_boolean_t valid;
  } data;
  uint64_t buf;
} mrapi_domain_state;

typedef struct{
  mrapi_uint16_t num_nodes; // not decremented
  mrapi_domain_state state;
  mrapi_node_data nodes [MRAPI_MAX_NODES];
} mrapi_domain_data;

typedef struct {
  mrapi_boolean_t valid;
  mrapi_boolean_t completed;
  mrapi_boolean_t cancelled;
  uint32_t node_num;
  mrapi_domain_t domain_id;
  mrapi_status_t status;
} mrapi_request_data;

/* atomic operation */
typedef struct {
  mrapi_atomic_t type;
  int spindex; /* source process index */
  mrapi_boolean_t valid;
  mrapi_shmem_hndl_t shmem;
  uint32_t dest; /* offset from base addr */
  union {
    struct {
      int notused;
    } open;
    struct {
      int notused;
    } close;
    struct {
      int s; /* shared memory index */
      int source; /* handle source */
    } dup;
    struct {
      int notused;
    } sync;
  } u;
} mrapi_atomic_op;

typedef union {
  struct {
    pid_t pid;
    mrapi_boolean_t allocated;
    mrapi_boolean_t valid;
  } data;
  uint64_t buf;
} mrapi_process_state;

/* process address space */
typedef struct {
  mrapi_process_state state;
  mrapi_uint16_t num_nodes;
  int proc; /* process ID for duplicating shared memory */
  int link[MRAPI_MAX_PROCESSES]; /* 1 if can be signaled */
#if !(__unix__)
  /* event object for atomic operation signaling:
     "Local\mrapi_atomic_<process-index>" naming convention */
  HANDLE hAtomicEvt;
#else
  /* signal SIGUSR1 indicates atomic operation signal */
  struct sigaction atomic;
#endif  /* (__unix__) */
  mrapi_atomic_op op;
} mrapi_process_data;

struct lock_t {
  volatile uint32_t lock;
};

typedef struct {
  int32_t num_locks;
  struct lock_t locks [MRAPI_MAX_SHARED_LOCKS];
  int32_t key; /* the shared key passed in on get/create */
  mrapi_boolean_t valid;
} mrapi_sys_sem_t;

typedef struct {
  struct lock_t global_lock; // not used
  mrapi_uint8_t num_shmems;
  mrapi_uint8_t num_sems; // not decremented
  mrapi_uint8_t num_rmems; // not decremented
  mrapi_uint8_t num_domains; // not used
  mrapi_uint8_t num_processes; // not used
  mrapi_shmem_t shmems[MRAPI_MAX_SHMEMS];
  mrapi_sem_t sems[MRAPI_MAX_SEMS];
  mrapi_sys_sem_t sys_sems[MRAPI_MAX_SEMS];
  mrapi_rmem_t rmems [MRAPI_MAX_RMEMS];
  mrapi_domain_data domains [MRAPI_MAX_DOMAINS];
  mrapi_request_data requests [MRAPI_MAX_REQUESTS];
  mrapi_process_data processes [MRAPI_MAX_PROCESSES];
  /* Rollover variables */
  void (*rollover_callbacks_ptr[MRAPI_MAX_CALLBACKS]) (void);
  mrapi_uint16_t rollover_index;
  /* Callback variables */
  mrapi_callback_t callbacks_array[MRAPI_MAX_CALLBACKS];
  mrapi_uint16_t   callback_index;
} mrapi_database;
