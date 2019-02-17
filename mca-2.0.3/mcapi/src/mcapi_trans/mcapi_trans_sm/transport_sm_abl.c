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

#include "Fragments/transport_sm_inc_abl.c"

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

#include "Fragments/transport_sm_declaration_abl.c"

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

#include "Fragments/transport_sm_request.c"
#include "Fragments/transport_sm_initialize_abl.c"
#include "Fragments/transport_sm_attributes.c"
#include "Fragments/transport_sm_finalize_abl.c"
#include "Fragments/transport_sm_check.c"
#include "Fragments/transport_sm_endpoint.c"
#include "Fragments/transport_sm_msg.c"
#include "Fragments/transport_sm_pktchan.c"
#include "Fragments/transport_sm_sclchan.c"
#include "Fragments/transport_sm_wait.c"

  //////////////////////////////////////////////////////////////////////////////
  //                                                                          //
  //                   misc helper functions                                  //
  //                                                                          //
  //////////////////////////////////////////////////////////////////////////////

#include "Fragments/transport_sm_signal.c"

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
             (unsigned)mcapi_db->domains[d].nodes[n].node_num,(unsigned)mcapi_db->domains[d].nodes[n].node_d.endpoints[e].port_num,
             (unsigned)mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.num_elements);

      printf("\n    endpoint: %u",e);
      printf("\n      valid:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].valid);
      printf("\n      anonymous:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].anonymous);
      printf("\n      open:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].open);
      printf("\n      connected:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].connected);
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
          if ((mcapi_db->domains[d].valid == MCAPI_TRUE) && (mcapi_db->domains[d].nodes[n].valid == MCAPI_TRUE)) {
            printf("\nVALID NODE: d=%u, nindex=%u domain_id=%u,node_num=%u,",
                   d,n,mcapi_db->domains[d].domain_id,(unsigned)mcapi_db->domains[d].nodes[n].node_num);
            printf("\n  num_endpoints:%u",mcapi_db->domains[d].nodes[n].node_d.num_endpoints);
            for (e = 0; e < mcapi_db->domains[d].nodes[n].node_d.num_endpoints; e++) {
              if (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].valid) {
                printf("\n    VALID ENDPT: e=%u",e);
                printf("\n    port_num: %u",(unsigned)mcapi_db->domains[d].nodes[n].node_d.endpoints[e].port_num);
                printf("\n      anonymous:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].anonymous);
                printf("\n      open:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].open);
                printf("\n      connected:%u",mcapi_db->domains[d].nodes[n].node_d.endpoints[e].connected);
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
  void mcapi_trans_connect_channel_have_lock (mcapi_endpoint_t send_endpoint,
                                             mcapi_endpoint_t receive_endpoint,
                                             channel_type type)
  {
    uint16_t sd,sn,se;
    uint16_t rd,rn,re;

    /* the database should already be locked */
    assert(locked == 1);

    mcapi_assert(mcapi_trans_decode_handle(send_endpoint,&sd,&sn,&se));
    mcapi_assert(mcapi_trans_decode_handle(receive_endpoint,&rd,&rn,&re));

    /* update the send endpoint */
    mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].connected = MCAPI_TRUE;
    mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.recv_endpt = receive_endpoint;
    mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.send_endpt = send_endpoint;
    mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.channel_type = type;

    /* update the receive endpoint */
    mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].connected = MCAPI_TRUE;
    mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.send_endpt = send_endpoint;
    mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.recv_endpt = receive_endpoint;
    mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.channel_type = type;


    mcapi_dprintf(1,"channel_type=%u connected sender (node=%u,port=%u) to receiver (node=%u,port=%u)",
                  type,mcapi_db->domains[sd].nodes[sn].node_num,
                  mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].port_num,
                  mcapi_db->domains[rd].nodes[rn].node_num,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].port_num);

  }

  /***************************************************************************
  NAME:mcapi_trans_send_have_lock
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
  mcapi_boolean_t mcapi_trans_send_have_lock (uint16_t sd, uint16_t sn,uint16_t se,
                                             uint16_t rd,uint16_t rn, uint16_t re,
                                             const char* buffer,
                                             size_t buffer_size,
                                             uint64_t scalar)
  {
    int qindex,i;
    buffer_entry* db_buff = NULL;

    mcapi_dprintf(3,"mcapi_trans_send_have_lock sender (node=%u,port=%u) to receiver (node=%u,port=%u) ",
                  mcapi_db->domains[sd].nodes[sn].node_num,
                  mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].port_num,
                  mcapi_db->domains[rd].nodes[rn].node_num,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].port_num);

    /* The database should already be locked! */
    assert(locked == 1);

    if (mcapi_trans_full_queue(mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue)) {
      /* we couldn't get space in the endpoints receive queue, try to compact the queue */
      mcapi_trans_compact_queue(&mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue);
      return MCAPI_FALSE;
    }

    /* find a free mcapi buffer (we only have to worry about this on the sending side) */
    for (i = 0; i < MCAPI_MAX_BUFFERS; i++) {
      if (!mcapi_db->buffers[i].magic_num) {
        mcapi_db->buffers[i].magic_num = MAGIC_NUM;
        db_buff = &mcapi_db->buffers[i];
        mcapi_dprintf(4,"using buffer index i=%u\n",i);
        break;
      }
    }
    if (i == MCAPI_MAX_BUFFERS) {
      /* we couldn't get a free buffer */
      mcapi_dprintf(2,"ERROR mcapi_trans_send_have_lock: No more buffers available - try freeing some buffers. ");
      return MCAPI_FALSE;
    }

    /* now go about updating buffer into the database... */
    /* find the next index in the circular queue */
    qindex = mcapi_trans_push_queue(&mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue);
    mcapi_dprintf(4,"send pushing %u byte buffer to qindex=%i, num_elements=%i, head=%i, tail=%i",
                  buffer_size,qindex,mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.num_elements,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.head,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.tail);
    /* printf(" send pushing to qindex=%i",qindex); */
    if (mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.channel_type == MCAPI_SCL_CHAN ) {
      db_buff->scalar = scalar;
    } else {
      /* copy the buffer parm into a mcapi buffer */
      memcpy (db_buff->buff,buffer,buffer_size);
    }
    /* set the size */
    db_buff->size = buffer_size;
    /* update the ptr in the receive_endpoints queue to point to our mcapi buffer */
    /* shared memory is zeroed, so we store our index as index with a valid bit so that we can tell if it's valid or not*/
    mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[qindex].buff_index = i | MCAPI_VALID_MASK;


    return MCAPI_TRUE;
  }

  /***************************************************************************
  NAME:  mcapi_trans_recv_have_lock_
  DESCRIPTION: Removes a message (at the given qindex) from the given
    receive endpoints queue.  This function is used both by check_receive_request
    and mcapi_trans_recv_have_lock.  We needed to separate the functionality
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
  void mcapi_trans_recv_have_lock_ (uint16_t rd,uint16_t rn, uint16_t re, void** buffer, size_t buffer_size,
                                   size_t* received_size,int qindex,uint64_t* scalar)
  {
    size_t size;
    int index = 0;

    /* the database should already be locked! */
    assert(locked == 1);

    /* shared memory is zeroed, so we store our index as index w/ a valid bit so that we can tell if it's valid or not*/
    index = mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[qindex].buff_index &~ MCAPI_VALID_MASK;
    mcapi_assert (index >= 0);

    mcapi_dprintf(3,"mcapi_trans_recv_have_lock_ for receiver (node=%u,port=%u)",
                  mcapi_db->domains[rd].nodes[rn].node_num,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].port_num);

    /* printf(" recv popping from qindex=%i",qindex); */
    /* first make sure buffer is big enough for the message */
    if ((buffer_size) < mcapi_db->buffers[index].size) {
      fprintf(stderr,"ERROR: mcapi_trans_recv_have_lock buffer not big enough - loss of data: buffer_size=%i, element_size=%i",
              (int)buffer_size,
              (int)mcapi_db->buffers[index].size);
      /* NOTE: MCAPI_ETRUNCATED will be set by the calling functions by noticing that buffer_size < received_size */
    }

    /* set the size */
    size = mcapi_db->buffers[index].size;

    /* fill in the size */
    *received_size = size;
    if (buffer_size < size) {
      size = buffer_size;
    }


    /* copy the buffer out of the receive_endpoint's queue and into the buffer parm */
    if (mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.channel_type == MCAPI_PKT_CHAN) {
      /* mcapi supplied buffer (pkt receive), so just update the pointer */
      *buffer = mcapi_db->buffers[index].buff;
    } else {
      /* user supplied buffer, copy it in and free the mcapi buffer */
      if   (mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.channel_type == MCAPI_SCL_CHAN) {
        /* scalar receive */
        *scalar = mcapi_db->buffers[index].scalar;
      } else {
        /* msg receive */
        memcpy (*buffer,mcapi_db->buffers[index].buff,size);
      }
      /* free the mcapi  buffer */
      memset(&mcapi_db->buffers[index],0,sizeof(mcapi_db->buffers[index]));
    }
    mcapi_dprintf(4,"receive popping %u byte buffer from qindex=%i, num_elements=%i, head=%i, tail=%i buffer=[",
                  size,qindex,mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.num_elements,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.head,
                  mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.tail);

    /* clear the buffer pointer in the receive queue entry */
    mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[qindex].buff_index = 0;

  }

  /***************************************************************************
  NAME: mcapi_trans_recv_have_lock
  DESCRIPTION: checks if a message is available, if so performs the pop (from
   the head of the queue) and sends the qindex to be used to mcapi_trans_recv_have_lock_
  PARAMETERS:
    rn - the receive node index
    re - the receive endpoint index
    buffer -
    buffer_size -
    received_size - the actual size (in bytes) of the data received
    blocking - whether or not this is a blocking receive
  RETURN VALUE: true/false indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_recv_have_lock (uint16_t rd,uint16_t rn, uint16_t re, void** buffer,
                                             size_t buffer_size, size_t* received_size,
                                             mcapi_boolean_t blocking,uint64_t* scalar)
  {
    int qindex;

    /* The database should already be locked! */
    assert(locked == 1);

    if ((!blocking) && (mcapi_trans_empty_queue(mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue))) {
      return MCAPI_FALSE;
    }

    while (mcapi_trans_empty_queue(mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue)) {
      mcapi_dprintf(5,"mcapi_trans_recv_have_lock to empty queue - attempting to yield");
      /* we have the lock, use this yield */
      mcapi_trans_yield_have_lock();
    }

    /* remove the element from the receive endpoints queue */
    qindex = mcapi_trans_pop_queue(&mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue);
    mcapi_trans_recv_have_lock_ (rd,rn,re,buffer,buffer_size,received_size,qindex,scalar);

    return MCAPI_TRUE;
  }

  /***************************************************************************
  NAME: mcapi_trans_open_channel_have_lock
  DESCRIPTION: marks the given endpoint as open
  PARAMETERS:
    n - the node index
    e - the endpoint index
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_open_channel_have_lock (uint16_t d,uint16_t n, uint16_t e)
  {
    /* The database should already be locked! */
    assert(locked == 1);

    /* mark the endpoint as open */
    mcapi_db->domains[d].nodes[n].node_d.endpoints[e].open = MCAPI_TRUE;

  }

  /***************************************************************************
  NAME:mcapi_trans_close_channel_have_lock
  DESCRIPTION: marks the given endpoint as closed
  PARAMETERS:
    n - the node index
    e - the endpoint index
  RETURN VALUE:none
  ***************************************************************************/
  void mcapi_trans_close_channel_have_lock (uint16_t d,uint16_t n, uint16_t e)
  {
    /* The database should already be locked! */
    assert(locked == 1);

    /* mark the endpoint as closed */
    mcapi_db->domains[d].nodes[n].node_d.endpoints[e].open = MCAPI_FALSE;
  }

  /***************************************************************************
  NAME:mcapi_trans_yield_have_lock
  DESCRIPTION: releases the lock, attempts to yield, re-acquires the lock.
  PARAMETERS: none
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_yield_have_lock ()
  {
    /* call this version of sched_yield when you have the lock */
    assert(locked == 1);

    /* release the lock */
    mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE);
    assert(locked == 0);

#if (__unix__||__MINGW32__)
    sched_yield();
#else
    SleepEx(0,0);
#endif  /* !(__unix__||__MINGW32__) */
    /* re-acquire the lock */
    mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE);
    assert (locked == 1);
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

#include "Fragments/transport_sm_queue.c"
#include "Fragments/transport_sm_config_abl.c"

#ifdef __cplusplus
}
#endif /* __cplusplus */
