  //////////////////////////////////////////////////////////////////////////////
  //                                                                          //
  //                   queue management                                       //
  //                                                                          //
  //////////////////////////////////////////////////////////////////////////////
  /*

  This is my least favorite section of code in our MRAPI implementation.  I'm sure
  there are queue libraries out there that are prettier than this but we can't use
  them.  Here's why.  Conceptually you can think of each endpoint as having a
  receive queue that messages/packets/scalars get put into.  The problem is that
  it's not really a queue because of complications due to handling non-blocking
  semantics.
  1) requests are cancellable, this leaves bubbles in the queue
  2) requests are satisfied in a fifo order however we have no idea when the
  user will call test/wait/cancel.  When the user calls test/wait/cancel we
  access the element directly (no longer fifo).
  */
  /***************************************************************************
  NAME: print_queue
  DESCRIPTION: Prints an endpoints receive queue (useful for debugging)
  PARAMETERS: q - the queue
  RETURN VALUE: none
  ***************************************************************************/
  void print_queue (queue q)
  {
#ifdef NOTUSED
    int i,qindex,index;
    uint16_t r;
    mcapi_requests* requests = &mcapi_rq;
    /*print the recv queue from head to tail*/
    printf("\n      recv_queue:");
    for (i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++) {
      /* walk the queue from the head to the tail */
      qindex = (q.head + i) % (MCAPI_MAX_QUEUE_ENTRIES);
      printf("\n          ----------------QINDEX: %i",qindex);
      if (q.head == qindex) { printf("\n           *** HEAD ***"); }
      if (q.tail == qindex) { printf("\n           *** TAIL ***"); }
      printf("\n          request:0x%lx",(long unsigned int)q.elements[qindex].request.reserve);
      if (q.elements[qindex].request.reserve) {
        r = q.elements[qindex].request.reserve;
        printf("\n             valid:%u",requests->data[r].valid);
        printf("\n             size:%u",(int)requests->data[r].size);
        switch (requests->data[r].type) {
        case (OTHER): printf("\n             type:OTHER"); break;
        case (SEND): printf("\n             type:SEND"); break;
        case (RECV): printf("\n             type:RECV"); break;
        case (GET_ENDPT): printf("\n             type:GET_ENDPT"); break;
        default:  printf("\n             type:UNKNOWN!!!"); break;
        };
        printf("\n             buffer:[%s]",(char*)requests->data[r].buffer);
        printf("\n             buffer_ptr:0x%lx",(long unsigned int)requests->data[r].buffer_ptr);
        printf("\n             completed:%u",requests->data[r].completed);
        printf("\n             cancelled:%u",requests->data[r].cancelled);
        printf("\n             handle:0x%i",(int)requests->data[r].handle);
        /*   printf("\n             status:%s",mcapi_display_status(mcapi_db->requests[r].status)); */
        printf("\n             status:%i",(int)requests->data[r].status);
        printf("\n             endpoint:0x%lx",(long unsigned int)requests->data[r].ep_endpoint);
      }
      printf("\n          invalid:%u",q.elements[qindex].request.invalid);

      printf("\n          b:0x%lx",(long unsigned int)q.elements[qindex].buff_index);
      if (q.elements[qindex].buff_index) {
        index = q.elements[qindex].buff_index &~ MCAPI_VALID_MASK;
        printf("\n             size:%u",(unsigned)mcapi_db->buffers[index].size);
        printf("\n             magic_num:%x",(unsigned)mcapi_db->buffers[index].magic_num);
        printf("\n             buff:[%s]",(char*)mcapi_db->buffers[index].buff);
      }
    }
#endif  /* NOTUSED */
  }

  /***************************************************************************
  NAME: queue_elements
  DESCRIPTION: Returns the current queue length
  PARAMETERS: q - the queue pointer
  RETURN VALUE: the number of elements
  ***************************************************************************/
  int mcapi_trans_queue_elements(queue* q)
  {
    unsigned tempAC = q->ack_counter;
    unsigned tempUC = q->update_counter;
    return (tempUC - tempAC)/2;
  }

  /***************************************************************************
  NAME: queue_head
  DESCRIPTION: Returns the next index that would be popped
  PARAMETERS: q - the queue pointer
              status - queue status
  RETURN VALUE: the entry index
  ***************************************************************************/
  int mcapi_trans_queue_head(queue* q,mcapi_status_t* status)
  {
    unsigned tempUC = q->update_counter;
    unsigned lastAC = 2*(q->ack_counter/2); /* last completed operation */
    if(tempUC == lastAC) {
      *status = MCAPI_ERR_QUEUE_EMPTY;
    }
    else if(tempUC - lastAC == 1) {
      *status = MCAPI_ERR_QUEUE_EMPTY_PRODUCER_INSERTING;
    }
    else {
      * status = MCAPI_SUCCESS;
    }
    return (lastAC/2)%MCAPI_MAX_QUEUE_ENTRIES;
  }

  /***************************************************************************
  NAME: queue_tail
  DESCRIPTION: Returns the next index that would be pushed
  PARAMETERS: q - the queue pointer
              status - queue status
  RETURN VALUE: the entry index
  ***************************************************************************/
  int mcapi_trans_queue_tail(queue* q,mcapi_status_t* status)
  {
    unsigned tempAC = q->ack_counter;
    unsigned lastUC = 2*(q->update_counter/2); /* last completed operation */
    signed diff = lastUC - tempAC;
    if(diff >= 2 * MCAPI_MAX_QUEUE_ENTRIES) {
      *status = MCAPI_ERR_QUEUE_FULL;
    }
    else if(diff == (2 * MCAPI_MAX_QUEUE_ENTRIES) - 1) {
      *status = MCAPI_ERR_QUEUE_FULL_CONSUMER_READING;
    }
    else {
      *status = MCAPI_SUCCESS;
    }
    return (lastUC/2)%MCAPI_MAX_QUEUE_ENTRIES;
  }

  /***************************************************************************
  NAME: push_queue
  DESCRIPTION: Returns the qindex that should be used for adding an element.
  PARAMETERS: q - the queue pointer
              status - queue status if full
  RETURN VALUE: the qindex to be used, or -1 if queue is full
  ***************************************************************************/
  int mcapi_trans_push_queue(queue* q,mcapi_status_t* status)
  {
    int x = -1;
    *status = MCAPI_SUCCESS;

    /* retry if multiple clients conflict */
    while(1) {
      unsigned tempAC;
      unsigned UC,lastUC,newUC;
      signed diff;
      mrapi_status_t mrapi_status;

      UC = q->update_counter;
      if(0 != UC % 2) {
        /* multiple clients, push in progress */
        sys_os_yield();
        continue;
      }
      lastUC = 2*(UC/2); /* last completed operation */
      tempAC = q->ack_counter;
      diff = lastUC - tempAC;

      /* counters incremented twice for each operation */
      if(diff >= 2 * MCAPI_MAX_QUEUE_ENTRIES) {
        *status = MCAPI_ERR_QUEUE_FULL;
        return -1;
      }
      if(diff == (2 * MCAPI_MAX_QUEUE_ENTRIES) - 1) {
        *status = MCAPI_ERR_QUEUE_FULL_CONSUMER_READING;
        return -1;
      }

      /* select queue entry */
      newUC = UC + 1;
      /* Producer is about to insert an item */
      mrapi_status = MRAPI_SUCCESS;
      mrapi_atomic_cas(NULL,&q->update_counter,&newUC,&UC,NULL,sizeof(q->update_counter),&mrapi_status);
      if(MRAPI_SUCCESS == mrapi_status) {
        /* original update counter value was not changed */
        x = (lastUC/2)%MCAPI_MAX_QUEUE_ENTRIES;
        /* Producer is finished inserting item */
        mrapi_atomic_inc(NULL,&q->update_counter,NULL,sizeof(q->update_counter),&mrapi_status);
        break;
      }
    }
    return x;
  }

  /***************************************************************************
  NAME: pop_queue
  DESCRIPTION: Returns the qindex that should be used for removing an element.
  PARAMETERS: q - the queue pointer
              status - queue status if empty
  RETURN VALUE: the qindex to be used, or -1 if the queue is empty.
  ***************************************************************************/
  int mcapi_trans_pop_queue (queue* q,mcapi_status_t* status)
  {
    int x = -1;
    *status = MCAPI_SUCCESS;

    /* retry if multiple clients conflict */
    while(1) {
      unsigned tempUC;
      unsigned AC,lastAC,newAC;
      mrapi_status_t mrapi_status;

      AC = q->ack_counter;
      if(0 != AC % 2) {
        /* multiple clients, pop in progress */
        sys_os_yield();
        continue;
      }
      lastAC = 2*(AC/2); /* last completed operation */
      tempUC = q->update_counter;

      if(tempUC == lastAC) {
        *status = MCAPI_ERR_QUEUE_EMPTY;
        return -1;
      }
      if(tempUC - lastAC == 1) {
        *status = MCAPI_ERR_QUEUE_EMPTY_PRODUCER_INSERTING;
        return -1;
      }

      /* select queue entry */
      newAC = AC + 1;
      /* Consumer is about to remove an item */
      mrapi_status = MRAPI_SUCCESS;
      mrapi_atomic_cas(NULL,&q->ack_counter,&newAC,&AC,NULL,sizeof(q->ack_counter),&mrapi_status);
      if(MRAPI_SUCCESS == mrapi_status) {
        /* original ack counter value was not changed */
        x = (lastAC/2)%MCAPI_MAX_QUEUE_ENTRIES;
        /* Consumer is finished removing item */
        mrapi_atomic_inc(NULL,&q->ack_counter,NULL,sizeof(q->ack_counter),&mrapi_status);
        break;
      }
    }
    return x;
  }

  /***************************************************************************
  NAME: mcapi_trans_queue_status
  DESCRIPTION: Checks queue status as if a push or pop was attempted
  PARAMETERS: q - the queue
  RETURN VALUE: queue status - 
    MCAPI_SUCCESS - queue is available for push or pop
    MCAPI_ERR_QUEUE_FULL - no room for push
    MCAPI_ERR_QUEUE_FULL_CONSUMER_READING - immediate retry push
    MCAPI_ERR_QUEUE_EMPTY - no entry available for pop
    MCAPI_ERR_QUEUE_EMPTY_PRODUCER_INSERTING - immediate retry pop
  ***************************************************************************/
  mcapi_status_t mcapi_trans_queue_status (queue* q)
  {
    unsigned tempAC;
    unsigned lastUC;
    unsigned tempUC;
    unsigned lastAC;
    signed diff;

    /* check write status */
    tempAC = q->ack_counter;
    lastUC = 2*(q->update_counter/2); /* last completed operation */
    diff = lastUC - tempAC;

    /* counters incremented twice for each operation */
    if(diff >= 2 * MCAPI_MAX_QUEUE_ENTRIES) {
      return MCAPI_ERR_QUEUE_FULL;
    }
    if(diff == (2 * MCAPI_MAX_QUEUE_ENTRIES) - 1) {
      return MCAPI_ERR_QUEUE_FULL_CONSUMER_READING;
    }

    /* check read status */
    tempUC = q->update_counter;
    lastAC = q->ack_counter; /* last completed operation */

    if(tempUC == lastAC) {
      return MCAPI_ERR_QUEUE_EMPTY;
    }
    if(tempUC - tempAC == 1) {
      return MCAPI_ERR_QUEUE_EMPTY_PRODUCER_INSERTING;
    }

    return MCAPI_SUCCESS;
  }
