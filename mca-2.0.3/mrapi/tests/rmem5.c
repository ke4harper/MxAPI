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

/* this test is specifically written for two threads */
#define NUM_THREADS 2

#define NUM_MSGS 50

#define MSG_SIZE 32
#define WRONG(x) wrong(__LINE__,x)
#define RMEM_KEY 0xdeadbabe
#define BUFF_SIZE (MSG_SIZE+16)*NUM_MSGS*NUM_THREADS

#define TIMEOUT 0xff

char buf[BUFF_SIZE]; // the remote memory buffer

typedef struct {
  mrapi_rmem_hndl_t rmem; /* mrapi rmem handle */
  mca_node_t tid;
  mca_node_t tid_other;
  mrapi_request_t read_requests[NUM_MSGS];
  mrapi_request_t write_requests[NUM_MSGS];
} thread_data;

void wrong(unsigned line,thread_data t)
{
  mca_status_t status;
  fprintf(stderr,"WRONG: line=%u\n", line);
  fflush(stdout);
  mrapi_rmem_detach(t.rmem,&status);
  mrapi_rmem_delete(t.rmem,&status);
  mrapi_finalize(&status);
  exit(1);
}


void *run_thread(void* tdata) {
  mca_status_t status;
  thread_data* t = (thread_data*)tdata;
  mrapi_info_t version;
  mrapi_parameters_t parms = 0;
  size_t msg_size;
  char msg [MSG_SIZE];  // what we will write each msg from
  char read_buf[MSG_SIZE]; // what we will read each msg into
  char expected_msg [MSG_SIZE];  // what we expect to read
  int i;
  size_t size = 0;
  mrapi_boolean_t creator = MRAPI_TRUE;

  unsigned done_reading_offset = 0;
  unsigned done_writing_offset = 4;
  unsigned buff_offset = 8;

  memset(msg,0,MSG_SIZE);
  memset(read_buf,0,MSG_SIZE);


  printf("run_thread tid=%d\n",t->tid);
  /* initialize mrapi */
  mrapi_initialize(DOMAIN,NODE+t->tid,parms,&version,&status);
  if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }
  
  /* create or get the remote memory and attach to it */
  t->rmem = mrapi_rmem_create(RMEM_KEY,&buf,MRAPI_RMEM_DUMMY,NULL /*attrs*/,sizeof(buf) /*size*/,&status);
  if (status == MRAPI_ERR_RMEM_EXISTS) {
    printf("rmem exists, try getting it from the other node");
    t->rmem = mrapi_rmem_get(RMEM_KEY,MRAPI_RMEM_DUMMY,&status);
    creator = MRAPI_FALSE;
  }
  if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }

  mrapi_rmem_attach(t->rmem,MRAPI_RMEM_DUMMY,&status);
  if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }
  
  // write the messages
  for (i = 0; i < NUM_MSGS; i++) {
#if (__unix||__MINGW32__)
    sprintf(msg,"Hello World from thread %d i=%d",t->tid,i);
#else
    sprintf_s(msg,sizeof(msg),"Hello World from thread %d i=%d",t->tid,i);
#endif  /* !(__unix||__MINGW32__) */
    msg_size = strlen(msg);
    
    mrapi_rmem_write_i(
                     t->rmem,
                     (BUFF_SIZE/NUM_THREADS)*t->tid+buff_offset+i*MSG_SIZE,
                     msg,
                     0,
                     msg_size,
                     1,
                     1*msg_size,
                     1*msg_size,
                     &t->write_requests[i],
                     &status
                     );
   if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }
    mrapi_wait( &t->write_requests[i],&size,TIMEOUT,&status);
    if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }
  }

  // write the valid/done bit 
  mrapi_rmem_write(
                   t->rmem,
                   done_writing_offset+t->tid,
                   "V",
                   0,
                   strlen("V"),
                   1,
                   1*strlen("V"),
                   1*strlen("V"),
                   &status
                   );
  if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }

  // spin on the other thread's valid/done writing bit
  while (strcmp(read_buf,"V")) {
    mrapi_rmem_read(
                    t->rmem,
                    done_writing_offset+t->tid_other,
                    read_buf,
                    0,
                    strlen("V"),
                    1,
                    1*strlen("V"),
                    1*strlen("V"),
                    &status
                    );
    if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }
#if (__unix||__MINGW32__)
    sched_yield();
#else
    SleepEx(0,0);
#endif  /* !(__unix||__MINGW32__) */
  }
  printf("other thread done writing: tid=%d rmem_read read_buf=[%s]\n",t->tid,read_buf);

  // now read the other thread's messages
  for ( i = 0; i < NUM_MSGS; i++) {
    mrapi_rmem_read_i(
                    t->rmem,
                    (BUFF_SIZE/NUM_THREADS)*t->tid_other+buff_offset+i*MSG_SIZE,
                    read_buf,
                    0,
                    msg_size,
                    1,
                    1*msg_size,
                    1*msg_size,
                    &t->read_requests[i],
                    &status
                    );
    if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }
    mrapi_wait( &t->read_requests[i],&size,TIMEOUT,&status);
    if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); } 
    
    /* check that we read the message from the other thread */
    memset(expected_msg,0,MSG_SIZE);
#if (__unix||__MINGW32__)
    sprintf(expected_msg,"Hello World from thread %d i=%d",t->tid_other,i);
#else
    sprintf_s(expected_msg,sizeof(expected_msg),"Hello World from thread %d i=%d",t->tid_other,i);
#endif  /* !(__unix||__MINGW32__) */
    if (strcmp(expected_msg,read_buf)) { 
      printf("expected_msg=[%s]\n",expected_msg);
      printf("read_buf=[%s]\n",read_buf);
      WRONG(*t); 
    }
    
    printf("tid=%d rmem_read read_buf=[%s]\n",t->tid,read_buf);
    memset(read_buf,0,MSG_SIZE);
  }
  
  // write the valid/done reading bit
  mrapi_rmem_write(
                   t->rmem,
                   done_reading_offset+t->tid,
                   "V",
                   0,
                   strlen("V"),
                   1,
                   1*strlen("V"),
                   1*strlen("V"),
                   &status
                   );
  if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }


  // spin on the other thread's valid/done reading bit
  memset(read_buf,0,MSG_SIZE);
  while (strcmp(read_buf,"V")) {
    mrapi_rmem_read(
                    t->rmem,
                    done_reading_offset+t->tid_other,
                    read_buf,
                    0,
                    strlen("V"),
                    1,
                    1*strlen("V"),
                    1*strlen("V"),
                    &status
                    );
    if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }
  }
   printf("other thread done reading: tid=%d rmem_read read_buf=[%s]\n",t->tid,read_buf);

  /* detach from the remote memory (it's okay if the other thread already deleted it) */
  mrapi_rmem_detach(t->rmem,&status);
  if (!((status == MRAPI_SUCCESS) || (status == MRAPI_ERR_RMEM_INVALID))) { 
    printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); 
    WRONG(*t); 
                                                        
  }

  if (creator) {
#if (__unix)
   sleep(2);
#else
   SleepEx(2000,0);
#endif  /* !(__unix) */
   /* free the remote memory */
    mrapi_rmem_delete(t->rmem,&status);
    if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }
  }

  /* finalize mrapi */
  mrapi_finalize(&status);
  if (status != MRAPI_SUCCESS) { printf("status=%s",mrapi_display_status(status,status_buff,sizeof(status_buff))); WRONG(*t); }
 
  return NULL;
}


int main () {
  int rc = 0;
  thread_data tdata[NUM_THREADS];
#if (__unix||__MINGW32__)
  int i;
#endif  /* !(__unix||__MINGW32__) */
  pthread_t threads[NUM_THREADS];
#if !(__unix||__MINGW32__)
  DWORD tid[NUM_THREADS] = { 0 };
#endif  /* !(__unix||__MINGW32__) */

   mrapi_set_debug_level(0);

  tdata[0].tid = 0;
  tdata[0].tid_other = 1;
  tdata[1].tid = 1;
  tdata[1].tid_other = 0;

  //run_thread ((void*)&tdata[0]);

  if (1) {
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
  }
   
  if (rc == 0) {
    printf("   Test PASSED\n");
  }

  return rc;
}
