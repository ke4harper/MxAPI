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

#ifndef _H_MTAPI
#define _H_MTAPI

#include <mca.h>

#include <stddef.h>  /* for size_t */
#include <stdint.h>  /*uint_t,etc.*/


/******************************************************************
           datatypes
******************************************************************/ 

/* the mca data types */
typedef mca_domain_t mtapi_domain_t;
typedef mca_node_t mtapi_node_t;
typedef mca_timeout_t mtapi_timeout_t;
typedef uint8_t mtapi_boolean_t;
/* TODO - is this really an OK datatype for this ? */
typedef int mtapi_task_t;

/* error codes */
typedef enum {
  MTAPI_ERROR,
  MTAPI_SUCCESS,
  MTAPI_EREMOTE,
  MTAPI_ENAME_LENGTH,
  MTAPI_EMAX_TASKS,
  MTAPI_EMAX_INSTANCES,
  MTAPI_EDATA_SIZE,
  MTAPI_EBAD_HANDLE
} mtapi_status_t;

typedef enum {
  MTAPI_TASK_ERROR,
  MTAPI_TASK_SUCCESS
} mtapi_task_status_t;


/* mtapi data */ 
typedef struct mtapi_task_descriptor {
  mtapi_domain_t domain;
  mtapi_node_t node;
  mtapi_task_t task;
} mtapi_task_descriptor_t;

typedef mtapi_task_descriptor_t* mtapi_task_handle_t;  

/* a task function pointer and its args*/
typedef void (*mtapi_impl_ptr_t) (mtapi_task_handle_t task, 
                                  void* input_data, 
                                  int input_data_size);

/******************************************************************
           API
******************************************************************/ 

/* TODO - add parameters to match mcapi/mrapi */
/* TODO - should one of the attributes be debug level?  same for mcapi/mrapi ? */
extern void mtapi_initialize(mtapi_domain_t domain,
                             mtapi_node_t node,
                             mtapi_status_t* status);

/* TODO - get node id, get domain */
/* TODO - do we need a way to send an impl to a slave, like OpenCL does ? */
/* TODO - how would we implement work stealing ? */
/* TODO - how would we implement work queues ?   */

/* TODO - need to add a version ? */
extern void mtapi_register_impl(char* task_name,
                                mtapi_impl_ptr_t impl,
                                mtapi_status_t* status);

/* TODO - need to add a version ? */
extern mtapi_task_handle_t mtapi_create_task(mtapi_domain_t domain,
                                             mtapi_node_t node,
                                             char* task_name,
                                             void* input_data,
                                             size_t input_data_size,
                                             mtapi_status_t* status);

extern void mtapi_task_exit(mtapi_task_handle_t task,
                            void* return_data,
                            size_t return_data_size,
                            mtapi_task_status_t task_status,
                            mtapi_status_t* status);

extern mtapi_task_status_t mtapi_task_wait(mtapi_task_handle_t task,
                                           void** return_data,
                                           size_t* return_data_size,
                                           mtapi_status_t* status);

/* TODO - are there other 'modes' ?? */
extern void mtapi_slave(void);

/* TODO - should this be generalized to "send_slave_command"    */
/*        one of which is "SHUTDOWN', and there may be others ? */
extern void mtapi_shutdown_slave(mtapi_domain_t domain,
                                 mtapi_node_t node,
                                 mtapi_status_t* status);

/* TO ADD - mtapi_strerr */

/*   TODO - remove the domain parameter after MCAPI updated to not have domain on finalize */
extern void mtapi_finalize(mtapi_domain_t domain,
                           mtapi_status_t* status);

#endif

#ifdef __cplusplus
extern } 
#endif /* __cplusplus */
