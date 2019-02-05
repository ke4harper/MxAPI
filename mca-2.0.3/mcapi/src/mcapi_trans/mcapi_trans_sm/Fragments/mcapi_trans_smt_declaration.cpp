#define MAGIC_NUM 0xdeadcafe
#define MCAPI_VALID_MASK 0x80000000

extern "C" {
  extern mcapi_database* mcapi_db;
  extern uint32_t global_rwl;
  mcapi_boolean_t mcapi_trans_whoami (mcapi_node_t* node_id,
                                      uint32_t* n_index,
                                      mcapi_domain_t* domain_id,
                                      uint32_t* d_index);
  uint32_t mcapi_trans_encode_handle (uint16_t domain_index,
                                               uint16_t node_index,
                                               uint16_t endpoint_index);
  mcapi_boolean_t mcapi_trans_decode_handle (uint32_t handle,
                                                      uint16_t* domain_index,
                                                      uint16_t *node_index,
                                                      uint16_t *endpoint_index);
  void mcapi_trans_init_indexed_array_have_lock();
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
  mcapi_boolean_t mcapi_trans_remove_request_have_lock(int r);
  mcapi_boolean_t mcapi_trans_access_database_pre (uint32_t handle,mcapi_boolean_t exclusive);
  mcapi_boolean_t mcapi_trans_access_database_post (uint32_t handle,mcapi_boolean_t exclusive);
  mcapi_boolean_t mcapi_trans_endpoint_get_have_lock (mcapi_endpoint_t *e,
                                                     mcapi_domain_t domain_id,
                                                     mcapi_uint_t node_num,
                                                     mcapi_uint_t port_num);
  mcapi_boolean_t mcapi_trans_decode_request_handle(mcapi_request_t* request,
                                                    uint16_t* r);
  void check_get_endpt_request_have_lock (mcapi_request_t *request);
  mcapi_boolean_t mcapi_trans_wait( mcapi_request_t* request,
                                    size_t* size,
                                    mcapi_status_t* mcapi_status,
                                    mcapi_timeout_t timeout);
  mcapi_boolean_t mcapi_trans_msg_send( mcapi_endpoint_t  send_endpoint,
                                        mcapi_endpoint_t  receive_endpoint,
                                        const char* buffer,
                                        size_t buffer_size);
  mcapi_boolean_t mcapi_trans_send_have_lock (uint16_t sd, uint16_t sn,uint16_t se,
                                             uint16_t rd, uint16_t rn,uint16_t re,
                                             const char* buffer,
                                             size_t buffer_size,
                                             uint64_t scalar);
  void print_queue (queue q);
  mcapi_boolean_t mcapi_trans_empty_queue (queue q);
  mcapi_boolean_t mcapi_trans_full_queue (queue q);
  void mcapi_trans_compact_queue (queue* q);
  int mcapi_trans_push_queue (queue *q);
  int mcapi_trans_pop_queue (queue *q);
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
  void mcapi_trans_pktchan_connect_i( mcapi_endpoint_t  send_endpoint,
                                      mcapi_endpoint_t  receive_endpoint,
                                      mcapi_request_t* request,
                                      mcapi_status_t* mcapi_status);
  void mcapi_trans_connect_channel_have_lock (mcapi_endpoint_t send_endpoint,
                                             mcapi_endpoint_t receive_endpoint,
                                             channel_type type);
  void mcapi_trans_open_channel_have_lock (uint16_t d,uint16_t n, uint16_t e);
  void mcapi_trans_close_channel_have_lock (uint16_t d,uint16_t n, uint16_t e);
}
