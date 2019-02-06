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
  //                   mcapi_trans API: scalar channels                       //
  //                                                                          //
  //////////////////////////////////////////////////////////////////////////////

  /***************************************************************************
  NAME:mcapi_trans_sclchan_connect_i
  DESCRIPTION: connects a scalar channel between the given two endpoints
  PARAMETERS:
      send_endpoint - the sending endpoint
      receive_endpoint - the receiving endpoint
      request - the request
      mcapi_status - the status
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_sclchan_connect_i( mcapi_endpoint_t  send_endpoint,
                                      mcapi_endpoint_t  receive_endpoint,
                                      mcapi_request_t* request,
                                      mcapi_status_t* mcapi_status)
  {
    int r;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request(&r)) {
      if (!completed) {
        mcapi_trans_connect_channel (send_endpoint,receive_endpoint,MCAPI_SCL_CHAN);
        completed = MCAPI_TRUE;
      }
      mcapi_assert(setup_request(&receive_endpoint,request,mcapi_status,completed,0,NULL,
          (mcapi_request_type)CONN_SCLCHAN,0,0,0,r));

    } else {
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }
  }

  /***************************************************************************
  NAME: mcapi_trans_sclchan_recv_open_i
  DESCRIPTION: opens the receive endpoint on a packet channel
  PARAMETERS:
    recv_handle - the receive channel handle to be filled in
    receive_endpoint - the receiving endpoint handle
    request - the request to be filled in when the task is complete
    mcapi_status
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_sclchan_recv_open_i( mcapi_sclchan_recv_hndl_t* recv_handle,
                                        mcapi_endpoint_t receive_endpoint,
                                        mcapi_request_t* request,
                                        mcapi_status_t* mcapi_status)
  {
    int r;
    uint16_t rd,rn,re;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    mcapi_dprintf(1,"mcapi_trans_sclchan_recv_open_i(recv_handle,0x%x,&request,&status);",receive_endpoint);

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request(&r)) {
      if (!completed) {
        mcapi_assert(mcapi_trans_decode_handle(receive_endpoint,&rd,&rn,&re));

        mcapi_trans_open_channel (rd,rn,re);

        /* fill in the channel handle */
        *recv_handle = mcapi_trans_encode_handle(rd,rn,re);

        /* has the channel been connected yet? */
        if ( mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.channel_type == MCAPI_SCL_CHAN){
          completed = MCAPI_TRUE;
        }

        mcapi_dprintf(2,"mcapi_trans_sclchan_recv_open_i (node_num=%u,port_num=%u) handle=0x%x",
                      mcapi_db->domains[rd].nodes[rn].state.data.node_num,
                      mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].state.data.port_num,
                      *recv_handle);
      }

      mcapi_assert(setup_request(&receive_endpoint,request,mcapi_status,completed,0,NULL,OPEN_SCLCHAN,0,0,0,r));
    } else{
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }
  }


  /***************************************************************************
  NAME: mcapi_trans_sclchan_send_open_i
  DESCRIPTION: opens the receive endpoint on a packet channel
  PARAMETERS:
    send_handle - the receive channel handle to be filled in
    receive_endpoint - the receiving endpoint handle
    request - the request to be filled in when the task is complete
    mcapi_status
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_sclchan_send_open_i( mcapi_sclchan_send_hndl_t* send_handle,
                                        mcapi_endpoint_t  send_endpoint,
                                        mcapi_request_t* request,
                                        mcapi_status_t* mcapi_status)
  {
    int r;
    uint16_t sd,sn,se;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    mcapi_dprintf(1,"mcapi_trans_sclchan_send_open_i(send_handle,0x%x,&request,&status);",send_endpoint);

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request(&r)) {
      if (!completed) {
        endpoint_state* state = NULL;
        endpoint_state oldstate;
        endpoint_state newstate;
        mrapi_status_t status;
        mcapi_assert(mcapi_trans_decode_handle(send_endpoint,&sd,&sn,&se));

        /* mark the endpoint as open */
        state = &mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].state;
        newstate = oldstate = *state;
        newstate.data.open = MCAPI_TRUE;
        status = MRAPI_SUCCESS;
        mrapi_atomic_cas(NULL,state,&newstate,&oldstate,NULL,
            sizeof(mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].state),&status);
        assert(MRAPI_SUCCESS == status);

        /* fill in the channel handle */
        *send_handle = mcapi_trans_encode_handle(sd,sn,se);

        /* has the channel been connected yet? */
        if ( mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.channel_type == MCAPI_SCL_CHAN){
          completed = MCAPI_TRUE;
        }

        mcapi_dprintf(2," mcapi_trans_sclchan_send_open_i (node_num=%u,port_num=%u) handle=0x%x",
                      mcapi_db->domains[sd].nodes[sn].state.data.node_num,
                      mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].state.data.port_num,
                      *send_handle);
      }

      mcapi_assert(setup_request(&send_endpoint,request,mcapi_status,completed,0,NULL,OPEN_SCLCHAN,0,0,0,r));
    } else{
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }
  }

  /***************************************************************************
  NAME:mcapi_trans_sclchan_send
  DESCRIPTION: sends a packet on a packet channel (blocking)
  PARAMETERS:
    send_handle - the send channel handle
    buffer - the buffer
    size - the size in bytes of the buffer
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_sclchan_send( mcapi_sclchan_send_hndl_t send_handle,
                                            uint64_t dataword,
                                            uint32_t size)
  {
    uint16_t sd,sn,se,rd,rn,re;
    mcapi_buffer_type bt;
    int rc = MCAPI_FALSE;
    mcapi_status_t status;

    mcapi_dprintf(1,"mcapi_trans_sclchan_send(0x%x,0x%x,%u);",send_handle,dataword,size);

    mcapi_assert(mcapi_trans_decode_handle(send_handle,&sd,&sn,&se));
    mcapi_assert(mcapi_trans_decode_handle(mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.state.data.recv_endpt,&rd,&rn,&re));
    mcapi_trans_endpoint_get_attribute(send_handle,MCAPI_ENDP_ATTR_BUFFER_TYPE,&bt,sizeof(bt),&status);
    mcapi_assert(MCAPI_SUCCESS == status);

    switch(bt) {
    case MCAPI_ENDP_ATTR_FIFO_BUFFER:
      rc = mcapi_trans_send (sd,sn,se,rd,rn,re,NULL,size,dataword);
      break;
    case MCAPI_ENDP_ATTR_STATE_BUFFER:
      rc = mcapi_trans_send_state (sd,sn,se,rd,rn,re,NULL,size,dataword);
      break;
    }

    return rc;
  }


  /***************************************************************************
  NAME:mcapi_trans_sclchan_recv
  DESCRIPTION: receives a packet on a packet channel (blocking)
  PARAMETERS:
    send_handle - the send channel handle
    buffer - the buffer
    received_size - the size in bytes of the buffer
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE (only returns MCAPI_FALSE if it couldn't get a buffer)
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_sclchan_recv( mcapi_sclchan_recv_hndl_t receive_handle,
                                            uint64_t *data,uint32_t size)
  {
    uint16_t rd,rn,re;
    mcapi_buffer_type bt;
    size_t received_size;
    int rc = MCAPI_FALSE;
    mcapi_status_t status;

    mcapi_dprintf(1,"uint64_t data;");
    mcapi_dprintf(1,"mcapi_trans_sclchan_send(0x%x,&data,%u);",receive_handle,size);

    mcapi_assert(mcapi_trans_decode_handle(receive_handle,&rd,&rn,&re));
    mcapi_trans_endpoint_get_attribute(receive_handle,MCAPI_ENDP_ATTR_BUFFER_TYPE,&bt,sizeof(bt),&status);
    mcapi_assert(MCAPI_SUCCESS == status);

    switch(bt) {
    case MCAPI_ENDP_ATTR_FIFO_BUFFER:
      if (mcapi_trans_recv (rd,rn,re,NULL,size,&received_size,MCAPI_TRUE,data) &&
          received_size == size) {
        rc = MCAPI_TRUE;
      }

      /* FIXME: (errata A2) if size != received_size then we shouldn't remove the item from the
         endpoints receive queue */
      break;
    case MCAPI_ENDP_ATTR_STATE_BUFFER:
      if (mcapi_trans_recv_state (rd,rn,re,NULL,size,&received_size,MCAPI_TRUE,data) &&
          received_size == size) {
        rc = MCAPI_TRUE;
      }
      break;
    }

    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_sclchan_available
  DESCRIPTION: counts the number of elements in the endpoint receive queue
    identified by the receive handle.
  PARAMETERS: receive_handle - the receive channel handle
  RETURN VALUE: the number of elements in the receive queue
  ***************************************************************************/
  mcapi_uint_t mcapi_trans_sclchan_available_i( mcapi_sclchan_recv_hndl_t receive_handle)
  {
    uint16_t rd,rn,re;
    int rc = MCAPI_FALSE;

    mcapi_dprintf(1,"mcapi_trans_sclchan_available_i(0x%x);",receive_handle);

    mcapi_assert(mcapi_trans_decode_handle(receive_handle,&rd,&rn,&re));
    rc = mcapi_trans_queue_elements(&mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue);

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_sclchan_recv_close_i
  DESCRIPTION: non-blocking close of the receiving end of the scalar channel
  PARAMETERS:
    receive_handle -
    request -
    mcapi_status -
  RETURN VALUE:none
  ***************************************************************************/
  void mcapi_trans_sclchan_recv_close_i( mcapi_sclchan_recv_hndl_t  recv_handle,
                                         mcapi_request_t* request,
                                         mcapi_status_t* mcapi_status)
  {
    uint16_t rd,rn,re;
    int r;

    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    mcapi_dprintf(1,"mcapi_trans_sclchan_recv_close_i(0x%x,&request,&status);",recv_handle);

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request(&r)) {
      if (!completed) {
        mcapi_assert(mcapi_trans_decode_handle(recv_handle,&rd,&rn,&re));
        mcapi_trans_close_channel (rd,rn,re);
        completed = MCAPI_TRUE;
      }
      mcapi_assert(setup_request(&recv_handle,request,mcapi_status,completed,0,NULL,
          (mcapi_request_type)CLOSE_SCLCHAN,0,0,0,r));
    } else{
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }
  }


  /***************************************************************************
  NAME:mcapi_trans_sclchan_send_close_i
  DESCRIPTION: non-blocking close of the sending end of the scalar channel
  PARAMETERS:
    send_handle -
    request -
    mcapi_status -
  RETURN VALUE:none
  ***************************************************************************/
  void mcapi_trans_sclchan_send_close_i( mcapi_sclchan_send_hndl_t send_handle,
                                         mcapi_request_t* request,
                                         mcapi_status_t* mcapi_status)
  {
    uint16_t sd,sn,se;
    int r;

    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    mcapi_dprintf(1,"mcapi_trans_sclchan_send_close_i(0x%x,&request,&status);",send_handle);

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request(&r)) {
      if (!completed) {
        mcapi_assert(mcapi_trans_decode_handle(send_handle,&sd,&sn,&se));
        mcapi_trans_close_channel (sd,sn,se);
        completed = MCAPI_TRUE;
      }
      mcapi_assert(setup_request(&send_handle,request,mcapi_status,completed,0,NULL,
          (mcapi_request_type)CLOSE_SCLCHAN,0,0,0,r));
    } else{
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }
  }
