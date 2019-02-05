  //////////////////////////////////////////////////////////////////////////////
  //                                                                          //
  //                   test and wait functions                                //
  //                                                                          //
  //////////////////////////////////////////////////////////////////////////////

  /***************************************************************************
  NAME:mcapi_trans_test_i
  DESCRIPTION: Tests if the request has completed yet (non-blocking).
        It is called mcapi_test at the mcapi level even though it's a non-blocking function.
  PARAMETERS:
    request -
    size -
    mcapi_status -
  RETURN VALUE: TRUE/FALSE indicating if the request has completed.
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_test_i( mcapi_request_t* request,
                                      size_t* size,
                                      mcapi_status_t* mcapi_status)
  {

    /* We return true if it's cancelled, invalid or completed.  We only return
       false if the user should continue polling.
    */
    mcapi_boolean_t rc = MCAPI_FALSE;
    uint16_t r;
    mcapi_requests* requests = &mcapi_rq;

    mcapi_dprintf(3,"mcapi_trans_test_i request handle:0x%lx",*request);

    if (!mcapi_trans_decode_request_handle(request,&r) ||
        (REQUEST_FREE == requests->data[r].state)) {
      *mcapi_status = MCAPI_ERR_REQUEST_INVALID;
      rc = MCAPI_TRUE;
    }
    else if (REQUEST_CANCELLED == requests->data[r].state) {
      requests->data[r].status = MCAPI_ERR_REQUEST_CANCELLED;
      rc = MCAPI_TRUE;
    }
    else {
      if (REQUEST_COMPLETED != requests->data[r].state) {
        /* try to complete the request */
        /*  receives to an empty channel or get_endpt for an endpt that
            doesn't yet exist are the only two types of non-blocking functions
            that don't complete immediately for this implementation */
        switch (requests->data[r].type) {
        case (SEND) :
          break;  /* always completes immediately */
        case (RECV) :
          check_receive_request (request); break;
        case (GET_ENDPT) :
          check_get_endpt_request (request);break;
        case (CONN_PKTCHAN) :
        case (CONN_SCLCHAN) :
          break;  /* always completes immediately */
        case (OPEN_PKTCHAN) :
        case (OPEN_SCLCHAN) :
          check_open_channel_request (request);
          break;
        case (CLOSE_PKTCHAN) :
        case (CLOSE_SCLCHAN) :
          check_close_channel_request (request);
          break;
        default:
          mcapi_assert(0);
          break;
        };
      }
    }

    /* query completed again because we may have just completed it */
    if (REQUEST_COMPLETED == requests->data[r].state ||
        REQUEST_CANCELLED == requests->data[r].state) {
      *size = requests->data[r].size;
      *mcapi_status = requests->data[r].status;
      *request=0;
      (void)mcapi_trans_remove_request(r);	/* by etem */
      rc = MCAPI_TRUE;
    }

    //mcapi_dprintf(2,"mcapi_trans_test_i returning rc=%u,status=%s",rc,mcapi_display_status(*mcapi_status));

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_wait
  DESCRIPTION:Tests if the request has completed yet (non-blocking).
  PARAMETERS:
    send_handle -
    request -
    mcapi_status -
  RETURN VALUE:  TRUE indicating the request has completed or FALSE
    indicating the request has been cancelled.
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_wait( mcapi_request_t* request,
                                    size_t* size,
                                    mcapi_status_t* mcapi_status,
                                    mcapi_timeout_t timeout)
  {
    mcapi_timeout_t time = 0;
    uint16_t r;

    if(0 == *request) {
      /* no active request */
      return MCAPI_TRUE;
    }

    mcapi_assert(mcapi_trans_decode_request_handle(request,&r));
    mcapi_dprintf(1,"mcapi_trans_wait(&request,&size,&status,%u);",timeout);
    while(1) {
      time++;
      if (mcapi_trans_test_i(request,size,mcapi_status)) {
        return MCAPI_TRUE;
      }
      /* yield */
      mcapi_dprintf(5,"mcapi_trans_wait - attempting to yield");
      /* we don't have the lock, it's safe to just yield */
      sys_os_yield();
      if ((timeout !=  MCA_INFINITE) && (time >= timeout)) {
        *mcapi_status = MCAPI_TIMEOUT;
        return MCAPI_FALSE;
      }
    }
  }

  /***************************************************************************
  NAME:mcapi_trans_wait_any
  DESCRIPTION:Tests if any of the requests have completed yet (blocking).
      Note: the request is now cleared if it has been completed or cancelled.
  PARAMETERS:
    send_handle -
    request -
    mcapi_status -
  RETURN VALUE:
  ***************************************************************************/
  unsigned mcapi_trans_wait_any(size_t number, mcapi_request_t** requests, size_t* size,
                                       mcapi_status_t* mcapi_status,
                                       mcapi_timeout_t timeout)
  {
    mcapi_timeout_t time = 0;
    int i;

    mcapi_dprintf(1,"mcapi_trans_wait_any");
    while(1) {
      time++;
      for (i = 0; i < (int)number; i++) {
        if (mcapi_trans_test_i(requests[i],size,mcapi_status)) {
          return i;
        }
        /* yield */
        mcapi_dprintf(5,"mcapi_trans_wait_any - attempting to yield");
        /* we don't have the lock, it's safe to just yield */
        sys_os_yield();
        if ((timeout !=  MCA_INFINITE) && (time >= timeout)) {
          *mcapi_status = MCAPI_TIMEOUT;
          return MCA_RETURN_VALUE_INVALID;
        }
      }
    }
  }

  /***************************************************************************
  NAME:mcapi_trans_cancel
  DESCRIPTION: Cancels the given request
  PARAMETERS:
    request -
    mcapi_status -
  RETURN VALUE:none
  ***************************************************************************/
  void mcapi_trans_cancel(mcapi_request_t* request,mcapi_status_t* mcapi_status)
  {
    uint16_t r;
    mcapi_request_data* req = NULL;
    mcapi_request_state oldstate;
    mcapi_request_state newstate;
    mcapi_requests* requests = &mcapi_rq;

    mcapi_dprintf(1,"mcapi_trans_cancel");

    mcapi_assert(mcapi_trans_decode_request_handle(request,&r));

    req = &requests->data[r];
    if (REQUEST_FREE == req->state) {
      *mcapi_status = MCAPI_ERR_REQUEST_INVALID;
    } else if (REQUEST_CANCELLED  == req->state) {
      /* this reqeust has already been cancelled */
      mcapi_dprintf(2,"mcapi_trans_cancel - request was already cancelled");
      *mcapi_status = MCAPI_ERR_REQUEST_CANCELLED;
    } else {
      mrapi_status_t status = MRAPI_SUCCESS;
      oldstate = REQUEST_VALID;
      newstate = REQUEST_CANCELLED;
      mrapi_atomic_cas(NULL,&req->state,&newstate,&oldstate,NULL,sizeof(req->state),&status);
      if(MRAPI_SUCCESS == status) {
        /* request is cancelled */
        switch (requests->data[r].type) {
        case (RECV) :
          cancel_receive_request (request); break;
        case (GET_ENDPT) :
          break;
        default:
          mcapi_assert(0);
          break;
        };
        *mcapi_status = MCAPI_SUCCESS;
      } else {
        /* it's too late, the request has already completed */
        mcapi_dprintf(2," mcapi_trans_cancel - Unable to cancel because request has already completed");
      }
    }
  }
