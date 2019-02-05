/*
Copyright (c) 2010, The Multicore Association
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

(1) Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

(2) Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

(3) Neither the name of the Multicore Association nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <mthread.h>
#include <mca_config.h>
#if (__unix__)
#include <sched.h>
#include <stdio.h>
#endif  /* (__unix__) */

static int mthread_initialized = 0;

/*****************************************************************************
 *  mthread_initialize
 *****************************************************************************/
mthread_status_t mthread_initialize(void)
{
  mthread_status_t ret_val = MTHREAD_SUCCESS;

  if (!mthread_initialized) {

    mthread_initialized = 1;

  } else {

    ret_val = MTHREAD_EINIT;

  }

  return ret_val;
}

/*****************************************************************************
 *  mthread_create
 *****************************************************************************/
mthread_status_t mthread_create(mthread_t* thread,
                                mthread_f_ptr_t body, 
                                void* data)
{
  mthread_status_t ret_val = MTHREAD_SUCCESS;

  if (!mthread_initialized) {

    ret_val = MTHREAD_ENOINIT;

#if (__unix__)
  } else if (pthread_create(thread, NULL, body, data) != 0) {
    ret_val = MTHREAD_ERROR;
  }
#else  /* !(__unix__) */
  } else {
    DWORD tid;
    *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)body, data, 0, &tid);
    if(NULL == *thread) {
      ret_val = MTHREAD_ERROR;
    }
  }
#endif  /* !(__unix__) */
  
  return ret_val;
}


/*****************************************************************************
 *  mthread_yield
 *****************************************************************************/
mthread_status_t mthread_yield(void)
{
  mthread_status_t ret_val = MTHREAD_SUCCESS;

  if (!mthread_initialized) {
    
    ret_val = MTHREAD_ENOINIT;
    
  } else {
    sys_os_yield();
  }
  
  return ret_val;
}

/*****************************************************************************
 *  mthread_wait
 *****************************************************************************/
mthread_status_t mthread_wait(mthread_t thread, 
                              void** data)
{
  mthread_status_t ret_val = MTHREAD_SUCCESS;
#if (__unix__)
  int errcode;
#endif  /* (__unix__) */

  if (!mthread_initialized) {
    
    ret_val = MTHREAD_ENOINIT;
    
#if (__unix__)
  } else if ((errcode = pthread_join(thread, data)) != 0) {
    ret_val = MTHREAD_ERROR;
  }
#else  /* !(__unix__) */
  } else {
    WaitForSingleObject(thread,INFINITE);
  }
#endif  /* !(__unix__) */
  
  return ret_val;
}


/* I think we want to hide exit and force people to call MTAPI's exit */
/* void mthread_exit(void* return_data) */
/* { */
/*   pthread_exit(return_data); */
/* } */


/*****************************************************************************
 *  mthread_finalize
 *****************************************************************************/
mthread_status_t mthread_finalize(void)
{
  mthread_status_t ret_val = MTHREAD_SUCCESS;

  if (mthread_initialized) {

    mthread_initialized = 0;

  } else {

    ret_val = MTHREAD_ENOINIT;
  }

  return ret_val;
}


/*****************************************************************************
 *  mthread_strerr
 *****************************************************************************/
char* mthread_strerr(mthread_status_t status)
{
  char* ret_val = NULL;

  switch (status) {
  case MTHREAD_ERROR:
    ret_val = "general error";
    break;
  case MTHREAD_EINIT:       /* already initialized */
    ret_val = "already initialized";
    break;
  case MTHREAD_ENOINIT:      /* not initialized     */
    ret_val = "not initialized";
    break;
  case MTHREAD_EMAXTHRD:     /* at max thread count */
    ret_val = "at max thread count";
    break;
  case MTHREAD_SUCCESS:
    ret_val = "success";
    break;
  default:
    ret_val = "UNKNOWN ERROR";
    break;
  }

  return ret_val;
}


#ifdef __cplusplus
extern } 
#endif /* __cplusplus */
