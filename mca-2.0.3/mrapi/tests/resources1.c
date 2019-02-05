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
/* This tests makes sure we can manage the resource tree in a multi-node testcase.


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


#define NUM_THREADS 10
#define WRONG(x) wrong(x,__LINE__);

void wrong(mca_status_t status,unsigned line)
{
  fprintf(stderr,"WRONG: line=%u status=%s",line,mrapi_display_status(status,status_buff,sizeof(status_buff)));
  fflush(stdout);
  exit(1);
}

typedef struct {
  mca_node_t tid;
} thread_data;

int x;

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


/* Callbacks */
mrapi_boolean_t rollover = MRAPI_FALSE;
void l3cache_hits_rollover(void) {
  rollover = MRAPI_TRUE;
}

void *run_thread(void* tdata) {
  mca_status_t status;
  mrapi_info_t version;

  mrapi_parameters_t parms = 0;
  mrapi_resource_t     *caches_root;
  mrapi_rsrc_filter_t   filter;
  mrapi_resource_t     *l3cache;
  size_t                attr_size;
  uint32_t              cache_hits;

  thread_data* t = (thread_data*)tdata;
  printf("run thread: t=%d TID: %s\n", t->tid,print_tid()); 

  mrapi_initialize(DOMAIN,NODE+t->tid,parms,&version,&status);
  if (status != MRAPI_SUCCESS) { WRONG(status) }
 
  printf("t=%d mrapi_resources_get TID: %s\n",t->tid,print_tid()); 
  /* Get the caches in a resource tree */
  filter = MRAPI_RSRC_CACHE;
  caches_root = mrapi_resources_get(filter, &status);
  if (status != MRAPI_SUCCESS) { WRONG(status) }

  /* Select the L3 cache */
  l3cache = caches_root->children[0];

  printf("t=%d mrapi_dynamic_attribute_start TID: %s\n",t->tid,print_tid());
  /* Start, reset, get, and then stop the attribute 'cache_hits' for the L3 cache */
  mrapi_dynamic_attribute_start(l3cache, 1, &l3cache_hits_rollover, &status);
  if (status != MRAPI_SUCCESS) { WRONG(status) }


  printf("t=%d mrapi_dynamic_attribute_reset TID:%s\n",t->tid,print_tid());
  mrapi_dynamic_attribute_reset(l3cache, 1, &status);
  if (status != MRAPI_SUCCESS) { WRONG(status) }

  printf("t=%d mrapi_get_attribute TID: %s\n",t->tid,print_tid());
  attr_size = 4;
  mrapi_resource_get_attribute(l3cache, 1, (void *)&cache_hits, attr_size, &status);
  if (status != MRAPI_SUCCESS) { WRONG(status) }

  printf("t=%d mrapi_dynamic_attribute_stop TID: %s\n",t->tid,print_tid());
  mrapi_dynamic_attribute_stop(l3cache, 1, &status);
  if (status != MRAPI_SUCCESS) { WRONG(status) }

  printf("t=%d mrapi_resource_tree_free TID: %s\n",t->tid,print_tid());
  /* Free the caches resource tree */
  mrapi_resource_tree_free(&caches_root, &status);
  if (status != MRAPI_SUCCESS) { WRONG(status) }

  printf("t=%d mrapi_finalize TID:%s\n",t->tid,print_tid());
  mrapi_finalize(&status);
  if (status != MRAPI_SUCCESS) { WRONG(status) }

  printf("t=%d DONE TID: %s\n",t->tid,print_tid());
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

  for (i = 0; i < NUM_THREADS; i++) {
    tdata[i].tid = i;
  }
  
#if (__unix||__MINGW32__)
  for (i = 0; i < NUM_THREADS; i++) {
    rc += pthread_create(&threads[i], NULL, run_thread,(void*)&tdata[i]);
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
  
  if (rc == 0) {
    printf("   Test PASSED\n");
  }
  
  return rc;
}
