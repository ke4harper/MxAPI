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

    mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE);

    mcapi_dprintf(3,"mcapi_trans_test_i request handle:0x%lx",*request);

    if (!mcapi_trans_decode_request_handle(request,&r) ||
        (mcapi_db->requests[r].valid == MCAPI_FALSE)) {
      *mcapi_status = MCAPI_ERR_REQUEST_INVALID;
      rc = MCAPI_TRUE;
    } else if (mcapi_db->requests[r].cancelled) {
      *mcapi_status = MCAPI_ERR_REQUEST_CANCELLED;
      rc = MCAPI_TRUE;
    } else {
      if (!(mcapi_db->requests[r].completed)) {
      /* try to complete the request */
      /*  receives to an empty channel or get_endpt for an endpt that
          doesn't yet exist are the only two types of non-blocking functions
          that don't complete immediately for this implementation */
      switch (mcapi_db->requests[r].type) {
      case (SEND) :
        break;  /* always completes immediately */
      case (RECV) :
        check_receive_request_have_lock (request); break;
      case (GET_ENDPT) :
        check_get_endpt_request_have_lock (request);break;
      case (CONN_PKTCHAN) :
      case (CONN_SCLCHAN) :
        break;  /* always completes immediately */
      case (OPEN_PKTCHAN) :
      case (OPEN_SCLCHAN) :
        check_open_channel_request_have_lock (request);
        break;
      case (CLOSE_PKTCHAN) :
      case (CLOSE_SCLCHAN) :
        check_close_channel_request_have_lock (request);
        break;
      default:
        mcapi_assert(0);
        break;
      };
    }

    /* query completed again because we may have just completed it */
    if (mcapi_db->requests[r].completed) {
      mcapi_trans_remove_request_have_lock(r);	/* by etem */
      *size = mcapi_db->requests[r].size;
      *mcapi_status = mcapi_db->requests[r].status;
      /* clear the entry so that it can be reused */
      memset(&mcapi_db->requests[r],0,sizeof(mcapi_request_data));
      *request=0;
      rc = MCAPI_TRUE;
    }
   }

    //mcapi_dprintf(2,"mcapi_trans_test_i returning rc=%u,status=%s",rc,mcapi_display_status(*mcapi_status));
    mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE);

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
#if (__unix__||__MINGW32__)
      sched_yield();
#else
      SleepEx(0,0);
#endif  /* !(__unix__||__MINGW32__) */
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
#if (__unix__||__MINGW32__)
        sched_yield();
#else
        SleepEx(0,0);
#endif  /* !(__unix__||__MINGW32__) */
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

    mcapi_dprintf(1,"mcapi_trans_cancel");

    /* lock the database */
    mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE);

    mcapi_assert(mcapi_trans_decode_request_handle(request,&r));

    if (mcapi_db->requests[r].valid == MCAPI_FALSE) {
      *mcapi_status = MCAPI_ERR_REQUEST_INVALID;
    } else if (mcapi_db->requests[r].cancelled) {
      /* this reqeust has already been cancelled */
      mcapi_dprintf(2,"mcapi_trans_cancel - request was already cancelled");
      *mcapi_status = MCAPI_ERR_REQUEST_CANCELLED;
    } else if (!(mcapi_db->requests[r].completed)) {
      /* cancel the request */
      mcapi_db->requests[r].cancelled = MCAPI_TRUE;
      switch (mcapi_db->requests[r].type) {
      case (RECV) :
        cancel_receive_request_have_lock (request); break;
      case (GET_ENDPT) :
        break;
      default:
        mcapi_assert(0);
        break;
      };
      /* clear the request so that it can be re-used */
      memset(&mcapi_db->requests[r],0,sizeof(mcapi_request_data));
      *mcapi_status = MCAPI_SUCCESS;
      /* invalidate the request handle */
      //*request = 0;
    } else {
      /* it's too late, the request has already completed */
      mcapi_dprintf(2," mcapi_trans_cancel - Unable to cancel because request has already completed");
    }

    /* unlock the database */
    mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE);
  }
