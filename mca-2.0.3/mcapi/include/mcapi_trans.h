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

Port to Windows: #if !(__unix||__MINGW32__), etc.

*/


/*
 * implementation_spec.h
 * V1.4, December 2009
*/

#ifndef IMPLEMENTATION_SPEC_H
#define IMPLEMENTATION_SPEC_H

#include <mcapi_impl_spec.h>
#include <mcapi.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef MCAPI_PSI	/* PolyCore Software, Inc. */
/*
 * This section contains PolyCore Software MCAPI implementation specific information
 */
#define IMPLEMENTATION_VERSION	1700	/* Version 1.700 (major # + minor # (3-digit)) */
#include "pmmcapi.h"

/*
 * Redefine the example implementation specific structures to your needs.
 */
typedef struct
{
	/* Change/add to this structure as needed */
	int num_nonblock_req; /* Max number of outstanding non-blocking requests */
	int dynamic_memory_size; /* Dynamically allocated memory, based on configuration parameters */
} impl_info_t;

typedef struct
{
	/* Change/add to this structure as needed */
	void* pMemory;
	void* pMCAPIParams;
	int	nContextType;
} mcapi_param_t;
#elif	MCAPI_FSL		/* Freescale, Inc. */
/*
 * This section contains Freescale MCAPI implementation specific information
 */
#elif	MCAPI_MGC		/* Mentor Graphics, Inc. */
/*
 * This section contains Mentor Graphics MCAPI implementation specific information
 */
#elif	TBD
/* And so forth */
#endif

/* FUNCTION DECLARATIONS: The transport API */
/* Error handling philosophy:
   mca_status_t is handled at the top level shared code in mcapi.c whenever possible.  All 
   functions in the mcapi_trans layer that need to return status, should return a boolean 
   indicating success or failure.  All functions that need to pass back additional info 
   (such as filling in a handle) should pass that by reference in a parameter.
*/

extern char* mcapi_trans_display_status (mcapi_status_t status,char* status_message,size_t size);
extern void mcapi_trans_set_debug_level (int d);
extern void mcapi_trans_display_state (void* handle);

/* maintain a simple mapping node_num <-> (pid,tid) */
extern mcapi_boolean_t mcapi_trans_get_node_num(mcapi_node_t* node_num);
extern mcapi_boolean_t mcapi_trans_set_node_num(mcapi_node_t node_num);

extern mcapi_boolean_t mcapi_trans_get_domain_num(mcapi_domain_t* domain_num);
/****************** error checking queries *************************/
/* checks if the given node is valid */
extern mcapi_boolean_t mcapi_trans_valid_node(mcapi_uint_t node_num);

/* checks to see if the port_num is a valid port_num for this system */
extern mcapi_boolean_t mcapi_trans_valid_port(mcapi_uint_t port_num);

/* checks if an enpoint exists on this node for a given port id */
extern mcapi_boolean_t mcapi_trans_endpoint_exists (mca_domain_t domain_id,uint32_t port_id);

/* checks if the endpoint handle refers to a valid endpoint */
extern mcapi_boolean_t mcapi_trans_valid_endpoint (mcapi_endpoint_t endpoint);
extern mcapi_boolean_t mcapi_trans_valid_endpoints (mcapi_endpoint_t endpoint1, 
                                             mcapi_endpoint_t endpoint2);

/* checks if the channel is open for a given endpoint */
extern mcapi_boolean_t mcapi_trans_endpoint_channel_isopen (mcapi_endpoint_t endpoint);

/* checks if the channel is open for a given pktchan receive handle */
extern mcapi_boolean_t mcapi_trans_pktchan_recv_isopen (mcapi_pktchan_recv_hndl_t receive_handle) ;

/* checks if the channel is open for a given pktchan send handle */
extern mcapi_boolean_t mcapi_trans_pktchan_send_isopen (mcapi_pktchan_send_hndl_t send_handle) ;

/* checks if the channel is open for a given sclchan receive handle */
extern mcapi_boolean_t mcapi_trans_sclchan_recv_isopen (mcapi_sclchan_recv_hndl_t receive_handle) ;

/* checks if the channel is open for a given sclchan send handle */
extern mcapi_boolean_t mcapi_trans_sclchan_send_isopen (mcapi_sclchan_send_hndl_t send_handle) ;

/* checks if the given endpoint is owned by the given node */
extern mcapi_boolean_t mcapi_trans_endpoint_isowner (mcapi_endpoint_t endpoint);

/* returns the channel type */
channel_type mcapi_trans_channel_type (mcapi_endpoint_t endpoint);

/* checks if the endpoint is connected */
extern mcapi_boolean_t mcapi_trans_channel_connected  (mcapi_endpoint_t endpoint);

/* checks if this endpoint can be a receive endpoint ie. has not already
   been connected as a send endpoint */
extern mcapi_boolean_t mcapi_trans_recv_endpoint (mcapi_endpoint_t endpoint);

/* checks if this endpoint can be a send endpoint ie. has not already
   been connected as a receive endpoint */
extern mcapi_boolean_t mcapi_trans_send_endpoint (mcapi_endpoint_t endpoint);

/* checks if this node has already called initialize */
extern mcapi_boolean_t mcapi_trans_initialized ();

/* returns the number of endpoints for the calling node */
extern mcapi_uint32_t mcapi_trans_num_endpoints();

/* used by msg_send to check if a given priority level is valid */
extern mcapi_boolean_t mcapi_trans_valid_priority(mcapi_priority_t priority);

/* checks if this endpoint is connected */
extern mcapi_boolean_t mcapi_trans_connected(mcapi_endpoint_t endpoint);

/* checks if the status parameter is valid */
extern mcapi_boolean_t mcapi_trans_valid_status_param (mca_status_t* mcapi_status);

/* checks if the version parameter is valid */
extern mcapi_boolean_t mcapi_trans_valid_version_param (mcapi_info_t* mcapi_version);

/* checks if the buffer parameter is valid */
extern mcapi_boolean_t mcapi_trans_valid_buffer_param (void* buffer);

/* checks if the request parameter is valid */
extern mcapi_boolean_t mcapi_trans_valid_request_handle (mcapi_request_t* request);

/* checks if the size parameter is valid */
extern mcapi_boolean_t mcapi_trans_valid_size_param (size_t* size);

/* checks if the given endpoints have compatible attributes */
extern mcapi_boolean_t mcapi_trans_compatible_endpoint_attributes  
        (mcapi_endpoint_t send_endpoint, mcapi_endpoint_t recv_endpoint);

/* checks if the given channel handle is valid */
extern mcapi_boolean_t mcapi_trans_valid_pktchan_send_handle( mcapi_pktchan_send_hndl_t handle);
extern mcapi_boolean_t mcapi_trans_valid_pktchan_recv_handle( mcapi_pktchan_recv_hndl_t handle);
extern mcapi_boolean_t mcapi_trans_valid_sclchan_send_handle( mcapi_sclchan_send_hndl_t handle);
extern mcapi_boolean_t mcapi_trans_valid_sclchan_recv_handle( mcapi_sclchan_recv_hndl_t handle);


/****************** initialization *************************/
/* initialize the transport layer */
  extern mcapi_boolean_t mcapi_trans_initialize(mca_domain_t domain_id,mcapi_node_t node_num,const mcapi_node_attributes_t* node_attrs);

/****************** node attributes *************************/
extern mcapi_boolean_t mcapi_trans_node_init_attributes(
	 mcapi_node_attributes_t* mcapi_node_attributes,
	 mcapi_status_t* mcapi_status
);

extern mcapi_boolean_t mcapi_trans_node_get_attribute(
	 mcapi_domain_t domain_id,
	 mcapi_node_t node_id,
	 mcapi_uint_t attribute_num,
	 void* attribute,
	 size_t attribute_size,
	 mcapi_status_t* mcapi_status
);

extern mcapi_boolean_t mcapi_trans_node_set_attribute(
        mcapi_node_attributes_t* mcapi_node_attribute,
	mcapi_uint_t attribute_num,
	const void* attribute,
	size_t attribute_size,
	mcapi_status_t* mcapi_status
);

/****************** tear down ******************************/
extern mcapi_boolean_t mcapi_trans_finalize();

/****************** endpoints ******************************/
/* create endpoint <node_num,port_num> and return it's handle */
extern mcapi_boolean_t mcapi_trans_endpoint_create(mcapi_endpoint_t *endpoint,   
                                            mcapi_uint_t port_num,
                                            mcapi_boolean_t anonymous);

/* non-blocking get endpoint for the given <node_num,port_num> and set 
   endpoint parameter to it's handle */
extern void mcapi_trans_endpoint_get_i(  mcapi_endpoint_t* endpoint, mca_domain_t domain_id,mcapi_uint_t node_num, 
                                  mcapi_uint_t port_num,mcapi_request_t* request,
                                  mca_status_t* mcapi_status);

/* blocking get endpoint for the given <node_num,port_num> and return it's handle */
extern void mcapi_trans_endpoint_get(mcapi_endpoint_t *endpoint,mca_domain_t domain_id,mcapi_uint_t node_num, 
                              mcapi_uint_t port_num);

/* delete the given endpoint */
extern void mcapi_trans_endpoint_delete( mcapi_endpoint_t endpoint);

/* get the attribute for the given endpoint and attribute_num */
extern void mcapi_trans_endpoint_get_attribute(
        mcapi_endpoint_t endpoint,
        mcapi_uint_t attribute_num,
        void* attribute,
        size_t attribute_size,
        mcapi_status_t* mcapi_status
);

extern void mcapi_trans_endpoint_set_attribute(
        mcapi_endpoint_t endpoint,
        mcapi_uint_t attribute_num,
        const void* attribute,
        size_t attribute_size,
        mcapi_status_t* mcapi_status
);



/****************** msgs **********************************/
extern void mcapi_trans_msg_send_i( mcapi_endpoint_t  send_endpoint, 
                             mcapi_endpoint_t  receive_endpoint, 
                             const char* buffer, size_t buffer_size, 
                             mcapi_request_t* request,mca_status_t* mcapi_status);

extern mcapi_boolean_t mcapi_trans_msg_send( mcapi_endpoint_t  send_endpoint, 
                                      mcapi_endpoint_t  receive_endpoint, 
                                      const char* buffer, size_t buffer_size);

extern void mcapi_trans_msg_recv_i( mcapi_endpoint_t  receive_endpoint,  
                             char* buffer, size_t buffer_size, 
                             mcapi_request_t* request,mca_status_t* mcapi_status);

extern mcapi_boolean_t mcapi_trans_msg_recv( mcapi_endpoint_t  receive_endpoint,  
                                      char* buffer, size_t buffer_size, 
                                      size_t* received_size);

extern mcapi_uint_t mcapi_trans_msg_available( mcapi_endpoint_t receive_endoint);

/****************** channels general ****************************/

/****************** pkt channels ****************************/
extern void mcapi_trans_pktchan_connect_i( mcapi_endpoint_t  send_endpoint, 
                                    mcapi_endpoint_t  receive_endpoint, 
                                    mcapi_request_t* request,
                                    mca_status_t* mcapi_status);

extern void mcapi_trans_pktchan_recv_open_i( mcapi_pktchan_recv_hndl_t* recv_handle, 
                                      mcapi_endpoint_t receive_endpoint, 
                                      mcapi_request_t* request,
                                      mca_status_t* mcapi_status); 

extern void mcapi_trans_pktchan_send_open_i( mcapi_pktchan_send_hndl_t* send_handle, 
                                      mcapi_endpoint_t  send_endpoint, 
                                      mcapi_request_t* request,
                                      mca_status_t* mcapi_status);

extern void  mcapi_trans_pktchan_send_i( mcapi_pktchan_send_hndl_t send_handle, 
                                  const void* buffer, size_t size, 
                                  mcapi_request_t* request,
                                  mca_status_t* mcapi_status);

extern mcapi_boolean_t  mcapi_trans_pktchan_send( mcapi_pktchan_send_hndl_t send_handle, 
                                           const void* buffer, size_t size);

extern void mcapi_trans_pktchan_recv_i( mcapi_pktchan_recv_hndl_t receive_handle,  
                                 void** buffer, mcapi_request_t* request,
                                 mca_status_t* mcapi_status);

extern mcapi_boolean_t mcapi_trans_pktchan_recv( mcapi_pktchan_recv_hndl_t receive_handle, 
                                          void** buffer, size_t* received_size);

extern mcapi_uint_t mcapi_trans_pktchan_available( mcapi_pktchan_recv_hndl_t receive_handle);

extern mcapi_boolean_t mcapi_trans_pktchan_free( void* buffer);

extern void mcapi_trans_pktchan_recv_close_i( mcapi_pktchan_recv_hndl_t receive_handle,
                                       mcapi_request_t* request,
                                       mca_status_t* mcapi_status);

extern void mcapi_trans_pktchan_send_close_i( mcapi_pktchan_send_hndl_t send_handle,
                                       mcapi_request_t* request,
                                       mca_status_t* mcapi_status);

/****************** scalar channels ****************************/
extern void mcapi_trans_sclchan_connect_i( mcapi_endpoint_t  send_endpoint, 
                                    mcapi_endpoint_t  receive_endpoint, 
                                    mcapi_request_t* request,
                                    mca_status_t* mcapi_status);

extern void mcapi_trans_sclchan_recv_open_i( mcapi_sclchan_recv_hndl_t* recv_handle, 
                                      mcapi_endpoint_t receive_endpoint, 
                                      mcapi_request_t* request,
                                      mca_status_t* mcapi_status); 

extern void mcapi_trans_sclchan_send_open_i( mcapi_sclchan_send_hndl_t* send_handle, 
                                      mcapi_endpoint_t  send_endpoint, 
                                      mcapi_request_t* request,
                                      mca_status_t* mcapi_status);

extern mcapi_boolean_t mcapi_trans_sclchan_send( mcapi_sclchan_send_hndl_t send_handle,  
                                          uint64_t dataword, uint32_t size);

extern mcapi_boolean_t mcapi_trans_sclchan_recv( mcapi_sclchan_recv_hndl_t receive_handle,
                                          uint64_t *data,uint32_t exp_size);

extern mcapi_uint_t mcapi_trans_sclchan_available_i( mcapi_sclchan_recv_hndl_t receive_handle);

extern void mcapi_trans_sclchan_recv_close_i( mcapi_sclchan_recv_hndl_t recv_handle,
                                       mcapi_request_t* mcapi_request,
                                       mca_status_t* mcapi_status);

extern void mcapi_trans_sclchan_send_close_i( mcapi_sclchan_send_hndl_t send_handle,
                                       mcapi_request_t* mcapi_request,
                                       mca_status_t* mcapi_status);

/****************** test,wait & cancel ****************************/
extern mcapi_boolean_t mcapi_trans_test_i( mcapi_request_t* request, size_t* size,
                                    mca_status_t* mcapi_status);

extern mcapi_boolean_t mcapi_trans_wait( mcapi_request_t* request, size_t* size,
                       mca_status_t* mcapi_status,
                       mca_timeout_t timeout);
extern unsigned mcapi_trans_wait_any(size_t number, mcapi_request_t** requests, size_t* size,
                          mca_status_t* mcapi_status,
                          mca_timeout_t timeout);

extern int mcapi_trans_wait_first( size_t number, mcapi_request_t** requests, 
                            size_t* size);

extern void mcapi_trans_cancel( mcapi_request_t* request,mca_status_t* mcapi_status);

/****************** stats ****************************/
extern void mcapi_trans_display_stats (void* handle);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* IMPLEMENTATION_SPEC_H */
