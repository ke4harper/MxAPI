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
/*
 *   This tests makes sure we can manage the resource tree in a multi-node testcase.
 */

#include <mrapi.h>

char status_buff[MRAPI_MAX_STATUS_SIZE];
#include <stdio.h>
#include <stdlib.h>
#if (__unix)
#include <unistd.h>
#include <pthread.h>
#endif  /* (__unix) */
#include <string.h>

#ifndef NODE
#define NODE 1
#endif

#ifndef DOMAIN
#define DOMAIN 1 
#endif

#define NUM_THREADS 2


typedef struct {
  mca_node_t tid;
  mrapi_boolean_t test_event;
  mrapi_boolean_t buffer_nearly_full;
} thread_data;

/* global but indexed by tid so no race conditions should exist */
thread_data tdata[NUM_THREADS];


#define WRONG_STATUS(x) wrong_status(x,__LINE__);
void wrong_status(mca_status_t status,unsigned line)
{
  fprintf(stderr,"WRONG: line=%u status=%s\n", line, mrapi_display_status(status,status_buff,sizeof(status_buff)));
  fflush(stdout);
  exit(1);
}
#define WRONG wrong(__LINE__);
void wrong(unsigned line)
{
  fprintf(stderr,"WRONG: line=%u\n", line);
  fflush(stdout);
  exit(1);
}

void crossbar_buffer_near_full(mrapi_event_t event) {
  mca_status_t status;
  mrapi_node_t node_id;
  
  node_id = mrapi_node_id_get(&status);
  printf("CALLBACK1: crossbar_buffer_near_full callback, node id %d\n", node_id); 
  if (node_id != 1) {
    mrapi_finalize(&status);
    printf("  FAIL: Incorrect node called this callback\n"); 
    exit(1);
    // or pthread_exit?
  }
  tdata[node_id-1].buffer_nearly_full = MRAPI_TRUE;
}

void test_callback(mrapi_event_t event) {
  mca_status_t status;
  mrapi_node_t node_id;
  
  node_id = mrapi_node_id_get(&status);
   printf("CALLBACK2: test_callback callback, node id %d\n", node_id); 
   if (node_id != 2) {
     mrapi_finalize(&status);
     printf("  FAIL: Incorrect node called this callback\n"); 
     exit(1);
     // or pthread_exit?
   }
   tdata[node_id-1].test_event = MRAPI_TRUE;
}


const char *print_tid() {
  static char buffer[100];
  char *p = buffer;
#if (__unix||__MINGW32__)
  pthread_t t = pthread_self();
#else
  pthread_t t = (pthread_t)GetCurrentThread();
#endif  /* !(__unix||__MINGW32__) */
#ifdef __linux
  /* We know that pthread_t is an unsigned long */
  sprintf(p, "%lu", t);
#else
  /* Just print out the contents of the pthread_t */ {
    char *const tend = (char *) ((&t)+1);
    char *tp = (char *) &t;
    while (tp < tend) {
#if (__unix||__MINGW32__)
      p += sprintf (p, "%02x", *tp);
#else
      size_t remain = (size_t)(100-(p-buffer));
      p += sprintf_s (p, remain, "%02x", *tp);
#endif  /* !(__unix||__MINGW32__) */
      tp++;
      if (tp < tend)
        *p++ = ':';
    }
  }
#endif
  return buffer;
}

void *run_thread(void* threaddata) {
  mrapi_info_t          version;
  mrapi_parameters_t    parms = 0;
  int i;
  mca_status_t mrapi_status;
  mrapi_event_t crossbar_event = 0;
  unsigned int crossbar_frequency = 0;

  thread_data* t = (thread_data*)threaddata;
  printf("run thread: t=%d TID: %s\n", t->tid, print_tid()); 

  mrapi_initialize(DOMAIN,NODE+t->tid,parms,&version,&mrapi_status);
  if (mrapi_status != MRAPI_SUCCESS) { WRONG_STATUS(mrapi_status) }

  /* Test the callback mechanism */
  crossbar_event = MRAPI_EVENT_CROSSBAR_BUFFER_OVER_80PERCENT;
  crossbar_frequency = 3;
  if (t->tid % 2 == 0) {
    mrapi_resource_register_callback(crossbar_event, crossbar_frequency,
				     &crossbar_buffer_near_full, &mrapi_status);
    if (mrapi_status != MRAPI_SUCCESS) { WRONG_STATUS(mrapi_status) }
    if (tdata[t->tid-1].buffer_nearly_full != MRAPI_FALSE) { WRONG }
    printf("registering crossbar_buffer_near_full\n");

  } else {
    mrapi_resource_register_callback(crossbar_event, crossbar_frequency,
				     &test_callback, &mrapi_status);
    if (mrapi_status != MRAPI_SUCCESS) { WRONG_STATUS(mrapi_status) }
    if (tdata[t->tid-1].test_event != MRAPI_FALSE) { WRONG }
    printf("registering test_callback\n");
  }

  /* Sleep several times to allow the 3 alarms to trigger the callback */
  for (i = 0; i < 20; i++) {
#if (__unix)
    sleep(1);
#else
    SleepEx(1000,0);
#endif  /*  !(__unix) */
  }

  printf("t=%d mrapi_finalize TID:%s\n",t->tid,print_tid());
  mrapi_finalize(&mrapi_status);
  if (mrapi_status != MRAPI_SUCCESS) { WRONG_STATUS(mrapi_status) }

  printf("t=%d DONE TID: %s\n",t->tid,print_tid());
  return NULL;
}

int main () {
  int rc = 0;
  int i = 0;
  int bnf = 0;
  int te = 0;
  pthread_t threads[NUM_THREADS];
#if !(__unix||__MINGW32__)
  DWORD tid[NUM_THREADS] = { 0 };
#endif  /* !(__unix||__MINGW32__) */


  mrapi_set_debug_level(6);

  for (i = 0; i < NUM_THREADS; i++) {
    tdata[i].tid = i;
    tdata[i].test_event = MRAPI_FALSE;
    tdata[i].buffer_nearly_full = MRAPI_FALSE;
  }

#if (__unix||__MINGW32__)
  for (i = 0; i < NUM_THREADS; i++) {
    rc += pthread_create(&threads[i], NULL, run_thread, (void*)&tdata[i]);
  }

  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i],NULL);
  }
#else
  for (i = 0; i < NUM_THREADS; i++) { 
    threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)run_thread, &tdata[i], CREATE_SUSPENDED, &tid[i]);
    SetThreadAffinityMask(threads[i], 0x1);
  }
  for (i = 0; i < NUM_THREADS; i++) { 
    ResumeThread(threads[i]);
  }
  WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
#endif  /* !(__unix||__MINGW32__) */

  /* Since the alarm in not tied to a specific node, any of the threads may */
  /* have had their callbacks invoked.  Check at least one of the callbacks */
  /* were invoked */
  for (i = 0; i < NUM_THREADS; i++) {
    if (tdata[i].buffer_nearly_full == MRAPI_TRUE) {bnf++;}
    if (tdata[i].test_event == MRAPI_TRUE) {te++;}
  }
  if ((bnf == 0) || (te == 0)) {
    rc = 1;
  }

  if (rc == 0) {
    printf("   Test PASSED\n");
  } else {
    printf("   Test FAILED bnf=%d te=%d\n",bnf,te);
  }

  return rc;
}
