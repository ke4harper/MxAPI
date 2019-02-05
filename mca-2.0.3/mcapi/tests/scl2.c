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
/*
   Test: scl2 
  (Note: this is the scalar version of test msg3)
  Description: This is a pthreads test that sends 256 messages using a sender thread 
  and a receiver thread.  The sender can (and often does) overflow the receive queue.  
  This is of course, dependent on the size of the receive queue and the scheduling 
  algorithm.  This test is run twice, once with the receiver first and once with the 
  sender first.  The sender will re-try TIMEOUT times if a send fails (usually due to
  the queue being full - ENO_BUFFER).  The receiver will re-try TIMEOUT times if a 
  recv fails (usually due to empty queue).

  Tests blocking send/recv.
*/

#include <mcapi.h>

char status_buff[MCAPI_MAX_STATUS_SIZE];
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>
#include <mca_config.h>
#include <pthread.h>

#define NUM_THREADS 2

#define BUFF_SIZE 32
#define DONE_PORT 20

#ifndef NODE
#define NODE 1
#endif

#ifndef DOMAIN
#define DOMAIN 1
#endif


#define NUM_MSGS 256

typedef struct {
  mcapi_boolean_t done;
} tdata;

tdata thread_data[NUM_THREADS];

void *sender(void* tdata);
void *receiver(void* tdata);
mcapi_sclchan_send_hndl_t send_handle;
mcapi_sclchan_recv_hndl_t recv_handle;

#define WRONG wrong(__LINE__);

void wrong(unsigned line)
{
  mca_status_t status;
  mcapi_finalize(&status);
  fprintf(stderr,"FAIL: line=%u\n", line);
  fflush(stdout);
  exit(1);
}


int main(void)
{  
  int i;
  pthread_t threads[NUM_THREADS];
  int rc = 0;
  mcapi_set_debug_level(3);
  // test receiver first and then sender
  printf("Test 1\n");
  rc += pthread_create(&threads[0], NULL, receiver,(void*)&thread_data[0]);
  rc += pthread_create(&threads[1], NULL, sender,(void*)&thread_data[1]);
  for (i = 0; i < 2; i++) {
    pthread_join(threads[i],NULL);
  }

  // now test sender first and then receiver
  printf("Test 2\n");
  rc += pthread_create(&threads[0], NULL, sender,(void*)&thread_data[0]);
  rc += pthread_create(&threads[1], NULL, receiver,(void*)&thread_data[1]);

 
  for (i = 0; i < 2; i++) {
    pthread_join(threads[i],NULL);
  }
  if (rc == 0) {
    printf("   Test PASSED\n");
  }
  return rc;
}

void *sender(void* data)
{
  char buffer[NUM_MSGS];

  char done_buffer[BUFF_SIZE];
  char exp_msg[BUFF_SIZE];

  int count = 0;
  mcapi_endpoint_t recv_endpt;
  mcapi_endpoint_t send_endpt;
  mcapi_endpoint_t done_ep;
  mca_status_t status;
  mcapi_request_t request;
  size_t size,recv_size;
  int i;
  //tdata *td = (tdata*) data;
  
  int port_num = 0;
  mcapi_param_t parms;
  mcapi_info_t version;
  
  /* create a node */
  mcapi_initialize(DOMAIN,NODE,NULL,&parms,&version,&status);
  
  if (status != MCAPI_SUCCESS) {
    fprintf(stderr,"\nERROR: Failed to initialize: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));  
    WRONG;
  }
  
  /* make send endpoint */
  send_endpt = mcapi_endpoint_create(port_num,&status);
  if (status != MCAPI_SUCCESS) {
    fprintf(stderr,"\nERROR: Failed to create send endpoint: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff))); 
    WRONG;
  }  else { 
    fprintf(stderr,"\nsender: Created send endpoint: %x\n",(unsigned)send_endpt); 
  }
  
  /* get the remote recv endpoint */
  recv_endpt = mcapi_endpoint_get(DOMAIN,NODE+1,1,MCA_INFINITE,&status);
  if (status != MCAPI_SUCCESS) {
    fprintf(stderr,"\nERROR: Failed to get recv endpoint: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff))); 
    WRONG;
  } else {
    fprintf(stderr,"\nsender: Got recv endpoint: %x\n",(unsigned)recv_endpt);
  }
  
  /* connect the channel */
  do {
    mcapi_sclchan_connect_i(send_endpt,recv_endpt,&request,&status);
    //retry if all request handles are in-use
  } while (status == MCAPI_ERR_REQUEST_LIMIT);
  mcapi_wait(&request,&size,TIMEOUT,&status);
  if (status == MCAPI_ERR_CHAN_CONNECTED) {
    fprintf(stderr,"\nokay, already connected: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));  
  }
  else if (status != MCAPI_SUCCESS) { 
    fprintf(stderr,"\nERROR: Failed to connect: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));  
    WRONG 
      } 
  
  /* open the send handle */
  do {
    mcapi_sclchan_send_open_i(&send_handle /*send_handle*/,send_endpt, &request, &status);
    //retry if all request handles are in-use
  } while (status == MCAPI_ERR_REQUEST_LIMIT);
  mcapi_wait(&request,&size,TIMEOUT,&status);
  if (status != MCAPI_SUCCESS) { 
    fprintf(stderr,"\nERROR: Failed to open send handle: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));  
    WRONG 
      }
  else {   
    fprintf(stderr,"\nsender: opened send handle: %x\n",(unsigned)send_handle);
  }
  
  /* do the sends */
  for (count = 0; count < NUM_MSGS; count++) {
    sprintf(buffer,"Sending: %d",count);
    
    printf("Sending: [%s]\n",buffer);
    
    mcapi_sclchan_send_uint32(send_handle,
                              count,
                              &status);
    
    if ((status != MCAPI_SUCCESS) && (TIMEOUT)) {
      /* yield and then retry */
      for (i = 0; i < TIMEOUT; i++) {
        if (status != MCAPI_SUCCESS) {
          fprintf(stderr,"WARNING: Send failed: reason:%s  send_count:%d.  Will yield and re-try.\n",mcapi_display_status(status,status_buff,sizeof(status_buff)),count);
          sched_yield();
          mcapi_sclchan_send_uint32(send_handle,count,&status);
          if (status == MCAPI_SUCCESS) {
            break;
          }
        }
      }
    }
    if (status != MCAPI_SUCCESS) {
      fprintf(stderr,"\nFAIL: Send failed: reason:%s  send_count:%d\n",mcapi_display_status(status,status_buff,sizeof(status_buff)),count);
      WRONG;
    }
  }
  
  /* wait to hear that the other node is done */
  sprintf(exp_msg,"%s","DONE");
  done_ep = mcapi_endpoint_create(DONE_PORT,&status);
  if (status != MCAPI_SUCCESS) { WRONG }
 
  memset(done_buffer,0,sizeof(done_buffer)); 
  mcapi_msg_recv(done_ep,done_buffer,BUFF_SIZE,&recv_size,&status);
  if (status != MCAPI_SUCCESS) { WRONG }
  if (strcmp(done_buffer,exp_msg) != 0) { printf("error: exp[%s] received[%s]\n",exp_msg,done_buffer); WRONG  }

  
  /* finalize */
  /* close the channel */
  do {
    mcapi_sclchan_send_close_i(send_handle,&request,&status);
    //retry if all request handles are in-use
  } while (status == MCAPI_ERR_REQUEST_LIMIT);
  mcapi_wait(&request,&size,TIMEOUT,&status);
  if (status != MCAPI_SUCCESS) { 
    fprintf(stderr,"\nERROR: Failed to close send handle: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));    
    WRONG 
      } else {
    fprintf(stderr,"\nsender closed send handle: %x\n",(unsigned)send_handle);
  }
  
  mcapi_finalize(&status);
  
  if (status != MCAPI_SUCCESS) {
    fprintf(stderr,"\nFAIL: Failed to finalize: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));   
    WRONG;
  }
  
  return NULL;
}

void *receiver(void* data)
{  
  int buffer;
  int count = 0;
 
  mca_status_t status;
  mcapi_request_t request;
  char done_buffer[BUFF_SIZE];
  mcapi_endpoint_t done_ep,done_send_ep;
  mcapi_endpoint_t recv_endpt, send_endpt;
  //tdata *td = (tdata*) data;
  int i;
  size_t size;

  int port_num = 1;
  mcapi_param_t parms;
  mcapi_info_t version;

  /* create a node */
  mcapi_initialize(DOMAIN,NODE+1,NULL,&parms,&version,&status);
  
  if (status != MCAPI_SUCCESS) {
    fprintf(stderr,"\nFAIL: Failed to initialize: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));   
    WRONG;
  }
  
  /* make recv endpoint */
  recv_endpt = mcapi_endpoint_create(port_num,&status);
  if (status != MCAPI_SUCCESS) {
    fprintf(stderr,"\nFAIL: Failed to create recv endpoint: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff))); 
    WRONG;
  }
  else {
    fprintf(stderr,"\nrecver: Created recv endpoint: %x\n",(unsigned)recv_endpt);
  }
  
  /* get remote endpoint */
  send_endpt = mcapi_endpoint_get(DOMAIN,NODE,0,MCA_INFINITE,&status);
  if (status != MCAPI_SUCCESS) {
    fprintf(stderr,"\nERROR: Failed to get send endpoint: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff))); 
    WRONG;
  }
  else {
    fprintf(stderr,"\nrecver: Got send endpoint: %x\n",(unsigned)send_endpt);
  }
  
  /* connect the channel */
  do {
    mcapi_sclchan_connect_i(send_endpt,recv_endpt,&request,&status);
    //retry if all request handles are in-use
  } while (status == MCAPI_ERR_REQUEST_LIMIT);
  mcapi_wait(&request,&size,TIMEOUT,&status);
  if (status == MCAPI_ERR_CHAN_CONNECTED) {
    fprintf(stderr,"\nokay, already connected: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));  
  }
  else if (status != MCAPI_SUCCESS) { 
    fprintf(stderr,"\nERROR: Failed to connect: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));  
    WRONG 
      }
  
  /* open the recv handle */
  do {
    mcapi_sclchan_recv_open_i(&recv_handle /*recv_handle*/,recv_endpt, &request, &status);
    //retry if all request handles are in-use
  } while (status == MCAPI_ERR_REQUEST_LIMIT);
  mcapi_wait(&request,&size,TIMEOUT,&status);
  if (status != MCAPI_SUCCESS) { 
    fprintf(stderr,"\nERROR: Failed to open recv handle: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));  
    WRONG 
      } 
  else {
    fprintf(stderr,"\nrecver: opened recv handle: %x\n",(unsigned)recv_handle);
  }                                                                                                                                           
  
  /* do the receives */
  for (count = 0; count < NUM_MSGS; count++) {
    buffer = mcapi_sclchan_recv_uint32(recv_handle,
                                       &status);
    if ((status != MCAPI_SUCCESS) && (TIMEOUT)) {
      /* yield and then retry */
      for (i = 0; i < TIMEOUT; i++) {
        fprintf(stderr,"\nWARNING: Recv failed: reason:%s  recv_count:%d.  Will yield and re-try.\n",mcapi_display_status(status,status_buff,sizeof(status_buff)),count);
        sched_yield();
        buffer = mcapi_sclchan_recv_uint32(recv_handle,&status);
        if (status == MCAPI_SUCCESS) {
          break;
        }
      }
    }
    
    if (status == MCAPI_SUCCESS) {
      printf("count=%d, Received: [%d]\n",count,buffer);
      if (buffer != count) {
          fprintf(stderr,"\nFAIL: Recv failed: expected buffer[%d] == count[%d].\n",buffer,count);
          WRONG;
      }
    } else {
      fprintf(stderr,"\nFAIL: Recv failed: reason:%s  recv_count:%d.\n",mcapi_display_status(status,status_buff,sizeof(status_buff)),count);
      WRONG;
    }
  }
  
  /* tell the other node I'm done */
  sprintf(done_buffer,"%s","DONE");
  done_ep = mcapi_endpoint_get(DOMAIN,NODE,DONE_PORT,MCA_INFINITE,&status);
  if (status != MCAPI_SUCCESS) { WRONG }
  
  done_send_ep = mcapi_endpoint_create(DONE_PORT+1,&status);
   if (status != MCAPI_SUCCESS) { WRONG }

  mcapi_msg_send(done_send_ep,done_ep,done_buffer,strlen(done_buffer),1,&status);
  if (status != MCAPI_SUCCESS) { WRONG }
    
  /* finalize */
  /* close the channel */
  do {
    mcapi_sclchan_send_close_i(recv_handle,&request,&status);
    //retry if all request handles are in-use
  } while (status == MCAPI_ERR_REQUEST_LIMIT);
  mcapi_wait(&request,&size,TIMEOUT,&status);
  if (status != MCAPI_SUCCESS) { 
    fprintf(stderr,"\nERROR: Failed to close recv handle: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));  
    WRONG 
      } else {
    fprintf(stderr,"\nrecver: closed the recv handle: %x\n",(unsigned)recv_handle);
  }
  
  mcapi_finalize(&status);  
  if (status != MCAPI_SUCCESS) {
    fprintf(stderr,"\nFAIL: Failed to finalize: %s\n",mcapi_display_status(status,status_buff,sizeof(status_buff)));   
    WRONG;
  }
  
  return NULL;
}

