#if !(__unix__||__MINGW32__)
#undef TLS
#define TLS __declspec(thread)
#else
#define TLS __thread
#endif  /* (__unix__||__MINGW32__) */

extern "C" {
  extern mrapi_database* mrapi_db;
  mrapi_boolean_t mrapi_impl_access_database_pre (int id,
                                                  int member,
                                                  mrapi_boolean_t fail_on_error);
  mrapi_boolean_t mrapi_impl_access_database_post (int id,int member);
  mrapi_boolean_t mrapi_impl_whoami (mrapi_node_t* node_id,
                                     uint32_t* n_index,
                                     mrapi_domain_t* domain_id,
                                     uint32_t* d_index);
  uint32_t mrapi_impl_encode_hndl (uint16_t type_index); 
  mrapi_boolean_t mrapi_impl_decode_hndl (uint32_t handle,uint16_t *type_index);
  unsigned mrapi_impl_setup_request();
  mrapi_boolean_t mrapi_impl_create_sys_semaphore (int* id, 
                                                   int num_locks, 
                                                   int key, 
                                                   mrapi_boolean_t lock) ;
  mrapi_boolean_t mrapi_impl_create_lock_locked(mrapi_sem_hndl_t* sem,  
                                       mrapi_sem_id_t key,
                                       mrapi_uint32_t shared_lock_limit,
                                       lock_type t,
                                       mrapi_status_t* mrapi_status);
  void mrapi_impl_yield_locked (int which_semid,int member);
  mrapi_boolean_t mrapi_impl_valid_mutex_hndl(mrapi_mutex_hndl_t mutex, mrapi_status_t* status);
}
