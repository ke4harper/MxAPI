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

#ifndef MAX_CFG_NAME_LEN
#define MAX_CFG_NAME_LEN 120
#endif

typedef enum
{
    MCAPI_MSG_MODE,
    MCAPI_PKTCHAN_MODE,
    MCAPI_SCLCHAN_MODE
} mcapi_test_mode_t;

typedef enum
{
    MCAPI_FULLDUPLEX_LINK
} mcapi_link_type_t;

typedef enum
{
    MCAPI_SEND_DIRECTION,
    MCAPI_RECV_DIRECTION
} mcapi_direction_t;

typedef enum
{
    MCAPI_INVALID_STATE,
    MCAPI_CONN_STATE,
    MCAPI_ACTIVE_STATE,
    MCAPI_RUNDOWN_STATE,
    MCAPI_DISCONN_STATE
} mcapi_chan_state_t;

/* An endpoint is uniquely specified by its domain, node, and port */
typedef struct {
  uint16_t domain;
  uint16_t node;
  uint16_t port;
} mcapi_endpoint_data_t;

/* Fully qualified endpoint index */
typedef struct {
  uint16_t domain;
  uint16_t node;
  uint16_t ep;
} mcapi_endpoint_index_t;

/* Used for endpoint binary matching */
/* The domain, node, port tuple is packed into a 64-bit integer */
typedef struct {
  int used;
  void* parent;  /* reference to parent endpoint */
  mcapi_endpoint_index_t path;
  union {
    uint64_t buf;
    mcapi_endpoint_data_t data;
  } u;
} mcapi_endpoint_tuple_t;

/* Endpoint configuration bound to its tuple */
typedef struct {
  char name[MAX_CFG_NAME_LEN+1];
  int valid;
  uintptr_t index;
  mcapi_endpoint_t endpoint;
  mcapi_endpoint_tuple_t* tuple;
} mcapi_endpoint_cfg_t;

/* Channel configuration bound to its endpoints and allocated handles */
typedef struct {
  char name[MAX_CFG_NAME_LEN+1];
  int valid;
  channel_type type;
  uintptr_t index;
  void* parent;
  mcapi_direction_t direction;
  mcapi_chan_state_t state;
  mcapi_request_t request;
  mxml_node_t* from;
  mxml_node_t* to;
  mcapi_endpoint_cfg_t* from_ep;
  mcapi_endpoint_cfg_t* to_ep;
  union {
    mcapi_pktchan_send_hndl_t pkt_hndl;
    mcapi_sclchan_send_hndl_t scl_hndl;
  } send;
  union {
    mcapi_pktchan_recv_hndl_t pkt_hndl;
    mcapi_sclchan_recv_hndl_t scl_hndl;
  } recv;
} mcapi_chan_cfg_t;

typedef struct
{
    char name[MAX_CFG_NAME_LEN+1];
    mcapi_link_type_t type;
    uintptr_t index;
    union {
        struct {
            mcapi_chan_cfg_t* send;
            mcapi_chan_cfg_t* ack;
        } fullduplex;
    } links;
} mcapi_link_cfg_t;

typedef struct
{
    int nchans;
    mcapi_endpoint_tuple_t tuple[MCAPI_MAX_ENDPOINTS];
    mcapi_endpoint_cfg_t endpoint[MCAPI_MAX_ENDPOINTS];
    mcapi_chan_cfg_t chan[MCAPI_MAX_ENDPOINTS];
    mcapi_link_cfg_t link[MCAPI_MAX_ENDPOINTS];
} mcapi_config_t;

typedef union
{
    uint64_t buf;
    struct {
        uint32_t tcount;
        uint32_t marker;
    } data;
} mcapi_test_scldata_t;

typedef struct
{
    int bproc;
#if (__unix__)
    pthread_cond_t* cv;
    pthread_mutex_t* sync;
    int* run;
#endif  // (__unix__)
    mcapi_uint_t mode;
    mxml_node_t* root;
	mcapi_domain_t domain;
	mcapi_node_t node;
    int affinity;
    int multicore;
    int iteration;
    int sample;
} mcapi_test_args_t;
