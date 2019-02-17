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
  mcapi_trans_remove_request_have_lock
  DESCRIPTION: Removes request from array
  PARAMETERS:
    r - request index
  RETURN VALUE: TRUE/FALSE indicating if the request has removed.
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_remove_request_have_lock(int r) {	/* by etem */

  	int temp_empty_head_index;
    mcapi_boolean_t rc = MCAPI_FALSE;
  	indexed_array_header *header = &mcapi_db->request_reserves_header;

  	if (header->full_head_index != -1) {
  	  if (header->full_head_index == r) {
        /* if r is head of the full list, its next link becomes the new head */
  		header->full_head_index = header->array[header->full_head_index].next_index;
  	  }
      /* unlink r from full list, linking its neighbors together */
  	  if (header->array[r].next_index != -1) {
  		header->array[header->array[r].next_index].prev_index = header->array[r].prev_index;
  	  }
  	  if (header->array[r].prev_index != -1) {
  		header->array[header->array[r].prev_index].next_index = header->array[r].next_index;
  	  }
      /* link r as new head of available list */
  	  temp_empty_head_index = header->empty_head_index;
  	  header->empty_head_index = r;
  	  header->array[header->empty_head_index].next_index = temp_empty_head_index;
      header->array[header->empty_head_index].prev_index = -1;
  	  if (temp_empty_head_index != -1) {
        header->array[temp_empty_head_index].prev_index = header->empty_head_index;
      }

  	  header->curr_count--;
  	  rc = MCAPI_TRUE;
  	}
  	return rc; // if rc=false, then there is no request available (array is empty)
  }
  /***************************************************************************
  NAME: mcapi_trans_reserve_request
  DESCRIPTION: Reserves an entry in the requests array
  PARAMETERS: *r - request index pointer
  RETURN VALUE: T/F
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_reserve_request_have_lock(int *r) {	/* by etem */

  	int temp_full_head_index;
    mcapi_boolean_t rc = MCAPI_FALSE;

  	indexed_array_header *header = &mcapi_db->request_reserves_header;

  	if (header->empty_head_index != -1) {
  		*r = header->empty_head_index;
      mcapi_db->requests[*r].valid = MCAPI_TRUE;
  		temp_full_head_index = header->full_head_index;
  		header->full_head_index = header->empty_head_index;
  		header->empty_head_index = header->array[header->empty_head_index].next_index;
  		header->array[header->empty_head_index].prev_index = -1;
  		header->array[header->full_head_index].next_index = temp_full_head_index;
  		header->array[header->full_head_index].prev_index = -1;
  		if (temp_full_head_index != -1) {
  			header->array[temp_full_head_index].prev_index = header->full_head_index;
  		}
  		header->curr_count++;
  		rc = MCAPI_TRUE;
  	}
    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_init_indexed_array_have_lock
  DESCRIPTION: initializes indexed array
  PARAMETERS:
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_init_indexed_array_have_lock() {	/* by etem */
  	int i;

    mcapi_db->request_reserves_header.curr_count = 0;
		mcapi_db->request_reserves_header.max_count = MCAPI_MAX_REQUESTS;
		mcapi_db->request_reserves_header.empty_head_index = 0;
		mcapi_db->request_reserves_header.full_head_index = -1;
		for (i = 0; i < MCAPI_MAX_REQUESTS; i++) {
			mcapi_db->request_reserves_header.array[i].next_index = i + 1;
			mcapi_db->request_reserves_header.array[i].prev_index = i - 1;
		}
		mcapi_db->request_reserves_header.array[MCAPI_MAX_REQUESTS - 1].next_index = -1;
		mcapi_db->request_reserves_header.array[0].prev_index = -1;

  }

  /***************************************************************************
  NAME: setup_request_have_lock
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
  mcapi_boolean_t setup_request_have_lock (mcapi_endpoint_t* handle,
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
    int i,qindex;
    uint16_t d,n,e;
    mcapi_boolean_t rc = MCAPI_TRUE;

    /* the database should already be locked */
    assert(locked == 1);

    mcapi_db->requests[r].status = *mcapi_status;
    mcapi_db->requests[r].size = size;
    mcapi_db->requests[r].cancelled = MCAPI_FALSE;
    mcapi_db->requests[r].completed = completed;

    //encode the request handle (this is the only place in the code we do this)
    *request = 0x80000000 | r;
    mcapi_dprintf(1,"setup_request_have_lock handle=0x%x",*request);
    /* this is hacky, there's probably a better way to do this */
    if ((buffer != NULL) && (!completed)) {
      mcapi_assert(mcapi_trans_decode_handle(*handle,&d,&n,&e));
      if ( mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type == MCAPI_PKT_CHAN) {
        /* packet buffer means system buffer, so save the users pointer to the buffer */
        mcapi_db->requests[r].buffer_ptr = buffer;
      } else {
        /* message buffer means user buffer, so save the users buffer */
        mcapi_db->requests[r].buffer = *buffer;
      }
    }
    mcapi_db->requests[r].type = type;
    mcapi_db->requests[r].handle = *handle;

    /* save the pointer so that we can fill it in (the endpoint may not have been created yet)
       an alternative is to make buffer a void* and use it for everything (messages, endpoints, etc.) */
    if (  mcapi_db->requests[r].type == GET_ENDPT) {
      mcapi_db->requests[r].ep_endpoint = handle;
      mcapi_db->requests[r].ep_node_num = node_num;
      mcapi_db->requests[r].ep_port_num = port_num;
      mcapi_db->requests[r].ep_domain_num = domain_num;
    }

    /* if this was a non-blocking receive to an empty queue, then reserve the next buffer */
    if ((type == RECV) && (!completed)) {
      mcapi_assert(mcapi_trans_decode_handle(*handle,&d,&n,&e));
      /*find the queue entry that doesn't already have a request associated with it */
      for (i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++) {
        /* walk from head to tail */
        qindex = (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.head + i) % (MCAPI_MAX_QUEUE_ENTRIES);
        if ((mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.elements[qindex].request==0) &&
            (!mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.elements[qindex].invalid)) {
          mcapi_dprintf(4,"receive request r=%u reserving qindex=%i",i,qindex);
          mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.elements[qindex].request = *request;
          break;
        }
      }
      if (i == MCAPI_MAX_QUEUE_ENTRIES) {
        mcapi_dprintf(1,"setup_request_have_lock: MCAPI_ERR_MEM_LIMIT all of this endpoint's buffers already have requests associated with them.  Your receives are outpacing your sends.  Either throttle this at the application layer or reconfigure with a larger endpoint receive queue.");
        /* all of this endpoint's buffers already have requests associated with them */
        mcapi_db->requests[r].status = MCAPI_ERR_MEM_LIMIT;
        mcapi_db->requests[r].completed = MCAPI_TRUE;
      }
    }

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
  void check_open_channel_request_have_lock (mcapi_request_t *request)
  {
    uint16_t d,n,e,r;

    assert(locked == 1);
    if (mcapi_trans_decode_request_handle(request,&r)) {

      mcapi_assert(mcapi_trans_decode_handle(mcapi_db->requests[r].handle,&d,&n,&e));

      /* has the channel been connected yet? */
      if ( mcapi_db->domains[d].nodes[n].node_d.endpoints[e].connected == MCAPI_TRUE) {
        mcapi_db->requests[r].completed = MCAPI_TRUE;
      }
    }
  }

  /***************************************************************************
  NAME:check_close_channel_request
  DESCRIPTION: Checks if the endpoint has been disconnected yet.
  PARAMETERS: the request pointer (to be filled in)
  RETURN VALUE: none
  ***************************************************************************/
  void check_close_channel_request_have_lock (mcapi_request_t *request)
  {
    uint16_t d,n,e,r;

    assert(locked == 1);
    if (mcapi_trans_decode_request_handle(request,&r)) {

      mcapi_assert(mcapi_trans_decode_handle(mcapi_db->requests[r].handle,&d,&n,&e));

      /* has the channel been closed yet? */
      if ( mcapi_db->domains[d].nodes[n].node_d.endpoints[e].open == MCAPI_FALSE) {
        mcapi_db->requests[r].completed = MCAPI_TRUE;
      }
    }
  }

  /***************************************************************************
  NAME:check_get_endpt_request
  DESCRIPTION: Checks if the request to get an endpoint has been completed or not.
  PARAMETERS: the request pointer (to be filled in)
  RETURN VALUE: none
  ***************************************************************************/
  void check_get_endpt_request_have_lock (mcapi_request_t *request)
  {

    uint16_t r;

    assert(locked == 1);
    mcapi_assert(mcapi_trans_decode_request_handle(request,&r));
    if (mcapi_trans_endpoint_get_have_lock (mcapi_db->requests[r].ep_endpoint,
                                           mcapi_db->requests[r].ep_domain_num,
                                           mcapi_db->requests[r].ep_node_num,
                                           mcapi_db->requests[r].ep_port_num)) {
      mcapi_db->requests[r].completed = MCAPI_TRUE;
      mcapi_db->requests[r].status = MCAPI_SUCCESS;
    }

  }

  /***************************************************************************
  NAME: cancel_receive_request_have_lock
  DESCRIPTION: Cancels an outstanding receive request.  This is a little tricky
     because we have to preserve FIFO which means we have to shift all other
     outstanding receive requests down.
  PARAMETERS:
     request -
  RETURN VALUE: none
  ***************************************************************************/
  void cancel_receive_request_have_lock (mcapi_request_t *request)
  {
    uint16_t rd,rn,re,r;
    int i,last,start,curr;

    /* the database should already be locked */
    assert(locked == 1);
    mcapi_assert(mcapi_trans_decode_request_handle(request,&r));
    mcapi_assert(mcapi_trans_decode_handle(mcapi_db->requests[r].handle,&rd,&rn,&re));
    for (i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++) {
      if (mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[i].request == *request) {
        /* we found the request, now clear the reservation */
        mcapi_dprintf(5,"cancel_receive_request - cancelling request at index %i BEFORE:",i);
        //print_queue(mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue);
        mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[i].request = 0;
        break;
      }
    }

    /* we should have found the outstanding request */
    mcapi_assert (i != MCAPI_MAX_QUEUE_ENTRIES);

    /* shift all pending reservations down*/
    start = i;
    last = start;
    for (i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++) {
      curr = (i+start)%MCAPI_MAX_QUEUE_ENTRIES;
      /* don't cross over the head or the tail */
      if ((curr == mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.tail) &&
          (curr != start)) {
        break;
      }
      if ((curr == mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.head) &&
          (curr != start)) {
        break;
      }
      if (mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[curr].request) {
        mcapi_dprintf(5,"cancel_receive_request - shifting request at index %i to index %i",curr,last);
        mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[last].request =
          mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[curr].request;
        mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[curr].request = 0;
        last = curr;
      }
    }

    mcapi_db->requests[r].cancelled = MCAPI_TRUE;
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
  void check_receive_request_have_lock (mcapi_request_t *request)
  {
    uint16_t rd,rn,re,r;
    int i;
    int32_t index=-1;
    size_t size;

    /* the database should already be locked */
    assert (locked == 1);

    mcapi_assert(mcapi_trans_decode_request_handle(request,&r));
    mcapi_assert(mcapi_trans_decode_handle(mcapi_db->requests[r].handle,&rd,&rn,&re));
    for (i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++) {
      if (mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[i].request == *request) {
        /* we found the request, check to see if there is valid data in the receive queue entry */
        if (mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[i].buff_index ) {
          /* clear the request reservation */
          mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[i].request = 0;
          /* shared memory is zeroed, so we store our index as index+1 so that we can tell if it's valid or not*/
          index = mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[i].buff_index &~ MCAPI_VALID_MASK;
#if (__MINGW32__)||(__unix__)
          index = index; // compiler warning
#endif  /* (__MINGW32__)||(__unix__) */
          /* update the request */
          mcapi_db->requests[r].completed = MCAPI_TRUE;
          mcapi_db->requests[r].status = MCAPI_SUCCESS;
          /* first take the entry out of the queue  this has the potential to fragment our
             receive queue since we may not be removing from the head */
          if ( mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.channel_type == MCAPI_PKT_CHAN) {
            /* packet buffer means system buffer, so save the users pointer to the buffer */
            mcapi_trans_recv_have_lock_ (rd,rn,re,mcapi_db->requests[r].buffer_ptr,mcapi_db->requests[r].size,&mcapi_db->requests[r].size,i,NULL);
          } else {
            /* message buffer means user buffer, so save the users buffer */
            size = mcapi_db->requests[r].size;
            mcapi_trans_recv_have_lock_ (rd,rn,re,&mcapi_db->requests[r].buffer,mcapi_db->requests[r].size,&mcapi_db->requests[r].size,i,NULL);
            if (mcapi_db->requests[r].size > size) {
              mcapi_db->requests[r].size = size;
              mcapi_db->requests[r].status = MCAPI_ERR_MSG_TRUNCATED;
            }
          }
          /* now update the receive queue state */
          mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.num_elements--;
          /* mark this entry as invalid so that the "bubble" won't be re-used */
          mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.elements[i].invalid = MCAPI_TRUE;
          mcapi_trans_compact_queue (&mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue);
          mcapi_dprintf(4,"receive request (test/wait) popped from qindex=%i, num_elements=%i, head=%i, tail=%i",
                        i,mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.num_elements,
                        mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.head,
                        mcapi_db->domains[rd].nodes[rn].node_d.endpoints[re].recv_queue.tail);
        }
        break;
      }
    }
    /* we should have found the outstanding request */
    mcapi_assert (i != MCAPI_MAX_QUEUE_ENTRIES);
  }
