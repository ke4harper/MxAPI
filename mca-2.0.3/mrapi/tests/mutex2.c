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

Copyright (c) 2012, ABB, Inc
All rights reserved.

Port to Windows: #if !(__unix||__MINGW32__), etc.

*/
#include <mrapi.h>

char status_buff[MRAPI_MAX_STATUS_SIZE];
#include <stdio.h>
#include <stdlib.h>
#if (__unix)
#include <pthread.h>
#endif  /* (__unix) */
#include <string.h>
#if (__unix)
#include <unistd.h>
#endif  /* (__unix) */

#ifndef NODE
#define NODE 1
#endif

#ifndef DOMAIN
#define DOMAIN 1
#endif

#define NUM_ITERATIONS 1024
#define LOCK_LIMIT 5

#define NUM_THREADS 2
#define WRONG(x) wrong(x,__LINE__);

void wrong(mca_status_t status,unsigned line)
{
  fprintf(stderr,"WRONG: line=%u status=%s",line,mrapi_display_status(status,status_buff,sizeof(status_buff)));
  mrapi_finalize(&status);
  fflush(stdout);
  exit(1);
}

typedef struct {
  mca_node_t tid;
} thread_data;

int x;

void *run_thread(void* tdata) {
  mca_status_t status;
  int mutexkey = 1;
  int i = 0;
  mrapi_info_t version;
  mrapi_mutex_hndl_t mutex;
  mrapi_parameters_t parms = 0;
  mrapi_key_t lock_key;
#if !(__MINGW32__)
  mrapi_boolean_t creator = MRAPI_TRUE;
#endif  /* !(__MINGW32__) */

  thread_data* t = (thread_data*)tdata;

  printf("calling init: %d \n",NODE+t->tid);
  mrapi_initialize(DOMAIN,NODE+t->tid,parms,&version,&status);
  if (status != MRAPI_SUCCESS) {  WRONG(status) }

  mutex=mrapi_mutex_create(mutexkey,NULL /*atttrs */,&status);
  if (status == MRAPI_ERR_MUTEX_EXISTS)
  {
    mutex=mrapi_mutex_get(mutexkey,&status);
#if !(__MINGW32__)
    creator = MRAPI_FALSE;
#endif  /* !(__MINGW32__) */
  }
  if (status != MRAPI_SUCCESS) { WRONG(status) }

  for (i = 0; i < NUM_ITERATIONS; i++) {
    mrapi_mutex_lock(mutex,&lock_key,MRAPI_TIMEOUT_INFINITE /*timeout*/, &status);
    if (status != MRAPI_SUCCESS) { WRONG(status) }
    x++;
    //printf("tid=%d, x=%d\n",t->tid,x);
    mrapi_mutex_unlock(mutex,&lock_key,&status);
    if (status != MRAPI_SUCCESS) { WRONG(status) }
#if !(__unix||__MINGW32__)
    SleepEx(0,0); // give other thread a chance to run
#endif  /* !(__unix||__MINGW32__) */
  }

  /* finalize will free up the mutex */
  mrapi_finalize(&status);
  if (status != MRAPI_SUCCESS) { WRONG(status) }

  return NULL;
}


int main () {
  int rc = 0;
  int i = 0;
  pthread_t threads[NUM_THREADS];
  thread_data tdata[NUM_THREADS];
#if !(__unix||__MINGW32__)
  DWORD tid[NUM_THREADS] = { 0 };
#endif  /* !(__unix||__MINGW32__) */


  mrapi_set_debug_level(0);


  tdata[0].tid = 0;
  tdata[1].tid = 1;

#if (__unix||__MINGW32__)
  rc += pthread_create(&threads[0], NULL, run_thread,(void*)&tdata[0]);
  rc += pthread_create(&threads[1], NULL, run_thread,(void*)&tdata[1]);

  
  for (i = 0; i < 2; i++) {
    pthread_join(threads[i],NULL);
  }
#else
  threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)run_thread, &tdata[0], CREATE_SUSPENDED, &tid[0]);
  threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)run_thread, &tdata[1], CREATE_SUSPENDED, &tid[1]);
  SetThreadAffinityMask(threads[0], 0x1);
  SetThreadAffinityMask(threads[1], 0x1);
  ResumeThread(threads[0]);
  ResumeThread(threads[1]);

  WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
#endif  /* !(__unix||__MINGW32__) */
  
  if (x != NUM_ITERATIONS*NUM_THREADS) { WRONG(0) }
  
  if (rc == 0) {
    printf("   Test PASSED\n");
  }
  
  return rc;
}
