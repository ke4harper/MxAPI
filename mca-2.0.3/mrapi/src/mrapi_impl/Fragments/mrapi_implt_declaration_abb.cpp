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

#if !(__unix__||__MINGW32__)
#undef TLS
#define TLS __declspec(thread)
#else
#define TLS __thread
#endif  /* (__unix__||__MINGW32__) */

typedef struct {
  uint8_t cValue;
  uint16_t wValue;
  uint32_t lValue;
  uint64_t llValue;
  uintptr_t pValue2[1];
  uintptr_t pValue1[1];
  unsigned index;
  mrapi_msg_t msg[4];
} atomic_data;

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
