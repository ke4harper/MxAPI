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
    int i,qindex,index;
    uint16_t r;
    /*print the recv queue from head to tail*/
    printf("\n      recv_queue:");
    for (i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++) {
      /* walk the queue from the head to the tail */
      qindex = (q.head + i) % (MCAPI_MAX_QUEUE_ENTRIES);
      printf("\n          ----------------QINDEX: %i",qindex);
      if (q.head == qindex) { printf("\n           *** HEAD ***"); }
      if (q.tail == qindex) { printf("\n           *** TAIL ***"); }
      printf("\n          request:0x%lx",(long unsigned int)q.elements[qindex].request);
      if (q.elements[qindex].request) {
        r = q.elements[qindex].request;
        printf("\n             valid:%u",mcapi_db->requests[r].valid);
        printf("\n             size:%u",(int)mcapi_db->requests[r].size);
        switch (mcapi_db->requests[r].type) {
        case (OTHER): printf("\n             type:OTHER"); break;
        case (SEND): printf("\n             type:SEND"); break;
        case (RECV): printf("\n             type:RECV"); break;
        case (GET_ENDPT): printf("\n             type:GET_ENDPT"); break;
        default:  printf("\n             type:UNKNOWN!!!"); break;
        };
        printf("\n             buffer:[%s]",(char*)mcapi_db->requests[r].buffer);
        printf("\n             buffer_ptr:0x%lx",(long unsigned int)mcapi_db->requests[r].buffer_ptr);
        printf("\n             completed:%u",mcapi_db->requests[r].completed);
        printf("\n             cancelled:%u",mcapi_db->requests[r].cancelled);
        printf("\n             handle:0x%i",(int)mcapi_db->requests[r].handle);
        /*   printf("\n             status:%s",mcapi_display_status(mcapi_db->requests[r].status)); */
        printf("\n             status:%i",(int)mcapi_db->requests[r].status);
        printf("\n             endpoint:0x%lx",(long unsigned int)mcapi_db->requests[r].ep_endpoint);
      }
      printf("\n          invalid:%u",q.elements[qindex].invalid);

      printf("\n          b:0x%lx",(long unsigned int)q.elements[qindex].buff_index);
      if (q.elements[qindex].buff_index) {
        index = q.elements[qindex].buff_index &~ MCAPI_VALID_MASK;
        printf("\n             size:%u",(unsigned)mcapi_db->buffers[index].size);
        printf("\n             magic_num:%x",(unsigned)mcapi_db->buffers[index].magic_num);
        printf("\n             buff:[%s]",(char*)mcapi_db->buffers[index].buff);
      }
    }
  }

  /***************************************************************************
  NAME: push_queue
  DESCRIPTION: Returns the qindex that should be used for adding an element.
     Also updates the num_elements, and tail pointer.
  PARAMETERS: q - the queue pointer
  RETURN VALUE: the qindex to be used
  ***************************************************************************/
  int mcapi_trans_push_queue(queue* q)
  {
    int i;

    /* the database should be locked */
    assert(locked == 1);
    if ( (q->tail + 1) % MCAPI_MAX_QUEUE_ENTRIES == q->head) {
      /* mcapi_assert (q->num_elements ==  MCAPI_MAX_QUEUE_ENTRIES);*/
      mcapi_assert(!"push_queue called on full queue");
    }
    q->num_elements++;
    i = q->tail;
    q->tail = (q->tail + 1) % MCAPI_MAX_QUEUE_ENTRIES;
    mcapi_assert (q->head != q->tail);
    return i;
  }

  /***************************************************************************
  NAME: pop_queue
  DESCRIPTION: Returns the qindex that should be used for removing an element.
     Also updates the num_elements, and head pointer.
  PARAMETERS: q - the queue pointer
  RETURN VALUE: the qindex to be used
  ***************************************************************************/
  int mcapi_trans_pop_queue (queue* q)
  {
    int i,qindex;
    int x = 0;

    /* the database should be locked */
    assert(locked == 1);
    if (q->head == q->tail) {
      /*mcapi_assert (q->num_elements ==  0);*/
      mcapi_assert (!"pop_queue called on empty queue");
    }

    /* we can't just pop the first element off the head of the queue, because it
       may be reserved for an earlier recv call, we need to take the first element
       that doesn't already have a request associated with it.  This can fragment
       our queue. */
    for (i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++) {
      /* walk the queue from the head to the tail */
      qindex = (q->head + i) % (MCAPI_MAX_QUEUE_ENTRIES);
      if ((!q->elements[qindex].request) &&
          (q->elements[qindex].buff_index)){
        x = qindex;
        break;
      }
    }
    if (i == MCAPI_MAX_QUEUE_ENTRIES) {
      /* all of this endpoint's buffers already have requests associated with them */
      mcapi_assert(0); /* mcapi_trans_empty_queue should have already checked for this case */
    }

    q->num_elements--;

    /* if we are removing from the front of the queue, then move head */
    if (x == q->head) {
      q->head = (q->head + 1) % MCAPI_MAX_QUEUE_ENTRIES;
    } else {
      /* we are fragmenting the queue, mark this entry as invalid */
      q->elements[qindex].invalid = MCAPI_TRUE;
    }

    if (q->num_elements > 0) {
      if (q->head == q->tail) { printf("num_elements=%d\n",q->num_elements); }
      mcapi_assert (q->head != q->tail);
    }

    mcapi_trans_compact_queue (q);

    return x;
  }

  /***************************************************************************
  NAME: compact_queue
  DESCRIPTION: Attempts to compact the queue.  It can become fragmented based
     on the order that blocking/non-blocking sends/receives/tests come in
  PARAMETERS: q - the queue pointer
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_compact_queue (queue* q)
  {
    int i;
    int qindex;

    /* the database should be locked */
    assert(locked == 1);
    mcapi_dprintf(7,"before mcapi_trans_compact_queue head=%i,tail=%i,num_elements=%i",q->head,q->tail,q->num_elements);
    for (i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++) {
    qindex = (q->head + i) % (MCAPI_MAX_QUEUE_ENTRIES);
    if ((qindex == q->tail) ||
        (q->elements[qindex].request) ||
        (q->elements[qindex].buff_index )){
      break;
    } else {
      /* advance the head pointer */
      q->elements[qindex].invalid = MCAPI_FALSE;
      q->head = (q->head + 1) % MCAPI_MAX_QUEUE_ENTRIES;
      i--;
    }
    }
    mcapi_dprintf(7,"after mcapi_trans_compact_queue head=%i,tail=%i,num_elements=%i",q->head,q->tail,q->num_elements);
    if (q->num_elements > 0) {
      mcapi_assert (q->head != q->tail);
    }

  }

  /***************************************************************************
  NAME: mcapi_trans_empty_queue
  DESCRIPTION: Checks if the queue is empty or not
  PARAMETERS: q - the queue
  RETURN VALUE: true/false
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_empty_queue (queue q)
  {
    int i,qindex;

    /* the database should be locked */
    assert(locked == 1);

    if  (q.head == q.tail) {
      /* mcapi_assert (q.num_elements ==  0); */
      return MCAPI_TRUE;
    }

    /* if we have any buffers in our queue that don't have
       reservations, then our queue is non-empty */
    for (i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++) {
      qindex = (q.head + i) % (MCAPI_MAX_QUEUE_ENTRIES);
      if ((!q.elements[qindex].request) &&
          (q.elements[qindex].buff_index )){
        break;
      }
    }
    if (i == MCAPI_MAX_QUEUE_ENTRIES) {
      return MCAPI_TRUE;
    }

    return MCAPI_FALSE;
  }

  /***************************************************************************
  NAME: mcapi_trans_full_queue
  DESCRIPTION: Checks if the queue is full or not
  PARAMETERS: q - the queue
  RETURN VALUE: true/false
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_full_queue (queue q)
  {
    /* the database should be locked */
    assert(locked == 1);
    if ( (q.tail + 1) % MCAPI_MAX_QUEUE_ENTRIES == q.head) {
      /*  mcapi_assert (q.num_elements ==  (MCAPI_MAX_QUEUE_ENTRIES -1)); */
      return MCAPI_TRUE;
    }
    return MCAPI_FALSE;
  }
