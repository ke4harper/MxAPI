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
  //                   mcapi_trans API: packet channels                       //
  //                                                                          //
  //////////////////////////////////////////////////////////////////////////////
  /***************************************************************************
  NAME:mcapi_trans_pktchan_connect_i
  DESCRIPTION: connects a packet channel
  PARAMETERS:
    send_endpoint - the sending endpoint handle
    receive_endpoint - the receiving endpoint handle
    request - the request to be filled in when the task is complete
    mcapi_status -
  RETURN VALUE:none
  ***************************************************************************/
  void mcapi_trans_pktchan_connect_i( mcapi_endpoint_t  send_endpoint,
                                      mcapi_endpoint_t  receive_endpoint,
                                      mcapi_request_t* request,
                                      mcapi_status_t* mcapi_status)
  {

    int r;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request_have_lock(&r)) {
      if (!completed) {
        mcapi_trans_connect_channel_have_lock (send_endpoint,receive_endpoint,MCAPI_PKT_CHAN);
        completed = MCAPI_TRUE;
      }
      mcapi_assert(setup_request_have_lock(&receive_endpoint,request,mcapi_status,completed,0,NULL,
          (mcapi_request_type)CONN_PKTCHAN,0,0,0,r));
    } else {
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }

  /***************************************************************************
  NAME: mcapi_trans_pktchan_recv_open_i
  DESCRIPTION: opens the receive endpoint on a packet channel
  PARAMETERS:
    recv_handle - the receive channel handle to be filled in
    receive_endpoint - the receiving endpoint handle
    request - the request to be filled in when the task is complete
    mcapi_status
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_pktchan_recv_open_i( mcapi_pktchan_recv_hndl_t* recv_handle,
                                        mcapi_endpoint_t receive_endpoint,
                                        mcapi_request_t* request,
                                        mcapi_status_t* mcapi_status)
  {
    int r;
    uint16_t rd,rn,re;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_dprintf(1,"mcapi_trans_pktchan_recv_open_i (recv_handle,0x%x,&request,&status);",receive_endpoint);

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request_have_lock(&r)) {
      if (!completed) {
        mcapi_assert(mcapi_trans_decode_handle(receive_endpoint,&rd,&rn,&re));

        mcapi_trans_open_channel_have_lock (rd,rn,re);

        /* fill in the channel handle */
        *recv_handle = mcapi_trans_encode_handle(rd,rn,re);


        /* has the channel been connected yet? */
        if ( mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.channel_type == MCAPI_PKT_CHAN) {
          completed = MCAPI_TRUE;
        }

        mcapi_dprintf(2,"mcapi_trans_pktchan_recv_open_i (node_num=%u,port_num=%u) handle=0x%x",
                      mcapi_db->domains[rd].nodes[rn].node_num,
                      mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].port_num,
                      *recv_handle);
      }

      mcapi_assert(setup_request_have_lock(&receive_endpoint,request,mcapi_status,completed,0,NULL,OPEN_PKTCHAN,0,0,0,r));
    } else {
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }


  /***************************************************************************
  NAME: mcapi_trans_pktchan_send_open_i
  DESCRIPTION: opens the send endpoint on a packet channel
  PARAMETERS:
    send_handle - the send channel handle to be filled in
    receive_endpoint - the receiving endpoint handle
    request - the request to be filled in when the task is complete
    mcapi_status
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_pktchan_send_open_i( mcapi_pktchan_send_hndl_t* send_handle,
                                        mcapi_endpoint_t send_endpoint,
                                        mcapi_request_t* request,
                                        mcapi_status_t* mcapi_status)
  {
    int r;
    uint16_t sd,sn,se;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_dprintf(1,"mcapi_trans_pktchan_send_open_i,send_handle,0x%x,&request,&status);",send_endpoint);

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request_have_lock(&r)) {
      if (!completed) {
        mcapi_assert(mcapi_trans_decode_handle(send_endpoint,&sd,&sn,&se));

        /* mark the endpoint as open */
        mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].open = MCAPI_TRUE;

        /* fill in the channel handle */
        *send_handle = mcapi_trans_encode_handle(sd,sn,se);

        /* has the channel been connected yet? */
        if ( mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.channel_type == MCAPI_PKT_CHAN) {
          completed = MCAPI_TRUE;
        }

        mcapi_dprintf(2," mcapi_trans_pktchan_send_open_i (node_num=%u,port_num=%u) handle=0x%x",
                      mcapi_db->domains[sd].nodes[sn].node_num,
                      mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].port_num,
                      *send_handle);
      }

      mcapi_assert(setup_request_have_lock(&send_endpoint,request,mcapi_status,completed,0,NULL,OPEN_PKTCHAN,0,0,0,r));
    } else {
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }

  /***************************************************************************
  NAME:mcapi_trans_pktchan_send_i
  DESCRIPTION: sends a packet on a packet channel (non-blocking)
  PARAMETERS:
    send_handle - the send channel handle
    buffer - the buffer
    size - the size in bytes of the buffer
    request - the request handle to be filled in when the task is complete
    mcapi_status -
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_pktchan_send_i( mcapi_pktchan_send_hndl_t send_handle,
                                   const void* buffer, size_t size,
                                   mcapi_request_t* request,
                                   mcapi_status_t* mcapi_status)
  {
    uint16_t sd,sn,se,rd,rn,re;
    int r;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    mcapi_dprintf(1,"mcapi_trans_pktchan_send_i(0x%x,buffer,%u,&request,&status);",send_handle,size);

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request_have_lock(&r)) {
      if (!completed) {
        mcapi_assert(mcapi_trans_decode_handle(send_handle,&sd,&sn,&se));
        mcapi_assert(mcapi_trans_decode_handle(mcapi_db->domains[sd].nodes[sn].node_d.endpoints[se].recv_queue.recv_endpt,&rd,&rn,&re));

        if (!mcapi_trans_send_have_lock (sd,sn,se,rd,rn,re,(char*)buffer,size,0)) {
          *mcapi_status = MCAPI_ERR_MEM_LIMIT;
        }
        completed = MCAPI_TRUE;
      }
      mcapi_assert(setup_request_have_lock(&send_handle,request,mcapi_status,completed,size,NULL,SEND,0,0,0,r));
      /* the non-blocking request succeeded, when they call test/wait they will see the status of the send */
      *mcapi_status = MCAPI_SUCCESS;
    } else{
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }

  /***************************************************************************
  NAME:mcapi_trans_pktchan_send
  DESCRIPTION: sends a packet on a packet channel (blocking)
  PARAMETERS:
    send_handle - the send channel handle
    buffer - the buffer
    size - the size in bytes of the buffer
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_pktchan_send( mcapi_pktchan_send_hndl_t send_handle,
                                            const void* buffer,
                                            size_t size)
  {
    mcapi_request_t request;
    mcapi_status_t status = MCAPI_SUCCESS;

    /* use non-blocking followed by wait */
    do {
      mcapi_trans_pktchan_send_i (send_handle,buffer,size,&request,&status);
    } while (status == MCAPI_ERR_REQUEST_LIMIT);

    mcapi_trans_wait (&request,&size,&status,MCA_INFINITE);

    if (status == MCAPI_SUCCESS) {
      return MCAPI_TRUE;
    }
    return MCAPI_FALSE;
  }


  /***************************************************************************
  NAME:mcapi_trans_pktchan_recv_i
  DESCRIPTION: receives a packet on a packet channel (non-blocking)
  PARAMETERS:
    receive_handle - the send channel handle
    buffer - a pointer to a pointer to a buffer
    request - the request handle to be filled in when the task is complete
    mcapi_status -
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_pktchan_recv_i( mcapi_pktchan_recv_hndl_t receive_handle,
                                   void** buffer,
                                   mcapi_request_t* request,
                                   mcapi_status_t* mcapi_status)
  {
    int r;
    uint16_t rd,rn,re;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    size_t size = MCAPI_MAX_PKT_SIZE;

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request_have_lock(&r)) {
      mcapi_dprintf(1,"mcapi_trans_pktchan_recv_i(0x%x,&buffer,&request,&status);",receive_handle,size);

      if (!completed) {
        mcapi_assert(mcapi_trans_decode_handle(receive_handle,&rd,&rn,&re));

        /* *buffer will be filled in with a ptr to an mcapi buffer */
        *buffer = NULL;
        if (mcapi_trans_recv_have_lock (rd,rn,re,buffer,MCAPI_MAX_PKT_SIZE,&size,MCAPI_FALSE,NULL)) {
          completed = MCAPI_TRUE;
        }
      }

      mcapi_assert(setup_request_have_lock(&receive_handle,request,mcapi_status,completed,size,buffer,RECV,0,0,0,r));
    } else{
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }

  /***************************************************************************
  NAME:mcapi_trans_pktchan_recv
  DESCRIPTION: receives a packet on a packet channel (blocking)
  PARAMETERS:
    send_handle - the send channel handle
    buffer - the buffer
    received_size - the size in bytes of the buffer
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE (only returns MCAPI_FALSE if it couldn't get a buffer)
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_pktchan_recv( mcapi_pktchan_recv_hndl_t receive_handle,
                                            void** buffer,
                                            size_t* received_size)
  {
    mcapi_request_t request;
    mcapi_status_t status = MCAPI_SUCCESS;

    /* use non-blocking followed by wait */
    do {
      mcapi_trans_pktchan_recv_i (receive_handle,buffer,&request,&status);
    } while (status == MCAPI_ERR_REQUEST_LIMIT);

    mcapi_trans_wait (&request,received_size,&status,MCA_INFINITE);

    if (status == MCAPI_SUCCESS) {
      return MCAPI_TRUE;
    }
    return MCAPI_FALSE;
  }

  /***************************************************************************
  NAME: mcapi_trans_pktchan_available
  DESCRIPTION: counts the number of elements in the endpoint receive queue
    identified by the receive handle.
  PARAMETERS: receive_handle - the receive channel handle
  RETURN VALUE: the number of elements in the receive queue
  ***************************************************************************/
  mcapi_uint_t mcapi_trans_pktchan_available( mcapi_pktchan_recv_hndl_t receive_handle)
  {
    uint16_t rd,rn,re;
    int rc = MCAPI_FALSE;

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_dprintf(1,"mcapi_trans_pktchan_available(0x%x);",receive_handle);

    mcapi_assert(mcapi_trans_decode_handle(receive_handle,&rd,&rn,&re));
    rc = mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.num_elements;

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_pktchan_free
  DESCRIPTION: frees the given buffer
  PARAMETERS: buffer pointer
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE indicating success or failure (buffer not found)
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_pktchan_free( void* buffer)
  {

    int rc = MCAPI_TRUE;
    buffer_entry* b_e;

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_dprintf(1,"mcapi_trans_pktchan_free(buffer);");
    /* optimization - just do pointer arithmetic on the buffer pointer to get
       the base address of the buffer_entry structure. */
#if (__unix__||__MINGW32__)
    // TODO: Confirm pktchan buffer free pointer arithmetic on 32- and 64-bit OS
    b_e = buffer-(sizeof(b_e->scalar)+sizeof(b_e->size)+sizeof(b_e->magic_num)+4);
#else
    // TODO: Confirm pktchan buffer free pointer arithmetic on 32-bit Windows OS
    b_e = (buffer_entry*)((char*)buffer-(sizeof(b_e->scalar)+sizeof(b_e->size)+sizeof(b_e->magic_num)));
#endif  /* !(__unix__||__MINGW32__) */
    if (b_e->magic_num == MAGIC_NUM) {
      memset(b_e,0,sizeof(buffer_entry));
    } else {
      /* didn't find the buffer */
      rc = MCAPI_FALSE;
    }

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));

    return rc;
  }


  /***************************************************************************
  NAME:mcapi_trans_pktchan_recv_close_i
  DESCRIPTION: non-blocking close of the receiving end of the packet channel
  PARAMETERS: receive_handle
  RETURN VALUE:none
  ***************************************************************************/
  void mcapi_trans_pktchan_recv_close_i( mcapi_pktchan_recv_hndl_t  receive_handle,
                                         mcapi_request_t* request, mcapi_status_t* mcapi_status)
  {
    int r;
    uint16_t rd,rn,re;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request_have_lock(&r)) {
      mcapi_dprintf(1,"mcapi_trans_pktchan_recv_close_i (0x%x,&request,&status);",receive_handle);

      if (!completed) {
        mcapi_assert(mcapi_trans_decode_handle(receive_handle,&rd,&rn,&re));
        mcapi_trans_close_channel_have_lock (rd,rn,re);
        completed = MCAPI_TRUE;
      }
      mcapi_assert(setup_request_have_lock(&receive_handle,request,mcapi_status,completed,0,NULL,
          (mcapi_request_type)CLOSE_PKTCHAN,0,0,0,r));
    } else {
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }


  /***************************************************************************
  NAME:mcapi_trans_pktchan_send_close_i
  DESCRIPTION: non-blocking close of the sending end of the packet channel
  PARAMETERS: receive_handle
  RETURN VALUE:none
  ***************************************************************************/
  void mcapi_trans_pktchan_send_close_i( mcapi_pktchan_send_hndl_t  send_handle,
                                         mcapi_request_t* request,mcapi_status_t* mcapi_status)
  {
    int r;
    uint16_t sd,sn,se;
    /* if errors were found at the mcapi layer, then the request is considered complete */
    mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;
    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    /* make sure we have a request entry */
    if ( mcapi_trans_reserve_request_have_lock(&r)) {
      mcapi_dprintf(1,"mcapi_trans_pktchan_send_close_i (0x%x,&request,&status);",send_handle);

      if (!completed) {
        mcapi_assert(mcapi_trans_decode_handle(send_handle,&sd,&sn,&se));
        mcapi_trans_close_channel_have_lock (sd,sn,se);
        completed = MCAPI_TRUE;
      }

      mcapi_assert(setup_request_have_lock(&send_handle,request,mcapi_status,completed,0,NULL,
          (mcapi_request_type)CLOSE_PKTCHAN,0,0,0,r));
    } else{
      *mcapi_status = MCAPI_ERR_REQUEST_LIMIT;
    }

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }
