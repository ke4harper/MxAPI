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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mthread.h> 

static char* hello_tb1 = "Hello to thread_body1";
static char* hello_from_tb1 = "Hello back from thread_body1";
/* static char* exit_from_tb2 = "Exit from thread_body2"; */

static int tb3_counter = 0;
static int tb4_counter = 0;


void* thread_body1(void* data)
{
  void* ret_value = NULL;

  if (strncmp((char*)data,hello_tb1,strlen(hello_tb1)) == 0) {
    ret_value = hello_from_tb1;
  } else {
    ret_value = "ERROR";
  }

  return ret_value;
}

void* thread_body2(void* data)
{
/*   mthread_exit(exit_from_tb2); */
  /* should never get here */
  return "ERROR";
}


/* the yield test wants to make sure both threads make progress */
void* thread_body3(void* data)
{
  int i, last_tb4;
/*   int fail_count = 0; */
  for (i = 0; i < 100000; i++) {
    last_tb4 = tb4_counter;
    tb3_counter++;
    if (i > 0 && (i % 200) == 0) {
      mthread_yield();
/*       if (last_tb4 == tb4_counter) { */
/*         fail_count++; */
/*         if (fail_count > 10) { */
/*           fprintf(stderr, "FAIL t3:[%d,%d]\n",tb3_counter,tb4_counter); */
/*           exit(1); */
/*         } */
/*       } */
    }
  }
  return NULL;
}

void* thread_body4(void* data)
{
  int i, last_tb3;
/*   int fail_count = 0; */

  for (i = 0; i < 100000; i++) {
    last_tb3 = tb4_counter;
    tb4_counter++;
    if (i > 0 && (i % 200) == 0) {
      mthread_yield();
/*       if (last_tb3 == tb3_counter) { */
/*         fail_count++; */
/*         if (fail_count > 10) { */
/*           fprintf(stderr, "FAIL t4:[%d,%d]\n",tb3_counter,tb4_counter); */
/*           exit(1); */
/*         } */
/*       } */
    }
  }
  return NULL;
}

int main(int argc, char** argv)
{
  mthread_status_t status = 0;
  mthread_t thread1, /* thread2,*/ thread3, thread4;
  char* return_data = NULL;

  /* test initialize/finalize */

  printf("testing normal init ... ");

  status = mthread_initialize();
  if (status != MTHREAD_SUCCESS) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  printf("PASS\n");

  printf("testing already init ... ");
  status = mthread_initialize();
  if (status != MTHREAD_EINIT) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  printf("PASS\n");

  printf("testing normal finalize ... ");
  status = mthread_finalize();
  if (status != MTHREAD_SUCCESS) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  printf("PASS\n");

  printf("testing already finalized ... ");
  status = mthread_finalize();
  if (status != MTHREAD_ENOINIT) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  printf("PASS\n");

  printf("testing create but not initialized ... ");
  status = mthread_create(&thread1,thread_body1,hello_tb1);
  if (status != MTHREAD_ENOINIT) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  printf("PASS\n");

  printf("testing create/wait ... ");  fflush(stdout);
  status = mthread_initialize();
  if (status != MTHREAD_SUCCESS) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  status = mthread_create(&thread1,thread_body1,hello_tb1);
  if (status != MTHREAD_SUCCESS) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  status = mthread_wait(thread1,(void*) &return_data);
  if (status != MTHREAD_SUCCESS) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  } else if (strncmp(return_data,hello_from_tb1,strlen(hello_from_tb1)) != 0) {
    fprintf(stderr,"FAIL bad return data [%s]\n", return_data);
    return 1;
  }
  printf("PASS\n");

  /* wait already exited */
  return_data = NULL;
  printf("testing wait on exited thread ... ");  fflush(stdout);
  status = mthread_wait(thread1,(void*) &return_data);
  if (status == MTHREAD_SUCCESS) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  printf("PASS\n");

  /* currently not allowing threads to call this */
  /* exit */
/*   printf("testing mthread_exit ... ");  fflush(stdout); */
/*   status = mthread_create(&thread2,thread_body2,NULL); */
/*   if (status != MTHREAD_SUCCESS) { */
/*     fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status)); */
/*     return 1; */
/*   } */
/*   status = mthread_wait(thread2,(void*) &return_data); */
/*   if (status != MTHREAD_SUCCESS) { */
/*     fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status)); */
/*     return 1; */
/*   } else if (strncmp(return_data,exit_from_tb2,strlen(exit_from_tb2)) != 0) { */
/*     fprintf(stderr,"FAIL bad return data [%s]\n", return_data); */
/*     return 1; */
/*   } */
/*   printf("PASS\n"); */
  

  /* yield */
  printf("testing mthread_yield ... ");  fflush(stdout);
  status = mthread_create(&thread3,thread_body3,NULL);
  if (status != MTHREAD_SUCCESS) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  status = mthread_create(&thread4,thread_body4,NULL);
  if (status != MTHREAD_SUCCESS) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  mthread_yield();
  status = mthread_wait(thread3,(void*) &return_data);
  if (status != MTHREAD_SUCCESS) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  status = mthread_wait(thread4,(void*) &return_data);
  if (status != MTHREAD_SUCCESS) {
    fprintf(stderr,"FAIL [%s]\n", mthread_strerr(status));
    return 1;
  }
  if (tb3_counter != tb4_counter || tb3_counter == 0 || tb4_counter == 0) {
    fprintf(stderr,"FAIL\n");
    return 1;
  }
  printf("PASS\n");
  
  printf("ALL TESTS PASSED\n");

  exit(0);
}
