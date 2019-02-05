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

#ifndef _H_MTHREAD
#define _H_MTHREAD

#include <mrapi.h>
#include <mrapi_sys.h>

/* TODO - how to make this type defined different for bare metal */
typedef pthread_t mthread_t;


/* error codes */
typedef enum {
  MTHREAD_ERROR,
  MTHREAD_EINIT,        /* already initialized */
  MTHREAD_ENOINIT,      /* not initialized     */
  MTHREAD_EMAXTHRD,     /* at max thread count */
  MTHREAD_SUCCESS
} mthread_status_t;

typedef void* (*mthread_f_ptr_t) (void* data);

extern mthread_status_t mthread_initialize(void);

extern mthread_status_t mthread_create(mthread_t* thread, 
                                       mthread_f_ptr_t body, 
                                       void* data);

extern mthread_status_t mthread_yield(void);

extern mthread_status_t mthread_wait(mthread_t thread, 
                                     void** data);

/* currently not allowing an mthread to call this */
/* extern void mthread_exit(void* return_data); */

extern mthread_status_t mthread_finalize(void);

extern char* mthread_strerr(mthread_status_t status);

#endif

#ifdef __cplusplus
extern } 
#endif /* __cplusplus */
