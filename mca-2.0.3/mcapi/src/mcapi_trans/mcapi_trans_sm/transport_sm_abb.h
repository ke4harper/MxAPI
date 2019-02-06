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

#ifndef _TRANSPORT_SM_ABB_H_
#define _TRANSPORT_SM_ABB_H_

#include "mcapi_impl_spec_abb.h"
#include <mca_config_abb.h>
#include <mrapi_abb.h>
#include <stdarg.h> /* for va_list */
#include <stdio.h> /* for the inlined dprintf routine */
#if (__unix__)
#include <pthread.h> /* for getting the tid (pthread_self) */
#endif  /* (__unix__) */
#include <sys/types.h> /* for pid_t */
#if (__unix__)
#include <unistd.h> /* for memset and getpid */
#endif  /* (__unix__) */
#include <signal.h> /* for signals */

/*******************************************************************
  definitions and constants
*******************************************************************/    

/* we leave one empty element so that the array implementation 
   can tell the difference between empty and full */
#define MCAPI_MAX_QUEUE_ENTRIES (MCAPI_MAX_QUEUE_ELEMENTS + 1)
  
#define MCAPI_MAX_REQUESTS MCA_MAX_REQUESTS 
#define MCAPI_MAX_REQUEST_PARTITIONS MCAPI_MAX_REQUESTS/(8*sizeof(uint32_t))+1

#define MCAPI_RAND_MAX RAND_MAX

#define mcapi_dprintf mca_dprintf
  
#define MCAPI_MAX(X,Y) ((X) > (Y) ? (X) : (Y))

/*******************************************************************
 The mcapi database
*******************************************************************/    
  
/* buffer entry is used for msgs, pkts and scalars */
/* NOTE: if you change the buffer_entry data structure then you also
   need to update the pointer arithmetic in mcapi_trans_pktchan_free */
typedef struct {
  uint32_t magic_num;
  size_t size; /* size (in bytes) of the buffer */
  uint64_t scalar;
  char buff [MCAPI_MAX(MCAPI_MAX_PKT_SIZE,MCAPI_MAX_MSG_SIZE)]; // the buffer is used for both pkts and msgs
  void* parent; /* queue entry buffer descriptor (local address) */
} buffer_entry;

typedef enum {
  OTHER,
  CONN_PKTCHAN,
  CONN_SCLCHAN,
  OPEN_PKTCHAN,
  OPEN_SCLCHAN,
  CLOSE_PKTCHAN,
  CLOSE_SCLCHAN,
  SEND,
  RECV,
  GET_ENDPT
} mcapi_request_type;

typedef enum {
  REQUEST_FREE = 0,
  REQUEST_VALID,
  REQUEST_RECEIVED,
  REQUEST_COMPLETED,
  REQUEST_CANCELLED
} mcapi_request_state;

typedef enum {
  BUFFER_FREE = 0,
  BUFFER_RESERVED,
  BUFFER_ALLOCATED,
  BUFFER_RECEIVED
} mcapi_buffer_state;

typedef struct {
  size_t size;
  mcapi_request_type type;
  void* buffer;
  void** buffer_ptr;
  uint32_t ep_node_num; /* used only for get_endpoint */
  uint32_t ep_port_num; /* used only for get_endpoint */
  mca_domain_t ep_domain_num; /* used only for get_endpoint */
  mcapi_request_state state;
  mcapi_endpoint_t handle;
  mca_status_t status;
  mcapi_endpoint_t* ep_endpoint;
  int ep_qindex;
} mcapi_request_data;

typedef struct  {
  uint32_t set[MCAPI_MAX_REQUEST_PARTITIONS];
  int curr_count;
  int max_count;
} indexed_array_header;

typedef struct {
  mcapi_buffer_state state;
  int32_t buff_index; /* the pointer to the actual buffer entry in the buffer pool */
  mcapi_boolean_t reserved;
} buffer_descriptor;

/* only valid for channels */
typedef union {
  struct {
    mcapi_endpoint_t send_endpt;
    mcapi_endpoint_t recv_endpt;
  } data;
  uint32_t buf;
} queue_state;

typedef struct {
  mrapi_msg_t msg; /* must be first member */
  uint8_t channel_type;
  queue_state state;
  int last_send_tx; /* for testing purposes */
  int last_recv_tx;
  unsigned update_counter;
  unsigned ack_counter;
  buffer_descriptor elements[MCAPI_MAX_QUEUE_ENTRIES+1];
} queue;

typedef union {
  struct {
    uint32_t port_num;
    mcapi_boolean_t allocated;
    mcapi_boolean_t valid;
    mcapi_boolean_t open;
    mcapi_boolean_t connected;
  } data;
  uint64_t buf;
} endpoint_state;

typedef struct {
  mrapi_msg_t msg; /* must be first member */
  mcapi_boolean_t anonymous;
  endpoint_state state;
  uint32_t num_attributes;
  mcapi_endpt_attributes_t attributes;
  queue recv_queue;
} endpoint_entry;

typedef union {
  struct {
    uint32_t node_num;
    mcapi_boolean_t allocated;
    mcapi_boolean_t valid;
    mcapi_boolean_t rundown;
  } data;
  uint64_t buf;
} mcapi_node_state;

typedef struct {
  unsigned num_endpoints;
  endpoint_entry endpoints[MCAPI_MAX_ENDPOINTS];
} mcapi_node_descriptor;

typedef struct {
  mrapi_msg_t msg; /* must be first member */
  struct sigaction signals[MCA_MAX_SIGNALS];
  mcapi_node_state state;
  uint32_t num_attributes;
  mcapi_node_attributes_t attributes;
  mcapi_node_descriptor node_d;
  pid_t pid;
  pthread_t tid;
} mcapi_node_entry;

typedef union {
  struct {
    mca_domain_t domain_id;
    mcapi_boolean_t allocated;
    mcapi_boolean_t valid;
  } data;
  uint64_t buf;
} mcapi_domain_state;

typedef struct {
  mrapi_msg_t msg; /* must be first member */
  unsigned num_nodes;
  mcapi_domain_state state;
  mcapi_node_entry nodes[MCA_MAX_NODES];
} mcapi_domain_entry;

// global header and array that we keep all requests for the process
typedef struct {
  mcapi_boolean_t initialized;
  indexed_array_header reserves_header;
  mcapi_request_data data[MCAPI_MAX_REQUESTS];
} mcapi_requests;

typedef struct {
  mcapi_domain_entry domains[MCA_MAX_DOMAINS];
  buffer_entry buffers[MCAPI_MAX_BUFFERS];
  uint16_t num_domains;
} mcapi_database;



/*******************************************************************
  mcapi_trans function prototypes (public)
*******************************************************************/    
/* initialization */
  extern mcapi_boolean_t transport_sm_initialize(mcapi_domain_t domain_id,
                                                 mcapi_node_t node_id,
                                                 uint32_t* lock_handle); 
  
  /* shared memory management */
  extern mcapi_boolean_t transport_sm_create_shared_mem(void** addr,
                                                        uint32_t shmkey,
                                                        uint32_t size);

  extern mcapi_boolean_t transport_sm_get_shared_mem(void** addr,
                                                     uint32_t shmkey,
                                                     uint32_t size);

  extern mcapi_boolean_t transport_sm_finalize(mcapi_boolean_t last_man_standing,
                                               mcapi_boolean_t last_man_standing_for_this_process,
                                               mcapi_boolean_t finalize_mrapi,
                                               uint32_t handle);

  extern mcapi_boolean_t transport_sm_lock_rwl(uint32_t handle,
                                               mcapi_boolean_t write);

  extern mcapi_boolean_t transport_sm_unlock_rwl(uint32_t handle,
                                                 mcapi_boolean_t write);

  extern mcapi_boolean_t transport_sm_create_rwl(uint32_t semkey,
                                                 uint32_t* handle,
                                                 uint32_t num_readers);

  extern void transport_sm_yield_internal();
  
  
  
#endif
  
#ifdef __cplusplus
} 
#endif /* __cplusplus */
