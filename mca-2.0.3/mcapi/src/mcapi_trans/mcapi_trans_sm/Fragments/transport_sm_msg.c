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
  //                   mcapi_trans API: messages                              //
  //                                                                          //
  //////////////////////////////////////////////////////////////////////////////
  /***************************************************************************
  NAME: mcapi_trans_msg_send
  DESCRIPTION: sends a connectionless message from one endpoint to another (blocking)
  PARAMETERS:
     send_endpoint - the sending endpoint's handle
     receive_endpoint - the receiving endpoint's handle
     buffer - the user supplied buffer
     buffer_size - the size in bytes of the buffer
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_msg_send( mcapi_endpoint_t  send_endpoint,
                                        mcapi_endpoint_t  receive_endpoint,
                                        const char* buffer,
                                        size_t buffer_size)
  {
    mcapi_request_t request;
    mcapi_status_t status = MCAPI_SUCCESS;
    size_t size;

    /* use non-blocking followed by wait */
    do {
      mcapi_trans_msg_send_i (send_endpoint,receive_endpoint,buffer,buffer_size,&request,&status);
    } while (status == MCAPI_ERR_REQUEST_LIMIT);

    mcapi_trans_wait (&request,&size,&status,MCA_INFINITE);

    if (status == MCAPI_SUCCESS) {
      return MCAPI_TRUE;
    }
    return MCAPI_FALSE;
  }

  /***************************************************************************
  NAME: mcapi_trans_msg_send_i
  DESCRIPTION: sends a connectionless message from one endpoint to another (non-blocking)
  PARAMETERS:
     send_endpoint - the sending endpoint's handle
     receive_endpoint - the receiving endpoint's handle
     buffer - the user supplied buffer
     buffer_size - the size in bytes of the buffer
     request - the request handle to be filled in when the task is complete
  RETURN VALUE:none
  ***************************************************************************/
  void mcapi_trans_msg_send_i( mcapi_endpoint_t  send_endpoint,
                               mcapi_endpoint_t  receive_endpoint,
                               const char* buffer,
                               size_t buffer_size,
                               mcapi_request_t* request,
                               mcapi_status_t* mcapi_status)
  {
    int r;
    uint16_t sd,sn,se;
    uint16_t rd,rn,re;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;
    mcapi_dprintf(1,"mcapi_trans_msg_send_i (0x%x,0x%x,buffer,%u,&request,&status);",send_endpoint,receive_endpoint,buffer_size);

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    /* make sure we have an available request entry*/
    if ( mcapi_trans_reserve_request_have_lock(&r)) {
      if (!completed) {
        completed = MCAPI_TRUE; /* sends complete immediately */
        mcapi_assert(mcapi_trans_decode_handle(send_endpoint,&sd,&sn,&se));
        mcapi_assert(mcapi_trans_decode_handle(receive_endpoint,&rd,&rn,&re));
        mcapi_assert (mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.channel_type == MCAPI_NO_CHAN);
        mcapi_assert (mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.channel_type == MCAPI_NO_CHAN);

        if (!mcapi_trans_send_have_lock (sd,sn,se,rd,rn,re,buffer,buffer_size,0)) {
          /* assume couldn't get a buffer */
          *mcapi_status = MCAPI_ERR_MEM_LIMIT;
        }
      }
      mcapi_assert(setup_request_have_lock(&receive_endpoint,request,mcapi_status,completed,buffer_size,NULL,SEND,0,0,0,r));
      /* the non-blocking request succeeded, when they call test/wait they will see the status of the send */
      *mcapi_status = MCAPI_SUCCESS;
    } else {
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }

  /***************************************************************************
  NAME:mcapi_trans_msg_recv
  DESCRIPTION:receives a message from this endpoints receive queue (blocking)
  PARAMETERS:
     receive_endpoint - the receiving endpoint
     buffer - the user supplied buffer to copy the message into
     buffer_size - the size of the user supplied buffer
     received_size - the actual size of the message received
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_msg_recv( mcapi_endpoint_t  receive_endpoint,
                                        char* buffer,
                                        size_t buffer_size,
                                        size_t* received_size)
  {
    mcapi_request_t request;
    mcapi_status_t status = MCAPI_SUCCESS;

    mcapi_dprintf(2,"mcapi_trans_msg_recv re=0x%x",receive_endpoint);

    /* use non-blocking followed by wait */
    do {
      mcapi_trans_msg_recv_i (receive_endpoint,buffer,buffer_size,&request,&status);
    } while (status == MCAPI_ERR_REQUEST_LIMIT);

    mcapi_trans_wait (&request,received_size,&status,MCA_INFINITE);

    if (status == MCAPI_SUCCESS) {
      return MCAPI_TRUE;
    }
    return MCAPI_FALSE;

  }

  /***************************************************************************
  NAME:mcapi_trans_msg_recv_i
  DESCRIPTION:receives a message from this endpoints receive queue (non-blocking)
  PARAMETERS:
     receive_endpoint - the receiving endpoint
     buffer - the user supplied buffer to copy the message into
     buffer_size - the size of the user supplied buffer
     received_size - the actual size of the message received
     request - the request to be filled in when the task is completed.
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE indicating success or failure
  ***************************************************************************/
  void mcapi_trans_msg_recv_i( mcapi_endpoint_t  receive_endpoint,  char* buffer,
                               size_t buffer_size, mcapi_request_t* request,
                               mcapi_status_t* mcapi_status)
  {
    uint16_t rd,rn,re;
    int r;
    size_t received_size = 0;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_dprintf(1,"mcapi_trans_msg_recv_i(0x%x,buffer,%u,&request,&status);",receive_endpoint,buffer_size);

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request_have_lock(&r)) {
      if (!completed) {
        mcapi_assert(mcapi_trans_decode_handle(receive_endpoint,&rd,&rn,&re));

        mcapi_assert (mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.channel_type == MCAPI_NO_CHAN);

        if (mcapi_trans_recv_have_lock(rd,rn,re,(void*)&buffer,buffer_size,&received_size,MCAPI_FALSE,NULL)) {
          completed = MCAPI_TRUE;
          buffer_size = received_size;
        }
      }

      mcapi_assert(setup_request_have_lock(&receive_endpoint,request,mcapi_status,completed,buffer_size,(void**)((void*)&buffer),RECV,0,0,0,r));
    } else {
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }
    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }

  /***************************************************************************
  NAME: mcapi_trans_msg_available
  DESCRIPTION: counts the number of messages in the endpoints receive queue
  PARAMETERS:  endpoint
  RETURN VALUE: the number of messages in the queue
  ***************************************************************************/
  mcapi_uint_t mcapi_trans_msg_available( mcapi_endpoint_t receive_endpoint)
  {
    uint16_t rd,rn,re;
    int rc = MCAPI_FALSE;

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_dprintf(1,"mcapi_trans_msg_available(0x%x);",receive_endpoint);
    mcapi_assert(mcapi_trans_decode_handle(receive_endpoint,&rd,&rn,&re));
    rc = mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.num_elements;

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));

    return rc;
  }
