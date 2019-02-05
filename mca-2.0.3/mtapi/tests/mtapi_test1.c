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

#include <mtapi.h> 

void task_body(mtapi_task_handle_t task, 
               void* input_data, 
               int input_data_size)
{
  mtapi_status_t status;

  printf("In task body 1\n");

  if (strcmp("hello test1",(char *) (input_data)) == 0) {
    mtapi_task_exit(task,
                    "task1 OK",
                    strlen("task1 OK"),
                    MTAPI_TASK_SUCCESS,
                    &status);   
    if (status == MTAPI_ERROR) {
      fprintf(stderr,"Error: calling mtapi_task_exit!\n");
      exit(1);
    }
  } else {
      fprintf(stderr,"Error: task_body received bad input data!\n");
      exit(1);
  }

  printf("Leaving task\n");
}




int main(int argc, char** argv)
{
  mtapi_status_t status;
  mtapi_task_handle_t task_handle;
  void* return_data;
  size_t return_data_size;
  mtapi_task_status_t task_status;

  mtapi_initialize(0,
                   0,
                   &status);

  if (status == MTAPI_ERROR) {
    fprintf(stderr,"Error: initializing mtapi!\n");
    exit(1);
  }

  /* make a good task and call it */

  printf("Test1: Making a normal task and calling it ... ");

  mtapi_register_impl("test1_task",
                      task_body,
                      &status);

  if (status == MTAPI_ERROR) {
    fprintf(stderr,"Error: registering task_body!\n");
    exit(1);
  }

  task_handle = mtapi_create_task(0,
                                  0,
                                  "test1_task",
                                  "hello test1",
                                  strlen("hello test1"),
                                  &status);

  if (status == MTAPI_ERROR) {
    fprintf(stderr,"Error: creating task_body!\n");
    exit(1);
  }

  task_status = mtapi_task_wait(task_handle,
                                &return_data,
                                &return_data_size,
                                &status);

  if (strncmp("task1 OK",(char *) return_data,strlen("task1 OK")) != 0 ||
      task_status != MTAPI_TASK_SUCCESS) {
    fprintf(stderr,"Error: bad return data received from task_body! [%s]\n",
            (char*) return_data);
    exit(1);
  }

  free(return_data);

  printf("PASSED\n");
  
  /* lastly, test finalizing */

  mtapi_finalize(0, &status);

  if (status == MTAPI_ERROR) {
    fprintf(stderr,"Error: finalizing mtapi!\n");
    exit(1);
  }
  
  printf("ALL TESTS PASSED\n");

  exit(0);
}
