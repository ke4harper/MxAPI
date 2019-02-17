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
*/

/////////////////////////////////////////////////////////////////////////////
// File: transport_sm.c
//
// Description: This file contains a shared memory implemenation of MCAPI.
//  This is not intended to be a high-performance real-world implementation.
//  Rather it is just intended as a prototype so that we can "kick the tires"
//  as the spec is being developed.
//
// Authors: Michele Reese
//
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "Fragments/transport_sm_inc_abb.c"

/* NOTE:
   This code is a bit brittle in that you have to be very careful when you
   exit a function with respect to locking and unlocking the database.  You
   also have to be careful that if you lock the database you unlock it when
   you are finished.  If you modify the code and put a return before you
   have released the lock, then other nodes will not be able to access the
   database.  You can dial up the debug level and lock/unlock matching
   will be turned on for you (see access_database_pre/post).  Also some
   functions assume the database is locked at the time they are called.
   These functions have an _have_lock in their name as well as a comment to
   indicate this.
*/

/* NOTE:
   The memory layout for the database is not compact.  It just uses
   simple multi-dimensional arrays of size MCAPI_MAX_*.  A more memory efficient
   and more flexible mechanism might be to use a heap memory manager for the
   shared memory segment.
*/

#include "Fragments/transport_sm_declaration_abb.c"

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                   mcapi_trans API                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

  /***************************************************************************
  Function:  mcapi_trans_display_status

  Description:  formats the status parameter as a string by copying it into the
    user supplied buffer

  Parameters:

  Returns:

  Note: This convenience function implementation was moved from mcapi.c to
    eliminate possible circular dependencies between the layers.

  ***************************************************************************/
char* mcapi_trans_display_status (mcapi_status_t status,char* status_message, size_t size) {
  if ((size < MCAPI_MAX_STATUS_SIZE) || (status_message == NULL)) {
    fprintf(stderr,"ERROR: size passed to mcapi_display_status must be at least %d and status_message must not be NULL.\n",MCAPI_MAX_STATUS_SIZE);
    return status_message;
  }
  memset(status_message,0,size);
switch (status) {
#if (__unix__||__MINGW32__)
  case (MCAPI_SUCCESS): strcpy(status_message,"MCAPI_SUCCESS"); return(status_message); break;
  case (MCAPI_PENDING): strcpy(status_message,"MCAPI_PENDING"); return(status_message); break;
  case (MCAPI_ERR_PARAMETER): strcpy(status_message,"MCAPI_ERR_PARAMETER"); return(status_message); break;
  case (MCAPI_ERR_DOMAIN_INVALID): strcpy(status_message,"MCAPI_ERR_DOMAIN_INVALID"); return(status_message); break;
  case (MCAPI_ERR_NODE_INVALID): strcpy(status_message,"MCAPI_ERR_NODE_INVALID"); return(status_message); break;
  case (MCAPI_ERR_NODE_INITFAILED): strcpy(status_message,"MCAPI_ERR_NODE_INITFAILED"); return(status_message); break;
  case (MCAPI_ERR_NODE_INITIALIZED): strcpy(status_message,"MCAPI_ERR_NODE_INITIALIZED"); return(status_message); break;
  case (MCAPI_ERR_NODE_NOTINIT): strcpy(status_message,"MCAPI_ERR_NODE_NOTINIT"); return(status_message); break;
  case (MCAPI_ERR_NODE_FINALFAILED): strcpy(status_message,"MCAPI_ERR_NODE_FINALFAILED"); return(status_message); break;
  case (MCAPI_ERR_PORT_INVALID): strcpy(status_message,"MCAPI_ERR_INVALID"); return(status_message); break;
  case (MCAPI_ERR_ENDP_INVALID): strcpy(status_message,"MCAPI_ERR_ENDP_INVALID"); return(status_message); break;
  case (MCAPI_ERR_ENDP_NOPORTS): strcpy(status_message,"MCAPI_ERR_ENDP_NOPORTS"); return(status_message); break;
  case (MCAPI_ERR_ENDP_LIMIT): strcpy(status_message,"MCAPI_ERR_ENDP_LIMIT"); return(status_message); break;
  case (MCAPI_ERR_ENDP_EXISTS): strcpy(status_message,"MCAPI_ERR_ENDP_EXISTS"); return(status_message); break;
  case (MCAPI_ERR_ENDP_NOTOWNER): strcpy(status_message,"MCAPI_ERR_ENDP_NOTOWNER"); return(status_message); break;
  case (MCAPI_ERR_ENDP_REMOTE): strcpy(status_message,"MCAPI_ERR_ENDP_REMOTE"); return(status_message); break;
  case (MCAPI_ERR_ATTR_INCOMPATIBLE): strcpy(status_message,"MCAPI_ERR_ATTR_INCOMPATIBLE"); return(status_message); break;
  case (MCAPI_ERR_ATTR_SIZE): strcpy(status_message,"MCAPI_ERR_ATTR_SIZE"); return(status_message); break;
  case (MCAPI_ERR_ATTR_NUM): strcpy(status_message,"MCAPI_ERR_ATTR_NUM"); return(status_message); break;
  case (MCAPI_ERR_ATTR_VALUE): strcpy(status_message,"MCAPI_ERR_ATTR_VALUE"); return(status_message); break;
  case (MCAPI_ERR_ATTR_NOTSUPPORTED): strcpy(status_message,"MCAPI_ERR_ATTR_NOTSUPPORTED"); return(status_message); break;
  case (MCAPI_ERR_ATTR_READONLY): strcpy(status_message,"MCAPI_ERR_ATTR_READONLY"); return(status_message); break;
  case (MCAPI_ERR_MSG_LIMIT): strcpy(status_message,"MCAPI_ERR_MSG_LIMIT"); return(status_message); break;
  case (MCAPI_ERR_MSG_TRUNCATED): strcpy(status_message,"MCAPI_ERR_MSG_TRUNCATED"); return(status_message); break;
  case (MCAPI_ERR_TRANSMISSION): strcpy(status_message,"MCAPI_ERR_TRANSMISSION"); return(status_message); break;
  case (MCAPI_ERR_MEM_LIMIT): strcpy(status_message,"MCAPI_ERR_MEM_LIMIT"); return(status_message); break;
  case (MCAPI_TIMEOUT): strcpy(status_message,"MCAPI_TIMEOUT"); return(status_message); break;
  case (MCAPI_ERR_REQUEST_LIMIT): strcpy(status_message,"MCAPI_ERR_REQUEST_LIMIT"); return(status_message); break;
  case (MCAPI_ERR_PRIORITY): strcpy(status_message,"MCAPI_ERR_PRIORITY"); return(status_message); break;
  case (MCAPI_ERR_CHAN_OPEN): strcpy(status_message,"MCAPI_ERR_CHAN_OPEN"); return(status_message); break;
  case (MCAPI_ERR_CHAN_TYPE): strcpy(status_message,"MCAPI_ERR_CHAN_TYPE"); return(status_message); break;
  case (MCAPI_ERR_CHAN_CONNECTED): strcpy(status_message,"MCAPI_ERR_CHAN_CONNECTED"); return(status_message); break;
  case (MCAPI_ERR_CHAN_OPENPENDING): strcpy(status_message,"MCAPI_ERR_CHAN_OPENPENDING"); return(status_message); break;
  case (MCAPI_ERR_CHAN_DIRECTION): strcpy(status_message,"MCAPI_ERR_CHAN_DIRECTION"); return(status_message); break;
  case (MCAPI_ERR_CHAN_NOTOPEN): strcpy(status_message,"MCAPI_ERR_CHAN_NOTOPEN"); return(status_message); break;
  case (MCAPI_ERR_CHAN_INVALID): strcpy(status_message,"MCAPI_ERR_CHAN_INVALID"); return(status_message); break;
  case (MCAPI_ERR_REQUEST_INVALID): strcpy(status_message,"MCAPI_ERR_REQUEST_INVALID"); return(status_message); break;
  case (MCAPI_ERR_PKT_LIMIT): strcpy(status_message,"MCAPI_ERR_PKT_LIMIT"); return(status_message); break;
  case (MCAPI_ERR_BUF_INVALID): strcpy(status_message,"MCAPI_ERR_BUF_INVALID"); return(status_message); break;
  case (MCAPI_ERR_SCL_SIZE): strcpy(status_message,"MCAPI_ERR_SCL_SIZE"); return(status_message); break;
  case (MCAPI_ERR_REQUEST_CANCELLED): strcpy(status_message,"MCAPI_ERR_REQUEST_CANCELLED"); return(status_message); break;
  case (MCAPI_ERR_GENERAL): strcpy(status_message,"MCAPI_ERR_GENERAL"); return(status_message); break;
  case (MCAPI_STATUSCODE_END): strcpy(status_message,"MCAPI_STATUSCODE_END"); return(status_message); break;
  default : strcpy(status_message,"UNKNOWN"); return(status_message); break;
#else
  case (MCAPI_SUCCESS): strcpy_s(status_message,size,"MCAPI_SUCCESS"); return(status_message); break;
  case (MCAPI_PENDING): strcpy_s(status_message,size,"MCAPI_PENDING"); return(status_message); break;
  case (MCAPI_ERR_PARAMETER): strcpy_s(status_message,size,"MCAPI_ERR_PARAMETER"); return(status_message); break;
  case (MCAPI_ERR_DOMAIN_INVALID): strcpy_s(status_message,size,"MCAPI_ERR_DOMAIN_INVALID"); return(status_message); break;
  case (MCAPI_ERR_NODE_INVALID): strcpy_s(status_message,size,"MCAPI_ERR_NODE_INVALID"); return(status_message); break;
  case (MCAPI_ERR_NODE_INITFAILED): strcpy_s(status_message,size,"MCAPI_ERR_NODE_INITFAILED"); return(status_message); break;
  case (MCAPI_ERR_NODE_INITIALIZED): strcpy_s(status_message,size,"MCAPI_ERR_NODE_INITIALIZED"); return(status_message); break;
  case (MCAPI_ERR_NODE_NOTINIT): strcpy_s(status_message,size,"MCAPI_ERR_NODE_NOTINIT"); return(status_message); break;
  case (MCAPI_ERR_NODE_FINALFAILED): strcpy_s(status_message,size,"MCAPI_ERR_NODE_FINALFAILED"); return(status_message); break;
  case (MCAPI_ERR_PORT_INVALID): strcpy_s(status_message,size,"MCAPI_ERR_INVALID"); return(status_message); break;
  case (MCAPI_ERR_ENDP_INVALID): strcpy_s(status_message,size,"MCAPI_ERR_ENDP_INVALID"); return(status_message); break;
  case (MCAPI_ERR_ENDP_NOPORTS): strcpy_s(status_message,size,"MCAPI_ERR_ENDP_NOPORTS"); return(status_message); break;
  case (MCAPI_ERR_ENDP_LIMIT): strcpy_s(status_message,size,"MCAPI_ERR_ENDP_LIMIT"); return(status_message); break;
  case (MCAPI_ERR_ENDP_EXISTS): strcpy_s(status_message,size,"MCAPI_ERR_ENDP_EXISTS"); return(status_message); break;
  case (MCAPI_ERR_ENDP_NOTOWNER): strcpy_s(status_message,size,"MCAPI_ERR_ENDP_NOTOWNER"); return(status_message); break;
  case (MCAPI_ERR_ENDP_REMOTE): strcpy_s(status_message,size,"MCAPI_ERR_ENDP_REMOTE"); return(status_message); break;
  case (MCAPI_ERR_ATTR_INCOMPATIBLE): strcpy_s(status_message,size,"MCAPI_ERR_ATTR_INCOMPATIBLE"); return(status_message); break;
  case (MCAPI_ERR_ATTR_SIZE): strcpy_s(status_message,size,"MCAPI_ERR_ATTR_SIZE"); return(status_message); break;
  case (MCAPI_ERR_ATTR_NUM): strcpy_s(status_message,size,"MCAPI_ERR_ATTR_NUM"); return(status_message); break;
  case (MCAPI_ERR_ATTR_VALUE): strcpy_s(status_message,size,"MCAPI_ERR_ATTR_VALUE"); return(status_message); break;
  case (MCAPI_ERR_ATTR_NOTSUPPORTED): strcpy_s(status_message,size,"MCAPI_ERR_ATTR_NOTSUPPORTED"); return(status_message); break;
  case (MCAPI_ERR_ATTR_READONLY): strcpy_s(status_message,size,"MCAPI_ERR_ATTR_READONLY"); return(status_message); break;
  case (MCAPI_ERR_MSG_LIMIT): strcpy_s(status_message,size,"MCAPI_ERR_MSG_LIMIT"); return(status_message); break;
  case (MCAPI_ERR_MSG_TRUNCATED): strcpy_s(status_message,size,"MCAPI_ERR_MSG_TRUNCATED"); return(status_message); break;
  case (MCAPI_ERR_TRANSMISSION): strcpy_s(status_message,size,"MCAPI_ERR_TRANSMISSION"); return(status_message); break;
  case (MCAPI_ERR_MEM_LIMIT): strcpy_s(status_message,size,"MCAPI_ERR_MEM_LIMIT"); return(status_message); break;
  case (MCAPI_TIMEOUT): strcpy_s(status_message,size,"MCAPI_TIMEOUT"); return(status_message); break;
  case (MCAPI_ERR_REQUEST_LIMIT): strcpy_s(status_message,size,"MCAPI_ERR_REQUEST_LIMIT"); return(status_message); break;
  case (MCAPI_ERR_PRIORITY): strcpy_s(status_message,size,"MCAPI_ERR_PRIORITY"); return(status_message); break;
  case (MCAPI_ERR_CHAN_OPEN): strcpy_s(status_message,size,"MCAPI_ERR_CHAN_OPEN"); return(status_message); break;
  case (MCAPI_ERR_CHAN_TYPE): strcpy_s(status_message,size,"MCAPI_ERR_CHAN_TYPE"); return(status_message); break;
  case (MCAPI_ERR_CHAN_CONNECTED): strcpy_s(status_message,size,"MCAPI_ERR_CHAN_CONNECTED"); return(status_message); break;
  case (MCAPI_ERR_CHAN_OPENPENDING): strcpy_s(status_message,size,"MCAPI_ERR_CHAN_OPENPENDING"); return(status_message); break;
  case (MCAPI_ERR_CHAN_DIRECTION): strcpy_s(status_message,size,"MCAPI_ERR_CHAN_DIRECTION"); return(status_message); break;
  case (MCAPI_ERR_CHAN_NOTOPEN): strcpy_s(status_message,size,"MCAPI_ERR_CHAN_NOTOPEN"); return(status_message); break;
  case (MCAPI_ERR_CHAN_INVALID): strcpy_s(status_message,size,"MCAPI_ERR_CHAN_INVALID"); return(status_message); break;
  case (MCAPI_ERR_REQUEST_INVALID): strcpy_s(status_message,size,"MCAPI_ERR_REQUEST_INVALID"); return(status_message); break;
  case (MCAPI_ERR_PKT_LIMIT): strcpy_s(status_message,size,"MCAPI_ERR_PKT_LIMIT"); return(status_message); break;
  case (MCAPI_ERR_BUF_INVALID): strcpy_s(status_message,size,"MCAPI_ERR_BUF_INVALID"); return(status_message); break;
  case (MCAPI_ERR_SCL_SIZE): strcpy_s(status_message,size,"MCAPI_ERR_SCL_SIZE"); return(status_message); break;
  case (MCAPI_ERR_REQUEST_CANCELLED): strcpy_s(status_message,size,"MCAPI_ERR_REQUEST_CANCELLED"); return(status_message); break;
  case (MCAPI_ERR_GENERAL): strcpy_s(status_message,size,"MCAPI_ERR_GENERAL"); return(status_message); break;
  case (MCAPI_STATUSCODE_END): strcpy_s(status_message,size,"MCAPI_STATUSCODE_END"); return(status_message); break;
  default : strcpy_s(status_message,size,"UNKNOWN"); return(status_message); break;
#endif  /* !(__unix__||__MINGW32__) */
  };
}
  /***************************************************************************
  NAME: mcapi_trans_get_node_num
  DESCRIPTION: gets the node_num (not the transport's node index!)
  PARAMETERS: node_num: the node_num pointer to be filled in
  RETURN VALUE: boolean indicating success (the node num was found) or failure
   (couldn't find the node num).
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_get_node_num(mcapi_node_t* node)
  {
    uint32_t d,n;
    mcapi_domain_t domain_id;

    mcapi_dprintf(1,"mcapi_trans_get_node_num(&node_dummy);");

    return mcapi_trans_whoami(node,&n,&domain_id,&d);
  }

  /***************************************************************************
  NAME: mcapi_trans_get_domain_num
  DESCRIPTION: gets the domain_num (not the transport's node index!)
  PARAMETERS: domain_num: the domain_num pointer to be filled in
  RETURN VALUE: boolean indicating success (the node num was found) or failure
   (couldn't find the domain num).
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_get_domain_num(mcapi_domain_t* domain)
  {
    uint32_t d,n;
    mcapi_node_t node;

    return mcapi_trans_whoami(&node,&n,domain,&d);
  }

#include "Fragments/transport_sm_request_abb.c"
#include "Fragments/transport_sm_initialize_abb.c"
#include "Fragments/transport_sm_attributes_abb.c"
#include "Fragments/transport_sm_finalize_abb.c"
#include "Fragments/transport_sm_check_abb.c"
#include "Fragments/transport_sm_endpoint_abb.c"
#include "Fragments/transport_sm_msg_abb.c"
#include "Fragments/transport_sm_pktchan_abb.c"
#include "Fragments/transport_sm_sclchan_abb.c"
#include "Fragments/transport_sm_wait_abb.c"

  //////////////////////////////////////////////////////////////////////////////
  //                                                                          //
  //                   misc helper functions                                  //
  //                                                                          //
  //////////////////////////////////////////////////////////////////////////////

#include "Fragments/transport_sm_signal_abb.c"

  /***************************************************************************
  NAME:mcapi_trans_display_state
  DESCRIPTION: This function is useful for debugging.  If the handle is null,
   we'll print out the state of the entire database.  Otherwise, we'll print out
   only the state of the endpoint that the handle refers to.
  PARAMETERS:
     handle
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_display_state (void* handle)
  {
    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
    mcapi_trans_display_state_have_lock(handle);
    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }

  /***************************************************************************
  NAME:mcapi_trans_display_state_have_lock
  DESCRIPTION: This function is useful for debugging.  If the handle is null,
   we'll print out the state of the entire database.  Otherwise, we'll print out
   only the state of the endpoint that the handle refers to.  Expects the database
   to be locked.
  PARAMETERS:
     handle
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_display_state_have_lock (void* handle)
  {
    uint16_t d,n,e,a;
    mcapi_endpoint_t* endpoint = (mcapi_endpoint_t*)handle;

    printf("DISPLAY STATE:");


    if (handle != NULL) {
      /* print the data for the given endpoint */
      mcapi_assert(mcapi_trans_decode_handle(*endpoint,&d,&n,&e));
      printf("\nnode: %u, port: %u, receive queue (num_elements=%i):",
             (unsigned)mcapi_db->domains[d].nodes[n].state.data.node_num,
             (unsigned)mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.port_num,
             (unsigned)mcapi_trans_queue_elements(&mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue));

      printf("\n    endpoint: %u",e);
      printf("\n      valid:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.valid);
      printf("\n      anonymous:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].anonymous);
      printf("\n      open:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.open);
      printf("\n      connected:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.connected);
      printf("\n      num_attributes:%u",(unsigned)mcapi_db->domains[d].nodes[n].node_d.endpoints[e].num_attributes);
      for (a = 0; a < mcapi_db->domains[d].nodes[n].node_d.endpoints[e].num_attributes; a++) {
        printf("\n        attribute:%u",a);
        printf("\n          valid:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].attributes.entries[a].valid);
        printf("\n          attribute_num:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].attributes.entries[a].attribute_num);
        printf("\n          bytes:%i",(unsigned)mcapi_db->domains[d].nodes[n].node_d.endpoints[e].attributes.entries[a].bytes);
      }
      //print_queue(mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue);
    } else {
      /* print the whole database */
      for (d = 0; d < MCA_MAX_DOMAINS; d++) {
        for (n = 0; n < MCA_MAX_NODES; n++) {
          if ((mcapi_db->domains[d].state.data.valid == MCAPI_TRUE) &&
              (mcapi_db->domains[d].nodes[n].state.data.valid == MCAPI_TRUE)) {
            printf("\nVALID NODE: d=%u, nindex=%u domain_id=%u,node_num=%u,",
                   d,n,mcapi_db->domains[d].state.data.domain_id,
                   (unsigned)mcapi_db->domains[d].nodes[n].state.data.node_num);
            printf("\n  num_endpoints:%u",mcapi_db->domains[d].nodes[n].node_d.num_endpoints);
            for (e = 0; e < mcapi_db->domains[d].nodes[n].node_d.num_endpoints; e++) {
              if (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.valid) {
                printf("\n    VALID ENDPT: e=%u",e);
                printf("\n    port_num: %u",(unsigned)mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.port_num);
                printf("\n      anonymous:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].anonymous);
                printf("\n      open:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.open);
                printf("\n      connected:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.connected);
                printf("\n      num_attributes:%u",(unsigned)mcapi_db->domains[d].nodes[n].node_d.endpoints[e].num_attributes);
                for (a = 0; a < mcapi_db->domains[d].nodes[n].node_d.endpoints[e].num_attributes; a++) {
                  printf("\n        a=%u",a);
                  printf("\n        attribute:%u",a);
                  printf("\n          valid:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].attributes.entries[a].valid);
                  printf("\n          attribute_num:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].attributes.entries[a].attribute_num);
                  printf("\n          bytes:%u",(unsigned)mcapi_db->domains[d].nodes[n].node_d.endpoints[e].attributes.entries[a].bytes);
                }
              }
              //print_queue(mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue);
            }
          }
        }
      }
    }
    printf("\n ");
  }

  /***************************************************************************
  NAME:mcapi_trans_connect_channel_have_lock
  DESCRIPTION: connects a channel
  PARAMETERS:
     send_endpoint
     receive_endpoint
     type
  RETURN VALUE:none
  ***************************************************************************/
  void mcapi_trans_connect_channel (mcapi_endpoint_t send_endpoint,
                                    mcapi_endpoint_t receive_endpoint,
                                    channel_type type)
  {
    uint16_t sd,sn,se;
    uint16_t rd,rn,re;
    unsigned eindex;
    queue_state* qstate = NULL;
    queue_state oldqstate;
    queue_state newqstate;
    endpoint_state* estate = NULL;
    endpoint_state oldestate;
    endpoint_state newestate;
    mrapi_status_t status;
    mrapi_atomic_barrier_t axb_endpoints;

    mcapi_assert(mcapi_trans_decode_handle(send_endpoint,&sd,&sn,&se));
    mcapi_assert(mcapi_trans_decode_handle(receive_endpoint,&rd,&rn,&re));

    eindex = se;
    mrapi_barrier_init(&axb_endpoints,0,(mrapi_msg_t*)mcapi_db->domains[sd].nodes[sn].node_d.endpoints,
      MCAPI_MAX_ENDPOINTS,sizeof(endpoint_entry),&eindex,MCA_INFINITE,&status);
    assert(MRAPI_SUCCESS == status);

    /* update the send endpoint */
    mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.channel_type = type;
    qstate = &mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.state;
    newqstate = oldqstate = *qstate;
    newqstate.data.recv_endpt = receive_endpoint;
    newqstate.data.send_endpt = send_endpoint;
    status = MRAPI_SUCCESS;
    /* TODO: should be CAS, but sender and receiver may both try and connect at the same time */
    mrapi_atomic_xchg(&axb_endpoints,qstate,&newqstate,NULL,
        sizeof(mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.state),&status);
    assert(MRAPI_SUCCESS == status);
    estate = &mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].state;
    newestate = oldestate = *estate;
    newestate.data.connected = MCAPI_TRUE;
    /* TODO: should be CAS, but sender and receiver may both try and connect at the same time */
    mrapi_atomic_xchg(&axb_endpoints,estate,&newestate,NULL,
        sizeof(mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].state),&status);
    assert(MRAPI_SUCCESS == status);

    eindex = re;
    mrapi_barrier_init(&axb_endpoints,0,(mrapi_msg_t*)mcapi_db->domains[rd].nodes[rn].node_d.endpoints,
      MCAPI_MAX_ENDPOINTS,sizeof(endpoint_entry),&eindex,MCA_INFINITE,&status);
    assert(MRAPI_SUCCESS == status);

    /* update the receive endpoint */
    mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.channel_type = type;
    qstate = &mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.state;
    newqstate = oldqstate = *qstate;
    newqstate.data.send_endpt = send_endpoint;
    newqstate.data.recv_endpt = receive_endpoint;
    /* TODO: should be CAS, but sender and receiver may both try and connect at the same time */
    mrapi_atomic_xchg(&axb_endpoints,qstate,&newqstate,NULL,
        sizeof(mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.state),&status);
    assert(MRAPI_SUCCESS == status);
    estate = &mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].state;
    newestate = oldestate = *estate;
    newestate.data.connected = MCAPI_TRUE;
    /* TODO: should be CAS, but sender and receiver may both try and connect at the same time */
    mrapi_atomic_xchg(&axb_endpoints,estate,&newestate,NULL,
        sizeof(mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].state),&status);
    assert(MRAPI_SUCCESS == status);

    mcapi_dprintf(1,"channel_type=%u connected sender (node=%u,port=%u) to receiver (node=%u,port=%u)",
                  type,mcapi_db->domains[sd].nodes[sn].state.data.node_num,
                  mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].state.data.port_num,
                  mcapi_db->domains[rd].nodes[rn].state.data.node_num,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].state.data.port_num);
  }

  /***************************************************************************
  NAME:mcapi_trans_alloc_buffer
  DESCRIPTION: Find a free mcapi buffer
  PARAMETERS:
    index - mcapi buffer index
    magic_num - buffer marker
     RESERVED_NUM for pktchan buffers that have not been overwritten,
     MAGIC_NUM or all others
  RETURN VALUE: mcapi buffer address or NULL
  ***************************************************************************/
  void* mcapi_trans_alloc_buffer (int* index,uint32_t magic_num)
  {
    int i = 0;
    buffer_entry* db_buff = NULL;
    mrapi_status_t status;

    for (i = 0; i < MCAPI_MAX_BUFFERS; i++) {
      uint32_t* num = &mcapi_db->buffers[i].magic_num;
      uint32_t oldnum = 0;
      uint32_t newnum = magic_num;
      status = MRAPI_SUCCESS;
      /* experiments with atomic barrier introduced significant latencies */
      mrapi_atomic_cas(NULL,num,&newnum,&oldnum,NULL,sizeof(mcapi_db->buffers[i].magic_num),&status);
      if(MRAPI_SUCCESS == status) {
        *index = i;
        db_buff = &mcapi_db->buffers[i];
        mcapi_dprintf(4,"mcapi_trans_alloc_buffer: using buffer index i=%u\n",i);
        break;
      }
    }
    return db_buff;
  }

  /***************************************************************************
  NAME:mcapi_trans_free_buffer
  DESCRIPTION: Release an mcapi buffer
  PARAMETERS:
    buf - mcapi buffer address
  RETURN VALUE: MCAPI_TRUE if buffer was allocated, MCAPI_FALSE otherwise
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_free_buffer (void* buf)
  {
    mcapi_boolean_t rc = MCAPI_TRUE;
    buffer_entry* db_buff = (buffer_entry*)buf;
    uint32_t oldnum;
    uint32_t newnum;
    mrapi_status_t status;

    //printf("free txn %d magic 0x%x\n",*(uint16_t*)db_buff->buff,db_buff->magic_num);

    mrapi_atomic_read(NULL,&db_buff->magic_num,&oldnum,sizeof(db_buff->magic_num),&status);
    assert(MRAPI_SUCCESS == status);
    newnum = 0;
    /* experiments with atomic barrier introduced significant latencies */
    mrapi_atomic_cas(NULL,&db_buff->magic_num,&newnum,&oldnum,NULL,sizeof(db_buff->magic_num),&status);
    if(MRAPI_SUCCESS != status) {
      /* didn't find the buffer */
      rc = MCAPI_FALSE;
    }
    //printf("free txn %d magic 0x%x success\n",*(uint16_t*)db_buff->buff,db_buff->magic_num);
    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_send
  DESCRIPTION: Attempts to send a message from one endpoint to another
  PARAMETERS:
    sn - the send node index (only used for verbose debug print)
    se - the send endpoint index (only used for verbose debug print)
    rn - the receive node index
    re - the receive endpoint index
    buffer -
    buffer_size -

  RETURN VALUE: true/false indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_send (uint16_t sd, uint16_t sn,uint16_t se,
                                    uint16_t rd,uint16_t rn, uint16_t re,
                                    const char* buffer,
                                    size_t buffer_size,
                                    uint64_t scalar)
  {
    int i,qindex = -1;
    int32_t buff_index;
    mrapi_status_t status;
    mcapi_status_t mcapi_status;
    buffer_entry* db_buff = NULL;
    queue* q = &mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue;
    buffer_descriptor* buf = NULL;
    mcapi_buffer_state oldbstate;
    mcapi_buffer_state newbstate;

    mcapi_dprintf(3,"mcapi_trans_send sender (node=%u,port=%u) to receiver (node=%u,port=%u) ",
                  mcapi_db->domains[sd].nodes[sn].state.data.node_num,
                  mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].state.data.port_num,
                  mcapi_db->domains[rd].nodes[rn].state.data.node_num,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].state.data.port_num);

    /* find the next index in the circular receive queue */
    while(1) {
      qindex = mcapi_trans_queue_tail(q,&mcapi_status);
	  if(MCAPI_SUCCESS == mcapi_status) {
	    buf = &q->elements[qindex];
        mrapi_atomic_read(NULL,&buf->buff_index,&buff_index,sizeof(buf->buff_index),&status);
        assert(MRAPI_SUCCESS == status);
        assert(0 == buf->buff_index);
        oldbstate = BUFFER_FREE;
        newbstate = BUFFER_RESERVED;
	    mrapi_atomic_cas(NULL,&buf->state,&newbstate,&oldbstate,NULL,sizeof(buf->state),&status);
	    if(MRAPI_SUCCESS == status) {
	      break;
	    }
      }
      if(MCAPI_ERR_QUEUE_FULL_CONSUMER_READING == mcapi_status) {
        sys_os_yield();
        continue;
      }
      else if(MCAPI_ERR_QUEUE_FULL == mcapi_status) {
        return MCAPI_FALSE;
      }
    }

#ifdef NOTUSED
    /* Verify FIFO order if first two bytes of buffer are a transaction ID */
    if(MCAPI_PKT_CHAN == q->channel_type) {
      int tx = *(mcapi_uint16_t*)buffer;
      if(10000 > tx) {
        assert(q->last_send_tx+1 == tx);
        q->last_send_tx = tx;
        /* Check if previous entry is in transaction ID order */
        if(0 < qindex) {
          int bip = q->elements[qindex-1].buff_index;
          if(0 < bip) {
            int txp = *(mcapi_uint16_t*)mcapi_db->buffers[bip &~ MCAPI_VALID_MASK].buff;
            assert(q->last_send_tx-2 == txp);
          }
        }
      }
    }
#endif  /* NOTUSED */

    /* find a free mcapi buffer (we only have to worry about this on the sending side) */
    if (NULL == (db_buff = (buffer_entry*)mcapi_trans_alloc_buffer(&i,MAGIC_NUM))) {
      /* we couldn't get a free buffer */
      mcapi_dprintf(2,"ERROR mcapi_trans_send: No more buffers available - try freeing some buffers. ");
      newbstate = BUFFER_FREE;
      mrapi_atomic_xchg(NULL,&buf->state,&newbstate,NULL,sizeof(buf->state),&status);
      return MCAPI_FALSE;
    }

    /* now go about updating buffer into the database... */
    mcapi_dprintf(4,"send pushing %u byte buffer to qindex=%i, num_elements=%i",
                  buffer_size,qindex,
                  mcapi_trans_queue_elements(q));
    /* printf(" send pushing to qindex=%i",qindex); */

    db_buff->parent = buf; /* local pointer in shared memory */
    if (q->channel_type == MCAPI_SCL_CHAN ) {
      db_buff->scalar = scalar;
    } else {
      /* copy the buffer parm into a mcapi buffer */
      memcpy (db_buff->buff,buffer,buffer_size);
    }
    /* set the size */
    db_buff->size = buffer_size;

    /* update the ptr in the receive_endpoints queue to point to our mcapi buffer */
    /* shared memory is zeroed, so we store our index as index with a valid bit so that we can tell if it's valid or not*/
    buff_index = i | MCAPI_VALID_MASK;
    mrapi_atomic_xchg(NULL,&buf->buff_index,&buff_index,NULL,sizeof(buf->buff_index),&status);
    assert(MRAPI_SUCCESS == status);

    oldbstate = BUFFER_RESERVED;
    newbstate = BUFFER_ALLOCATED;
    mrapi_atomic_cas(NULL,&buf->state,&newbstate,&oldbstate,NULL,sizeof(buf->state),&status);
    assert(MRAPI_SUCCESS == status);

    /* Add entry to queue */
    assert(qindex == mcapi_trans_push_queue(q,&mcapi_status));
    return MCAPI_TRUE;
  }

  /***************************************************************************
  NAME:mcapi_trans_send_state
  DESCRIPTION: Attempts to send a message from one endpoint to another using
   state message exchange
  PARAMETERS:
    sn - the send node index (only used for verbose debug print)
    se - the send endpoint index (only used for verbose debug print)
    rn - the receive node index
    re - the receive endpoint index
    buffer -
    buffer_size -

  RETURN VALUE: true/false indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_send_state (uint16_t sd, uint16_t sn,uint16_t se,
                                          uint16_t rd,uint16_t rn, uint16_t re,
                                          const char* buffer,
                                          size_t buffer_size,
                                          uint64_t scalar)
  {
    int i,qindex = -1;
    int index;
    int32_t buff_index;
    uint32_t oldnum;
    uint32_t newnum;
    unsigned counter,ncounter;
    mrapi_status_t status;
    buffer_entry* db_buff = NULL;
    attribute_entry_t* ae  = mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].attributes.entries;
    queue* q = &mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue;
    buffer_descriptor* buf = NULL;
    mcapi_buffer_state oldbstate;
    mcapi_buffer_state newbstate;

    mcapi_dprintf(3,"mcapi_trans_send_state: sender (node=%u,port=%u) to receiver (node=%u,port=%u) ",
                  mcapi_db->domains[sd].nodes[sn].state.data.node_num,
                  mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].state.data.port_num,
                  mcapi_db->domains[rd].nodes[rn].state.data.node_num,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].state.data.port_num);

    /* select next index for write; operation is never blocked */
    mrapi_atomic_inc(NULL,&q->update_counter,&counter,sizeof(q->update_counter),&status);
    assert(MRAPI_SUCCESS == status);
    qindex = (counter/2) % ae[MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS].attribute_d.value;
    buf = &q->elements[qindex];
    mrapi_atomic_read(NULL,&buf->state,&oldbstate,sizeof(buf->state),&status);
    assert(MRAPI_SUCCESS == status);
    mrapi_atomic_read(NULL,&buf->buff_index,&buff_index,sizeof(buf->buff_index),&status);
    assert(MRAPI_SUCCESS == status);

    /* reuse existing buffer if possible for state communication */
    /* mcapi_trans_recv_state will detect collision if not MCAPI_PKT_CHAN,
       otherwise buffer needs to be replaced to prevent overwrite */
    index = buff_index &~ MCAPI_VALID_MASK;
    if(0 != buff_index) {
      assert(MRAPI_SUCCESS == status);
      if(MCAPI_PKT_CHAN != q->channel_type ||
          BUFFER_RECEIVED != oldbstate) {
        db_buff = &mcapi_db->buffers[index];
        mcapi_dprintf(4,"mcapi_trans_send_state: using buffer index i=%u\n",index);
      }
    }

    /* reserve entry so mcapi_trans_recv_state does not attempt to read */
    newbstate = BUFFER_RESERVED;
    mrapi_atomic_xchg(NULL,&buf->state,&newbstate,NULL,sizeof(buf->state),&status);
    assert(MRAPI_SUCCESS == status);

    if(NULL == db_buff) {
      if(MCAPI_PKT_CHAN == q->channel_type &&
          0 != buff_index &&
          BUFFER_RECEIVED == oldbstate) {
        /* change buffer marker to MAGIC_NUM so mcapi_trans_pktchan_free
           will garbage collect */
        oldnum = RESERVED_NUM;
        newnum = MAGIC_NUM;
        mrapi_atomic_cas(NULL,&mcapi_db->buffers[index].magic_num,&newnum,&oldnum,NULL,
            sizeof(mcapi_db->buffers[index].magic_num),&status);
        assert(MRAPI_SUCCESS == status);
      }

      /* buffer is initially marked RESERVED_NUM to indicate it can be reused */
      if (NULL == (db_buff = mcapi_trans_alloc_buffer(&i,RESERVED_NUM))) {
        /* we couldn't get a free buffer */
        mcapi_dprintf(2,"ERROR mcapi_trans_send_state: No more buffers available - try freeing some buffers. ");
        newbstate = BUFFER_FREE;
        mrapi_atomic_xchg(NULL,&buf->state,&newbstate,NULL,sizeof(buf->state),&status);
        /* clear buffer index */
        buff_index = 0;
        mrapi_atomic_xchg(NULL,&buf->buff_index,&buff_index,NULL,sizeof(buf->buff_index),&status);
        /* increment counter to complete write operation */
        ncounter = counter + 1;
        mrapi_atomic_cas(NULL,&q->update_counter,&ncounter,&counter,NULL,sizeof(q->update_counter),&status);
        assert(MRAPI_SUCCESS == status);
        return MCAPI_FALSE;
      }
      else {
        /* update the ptr in the receive_endpoints queue to point to our mcapi buffer */
        /* shared memory is zeroed, so we store our index as index with a valid bit so that we can tell if it's valid or not*/
        buff_index = i | MCAPI_VALID_MASK;
        mrapi_atomic_xchg(NULL,&buf->buff_index,&buff_index,NULL,sizeof(buf->buff_index),&status);
        assert(MRAPI_SUCCESS == status);
      }
    }

    /* now go about updating buffer into the database... */
    mcapi_dprintf(4,"mcapi_tran_send_state: pushing %u byte buffer to qindex=%i, num_elements=%i",
                  buffer_size,qindex,
                  mcapi_trans_queue_elements(q));
    /* printf(" send pushing to qindex=%i",qindex); */
    db_buff->parent = buf; /* local pointer in shared memory */
    if (q->channel_type == MCAPI_SCL_CHAN ) {
      db_buff->scalar = scalar;
    } else {
      /* copy the buffer parm into a mcapi buffer */
      memcpy (db_buff->buff,buffer,buffer_size);
    }
    /* set the size */
    db_buff->size = buffer_size;

    mrapi_atomic_read(NULL,&buf->state,&oldbstate,sizeof(buf->state),&status);
    assert(MRAPI_SUCCESS == status);
    if(BUFFER_ALLOCATED != oldbstate) {
      /* mark buffer as allocated */
      newbstate = BUFFER_ALLOCATED;
      mrapi_atomic_cas(NULL,&buf->state,&newbstate,&oldbstate,NULL,sizeof(buf->state),&status);
      assert(MRAPI_SUCCESS == status);
    }
    /* increment counter to complete write operation */
    ncounter = counter + 1;
    mrapi_atomic_cas(NULL,&q->update_counter,&ncounter,&counter,NULL,sizeof(q->update_counter),&status);
    assert(MRAPI_SUCCESS == status);

    return MCAPI_TRUE;
  }

  /***************************************************************************
  NAME:  mcapi_trans_recv_
  DESCRIPTION: Removes a message (at the given qindex) from the given
    receive endpoints queue.  This function is used both by check_receive_request
    and mcapi_trans_recv.  We needed to separate the functionality
    because in order to preserve FIFO, if recv was called to an empty queue we
    had to set a reservation at the head of the queue.  Thus we can't always
    just pop from the head of the queue.
  PARAMETERS:
    rn - the receive node index
    re - the receive endpoint index
    buffer -
    buffer_size -
    received_size - the actual size (in bytes) of the data received
    qindex - index into the receive endpoints queue that we should remove from
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_recv_ (uint16_t rd,uint16_t rn, uint16_t re, void** buffer, size_t buffer_size,
                          size_t* received_size,int qindex,uint64_t* scalar)
  {
    size_t size;
    int index = 0;
    int32_t buff_index;
    buffer_entry* db_buff = NULL;
    queue* q = &mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue;
    buffer_descriptor* buf = &q->elements[qindex];
    mcapi_buffer_state newbstate;
    mrapi_status_t status;

    /* shared memory is zeroed, so we store our index as index w/ a valid bit so that we can tell if it's valid or not*/
    mrapi_atomic_read(NULL,&buf->buff_index,&buff_index,sizeof(buf->buff_index),&status);
    assert(MRAPI_SUCCESS == status);
    index = buff_index &~ MCAPI_VALID_MASK;
    mcapi_assert (index >= 0);
    db_buff = &mcapi_db->buffers[index];
    db_buff->parent = buf; /* local pointer in shared memory */
    mcapi_dprintf(3,"mcapi_trans_recv_ for receiver (node=%u,port=%u)",
                  mcapi_db->domains[rd].nodes[rn].state.data.node_num,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].state.data.port_num);

    /* printf(" recv popping from qindex=%i",qindex); */
    /* first make sure buffer is big enough for the message */
    if ((buffer_size) < db_buff->size) {
      fprintf(stderr,"ERROR: mcapi_trans_recv_ buffer not big enough - loss of data: buffer_size=%i, element_size=%i",
              (int)buffer_size,
              (int)db_buff->size);
      /* NOTE: MCAPI_ETRUNCATED will be set by the calling functions by noticing that buffer_size < received_size */
    }

    /* set the size */
    size = db_buff->size;

    /* fill in the size */
    *received_size = size;
    if (buffer_size < size) {
      size = buffer_size;
    }

    /* copy the buffer out of the receive_endpoint's queue and into the buffer parm */
    if (q->channel_type == MCAPI_PKT_CHAN) {
      /* mcapi supplied buffer (pkt receive), so just update the pointer */
      assert(NULL != db_buff->buff);
      *buffer = db_buff->buff;

#ifdef NOTUSED
      /* Verify FIFO order if first two bytes of buffer are a transaction ID */
      if(MCAPI_PKT_CHAN == q->channel_type) {
        int tx = **(mcapi_uint16_t**)buffer;
        if(10000 > tx) {
          assert(q->last_recv_tx+1 == tx);
          q->last_recv_tx = tx;
          if(MCAPI_MAX_QUEUE_ENTRIES-1 > qindex) {
            /* Check if next entry is in transaction ID order */
            int bin = q->elements[qindex+1].buff_index;
            if(0 < bin) {
              int txn = *(mcapi_uint16_t*)mcapi_db->buffers[bin &~ MCAPI_VALID_MASK].buff;
              assert(q->last_recv_tx+2 == txn);
            }
          }
        }
      }
#endif  /* NOTUSED */

    } else {
      uint32_t* num = &db_buff->magic_num;
      uint32_t oldnum = MAGIC_NUM;
      uint32_t newnum = 0;
      /* user supplied buffer, copy it in and free the mcapi buffer */
      if (q->channel_type == MCAPI_SCL_CHAN) {
        /* scalar receive */
        *scalar = db_buff->scalar;
      } else {
        /* msg receive */
        memcpy (*buffer,db_buff->buff,size);
      }
      /* free the mcapi buffer */
      status = MRAPI_SUCCESS;
      mrapi_atomic_cas(NULL,num,&newnum,&oldnum,NULL,sizeof(db_buff->magic_num),&status);
      assert(MRAPI_SUCCESS == status);
    }
    mcapi_dprintf(4,"receive popping %u byte buffer from qindex=%i, num_elements=%i buffer=[",
                  size,qindex,
                  mcapi_trans_queue_elements(q));

    /* clear the buffer pointer in the receive queue entry */
    status = MRAPI_SUCCESS;
    buf->reserved = MCAPI_FALSE;
    buf->buff_index = 0;
    newbstate = BUFFER_FREE;
    mrapi_atomic_xchg(NULL,&buf->state,&newbstate,NULL,sizeof(buf->state),&status);
    assert(MRAPI_SUCCESS == status);
  }

  /***************************************************************************
  NAME:  mcapi_trans_recv_state_
  DESCRIPTION: Removes a message (at the given qindex) from the given
    receive buffer.  This function is used both by check_receive_request_state
    and mcapi_trans_recv_state.
  PARAMETERS:
    rn - the receive node index
    re - the receive endpoint index
    buffer -
    buffer_size -
    received_size - the actual size (in bytes) of the data received
    qindex - index into the receive endpoints buffer that we should remove from
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_recv_state_ (uint16_t rd,uint16_t rn, uint16_t re, void** buffer, size_t buffer_size,
                                size_t* received_size,int qindex,uint64_t* scalar)
  {
    size_t size;
    int index = 0;
    int32_t buff_index;
    buffer_entry* db_buff = NULL;
    queue* q = &mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue;
    buffer_descriptor* buf = &q->elements[qindex];
    mrapi_status_t status;

    /* shared memory is zeroed, so we store our index as index w/ a valid bit so that we can tell if it's valid or not*/
    mrapi_atomic_read(NULL,&buf->buff_index,&buff_index,sizeof(buf->buff_index),&status);
    assert(MRAPI_SUCCESS == status);
    index = buff_index &~ MCAPI_VALID_MASK;
    mcapi_assert (index >= 0);
    db_buff = &mcapi_db->buffers[index];
    db_buff->parent = buf; /* local pointer in shared memory */

    mcapi_dprintf(3,"mcapi_trans_recv_state_ for receiver (node=%u,port=%u)",
                  mcapi_db->domains[rd].nodes[rn].state.data.node_num,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].state.data.port_num);

    /* printf(" recv popping from qindex=%i",qindex); */
    /* first make sure buffer is big enough for the message */
    if ((buffer_size) < db_buff->size) {
      fprintf(stderr,"ERROR: mcapi_trans_recv_state_: buffer not big enough - loss of data: buffer_size=%i, element_size=%i",
              (int)buffer_size,
              (int)db_buff->size);
      /* NOTE: MCAPI_ETRUNCATED will be set by the calling functions by noticing that buffer_size < received_size */
    }

    /* set the size */
    size = db_buff->size;

    /* fill in the size */
    *received_size = size;
    if (buffer_size < size) {
      size = buffer_size;
    }

    /* copy the buffer out of the receive_endpoint's queue and into the buffer parm */
    if (q->channel_type == MCAPI_PKT_CHAN) {
      /* mcapi supplied buffer (pkt receive), so just update the pointer */
      assert(NULL != db_buff->buff);
      *buffer = db_buff->buff;
    } else {
      /* user supplied buffer, copy it in and free the mcapi buffer */
      if (q->channel_type == MCAPI_SCL_CHAN) {
        /* scalar receive */
        *scalar = db_buff->scalar;
      } else {
        /* msg receive */
        memcpy (*buffer,db_buff->buff,size);
      }
    }
  }

  /***************************************************************************
  NAME: mcapi_trans_recv
  DESCRIPTION: checks if a message is available, if so performs the pop (from
   the head of the queue) and sends the qindex to be used to mcapi_trans_recv_
  PARAMETERS:
    rn - the receive node index
    re - the receive endpoint index
    buffer -
    buffer_size -
    received_size - the actual size (in bytes) of the data received
    blocking - whether or not this is a blocking receive
  RETURN VALUE: true/false indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_recv (uint16_t rd,uint16_t rn, uint16_t re, void** buffer,
                                    size_t buffer_size, size_t* received_size,
                                    mcapi_boolean_t blocking,uint64_t* scalar)
  {
    int qindex;
    int32_t buff_index;
    mrapi_status_t status;
    mcapi_status_t mcapi_status;
    buffer_descriptor* buf = NULL;
    mcapi_buffer_state oldbstate;
    mcapi_buffer_state newbstate;

    queue* q = &mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue;
    while(1) {
      qindex = mcapi_trans_queue_head(q,&mcapi_status);
      if(MCAPI_SUCCESS == mcapi_status) {
        buf = &q->elements[qindex];
        mrapi_atomic_read(NULL,&buf->state,&oldbstate,sizeof(buf->state),&status);
        assert(MRAPI_SUCCESS == status);
        mrapi_atomic_read(NULL,&buf->buff_index,&buff_index,sizeof(buf->buff_index),&status);
        assert(MRAPI_SUCCESS == status);
        if(BUFFER_ALLOCATED != oldbstate ||
            0 == buff_index) {
          if(!blocking) {
            return MCAPI_FALSE;
          }
          else {
            continue;
          }
        }
        else {
          /* mark entry as received */
          status = MRAPI_SUCCESS;
          oldbstate = BUFFER_ALLOCATED;
          newbstate = BUFFER_RECEIVED;
          mrapi_atomic_cas(NULL,&buf->state,&newbstate,&oldbstate,NULL,sizeof(buf->state),&status);
          assert(MRAPI_SUCCESS == status);
          break;
        }
      }
      if(MCAPI_ERR_QUEUE_EMPTY_PRODUCER_INSERTING == mcapi_status) {
        continue;
      }
      else if(MCAPI_ERR_QUEUE_EMPTY == mcapi_status) {
        if(!blocking) {
          return MCAPI_FALSE;
        }
        else {
          mcapi_dprintf(5,"mcapi_trans_recv to empty queue - attempting to yield");
          /* we have the lock, use this yield */
          mcapi_trans_yield();
        }
      }
    }

    mcapi_trans_recv_ (rd,rn,re,buffer,buffer_size,received_size,qindex,scalar);

    /* Remove entry from queue */
    (void)mcapi_trans_pop_queue(q,&mcapi_status);
    assert(MCAPI_SUCCESS == mcapi_status);

    return MCAPI_TRUE;
  }

  /***************************************************************************
  NAME: mcapi_trans_recv_state
  DESCRIPTION: checks if a message is available, if so sends the qindex to be
   used to mcapi_trans_recv_
  PARAMETERS:
    rn - the receive node index
    re - the receive endpoint index
    buffer -
    buffer_size -
    received_size - the actual size (in bytes) of the data received
    blocking - whether or not this is a blocking receive
  RETURN VALUE: true/false indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_recv_state (uint16_t rd,uint16_t rn, uint16_t re, void** buffer,
                                          size_t buffer_size, size_t* received_size,
                                          mcapi_boolean_t blocking,uint64_t* scalar)
  {
    mcapi_boolean_t rc = MCAPI_TRUE;
    int qindex;
    int32_t buff_index;
    unsigned counter;
    unsigned update_counter;
    mcapi_buffer_state prevbstate;
    mcapi_buffer_state oldbstate;
    mcapi_buffer_state newbstate;
    mrapi_status_t status;
    buffer_descriptor* buf = NULL;
    attribute_entry_t* ae = mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].attributes.entries;
    queue* q = &mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue;

    while(1) {
      /* save buffer index counter for later read success confirmation */
      mrapi_atomic_read(NULL,&q->update_counter,&update_counter,sizeof(q->update_counter),&status);
      assert(MRAPI_SUCCESS == status);
      counter = update_counter;
      qindex = ((counter/2)-1) % ae[MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS].attribute_d.value;
      buf = &q->elements[qindex];
      mrapi_atomic_read(NULL,&buf->state,&oldbstate,sizeof(buf->state),&status);
      assert(MRAPI_SUCCESS == status);
      mrapi_atomic_read(NULL,&buf->buff_index,&buff_index,sizeof(buf->buff_index),&status);
      assert(MRAPI_SUCCESS == status);
      if(0 == buff_index) {
        /* no message to receive */
        rc = MCAPI_FALSE;
        break;
      }

      if(MCAPI_PKT_CHAN == q->channel_type) {
        /* prevent buffer reuse, mcapi_trans_send_state resets to BUFFER_RESERVED
           when entry is written */
        oldbstate = BUFFER_ALLOCATED;
        newbstate = BUFFER_RECEIVED;
        status = MRAPI_SUCCESS;
        mrapi_atomic_cas(NULL,&buf->state,&newbstate,&oldbstate,&prevbstate,sizeof(buf->state),&status);
        if(MRAPI_SUCCESS != status) {
          if(BUFFER_RESERVED == prevbstate) {
            /* write collision */
            continue;
          }
          else {
            assert(BUFFER_RECEIVED == prevbstate);
          }
        }
      }

      mcapi_trans_recv_state_ (rd,rn,re,buffer,buffer_size,received_size,qindex,scalar);

      /* confirm successful read */
      mrapi_atomic_read(NULL,&q->update_counter,&update_counter,sizeof(q->update_counter),&status);
      assert(MRAPI_SUCCESS == status);
      if((update_counter-(counter/2)*2) <= (2*ae[MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS].attribute_d.value)-2) {

        mcapi_dprintf(4,"mcapi_trans_receive_state: %u byte buffer from qindex=%i, num_elements=%i buffer=[",
                      received_size,qindex,
                      mcapi_trans_queue_elements(q));
        break;
      }
    }

    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_open_channel
  DESCRIPTION: marks the given endpoint as open
  PARAMETERS:
    n - the node index
    e - the endpoint index
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_open_channel (uint16_t d,uint16_t n, uint16_t e)
  {
    endpoint_state* state = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state;
    endpoint_state oldstate;
    endpoint_state newstate;
    mrapi_status_t status;

    /* mark the endpoint as open */
    newstate = oldstate = *state;
    newstate.data.open = MCAPI_TRUE;
    status = MRAPI_SUCCESS;
    mrapi_atomic_cas(NULL,state,&newstate,&oldstate,NULL,
        sizeof(mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state),&status);
    assert(MRAPI_SUCCESS == status);
  }

  /***************************************************************************
  NAME:mcapi_trans_close_channel
  DESCRIPTION: marks the given endpoint as closed
  PARAMETERS:
    n - the node index
    e - the endpoint index
  RETURN VALUE:none
  ***************************************************************************/
  void mcapi_trans_close_channel (uint16_t d,uint16_t n, uint16_t e)
  {
    endpoint_state* state = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state;
    endpoint_state oldstate;
    endpoint_state newstate;
    mrapi_status_t status;
    /* mark the endpoint as closed */
    newstate = oldstate = *state;
    newstate.data.open = MCAPI_FALSE;
    status = MRAPI_SUCCESS;
    mrapi_atomic_cas(NULL,state,&newstate,&oldstate,NULL,
        sizeof(mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state),&status);
    assert(MRAPI_SUCCESS == status);
  }

  /***************************************************************************
  NAME:mcapi_trans_yield
  DESCRIPTION: attempts to yield.
  PARAMETERS: none
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_yield ()
  {
    sys_os_yield();
  }

  /***************************************************************************
  NAME: mcapi_trans_access_database_pre
  DESCRIPTION: This function acquires the semaphore.
  PARAMETERS: none
  RETURN VALUE:none
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_access_database_pre (uint32_t handle,
                                                   mcapi_boolean_t exclusive)
  {
    /* first acquire the semaphore, this is a blocking function */
    assert(locked == 0);
    if (transport_sm_lock_rwl(handle,exclusive)) {
      locked++;
      mcapi_dprintf(4,"mcapi_trans_access_database_pre ()");
      return MCAPI_TRUE;
    }
    return MCAPI_FALSE;
  }

  /***************************************************************************
  NAME:mcapi_trans_access_database_post
  DESCRIPTION: This function releases the semaphore.
  PARAMETERS: none
  RETURN VALUE: none
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_access_database_post (uint32_t handle,
                                                    mcapi_boolean_t exclusive)
  {
    assert(locked == 1);
    mcapi_dprintf(4,"mcapi_trans_access_database_post ()");

    /* finally, release the semaphore, this should always work */
    if (transport_sm_unlock_rwl(handle,exclusive)) {
      locked--;
      return MCAPI_TRUE;
    }
    return MCAPI_FALSE;
  }

  /***************************************************************************
  NAME:mcapi_trans_encode_handle
  DESCRIPTION:
   Our handles are very simple - a 32 bit integer is encoded with
   an index (16 bits gives us a range of 0:64K indices).
   Currently, we only have 3 indices for each of: domain array,
   node array, and endpoint array.
  PARAMETERS:
   node_index -
   endpoint_index -
  RETURN VALUE: the handle
  ***************************************************************************/
  uint32_t mcapi_trans_encode_handle (uint16_t domain_index,uint16_t node_index,uint16_t endpoint_index)
  {
    /* The database should already be locked */
    uint32_t handle = 0;
    uint8_t shift = 8;

    mcapi_assert ((domain_index < MCA_MAX_DOMAINS) &&
            (node_index < MCA_MAX_NODES) &&
            (endpoint_index < MCAPI_MAX_ENDPOINTS));

    handle = domain_index;
    handle <<= shift;
    handle |= node_index;
    handle <<= shift;
    handle |= endpoint_index;

    return handle;
  }

  /***************************************************************************
  NAME:mcapi_trans_decode_handle
  DESCRIPTION: Decodes the given handle into it's database indices
  PARAMETERS:
   handle -
   node_index -
   endpoint_index -
  RETURN VALUE: true/false indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_decode_handle (uint32_t handle, uint16_t *domain_index,uint16_t *node_index,
                                                      uint16_t *endpoint_index)
  {
    int rc = MCAPI_FALSE;
    uint8_t shift = 8;

    /* The database should already be locked */
    *domain_index            = (handle & 0x00ff0000) >> (shift * 2);
    *node_index              = (handle & 0x0000ff00) >> shift;
    *endpoint_index          = (handle & 0x000000ff);

    if ((*domain_index < MCA_MAX_DOMAINS) &&
        (*node_index < MCA_MAX_NODES) &&
        (*endpoint_index < MCAPI_MAX_ENDPOINTS)) {
      rc = MCAPI_TRUE;
    }

    return rc;
  }


  /***************************************************************************
  Function: mcapi_trans_whoami

  Description: Gets the pid,tid pair for the caller and  then
      looks up the corresponding node and domain info in our database.

  Parameters:

  Returns: boolean indicating success or failure

  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_whoami (mcapi_node_t* node_id,uint32_t* n_index,
                                                      mcapi_domain_t* domain_id,uint32_t* d_index)
  {
    if (mcapi_db == NULL) { return MCAPI_FALSE;}
    if (mcapi_pid == (pid_t)-1)  { return MCAPI_FALSE;}
    else {
      *n_index = mcapi_nindex;
      *d_index = mcapi_dindex;
      *node_id = mcapi_node_num;
      *domain_id = mcapi_domain_id;
    }
   return MCAPI_TRUE;
  }

#include "Fragments/transport_sm_queue_abb.c"
#include "Fragments/transport_sm_config_abb.c"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */
