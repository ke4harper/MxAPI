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

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                   Constants and defines                                  //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define MAGIC_NUM 0xdeadcafe
#define MCAPI_VALID_MASK 0x80000000


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                   Function prototypes (private)                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
  mcapi_boolean_t mcapi_trans_access_database_pre (uint32_t handle,mcapi_boolean_t exclusive);

  mcapi_boolean_t mcapi_trans_access_database_post (uint32_t handle,mcapi_boolean_t exclusive);

  mcapi_boolean_t mcapi_trans_decode_request_handle(mcapi_request_t* request,
                                                    uint16_t* r);

  mcapi_boolean_t mcapi_trans_decode_handle (uint32_t handle,
                                                      uint16_t* domain_index,
                                                      uint16_t *node_index,
                                                      uint16_t *endpoint_index);

  uint32_t mcapi_trans_encode_handle (uint16_t domain_index,
                                               uint16_t node_index,
                                               uint16_t endpoint_index);
  void mcapi_trans_signal_handler ( int sig );

  mcapi_boolean_t mcapi_trans_send_have_lock (uint16_t sd, uint16_t sn,uint16_t se,
                                             uint16_t rd, uint16_t rn,uint16_t re,
                                             const char* buffer,
                                             size_t buffer_size,
                                             uint64_t scalar);

  mcapi_boolean_t mcapi_trans_recv_have_lock (uint16_t rd, uint16_t rn, uint16_t re,
                                             void** buffer,
                                             size_t buffer_size,
                                             size_t* received_size,
                                             mcapi_boolean_t blocking,
                                             uint64_t* scalar);

  void mcapi_trans_recv_have_lock_ (uint16_t rd, uint16_t rn, uint16_t re,
                                   void** buffer,
                                   size_t buffer_size,
                                   size_t* received_size,
                                   int qindex,
                                   uint64_t* scalar);

  mcapi_boolean_t mcapi_trans_endpoint_get_have_lock (mcapi_endpoint_t *e,
                                                     mcapi_domain_t domain_id,
                                                     mcapi_uint_t node_num,
                                                     mcapi_uint_t port_num);

  void mcapi_trans_open_channel_have_lock (uint16_t d,uint16_t n, uint16_t e);

  void mcapi_trans_close_channel_have_lock (uint16_t d,uint16_t n, uint16_t e);

  void mcapi_trans_connect_channel_have_lock (mcapi_endpoint_t send_endpoint,
                                             mcapi_endpoint_t receive_endpoint,
                                             channel_type type);

  mcapi_boolean_t mcapi_trans_reserve_request_have_lock(int* r);

  mcapi_boolean_t setup_request_have_lock (mcapi_endpoint_t* endpoint,
                                          mcapi_request_t* request,
                                          mcapi_status_t* mcapi_status,
                                          mcapi_boolean_t completed,
                                          size_t size,
                                          void** buffer,
                                          mcapi_request_type type,
                                          mcapi_uint_t node_num,
                                          mcapi_uint_t port_num,
                                          mcapi_domain_t domain_num,
                                          int r);

  void check_receive_request_have_lock (mcapi_request_t *request);

  void cancel_receive_request_have_lock (mcapi_request_t *request);

  void check_get_endpt_request_have_lock (mcapi_request_t *request);

  void check_open_channel_request_have_lock (mcapi_request_t *request);

  void check_close_channel_request_have_lock (mcapi_request_t *request);

  mcapi_boolean_t mcapi_trans_whoami (mcapi_node_t* node_id,
                                                      uint32_t* n_index,
                                                      mcapi_domain_t* domain_id,
                                                      uint32_t* d_index);

  void mcapi_trans_display_state_have_lock (void* handle);

  void mcapi_trans_yield_have_lock();

  /* queue management */
  void print_queue (queue q);
  int mcapi_trans_pop_queue (queue *q);
  int mcapi_trans_push_queue (queue *q);
  mcapi_boolean_t mcapi_trans_empty_queue (queue q);
  mcapi_boolean_t mcapi_trans_full_queue (queue q);
  void mcapi_trans_compact_queue (queue* q);
  mcapi_boolean_t mcapi_trans_endpoint_create_(mcapi_endpoint_t* ep,
                                               mcapi_domain_t domain_id,
                                               mcapi_node_t node_num,
                                               mcapi_uint_t port_num,
                                               mcapi_boolean_t anonymous);


#define mcapi_assert(x) MCAPI_ASSERT(x,__LINE__);
  void MCAPI_ASSERT(mcapi_boolean_t condition,unsigned line) {
    if (!condition) {
      fprintf(stderr,"INTERNAL ERROR: MCAPI failed assertion (transport_sm.c:%d) shutting down\n",line);
      mcapi_trans_signal_handler(SIGABRT);
      exit(1);
    }
  }

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                   Data                                                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

/* public globals (use thread-local-storage) */

#if !(__unix__||__MINGW32__)
#undef TLS
#define TLS __declspec(thread)
#else
#define TLS __thread
#endif  /* (__unix__||__MINGW32__) */

  /* do not put the database in thread-local-storage, it causes the pointer to be nulled when a
     system call occurs (like cntrl-c) and gdb can't see into it */
  mcapi_database* mcapi_db = NULL; /* our shared memory addr for our have_lock database */
  uint32_t global_rwl = 0; /* the global database lock */
  int mcapi_initialize_ctr = 0; /* guard against rundown during concurrent initialization */
  int mcapi_finalize_ctr = 0; /* guard against initialization during concurrent rundown */
  TLS pid_t mcapi_pid = -1;
  TLS pthread_t mcapi_tid;
  TLS unsigned mcapi_nindex = 0;
  TLS unsigned mcapi_dindex = 0;
  TLS int mcapi_debug = 0;
  TLS mcapi_node_t mcapi_node_num = 0;
  TLS mcapi_domain_t mcapi_domain_id = 0;
  TLS int locked = 0; /* detect out-of-order lock operations on single thread */
