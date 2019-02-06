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

/* do not put the following globals in thread-local-storage for two reasons:
     1) they are reset when a system call occurs which prevents us from being able
     to clean up properly
     2) gdb can't see thread local storage and since the database is one of the main */
  mrapi_database* mrapi_db = NULL; /* our shared memory addr for our internal database */
  int shmemid = -1; /* our shared memory id for our internal database */
  int semid = -1; /* our global semaphore id for locking/unlocking our database */

#if !(__unix__)
HANDLE hAtomicListener = NULL;
HANDLE mrapi_atomic_evt[MRAPI_MAX_PROCESSES] = { 0 }; /* Windows atomic event handles */
static wchar_t wszLocal[] = L"Local\\";
static wchar_t wszAtomic[] = L"_mrapi_atomic_";
#endif  /* !(__unix__) */

#if !(__unix__||__MINGW32__)
#undef TLS
#define TLS __declspec(thread)
#else
#define TLS __thread
#endif  /* (__unix__||__MINGW32__) */
  TLS mrapi_resource_t* resource_root; /* root of the resource tree */
  TLS struct sigaction alarm_struct; /* used for testing resource tree */
  TLS pid_t mrapi_pid = (pid_t)-1;
  TLS int mrapi_proc = (unsigned)0;
  TLS pthread_t mrapi_tid;
  TLS unsigned mrapi_nindex = (unsigned)-1;
  TLS unsigned mrapi_dindex = (unsigned)-1;
  TLS unsigned mrapi_pindex = (unsigned)-1;
  TLS mrapi_node_t mrapi_node_id;
  TLS mrapi_domain_t mrapi_domain_id;
  // finer grained locks for these sections of the database
  TLS int requests_semid; // requests array
  TLS int sems_semid;   // sems array
  TLS int shmems_semid; // shmems array
  TLS int rmems_semid;  // rmems array

  // tell the system whether or not to use the finer-grained locking
#define use_global_only 0

/*-------------------------------------------------------------------
  the mrapi_impl private function declarations (not part of the API)
  -------------------------------------------------------------------*/
/* Note:
  Anytime we read or write the database, we must be in a critical section.  Thus
  you will see a call to access_database_pre(acquires the semaphore) at the beginning
  of most functions and a call to access_database_post (releases the semaphore) at the
  end of most functions.  If a function has _locked appended to it's name, then
  it expects the semaphore to already be locked. */

/* mutexes, semaphores and reader-writer locks share a lot of the same code in this implementation */
  int32_t mrapi_impl_acquire_lock_locked(mrapi_sem_hndl_t sem,
                                        int32_t num_locks,
                                        mrapi_status_t* status);

  mrapi_boolean_t mrapi_impl_release_lock(mrapi_sem_hndl_t sem,
                                         int32_t num_locks,
                                         mrapi_status_t* mrapi_status);

  mrapi_boolean_t mrapi_impl_create_lock_locked(mrapi_sem_hndl_t* sem,
                                                  mrapi_sem_id_t key,
                                                  mrapi_uint32_t shared_lock_limit,
                                                  lock_type t,
                                                  mrapi_status_t* mrapi_status);

  mrapi_boolean_t mrapi_impl_valid_lock_hndl (mrapi_sem_hndl_t sem,
                                              mrapi_status_t* status);

  mrapi_boolean_t mrapi_impl_access_database_pre (int id,
                                                  int member,
                                                  mrapi_boolean_t fail_on_error);

  mrapi_boolean_t mrapi_impl_access_database_post (int id,int member);

  mrapi_boolean_t mrapi_impl_whoami (mrapi_node_t* node_id,
                                              uint32_t* n_index,
                                              mrapi_domain_t* domain_id,
                                              uint32_t* d_index);

  mrapi_boolean_t mrapi_impl_free_resources(mrapi_boolean_t panic);

  uint32_t mrapi_impl_encode_hndl (uint16_t type_index);

  mrapi_boolean_t mrapi_impl_decode_hndl (uint32_t handle,uint16_t *type_index);

  /* resource utilities */
  uint16_t mrapi_impl_get_number_of_nodes(mrapi_resource_type rsrc_type,
                                          mrapi_resource_t *tree);

  uint16_t mrapi_impl_create_rsrc_tree(mrapi_resource_type rsrc_type,
                                       mrapi_resource_t *src_tree,
                                       uint16_t start_index_child,
                                       mrapi_resource_t *filtered_tree);

  void mrapi_impl_increment_cache_hits(mrapi_resource_t *resource,
                                       int increment);

  void mrapi_impl_trigger_rollover(uint16_t index);

  void mrapi_impl_cause_event();

  void mrapi_impl_alarm_catcher(int signal);
  void mrapi_impl_atomic_listener(void* arg);
  mrapi_boolean_t mrapi_impl_atomic_forward(uint16_t s,mrapi_atomic_op *op,mrapi_status_t* status);

#define mrapi_assert(x) MRAPI_ASSERT(x,__FILE__,__LINE__);
  void MRAPI_ASSERT(mrapi_boolean_t condition,char* file,unsigned line) {
    if (!condition) {
      fprintf(stderr,"INTERNAL ERROR: MRAPI failed assertion (%s:%d) shutting down\n",file,line);
      mrapi_impl_free_resources(1/*panic*/);
      exit(1);
    }
  }
