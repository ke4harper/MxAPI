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

  /****************************************************************************
  mcapi_trans_request_get_index
  DESCRIPTION: Randomly select and reserve request index from bit set
  PARAMETERS: None
  RETURN VALUE: request_index or (uint16_t)-1 if none available
  ***************************************************************************/
  uint16_t mcapi_trans_request_get_index() {
    indexed_array_header *header = &mcapi_rq.reserves_header;
    uint16_t r = (uint16_t)-1;
    int partition;
    int bit;
    mrapi_status_t status;
    while(header->max_count > header->curr_count) {
      uint16_t test_r = ((uint16_t)((float)(MCAPI_MAX_REQUESTS-1) * sys_os_rand() / MCAPI_RAND_MAX));
      partition = test_r / (8 * sizeof(uint32_t));
      if(~(uint32_t)0 != header->set[partition]) {
        bit = test_r % (8 * sizeof(uint32_t));
        status = MRAPI_SUCCESS;
        mrapi_atomic_set(NULL,&header->set[partition],bit,NULL,sizeof(uint32_t),&status);
        if(MRAPI_SUCCESS == status) {
          r = test_r;
          break;
        }
        sys_os_yield();
      }
    }
    return r;
  }

  /****************************************************************************
  mcapi_trans_request_release_index
  DESCRIPTION: Release request index from bit set
  PARAMETERS:
    r - request index
  RETURN VALUE: MCAPI_TRUE if bit was set, MCAPI_FALSE otherwise
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_request_release_index(uint16_t r) {
    indexed_array_header *header = &mcapi_rq.reserves_header;
    int partition = r / (8 * sizeof(uint32_t));
    int bit = r % (8 * sizeof(uint32_t));
    mrapi_status_t status;
    mrapi_atomic_clear(NULL,&header->set[partition],bit,NULL,sizeof(uint32_t),&status);
    return(MRAPI_SUCCESS == status);
  }

  /****************************************************************************
  mcapi_trans_remove_request
  DESCRIPTION: Removes request from array
  PARAMETERS:
    r - request index
  RETURN VALUE: TRUE/FALSE indicating if the request has removed.
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_remove_request(int r) {
    indexed_array_header *header = &mcapi_rq.reserves_header;
    mcapi_request_data* req = &mcapi_rq.data[r];
    mcapi_request_state oldstate;
    mcapi_request_state newstate;
    mrapi_status_t status;

    /* Make request available */
    if(mcapi_trans_request_release_index(r)) {
      mrapi_atomic_dec(NULL,&header->curr_count,NULL,sizeof(header->curr_count),&status);
    }

    /* Mark the request as available */
    status = MRAPI_SUCCESS;
    mcapi_rq.data[r].ep_qindex = -1;
    oldstate = REQUEST_COMPLETED;
    newstate = REQUEST_FREE;
    mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
    if(MRAPI_SUCCESS != status) {
      status = MRAPI_SUCCESS;
      oldstate = REQUEST_CANCELLED;
      mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
    }
    assert(MRAPI_SUCCESS == status);

    return MCAPI_TRUE;
  }

  /***************************************************************************
  NAME: mcapi_trans_reserve_request
  DESCRIPTION: Reserves an entry in the requests array
  PARAMETERS: *r - request index pointer
  RETURN VALUE: T/F
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_reserve_request(int *r) {
    mcapi_boolean_t rc = MCAPI_FALSE;
    indexed_array_header *header = &mcapi_rq.reserves_header;
    mcapi_request_data* req = NULL;
    mcapi_request_state oldstate;
    mcapi_request_state newstate;
    mrapi_status_t status;

    /* Find available request */
    while(1) {
      if((uint16_t)-1 == (*r = mcapi_trans_request_get_index())) {
        mcapi_dprintf(1,"reserve_request: MCAPI_ERR_MEM_LIMIT all of the target's endpoint's buffers already have requests associated with them. \
                        Your sends are outpacing your receives.  Either throttle this at the application layer or reconfigure with a larger endpoint receive queue.");
        rc = MCAPI_FALSE;
        break;
      }
      req = &mcapi_rq.data[*r];

      oldstate = REQUEST_FREE;
      newstate = REQUEST_VALID;
      status = MRAPI_SUCCESS;
      mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
      if(MRAPI_SUCCESS == status) {
        mrapi_atomic_inc(NULL,&header->curr_count,NULL,sizeof(header->curr_count),&status);
        rc = MCAPI_TRUE;
        break;
      }
      sys_os_yield();
    }

  	return rc; // if rc=false, then there is no request available (array is empty)
  }

  /***************************************************************************
  NAME: mcapi_trans_init_indexed_array
  DESCRIPTION: initializes indexed array
  PARAMETERS:
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_init_indexed_array() {	/* by etem */
  	int i;
    mcapi_boolean_t oldinit = MCAPI_FALSE;
    mcapi_boolean_t newinit = MCAPI_TRUE;
    mcapi_requests* requests = &mcapi_rq;
    indexed_array_header* header = &requests->reserves_header;
    mrapi_status_t status = MRAPI_SUCCESS;

    mrapi_atomic_cas(NULL,&requests->initialized,&newinit,&oldinit,NULL,sizeof(requests->initialized),&status);
    if(MRAPI_SUCCESS != status) {
      /* already initialized for this process */
      return;
    }

    header->curr_count = 0;
    header->max_count = MCAPI_MAX_REQUESTS;
    for(i = 0; i < MCAPI_MAX_REQUEST_PARTITIONS; i++) {
      header->set[i] = 0;
    }
    for(i = 0; i < MCAPI_MAX_REQUESTS; i++) {
      requests->data[i].ep_qindex = -1;
    }

    /* seed random number generation */
    sys_os_srand((unsigned int) mcapi_tid);
  }

  /***************************************************************************
  NAME: setup_request
  DESCRIPTION: Sets up the request for a non-blocking function.
  PARAMETERS:
     handle -
     request -
     mcapi_status -
     completed - whether the request has already been completed or not (usually
       it has - receives to an empty queue or endpoint_get for endpoints that
       don't yet exist are the two main cases where completed will be false)
     size -
     buffer - the buffer
     type - the type of the request
  RETURN VALUE:
  ***************************************************************************/
  mcapi_boolean_t setup_request (mcapi_endpoint_t* handle,
                                 mcapi_request_t* request,
                                 mcapi_status_t* mcapi_status,
                                 mcapi_boolean_t completed,
                                 size_t size,void** buffer,
                                 mcapi_request_type type,
                                 mcapi_uint_t node_num,
                                 mcapi_uint_t port_num,
                                 mcapi_domain_t domain_num,
                                 int r)
  {
    uint16_t d,n,e;
    mcapi_boolean_t rc = MCAPI_TRUE;
    mcapi_requests* requests = &mcapi_rq;
    mcapi_request_data* req = &requests->data[r];
    mcapi_request_state oldstate;
    mcapi_request_state newstate;
    mrapi_status_t status;

    req->status = *mcapi_status;
    req->size = size;
    if(completed) {
      oldstate = REQUEST_VALID;
      newstate = REQUEST_COMPLETED;
      status = MRAPI_SUCCESS;
      mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
      assert(MRAPI_SUCCESS == status);
    }

    //encode the request handle (this is the only place in the code we do this)
    *request = 0x80000000 | r;
    mcapi_dprintf(1,"setup_request handle=0x%x",*request);
    /* this is hacky, there's probably a better way to do this */
    if ((buffer != NULL) && (!completed)) {
      mcapi_assert(mcapi_trans_decode_handle(*handle,&d,&n,&e));
      if ( mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type == MCAPI_PKT_CHAN) {
        /* packet buffer means system buffer, so save the users pointer to the buffer */
        req->buffer_ptr = buffer;
      } else {
        /* message buffer means user buffer, so save the users buffer */
        req->buffer = *buffer;
      }
    }
    req->type = type;
    req->handle = *handle;

    /* save the pointer so that we can fill it in (the endpoint may not have been created yet)
       an alternative is to make buffer a void* and use it for everything (messages, endpoints, etc.) */
    if (req->type == GET_ENDPT) {
      req->ep_endpoint = handle;
      req->ep_node_num = node_num;
      req->ep_port_num = port_num;
      req->ep_domain_num = domain_num;
    }

    /* if this was a non-blocking receive to an empty queue, then reserve the next buffer */
    if ((type == RECV) && (!completed)) {
      int start,i;
      queue* q = NULL;
      buffer_descriptor* buf = NULL;
      mcapi_boolean_t oldres;
      mcapi_boolean_t newres;
      mcapi_status_t cstatus;

      mcapi_assert(mcapi_trans_decode_handle(*handle,&d,&n,&e));
      q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue;

      /* find the queue entry that doesn't already have a request associated with it */
      /* walk from head to tail */
      start = mcapi_trans_queue_head(q,&cstatus);
      for (i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++) {
        /* find next entry that will be filled */
        int qindex = (start + i) % (MCAPI_MAX_QUEUE_ENTRIES);
        status = MRAPI_SUCCESS;
        buf = &q->elements[qindex];
        oldres = MCAPI_FALSE;
        newres = MCAPI_TRUE;
        mrapi_atomic_cas(NULL,&buf->reserved,&newres,&oldres,NULL,sizeof(buf->reserved),&status);
        if(MRAPI_SUCCESS == status) {
          mcapi_dprintf(4,"receive request r=%u reserving qindex=%i",r,qindex);
          requests->data[r].ep_qindex = qindex;
          break;
        }
      }
      if(i == MCAPI_MAX_QUEUE_ENTRIES) {
        mcapi_request_state oldstate;
        mcapi_request_state newstate;
        mrapi_status_t status;
        /* all of this endpoint's buffers already have requests associated with them */
        req->status = MCAPI_ERR_MEM_LIMIT;
        oldstate = REQUEST_VALID;
        newstate = REQUEST_COMPLETED;
        status = MRAPI_SUCCESS;
        mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
        assert(MRAPI_SUCCESS == status);
        mcapi_dprintf(1,"setup_request: MCAPI_ERR_MEM_LIMIT all of this endpoint's buffers already have requests associated with them. \
Your receives are outpacing your sends.  Either throttle this at the application layer or reconfigure with a larger endpoint receive queue.");
        return rc;
      }
    }

    return rc;
  }

  /***************************************************************************
  NAME: setup_request_state
  DESCRIPTION: Sets up the request for a non-blocking function performing
   state message data exchange.
  PARAMETERS:
     handle -
     request -
     mcapi_status -
     completed - whether the request has already been completed or not (usually
       it has - receives to an empty queue or endpoint_get for endpoints that
       don't yet exist are the two main cases where completed will be false)
     size -
     buffer - the buffer
     type - the type of the request
  RETURN VALUE:
  ***************************************************************************/
  mcapi_boolean_t setup_request_state (mcapi_endpoint_t* handle,
                                       mcapi_request_t* request,
                                       mcapi_status_t* mcapi_status,
                                       mcapi_boolean_t completed,
                                       size_t size,void** buffer,
                                       mcapi_request_type type,
                                       mcapi_uint_t node_num,
                                       mcapi_uint_t port_num,
                                       mcapi_domain_t domain_num,
                                       int r)
  {
    uint16_t d,n,e;
    mcapi_boolean_t rc = MCAPI_TRUE;
    mcapi_requests* requests = &mcapi_rq;
    mcapi_request_data* req = &requests->data[r];
    mcapi_request_state oldstate;
    mcapi_request_state newstate;
    mrapi_status_t status;

    req->status = *mcapi_status;
    req->size = size;
    if(completed) {
      oldstate = REQUEST_VALID;
      newstate = REQUEST_COMPLETED;
      status = MRAPI_SUCCESS;
      mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
      assert(MRAPI_SUCCESS == status);
    }

    //encode the request handle (this is the only place in the code we do this)
    *request = 0x80000000 | r;
    mcapi_dprintf(1,"setup_request_state handle=0x%x",*request);
    /* this is hacky, there's probably a better way to do this */
    if ((buffer != NULL) && (!completed)) {
      mcapi_assert(mcapi_trans_decode_handle(*handle,&d,&n,&e));
      if ( mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type == MCAPI_PKT_CHAN) {
        /* packet buffer means system buffer, so save the users pointer to the buffer */
        req->buffer_ptr = buffer;
      } else {
        /* message buffer means user buffer, so save the users buffer */
        req->buffer = *buffer;
      }
    }
    req->type = type;
    req->handle = *handle;

    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_decode_request_handle
  DESCRIPTION:
  PARAMETER:
  RETURN VALUE:
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_decode_request_handle(mcapi_request_t* request,uint16_t* r)
  {
    *r = *request;
    if ((*r < MCAPI_MAX_REQUESTS) && (*request & 0x80000000)) {
      return MCAPI_TRUE;
    }
    return MCAPI_FALSE;
  }

  /***************************************************************************
  NAME:check_open_channel_request
  DESCRIPTION: Checks if the endpoint has been connected yet.
  PARAMETERS: the request pointer (to be filled in)
  RETURN VALUE: none
  ***************************************************************************/
  void check_open_channel_request (mcapi_request_t *request)
  {
    uint16_t d,n,e,r;
    mcapi_requests* requests = &mcapi_rq;

    if (mcapi_trans_decode_request_handle(request,&r)) {

      mcapi_assert(mcapi_trans_decode_handle(requests->data[r].handle,&d,&n,&e));

      /* has the channel been connected yet? */
      if ( mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.connected == MCAPI_TRUE) {
        mcapi_request_data* req = &requests->data[r];
        mcapi_request_state oldstate;
        mcapi_request_state newstate;
        mrapi_status_t status;
        oldstate = REQUEST_VALID;
        newstate = REQUEST_COMPLETED;
        status = MRAPI_SUCCESS;
        mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
        assert(MRAPI_SUCCESS == status);
      }
    }
  }

  /***************************************************************************
  NAME:check_close_channel_request
  DESCRIPTION: Checks if the endpoint has been disconnected yet.
  PARAMETERS: the request pointer (to be filled in)
  RETURN VALUE: none
  ***************************************************************************/
  void check_close_channel_request (mcapi_request_t *request)
  {
    uint16_t d,n,e,r;
    mcapi_requests* requests = &mcapi_rq;

    if (mcapi_trans_decode_request_handle(request,&r)) {

      mcapi_assert(mcapi_trans_decode_handle(requests->data[r].handle,&d,&n,&e));

      /* has the channel been closed yet? */
      if ( mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.open == MCAPI_FALSE) {
        mcapi_request_data* req = &requests->data[r];
        mcapi_request_state oldstate;
        mcapi_request_state newstate;
        mrapi_status_t status;
        oldstate = REQUEST_VALID;
        newstate = REQUEST_COMPLETED;
        status = MRAPI_SUCCESS;
        mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
        assert(MRAPI_SUCCESS == status);
      }
    }
  }

  /***************************************************************************
  NAME:check_get_endpt_request
  DESCRIPTION: Checks if the request to get an endpoint has been completed or not.
  PARAMETERS: the request pointer (to be filled in)
  RETURN VALUE: none
  ***************************************************************************/
  void check_get_endpt_request (mcapi_request_t *request)
  {

    uint16_t r;
    mcapi_requests* requests = &mcapi_rq;

    mcapi_assert(mcapi_trans_decode_request_handle(request,&r));
    if (mcapi_trans_endpoint_get_(requests->data[r].ep_endpoint,
                                 requests->data[r].ep_domain_num,
                                 requests->data[r].ep_node_num,
                                 requests->data[r].ep_port_num)) {
      mcapi_request_data* req = &requests->data[r];
      mcapi_request_state oldstate;
      mcapi_request_state newstate;
      mrapi_status_t status;
      status = MRAPI_SUCCESS;
      oldstate = REQUEST_VALID;
      newstate = REQUEST_COMPLETED;
      mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
      assert(MRAPI_SUCCESS == status);
      requests->data[r].status = MCAPI_SUCCESS;
    }

  }

  /***************************************************************************
  NAME: cancel_receive_request
  DESCRIPTION: Cancels an outstanding receive request.  This is a little tricky
     because we have to preserve FIFO which means we have to shift all other
     outstanding receive requests down.
  PARAMETERS:
     request -
  RETURN VALUE: none
  ***************************************************************************/
  void cancel_receive_request (mcapi_request_t *request)
  {
    uint16_t rd,rn,re,r;
    int qindex = -1;
    mcapi_requests* requests = &mcapi_rq;

    mcapi_assert(mcapi_trans_decode_request_handle(request,&r));
    mcapi_assert(mcapi_trans_decode_handle(requests->data[r].handle,&rd,&rn,&re));
    if(-1 != (qindex = requests->data[r].ep_qindex)) {
      /* we found the request, now clear the reservation */
      mcapi_dprintf(5,"cancel_receive_request - cancelling request at index %i BEFORE:",qindex);
      //print_queue(*q);
      requests->data[r].buffer = NULL;
      requests->data[r].ep_qindex = -1;
    }
  }

  /***************************************************************************
  NAME: check_receive_request_
  DESCRIPTION: Checks if the given non-blocking receive request has completed.
     This is a little tricky because we can't just pop from the head of the
     endpoints receive queue.  We have to locate the reservation that was
     made in the queue (to preserve FIFO) at the time the request was made.
  PARAMETERS: the request pointer (to be filled in
  RETURN VALUE: none
  ***************************************************************************/
  void check_receive_request_ (mcapi_request_t *request)
  {
    uint16_t rd,rn,re,r;
    int qindex,front;
    int32_t index = -1;
    int32_t buff_index;
    size_t size;
    queue* q = NULL;
    buffer_descriptor* buf = NULL;
    mcapi_buffer_state oldbstate;
    mcapi_buffer_state newbstate;
    mrapi_status_t status;
    mcapi_request_data* req = NULL;
    mcapi_request_state oldstate;
    mcapi_request_state newstate;
    mcapi_status_t mcapi_status;
    mcapi_requests* requests = &mcapi_rq;

    mcapi_assert(mcapi_trans_decode_request_handle(request,&r));
    req = &requests->data[r];
    mcapi_assert(mcapi_trans_decode_handle(req->handle,&rd,&rn,&re));

    q = &mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue;

    /* Confirm the request is at the front of the queue */
    qindex = req->ep_qindex;
    mcapi_dprintf(4,"check_receive_request - checking request at index %i BEFORE:",qindex);
    front = mcapi_trans_queue_head(q,&mcapi_status);
    if(MCAPI_SUCCESS != mcapi_status ||
        front != qindex) {
      return;
    }

    /* Confirm the entry has been sent */
    buf = &q->elements[qindex];
    mrapi_atomic_read(NULL,&buf->buff_index,&buff_index,sizeof(buf->buff_index),&status);
    assert(MRAPI_SUCCESS == status);
    if(BUFFER_ALLOCATED != buf->state ||
        0 == buff_index) {
      return;
    }

    /* mark entry as received */
    status = MRAPI_SUCCESS;
    oldbstate = BUFFER_ALLOCATED;
    newbstate = BUFFER_RECEIVED;
    mrapi_atomic_cas(NULL,&buf->state,&newbstate,&oldbstate,NULL,sizeof(buf->state),&status);
    assert(MRAPI_SUCCESS == status);

    /* shared memory is zeroed, so we store our index as index+1 so that we can tell if it's valid or not*/
    index = buff_index &~ MCAPI_VALID_MASK;
#if (__MINGW32__)||(__unix__)
    index = index; // compiler warning
#endif  /* (__MINGW32__)||(__unix__) */

    /* mark request as received */
    oldstate = REQUEST_VALID;
    newstate = REQUEST_RECEIVED;
    status = MRAPI_SUCCESS;
    mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
    assert(MRAPI_SUCCESS == status);

    req->status = MCAPI_SUCCESS;
    if ( q->channel_type == MCAPI_PKT_CHAN) {
      /* packet buffer means system buffer, so save the users pointer to the buffer */
      mcapi_trans_recv_ (rd,rn,re,req->buffer_ptr,req->size,&req->size,qindex,NULL);
    } else {
      /* message buffer means user buffer, so save the users buffer */
      size = req->size;
      mcapi_trans_recv_ (rd,rn,re,&req->buffer,req->size,&req->size,qindex,NULL);
      if (req->size > size) {
        req->size = size;
        req->status = MCAPI_ERR_MSG_TRUNCATED;
      }
    }

    /* mark request as completed */
    oldstate = REQUEST_RECEIVED;
    newstate = REQUEST_COMPLETED;
    status = MRAPI_SUCCESS;
    mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
    assert(MRAPI_SUCCESS == status);

    /* remove item from queue */
    assert(-1 != (qindex = mcapi_trans_pop_queue(q,&mcapi_status)));
  }

  /***************************************************************************
  NAME: check_receive_request_state_
  DESCRIPTION: Checks if the given non-blocking receive state request has
   completed.
  PARAMETERS: the request pointer (to be filled in
  RETURN VALUE: none
  ***************************************************************************/
  void check_receive_request_state_ (mcapi_request_t *request)
  {
    uint16_t rd,rn,re,r;
    int qindex;
    unsigned counter;
    unsigned update_counter;
    int32_t buff_index;
    size_t size;
    attribute_entry_t* ae = NULL;
    queue* q = NULL;
    buffer_descriptor* buf = NULL;
    mcapi_buffer_state prevbstate;
    mcapi_buffer_state oldbstate;
    mcapi_buffer_state newbstate;
    mrapi_status_t status;
    mcapi_request_data* req = NULL;
    mcapi_request_state newstate;
    mcapi_requests* requests = &mcapi_rq;

    mcapi_assert(mcapi_trans_decode_request_handle(request,&r));
    req = &requests->data[r];
    mcapi_assert(mcapi_trans_decode_handle(req->handle,&rd,&rn,&re));
    ae = mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].attributes.entries;

    q = &mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue;

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
        break;
      }

      req->status = MCAPI_SUCCESS;
      if ( q->channel_type == MCAPI_PKT_CHAN) {
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

        /* packet buffer means system buffer, so save the users pointer to the buffer */
        mcapi_trans_recv_state_ (rd,rn,re,req->buffer_ptr,req->size,&req->size,qindex,NULL);
      } else {
        /* message buffer means user buffer, so save the users buffer */
        size = req->size;
        mcapi_trans_recv_state_ (rd,rn,re,&req->buffer,req->size,&req->size,qindex,NULL);
        if (req->size > size) {
          req->size = size;
          req->status = MCAPI_ERR_MSG_TRUNCATED;
        }
      }

      /* confirm successful read */
      mrapi_atomic_read(NULL,&q->update_counter,&update_counter,sizeof(q->update_counter),&status);
      assert(MRAPI_SUCCESS == status);
      if((update_counter-(counter/2)*2) <= (2*ae[MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS].attribute_d.value)-2) {

        mcapi_dprintf(4,"check_receive_request: %u byte buffer from qindex=%i, num_elements=%i buffer=[",
                      req->size,qindex,
                      mcapi_trans_queue_elements(q));

        /* mark request as completed */
        newstate = REQUEST_COMPLETED;
        status = MRAPI_SUCCESS;
        mrapi_atomic_xchg(NULL,&req->state,&newstate,NULL,sizeof(req->state),&status);
        assert(MRAPI_SUCCESS == status);
        break;
      }
    }
  }

  /***************************************************************************
  NAME: check_receive_request
  DESCRIPTION: Checks if the given non-blocking receive request has completed.
     This is a little tricky because we can't just pop from the head of the
     endpoints receive queue.  We have to locate the reservation that was
     made in the queue (to preserve FIFO) at the time the request was made.
  PARAMETERS: the request pointer (to be filled in
  RETURN VALUE: none
  ***************************************************************************/
  void check_receive_request (mcapi_request_t *request)
  {
    uint16_t r;
    mcapi_buffer_type bt;
    mcapi_requests* requests = &mcapi_rq;
    mcapi_status_t status;

    mcapi_trans_decode_request_handle(request,&r);
    mcapi_trans_endpoint_get_attribute(requests->data[r].handle,MCAPI_ENDP_ATTR_BUFFER_TYPE,&bt,sizeof(bt),&status);
    mcapi_assert(MCAPI_SUCCESS == status);

    switch(bt) {
    case MCAPI_ENDP_ATTR_FIFO_BUFFER:
      check_receive_request_ (request); break;
    case MCAPI_ENDP_ATTR_STATE_BUFFER:
      check_receive_request_state_ (request); break;
    }
  }
