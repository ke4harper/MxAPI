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
#include <unistd.h>

#include <mtapi.h> 


void task_body(mtapi_task_handle_t task, 
               void* input_data, 
               int input_data_size)
{
  mtapi_status_t status;

  printf("In task body, test 2 task, pid=%d\n",getpid());

  if (strcmp("hello test2",(char *) (input_data)) == 0) {
    mtapi_task_exit(task,
                    "task2 OK",
                    strlen("task2 OK"),
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

  printf("End of task\n");
}



int do_fork(void)
{
  pid_t pid;
  mtapi_task_handle_t task_handle;
  void* return_data;
  size_t return_data_size;
  mtapi_status_t mtapi_status;
  mtapi_task_status_t task_status;
 
  /* child will. register the task */
  /* parent will invoke and wait on it */

  if ((pid = fork()) < 0) {
    /* error forking */
    fprintf(stderr,"ERROR: error performing fork() call!\n");
    exit(3);
  } else if (pid == 0) {

    /* in child */
        
    printf("In child, pid = %d\n",getpid());

    mtapi_initialize(0,
                     0,
                     &mtapi_status);
    
    if (mtapi_status == MTAPI_ERROR) {
      fprintf(stderr,"Error: initializing mtapi in child\n");
      exit(4);
    }

    mtapi_register_impl("test2_task",
                        task_body,
                        &mtapi_status);
    
    if (mtapi_status == MTAPI_ERROR) {
      fprintf(stderr,"Error: registering task_body!\n");
      exit(1);
    }

    mtapi_slave();
        
    mtapi_finalize(0,&mtapi_status);

    printf("Child should get here\n");

  } else {

    /* in parent */

    printf("In parent PID %d\n",getpid());
        
    mtapi_initialize(0,
                     10,
                     &mtapi_status);
    
    if (mtapi_status == MTAPI_ERROR) {
      fprintf(stderr,"Error: initializing mtapi in parent\n");
      exit(4);
    }

    task_handle = mtapi_create_task(0,
                                    0,
                                    "test2_task",
                                    "hello test2",
                                    strlen("hello test2"),
                                    &mtapi_status);
    
    if (mtapi_status == MTAPI_ERROR) {
      fprintf(stderr,"Error: creating task_body!\n");
      exit(1);
    }
    
    task_status = mtapi_task_wait(task_handle,
                                  &return_data,
                                  &return_data_size,
                                  &mtapi_status);
    
    if (strncmp("task2 OK",(char *) return_data,strlen("task2 OK")) != 0 ||
        task_status != MTAPI_TASK_SUCCESS) {
      fprintf(stderr,"Error: bad return data received from task_body! [%s]\n",
              (char*) return_data);
      exit(1);
    } else {
      printf("Parent wait done, return data and size are GOOD\n");
    }
    
    free(return_data);
    
    /* tell other node to shut down? */

    mtapi_shutdown_slave(0,0,&mtapi_status);

    mtapi_finalize(0,&mtapi_status);
    
  }
  
  return 0;
}

int main(int argc, char** argv)
{
  /* we are going to fork two processes which then interact via MTAPI */

/*   mcapi_set_debug_level(8); */

  if (do_fork() != 0) {
    fprintf(stderr,"ERROR: failed during do_fork!\n");
    exit(1);
  }

  exit(0);

}
