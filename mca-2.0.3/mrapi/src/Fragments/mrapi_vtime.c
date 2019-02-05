/************************************************************************
mrapi_test

DESCRIPTION
mrapi_test() checks if a non-blocking operation has completed. 
 The function returns in a timely fashion.  request is the identifier 
for the non-blocking operation.  

RETURN VALUE
On success, MRAPI_TRUE is returned and *status is set to MRAPI_SUCCESS. 
 If the operation has not completed MRAPI_FALSE is returned and 
*status is set to MRAPI_INCOMPLETE.  On error, MRAPI_FALSE is 
returned and *status is se to the appropriate error defined 
below.

ERRORS
MRAPI_ERR_REQUEST_INVALID	Argument is not a valid request handle.
MRAPI_ERR_REQUEST_CANCELED	The request was canceled.

NOTE

***********************************************************************/
mrapi_boolean_t mrapi_test(
        MRAPI_IN mrapi_request_t* request,
        MRAPI_OUT size_t* size,
        MRAPI_OUT mrapi_status_t* status)
{

  *status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (!mrapi_impl_valid_request_hndl(request)) {
    *status = MRAPI_ERR_REQUEST_INVALID;
  } else  {
    return mrapi_impl_test(request,status);
  }
 return MRAPI_FALSE;
}

/************************************************************************
mrapi_wait

DESCRIPTION
mrapi_wait() waits until a non-blocking operation has completed. 
 It is a blocking function and returns when the operation has 
completed, has been canceled, or a timeout has occurred.  request 
is the identifier for the non-blocking operation.  


RETURN VALUE
On success, status is set to MRAPI_SUCCESS.  On error, *status is set to 
the appropriate error defined below.

ERRORS
MRAPI_ERR_REQUEST_INVALID Argument is not a valid request handle.
MRAPI_ERR_REQUEST_CANCELED	The request was canceled.
MRAPI_TIMEOUT	The operation timed out.

***********************************************************************/
mrapi_boolean_t mrapi_wait(
        MRAPI_IN mrapi_request_t* request,
        MRAPI_OUT size_t* size,
        MRAPI_IN mrapi_timeout_t timeout,
        MRAPI_OUT mrapi_status_t* status)
{
  unsigned i = 0;

  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else {
    while(i < timeout) {
      i++;
      if (!mrapi_impl_valid_request_hndl(request)) {
        *status = MRAPI_ERR_REQUEST_INVALID;
        return MRAPI_FALSE;
      } else if (mrapi_impl_canceled_request(request)) {
        *status = MRAPI_ERR_REQUEST_CANCELED;
        return MRAPI_FALSE;
      } else {
        if (mrapi_impl_test(request,status)) {
          *status = MRAPI_SUCCESS;
          return MRAPI_TRUE;
        }
      }
    }
    *status = MRAPI_TIMEOUT;
  }
 return MRAPI_FALSE;
}

/************************************************************************
mrapi_wait_any

DESCRIPTION
mrapi_wait_any() waits until any non-blocking operation of a 
list has completed.  It is a blocking function and returns the 
index into the requests array (starting from 0) indicating which 
of any outstanding operation has completed.  If more than one 
request has completed, it will return the first one it finds. 
 number is the number of requests in the array.  requests is 
the array of mrapi_request_t identifiers for the non-blocking 
operations.  

RETURN VALUE
On success, the index into the requests array of the mrapi_request_t 
identifier that has completed or has been canceled is returned 
and *status is set to MRAPI_SUCCESS.  On error, -1 is returned 
and *status is set to the appropriate error defined below.

ERRORS
MRAPI_ERR_REQUEST_INVALID Argument is not a valid request handle.
MRAPI_ERR_REQUEST_CANCELED	The request was canceled.
MRAPI_TIMEOUT	The operation timed out.
MRAPI_ERR_PARAMETER	Incorrect number (if=0) requests parameter.

NOTE

***********************************************************************/
mrapi_uint_t mrapi_wait_any(
        MRAPI_IN size_t number,
        MRAPI_IN mrapi_request_t* requests,
        MRAPI_OUT size_t* size,
        MRAPI_IN mrapi_timeout_t timeout,
        MRAPI_OUT mrapi_status_t* status)
{
  mrapi_uint_t i = 0;

  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else {
    *status = MRAPI_TIMEOUT;
    do {
      for (i = 0; i < number; i++) {
        if (!mrapi_impl_valid_request_hndl(&requests[i])) {
          *status = MRAPI_ERR_REQUEST_INVALID;
        } else if (mrapi_impl_canceled_request(&requests[i])) {
          *status = MRAPI_ERR_REQUEST_CANCELED;
        } else if (mrapi_impl_test(&requests[i],status)) {
          return i;
        }
      }
      i++;
    } while (i < timeout);
  }
  return MRAPI_RETURN_VALUE_INVALID;
}

/************************************************************************
mrapi_cancel

DESCRIPTION
mrapi_cancel() cancels an outstanding request.  Any pending calls 
to mrapi_wait() or mrapi_wait_any() for this request will also 
be cancelled. The returned status of a canceled mrapi_wait() 
or mrapi_wait_any() call will indicate that the request was cancelled. 
 Only the node that initiated the request may call cancel.

RETURN VALUE
On success, *status is set to MRAPI_SUCCESS.  On error, *status is set to 
the appropriate error defined below.

ERRORS
MRAPI_ERR_REQUEST_INVALID Argument is not a valid request handle for this node.

NOTE

***********************************************************************/
void mrapi_cancel(
        MRAPI_IN mrapi_request_t* request,
        MRAPI_OUT mrapi_status_t* mrapi_status)
{
  // In our implementation, requests are satisfied immediately and are thus
  // not cancellable.

  *mrapi_status = MRAPI_SUCCESS;
  if  (!mrapi_impl_initialized()) {
    *mrapi_status = MRAPI_ERR_NODE_NOTINIT;
  }
}
