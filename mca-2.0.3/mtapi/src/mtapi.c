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

#include <mtapi.h>
#include <mca.h>
#include <mca_config.h>
#include <mcapi.h>

char status_buff[MCAPI_MAX_STATUS_SIZE];
#include <mthread.h>
#if (__unix__)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#endif  /* (__unix__) */

/******************

  TODO global items

  - add debug logging and convert all printfs

*******************/


#define TIMEOUT 0xffff

typedef struct task_impl {
  char* name;
  mtapi_impl_ptr_t impl;
} task_impl_t;  

typedef struct local_task_descriptor {
  mthread_t thread;
  mtapi_task_handle_t handle; 
  task_impl_t* task_impl;
  mtapi_task_status_t status;
  int input_data_size;
  void* input_data;
  int return_data_size;
  void* return_data;
} local_task_descriptor_t;

typedef enum {
  MTAPI_CMD_CREATE_TASK,
  MTAPI_CMD_WAIT_TASK,
  MTAPI_CMD_SHUTDOWN
} listener_cmd_t;

/* TODO - how to control this size in autoconfig and make it jive with MCAPI sizes */
/* TODO - error checking to make sure we are never violating sizes here */
#define MTAPI_MAX_TASK_NAME_LENGTH 32
#define MTAPI_MAX_INPUT_DATA_SIZE \
  (MCAPI_MAX_MSG_SIZE - \
   (sizeof(listener_cmd_t) + \
    sizeof(mtapi_domain_t) + \
    sizeof(mtapi_node_t) + \
   sizeof(int) + \
   (MTAPI_MAX_TASK_NAME_LENGTH + 1)))

typedef struct mtapi_remote_cmd {
  listener_cmd_t cmd;
  mtapi_domain_t from_domain;
  mtapi_node_t from_node;
  int data_size;
  char task_name[MTAPI_MAX_TASK_NAME_LENGTH + 1];
  char data[MTAPI_MAX_INPUT_DATA_SIZE];
} mtapi_remote_cmd_t;

typedef enum {
  MTAPI_REPLY_ACK,
  MTAPI_REPLY_NACK
} listener_reply_t;

/* TODO - how to control this size in autoconfig and make it jive with MCAPI sizes */
/* TODO - error checking to make sure we are never violating sizes here */
#define MTAPI_MAX_RETURN_DATA_SIZE \
  (MCAPI_MAX_MSG_SIZE - \
   (sizeof(listener_reply_t) + \
    sizeof(mtapi_task_descriptor_t) + \
   sizeof(int)))

typedef struct mtapi_remote_reply {
  listener_reply_t reply;
  mtapi_task_descriptor_t task;
  int data_size;
  char data[MTAPI_MAX_RETURN_DATA_SIZE];
} mtapi_remote_reply_t;

/* TODO -- need a separate define for MTAPI_MAX_INSTANCES */
#define MTAPI_MAX_INSTANCES MTAPI_MAX_TASKS
static task_impl_t task_impls[MTAPI_MAX_TASKS];
static int task_impl_count = 0;
static local_task_descriptor_t task_instances[MTAPI_MAX_INSTANCES];
static int task_instance_count = 0;

static mtapi_domain_t the_domain = -1;
static mtapi_node_t the_listener_node = -1;  /* endpoints owned by listener */
static mtapi_node_t the_requester_node = -1; /* endpoints owned by main */

static int finalize_mcapi = 0;
static int keep_listening = 1;
static mthread_t listener;
static int listener_send_port = 0;
static int listener_recv_port = 1;
static mcapi_endpoint_t listener_send_endpt;
static mcapi_endpoint_t listener_recv_endpt;
static int requester_send_port = 2;
static int requester_recv_port = 3;
static mcapi_endpoint_t requester_send_endpt;
static mcapi_endpoint_t requester_recv_endpt;

/* function prototypes */

int get_task_impl_idx(char* task_name);

int get_task_instance_idx(mtapi_task_handle_t task_handle);

int create_task_id(void);

void* local_dispatcher(void* instance_ptr);

mtapi_task_handle_t start_local_task(char* task_name, 
                                     void* input_data, 
                                     int input_data_size);

mtapi_task_handle_t start_remote_task(mtapi_domain_t domain,
                                      mtapi_node_t node,
                                      char* task_name, 
                                      void* input_data, 
                                      int input_data_size);

mtapi_status_t send_remote_start_response(mtapi_remote_cmd_t cmd,
                                          mtapi_task_handle_t task);

mtapi_task_status_t wait_local_task(mtapi_task_handle_t task,
                                    void** return_data,
                                    size_t* return_data_size,
                                    mtapi_status_t* status);

mtapi_task_status_t wait_remote_task(mtapi_task_handle_t task,
                                     void** return_data,
                                     size_t* return_data_size,
                                     mtapi_status_t* status);

mtapi_status_t send_remote_wait_response(listener_reply_t which,
                                         mtapi_remote_cmd_t cmd, 
                                         void* return_data, 
                                         size_t return_data_size, 
                                         mtapi_task_status_t task_status);

mtapi_status_t send_remote_shutdown_response(mtapi_remote_cmd_t cmd);

void* mtapi_listener(void* data);


/*****************************************************************************
 *  mtapi_initialize
 *****************************************************************************/

void mtapi_initialize(mtapi_domain_t domain,
                      mtapi_node_t node,
                      mtapi_status_t* status)
{
  int i;
  mcapi_status_t mcapi_status;
  mcapi_info_t mcapi_version;
  mcapi_param_t parms;
  mcapi_node_attributes_t node_attrs;

  for (i = 0; i < MTAPI_MAX_TASKS; i++) {
    task_impls[i].name = NULL;
    task_impls[i].impl = NULL;

    task_instances[i].handle = NULL;
    task_instances[i].return_data = NULL;
    task_instances[i].return_data_size = 0;
    task_instances[i].status = MTAPI_TASK_ERROR;
  }
    
  the_domain = domain;
  the_requester_node = node+1;
  the_listener_node = node;

  /* TODO - need a testcase for the case where MCAPI is initialized already */
  
  finalize_mcapi = 1;

  mcapi_initialize(the_domain, 
                   the_requester_node, 
                   &node_attrs,
                   &parms,
                   &mcapi_version, 
                   &mcapi_status);
  
  /* init mcapi for the main thread and create two endpoints */
#if (__unix__)
  printf("MTAPI: main_thread initializing MCAPI as <d=%d,n=%d>, PID=%d, TID=%d\n",
         the_domain,the_requester_node,getpid(),(int) pthread_self());
#else  /* !(__unix__) */
  printf("MTAPI: main_thread initializing MCAPI as <d=%d,n=%d>, PID=%d, TID=%d\n",
         the_domain,the_requester_node,GetCurrentProcessId(),(int) GetCurrentThreadId());
#endif  /* !(__unix__) */

  if (mcapi_status == MCAPI_ERR_NODE_INITFAILED) {

    finalize_mcapi = 0;
    the_domain = mcapi_domain_id_get(&mcapi_status);
    if (mcapi_status != MCAPI_SUCCESS) {
      fprintf(stderr,"ERROR: MTAPI failed to get MCAPI domain id: %s\n",
              mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
      *status = MTAPI_ERROR;
      return;
    } 

    printf("MTAPI INIT: MCAPI ALREADY INITIALIZED FOR DOMAIN: %d\n",the_domain);

  } else if (mcapi_status != MCAPI_SUCCESS) {

    fprintf(stderr,"ERROR: MTAPI failed to initialize MCAPI: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    *status = MTAPI_ERROR;
    return;
  }

  /* make a pair of endpoints for the main thread */
  requester_recv_endpt = 
    mcapi_endpoint_create(requester_recv_port, &mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: MTAPI failed to create MCAPI endpoint: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    *status = MTAPI_ERROR;
    return;
  }
#if (__unix__)
  printf("PID %d creates requester recv endpt <%d,%d,%d>\n",
         getpid(),the_domain,the_requester_node,requester_recv_port);
#else  /* !(__unix__) */
  printf("PID %d creates requester recv endpt <%d,%d,%d>\n",
         GetCurrentProcessId(),the_domain,the_requester_node,requester_recv_port);
#endif  /* !(__unix__) */

  requester_send_endpt = 
    mcapi_endpoint_create(requester_send_port, &mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: MTAPI failed to create MCAPI endpoint: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    *status = MTAPI_ERROR;
    return;
  }
#if (__unix__)
  printf("PID %d creates requester send endpt <%d,%d,%d>\n",
         getpid(),the_domain,the_requester_node, requester_send_port);
#else  /* !(__unix__) */
  printf("PID %d creates requester send endpt <%d,%d,%d>\n",
         GetCurrentProcessId(),the_domain,the_requester_node, requester_send_port);
#endif  /* !(__unix__) */

  /* mtapi runs a listener in a separate thread */

  if (mthread_initialize() != MTHREAD_SUCCESS) {
    *status = MTAPI_ERROR;
    printf("MTAPI failed to initialize mthreads!\n");
  } else if (mthread_create(&listener, mtapi_listener, NULL) != MTHREAD_SUCCESS) {
    *status = MTAPI_ERROR;
    printf("MTAPI failed to create listener thread!\n");
  } else {
    /* listener created, give it a chance to do its thing */
    mthread_yield();
    *status = MTAPI_SUCCESS;
  }
}


/*****************************************************************************
 *  mtapi_register_impl
 *****************************************************************************/
void mtapi_register_impl(char* task_name,
                         mtapi_impl_ptr_t impl,
                         mtapi_status_t* status)
{
  /* TODO - need to check for duplicate name */
  /* TODO - make a testcase for duplicate name */

  if (strlen(task_name) > MTAPI_MAX_TASK_NAME_LENGTH) {
    /* TODO - testcase for this */
    *status = MTAPI_ENAME_LENGTH;
  } else if (task_impl_count >= MTAPI_MAX_TASKS) {
    /* TODO - testcase for this */
    *status = MTAPI_EMAX_TASKS;
  } else {
#if (__unix__)
    task_impls[task_impl_count].name = strdup(task_name);
#else  /* !(__unix__) */
    task_impls[task_impl_count].name = _strdup(task_name);
#endif  /* !(__unix__) */
    task_impls[task_impl_count].impl = impl;
    *status = MTAPI_SUCCESS;
  }
}


/*****************************************************************************
 *  mtapi_create_task
 *****************************************************************************/
mtapi_task_handle_t mtapi_create_task(mtapi_domain_t domain,
                                      mtapi_node_t node,
                                      char* task_name,
                                      void* input_data,
                                      size_t input_data_size,
                                      mtapi_status_t* status)
{
  mtapi_task_handle_t ret_val = NULL;
  
  if (input_data_size > MTAPI_MAX_INPUT_DATA_SIZE) {    
    /* TODO - make a testcase for this */
    *status = MTAPI_EDATA_SIZE;
  } else if (task_instance_count >= MTAPI_MAX_INSTANCES) {
    /* TODO - make a testcase for this */
    *status = MTAPI_EMAX_INSTANCES;
  } else if (domain == the_domain && 
             node == the_listener_node) {
    /* it's a local task! */
    ret_val = start_local_task(task_name, input_data, input_data_size);
  } else {
    /* it's on another MTAPI instance */
#if (__unix__)
    printf("mtapi_create_task: PID %d starting task on <%d,%d> ....\n",
           getpid(),domain,node);
#else  /* !(__unix__) */
    printf("mtapi_create_task: PID %d starting task on <%d,%d> ....\n",
           GetCurrentProcessId(),domain,node);
#endif  /* !(__unix__) */
    
    ret_val = start_remote_task(domain, node, task_name, 
                                input_data, input_data_size);
  }
  
  if (ret_val == NULL) {
    fprintf(stderr,"ERROR: MTAPI failed to create task : <%d,%d,0>:%s\n",
            domain, node, task_name);
    *status = MTAPI_EREMOTE;
  } else {
    *status = MTAPI_SUCCESS;
  }
  
  return ret_val;
}

/*****************************************************************************
 *  mtapi_task_exit
 *****************************************************************************/
void mtapi_task_exit(mtapi_task_handle_t task,
                     void* return_data,
                     size_t return_data_size,
                     mtapi_task_status_t task_status,
                     mtapi_status_t* status)
{
  int instance_idx = get_task_instance_idx(task);
  
  /* exit is always local, and we keep data around until we get a 'wait' */
  /* the wait may be local or remote, but the data is stored locally     */
  
  printf("TASK EXIT, idx=%d\n",instance_idx);
  
  if (instance_idx < 0) {
    /* could not find instance */
    /* TODO - need a testcase for this */
    *status = MTAPI_EBAD_HANDLE;
  } else if (return_data_size > MTAPI_MAX_RETURN_DATA_SIZE) {
    /* TODO - need a testcase for this */
    *status = MTAPI_EDATA_SIZE;
  } else {
    task_instances[instance_idx].return_data = malloc(return_data_size);
    memcpy(task_instances[instance_idx].return_data,return_data,return_data_size);
    task_instances[instance_idx].return_data_size = return_data_size;
    task_instances[instance_idx].status = task_status;
    *status = MTAPI_SUCCESS;
  }
}

/*****************************************************************************
 *  mtapi_task_wait
 *****************************************************************************/
mtapi_task_status_t mtapi_task_wait(mtapi_task_handle_t task,
                                    void** return_data,
                                    size_t* return_data_size,
                                    mtapi_status_t* status)
{
  mtapi_task_status_t task_status = MTAPI_TASK_SUCCESS;

  if (task->domain == the_domain &&
      task->node == the_listener_node) {
    task_status = wait_local_task(task,return_data,return_data_size,status);
  } else {
    task_status = wait_remote_task(task,return_data,return_data_size,status);
  }

  return task_status;
}


/*****************************************************************************
 *  mtapi_slave
 *****************************************************************************/
void mtapi_slave(void) {
  void *dummy;
  mthread_wait(listener,&dummy);
}


/*****************************************************************************
 *  mtapi_shutdown_slave
 *****************************************************************************/
void mtapi_shutdown_slave(mtapi_domain_t domain,
                          mtapi_node_t node,
                          mtapi_status_t* status)
{
  size_t recv_size;
  mcapi_endpoint_t remote_endpt;
  mcapi_status_t mcapi_status;
  mtapi_remote_cmd_t cmd;
  mtapi_remote_reply_t reply;

  *status = MTAPI_SUCCESS;

  /* get the endpoint to talk to that <domain,node> */
  /* send the request to start the task */
  /* wait for reply from other MTAPI */
  
  remote_endpt = mcapi_endpoint_get(domain, 
                                    node, 
                                    listener_recv_port, 
                                    TIMEOUT,&mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: local MTAPI failed to locate remote MTAPI: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    *status = MTAPI_ERROR;
  } else {
    
#if (__unix__)
    printf("shutdown_slave: PID %d got the endpoint for <%d,%d,%d>...\n",
           getpid(),domain,node,listener_recv_port);
#else  /* !(__unix__) */
    printf("shutdown_slave: PID %d got the endpoint for <%d,%d,%d>...\n",
           GetCurrentProcessId(),domain,node,listener_recv_port);
#endif  /* !(__unix__) */
    
    cmd.cmd = MTAPI_CMD_SHUTDOWN;
    cmd.from_domain = the_domain;
    cmd.from_node = the_requester_node;
#if (__unix__)
    strcpy(cmd.task_name,"");
#else  /* !(__unix__) */
    strcpy_s(cmd.task_name,sizeof(cmd.task_name),"");
#endif  /* !(__unix__) */
    cmd.data_size = 0;
    
    mcapi_msg_send(requester_send_endpt, remote_endpt, &cmd, 
                   sizeof(cmd), 1, &mcapi_status);
    if (mcapi_status != MCAPI_SUCCESS) {
      fprintf(stderr,"ERROR: unable to send cmd to remote MTAPI: %s\n",
              mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
      *status = MTAPI_ERROR;
    } else {
      
#if (__unix__)
      printf("shutdown_slave: PID %d Sent the cmd ...\n",getpid());
#else  /* !(__unix__) */
      printf("shutdown_slave: PID %d Sent the cmd ...\n",GetCurrentProcessId());
#endif  /* !(__unix__) */

      /* ENHANCEMENT - should be async read with error/timeout */
      mcapi_msg_recv(requester_recv_endpt, &reply, sizeof(reply), 
                     &recv_size, &mcapi_status); 
      if (mcapi_status != MCAPI_SUCCESS || recv_size != sizeof(reply)) {
        fprintf(stderr,"ERROR: unable to recv ack/nack from remote MTAPI: %s\n",
                mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
        *status = MTAPI_ERROR;
      } else if (reply.reply == MTAPI_REPLY_NACK) {
        /* ENHANCEMENT - can we encode more status in the NACK ? */
        fprintf(stderr,"ERROR: received nack from remote MTAPI\n");
      } else {
#if (__unix__)
        printf("shutdown_slave: PID %d Got ACK response...\n",getpid());    
#else  /* !(__unix__) */
        printf("shutdown_slave: PID %d Got ACK response...\n",GetCurrentProcessId());    
#endif  /* !(__unix__) */
      }
    }
  }

}

/*****************************************************************************
 *  mtapi_finalize
 *****************************************************************************/
void mtapi_finalize(mtapi_domain_t domain,
                    mtapi_status_t* status)
{
  void* listener_data;
  int i = 0;
  mcapi_status_t mcapi_status;

  *status = MTAPI_SUCCESS;


  /* if keep_listening is zero then the listener shut itslef down already */
  if (keep_listening == 1) {
#if (__unix__)
    printf("mtapi_finalize: PID %d waits for listener...\n",getpid());
#else  /* !(__unix__) */
    printf("mtapi_finalize: PID %d waits for listener...\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
    keep_listening = 0;
    if (mthread_wait(listener,&listener_data) != MTHREAD_SUCCESS) {
      *status = MTAPI_ERROR;
      printf("ERROR waiting on listener thread!\n");
    }
#if (__unix__)
    printf("mtapi_finalize: PID %d done waiting for listener...\n",getpid());
#else  /* !(__unix__) */
    printf("mtapi_finalize: PID %d done waiting for listener...\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
  } else {
#if (__unix__)
    printf("mtapi_finalize: PID %d listener already shut down...\n",getpid());
#else  /* !(__unix__) */
    printf("mtapi_finalize: PID %d listener already shut down...\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
  }

  if (mthread_finalize() != MTHREAD_SUCCESS) {
    *status = MTAPI_ERROR;
    printf("ERROR finalizing mthread library\n");
  }

  for (i = 0; i < task_impl_count; i++) {
    free(task_impls[i].name);
  }
  for (i = 0; i < task_instance_count; i++) {
    free(task_instances[i].handle);
    free(task_instances[i].return_data);
  }

  if (finalize_mcapi) {
    mcapi_finalize(&mcapi_status);
    if (mcapi_status != MCAPI_SUCCESS) {
      fprintf(stderr,"ERROR: MTAPI failed to finalize MCAPI: %s\n",
              mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
      *status = MTAPI_ERROR;
    }
  }

#if (__unix__)
  printf("MTAPI FINALIZED for PID %d\n",getpid());
#else  /* !(__unix__) */
  printf("MTAPI FINALIZED for PID %d\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
}


/**** INTERNAL FUNCTIONS ****/

/*****************************************************************************
 *  get_task_impl_idx
 *****************************************************************************/
int get_task_impl_idx(char* task_name)
{
  int idx = -1;
  int i = 0;
  int found = 0;

  while (i < MTAPI_MAX_TASKS && !found) {
    if (strcmp(task_name,task_impls[i].name) == 0) {
      found = 1;
      idx = i;
    } else {
      ++i;
    }
  }
  
  return idx;
}


/*****************************************************************************
 *  get_task_instance_idx
 *****************************************************************************/
int get_task_instance_idx(mtapi_task_handle_t task_handle)
{
  int idx = -1;
  int i = 0;
  int found = 0;

  while (i < MTAPI_MAX_TASKS && !found) {
    if (task_instances[i].handle->domain == task_handle->domain &&
        task_instances[i].handle->node == task_handle->node &&
        task_instances[i].handle->task == task_handle->task) {
      found = 1;
      idx = i;
    } else {
      ++i;
    }
  }
  
  return idx;
}

/*****************************************************************************
 *  create_task_id
 *****************************************************************************/
int create_task_id(void) {
  static int task_create_count = 0;
  int retval = task_create_count;
  task_create_count++;
  return retval;
}

/*****************************************************************************
 *  local_dispatcher
 *****************************************************************************/
void* local_dispatcher(void* instance_ptr)
{
  local_task_descriptor_t* instance = 
    (local_task_descriptor_t*) instance_ptr;

  instance->task_impl->impl(instance->handle, 
                            instance->input_data, 
                            instance->input_data_size);
  
  return NULL;
}


/*****************************************************************************
 *  start_local_task
 *****************************************************************************/
mtapi_task_handle_t start_local_task(char* task_name, 
                                     void* input_data, 
                                     int input_data_size)
{
  mtapi_task_handle_t ret_val = NULL;
  int task_idx = get_task_impl_idx(task_name);
  
  if (task_idx >= 0) {
    
    task_instances[task_instance_count].handle = 
      malloc(sizeof(mtapi_task_descriptor_t));
    task_instances[task_instance_count].handle->domain = the_domain;
    task_instances[task_instance_count].handle->node = the_listener_node;
    task_instances[task_instance_count].handle->task = create_task_id();
    task_instances[task_instance_count].task_impl = &(task_impls[task_idx]);
    task_instances[task_instance_count].input_data_size = input_data_size;
    task_instances[task_instance_count].input_data =
      malloc(input_data_size);
    memcpy(task_instances[task_instance_count].input_data,
           input_data, input_data_size);

    if (mthread_create(&task_instances[task_instance_count].thread,
                       local_dispatcher, 
                       &task_instances[task_instance_count]) == MTHREAD_SUCCESS) {
      task_instance_count++;
      ret_val = task_instances[0].handle;
    } else {
      printf("error creating local task!\n");
      free(task_instances[task_instance_count].handle);
      task_instances[task_instance_count].handle->domain = -1;
      task_instances[task_instance_count].handle->node = -1;
      task_instances[task_instance_count].handle->task = -1;
      task_instances[task_instance_count].task_impl = NULL;
      task_instances[task_instance_count].input_data_size = 0;
      free(task_instances[task_instance_count].input_data);
    }
  }

  return ret_val;
}

/*****************************************************************************
 *  start_remote_task
 *****************************************************************************/
mtapi_task_handle_t start_remote_task(mtapi_domain_t domain,
                                      mtapi_node_t node,
                                      char* task_name, 
                                      void* input_data, 
                                      int input_data_size)
{
  mtapi_task_handle_t ret_val = NULL;
  size_t recv_size;
  mcapi_endpoint_t remote_endpt;
  mcapi_status_t mcapi_status;
  mtapi_remote_cmd_t cmd;
  mtapi_remote_reply_t reply;

  /* get the endpoint to talk to that <domain,node> */
  /* send the request to start the task */
  /* wait for reply from other MTAPI */
  
  remote_endpt = mcapi_endpoint_get(domain, 
                                    node, 
                                    listener_recv_port, 
                                    TIMEOUT,&mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: local MTAPI failed to locate remote MTAPI: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
  } else {
    
#if (__unix__)
    printf("start_remote_task: PID %d got the endpoint for <%d,%d,%d>...\n",
           getpid(),domain,node,listener_recv_port);
#else  /* !(__unix__) */
    printf("start_remote_task: PID %d got the endpoint for <%d,%d,%d>...\n",
           GetCurrentProcessId(),domain,node,listener_recv_port);
#endif  /* !(__unix__) */
    
    cmd.cmd = MTAPI_CMD_CREATE_TASK;
    cmd.from_domain = the_domain;
    cmd.from_node = the_requester_node;
#if (__unix__)
    strcpy(cmd.task_name,task_name);
#else  /* !(__unix__) */
    strcpy_s(cmd.task_name,sizeof(cmd.task_name),task_name);
#endif  /* !(__unix__) */
    cmd.data_size = input_data_size;
    memcpy(cmd.data,input_data,input_data_size);
    cmd.data[input_data_size] = 0;
    
    mcapi_msg_send(requester_send_endpt, remote_endpt, &cmd, 
                   sizeof(cmd), 1, &mcapi_status);
    if (mcapi_status != MCAPI_SUCCESS) {
      fprintf(stderr,"ERROR: unable to send cmd to remote MTAPI: %s\n",
              mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    } else {
      
#if (__unix__)
      printf("start_remote_task: PID %d Sent the cmd ...\n",getpid());
#else  /* !(__unix__) */
      printf("start_remote_task: PID %d Sent the cmd ...\n",GetCurrentProcessId());
#endif  /* !(__unix__) */

      /* ENHANCEMENT - should be async read with error/timeout */
      mcapi_msg_recv(requester_recv_endpt, &reply, sizeof(reply), 
                     &recv_size, &mcapi_status); 
      if (mcapi_status != MCAPI_SUCCESS || recv_size != sizeof(reply)) {
        fprintf(stderr,"ERROR: unable to recv ack/nack from remote MTAPI: %s\n",
                mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
      } else if (reply.reply == MTAPI_REPLY_NACK) {
        /* ENHANCEMENT - can we encode more status in the NACK ? */
        fprintf(stderr,"ERROR: received nack from remote MTAPI\n");
      } else {
#if (__unix__)
        printf("start_remote_task: PID %d Got ACK response...\n",getpid());    
#else  /* !(__unix__) */
        printf("start_remote_task: PID %d Got ACK response...\n",GetCurrentProcessId());    
#endif  /* !(__unix__) */
        ret_val = malloc(sizeof(mtapi_task_descriptor_t));
        memcpy(ret_val, &reply.task, sizeof(mtapi_task_descriptor_t));      
      }
    }
  }

  return ret_val;
}


/*****************************************************************************
 *  send_remote_start_response
 *****************************************************************************/
mtapi_status_t send_remote_start_response(mtapi_remote_cmd_t cmd, 
                                          mtapi_task_handle_t task)
{
  mcapi_endpoint_t remote_endpt;
  mcapi_status_t mcapi_status;
  mtapi_remote_reply_t reply;

  /* get the requestors recv endpoint */
  remote_endpt = mcapi_endpoint_get(cmd.from_domain, 
                                    cmd.from_node, 
                                    requester_recv_port, 
                                    TIMEOUT,&mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: local MTAPI failed to locate remote MTAPI: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    return MTAPI_EREMOTE;
  } else {
    if (task != NULL) {
      /* send ACK + the task handle */
      reply.reply = MTAPI_REPLY_ACK;
      reply.task.domain = task->domain;
      reply.task.node = task->node;
      reply.task.task = task->task;
      reply.data_size = 0;
      reply.data[0] = 0;
    } else {
      /* send a NACK */
      reply.reply = MTAPI_REPLY_NACK;
      reply.task.domain = the_domain;
      reply.task.node = the_listener_node;
      reply.task.task = -1;
      reply.data_size = 0;
      reply.data[0] = 0;
    }

    /* now send it */
    mcapi_msg_send(listener_send_endpt, remote_endpt, &reply, 
                   sizeof(reply), 1, &mcapi_status);
    if (mcapi_status != MCAPI_SUCCESS) {
      fprintf(stderr,"ERROR: local MTAPI failed to send start ACK/NACK: %s\n",
              mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
      return MTAPI_EREMOTE;
    }

  }

#if (__unix__)
  printf("PID %d SENT THE START ACK\n",getpid());
#else  /* !(__unix__) */
  printf("PID %d SENT THE START ACK\n",GetCurrentProcessId());
#endif  /* !(__unix__) */

  return MTAPI_SUCCESS;
}


/*****************************************************************************
 *  wait_local_task
 *****************************************************************************/
mtapi_task_status_t wait_local_task(mtapi_task_handle_t task,
                                    void** return_data,
                                    size_t* return_data_size,
                                    mtapi_status_t* status)
{
  int instance_idx = get_task_instance_idx(task);
  void* null_data;

  /* NEXT NEXT NEXT */

  /* TODO - add error handling */

  mthread_wait(task_instances[instance_idx].thread,&null_data);

  *return_data_size = task_instances[instance_idx].return_data_size;
  *return_data = malloc(task_instances[instance_idx].return_data_size);
  memcpy(*return_data,task_instances[instance_idx].return_data,
         task_instances[instance_idx].return_data_size);
  *status = MTAPI_SUCCESS;

  /* TODO -- remove instance and collapse database */
  /* TODO - make a testcase for this */

  return MTAPI_TASK_SUCCESS;
}


/*****************************************************************************
 *  wait_remote_task
 *****************************************************************************/
mtapi_task_status_t wait_remote_task(mtapi_task_handle_t task,
                                     void** return_data,
                                     size_t* return_data_size,
                                     mtapi_status_t* status)
{
  mcapi_endpoint_t remote_endpt;
  mcapi_status_t mcapi_status;
  mtapi_remote_cmd_t cmd;
  mtapi_remote_reply_t reply;
  size_t recv_size;

  *status = MTAPI_SUCCESS;

  /* get the endpoint to talk to that <domain,node> */
  /* send the request to wait on the task */
  /* wait for reply from other MTAPI */
  
  remote_endpt = mcapi_endpoint_get(task->domain, 
                                    task->node, 
                                    listener_recv_port, 
                                    TIMEOUT,&mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: local MTAPI failed to locate remote MTAPI: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    *status = MTAPI_EREMOTE;
    return MTAPI_TASK_ERROR;
  } else {
    
#if (__unix__)
    printf("wait_remote_task: PID %d got the endpoint <%d,%d>...\n",
           getpid(),task->domain,task->node);
#else  /* !(__unix__) */
    printf("wait_remote_task: PID %d got the endpoint <%d,%d>...\n",
           GetCurrentProcessId(),task->domain,task->node);
#endif  /* !(__unix__) */
    
    cmd.cmd = MTAPI_CMD_WAIT_TASK;
    cmd.from_domain = the_domain;
    cmd.from_node = the_requester_node;
#if (__unix__)
    strcpy(cmd.task_name,"");
#else  /* !(__unix__) */
    strcpy_s(cmd.task_name,sizeof(cmd.task_name),"");
#endif  /* !(__unix__) */
    cmd.data_size = sizeof(mtapi_task_descriptor_t);
    memcpy(cmd.data,task , cmd.data_size);
    
    mcapi_msg_send(requester_send_endpt, remote_endpt, &cmd, 
                   sizeof(cmd), 1, &mcapi_status);
    if (mcapi_status != MCAPI_SUCCESS) {
      fprintf(stderr,"ERROR: unable to send cmd to remote MTAPI: %s\n",
              mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
      *status = MTAPI_EREMOTE;
      return MTAPI_TASK_ERROR;
    } else {
#if (__unix__)
      printf("wait_remote_task: PID %d Sent the cmd ...\n",getpid());
#else  /* !(__unix__) */
      printf("wait_remote_task: PID %d Sent the cmd ...\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
      
      /* ENHANCEMENT - this needs to be async I/O with a timeout/error */
      mcapi_msg_recv(requester_recv_endpt, &reply, sizeof(mtapi_remote_reply_t), 
                     &recv_size, &mcapi_status); 
      if (mcapi_status != MCAPI_SUCCESS || recv_size != sizeof(reply)) {
        fprintf(stderr,"ERROR: unable to recv ack/nack from remote MTAPI: %s\n",
                mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
        *status = MTAPI_EREMOTE;
        return MTAPI_TASK_ERROR;
      } else if (reply.reply == MTAPI_REPLY_NACK) {
        /* ENHANCEMENT - can we encode more status in the NACK ? */
        fprintf(stderr,"ERROR: received nack from remote MTAPI\n");
        return MTAPI_TASK_ERROR;
      } else {
#if (__unix__)
        printf("wait_remote_task: PID %d Got ACK response...\n",getpid());   
#else  /* !(__unix__) */
        printf("wait_remote_task: PID %d Got ACK response...\n",GetCurrentProcessId());   
#endif  /* !(__unix__) */
        printf("RETURN DATA [%s], RETURN DATA SIZE [%d]\n",reply.data, reply.data_size);
        *return_data_size = reply.data_size;
        *return_data = malloc(reply.data_size);
        memcpy(*return_data,reply.data,reply.data_size);
      }
    }
  }

  return MTAPI_TASK_SUCCESS;
}


/*****************************************************************************
 *  send_remote_wait_response
 *****************************************************************************/
mtapi_status_t send_remote_wait_response(listener_reply_t which,
                                         mtapi_remote_cmd_t cmd, 
                                         void* return_data, 
                                         size_t return_data_size, 
                                         mtapi_task_status_t task_status)
{
  mcapi_endpoint_t remote_endpt;
  mcapi_status_t mcapi_status;
  mtapi_remote_reply_t reply;

  /* get the requestors recv endpoint */
  remote_endpt = mcapi_endpoint_get(cmd.from_domain, 
                                    cmd.from_node, 
                                    requester_recv_port, 
                                    TIMEOUT,&mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: local MTAPI failed to locate remote MTAPI: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    return MTAPI_EREMOTE;
  } else {
    if (which == MTAPI_REPLY_ACK) {
      /* send ACK + the task handle */
      reply.reply = MTAPI_REPLY_ACK;
      reply.task.domain = the_domain;
      reply.task.node = the_listener_node;
      reply.task.task = ((mtapi_task_handle_t) &cmd.data)->task;
      reply.data_size = return_data_size;
      /* don't worry, return data size was error checked on task_exit call */
      memcpy(reply.data,return_data,return_data_size);
    } else {
      /* send a NACK */
      reply.reply = MTAPI_REPLY_NACK;
      reply.task.domain = the_domain;
      reply.task.node = the_listener_node;
      reply.task.task = ((mtapi_task_handle_t) &cmd.data)->task;
      reply.data_size = return_data_size;
      reply.data[0] = 0;
    }

    /* now send it */
    mcapi_msg_send(listener_send_endpt, remote_endpt, &reply, 
                   sizeof(reply), 1, &mcapi_status);
    if (mcapi_status != MCAPI_SUCCESS) {
      fprintf(stderr,"ERROR: local MTAPI failed to send 'wait' ACK/NACK: %s\n",
              mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
      return MTAPI_EREMOTE;
    }

  }

#if (__unix__)
  printf("PID %d SENT THE WAIT ACK\n",getpid());
#else  /* !(__unix__) */
  printf("PID %d SENT THE WAIT ACK\n",GetCurrentProcessId());
#endif  /* !(__unix__) */

  return MTAPI_SUCCESS;
}


/*****************************************************************************
 *  send_remote_shutdown_response
 *****************************************************************************/
mtapi_status_t send_remote_shutdown_response(mtapi_remote_cmd_t cmd)
{
  mcapi_endpoint_t remote_endpt;
  mcapi_status_t mcapi_status;
  mtapi_remote_reply_t reply;

  /* get the requesters recv endpoint */
  remote_endpt = mcapi_endpoint_get(cmd.from_domain, 
                                    cmd.from_node, 
                                    requester_recv_port, 
                                    TIMEOUT,&mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: local MTAPI failed to locate remote MTAPI: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    return MTAPI_EREMOTE;
  } else {
    /* send ACK */
    reply.reply = MTAPI_REPLY_ACK;
    reply.task.domain = the_domain;
    reply.task.node = the_listener_node;
    reply.task.task = 0;
    reply.data_size = 0;
    
    /* now send it */
    mcapi_msg_send(listener_send_endpt, remote_endpt, &reply, 
                   sizeof(reply), 1, &mcapi_status);
    if (mcapi_status != MCAPI_SUCCESS) {
      fprintf(stderr,"ERROR: local MTAPI failed to send 'wait' ACK/NACK: %s\n",
              mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
      return MTAPI_EREMOTE;
    }
  }

#if (__unix__)
  printf("PID %d SENT THE SHUTDOWN ACK\n",getpid());
#else  /* !(__unix__) */
  printf("PID %d SENT THE SHUTDOWN ACK\n",GetCurrentProcessId());
#endif  /* !(__unix__) */

  return MTAPI_SUCCESS;
}


/*****************************************************************************
 *  mtapi_listener
 *****************************************************************************/
void* mtapi_listener(void* data)
{
  mtapi_remote_cmd_t cmd;
  size_t recv_size;
  mcapi_request_t request;
  mcapi_status_t mcapi_status;
  mtapi_status_t mtapi_status;
  mtapi_task_status_t task_status;
  mtapi_task_handle_t task = NULL;
  mcapi_info_t mcapi_version;
  mcapi_param_t parms;
  void* return_data;
  size_t return_data_size;
  mcapi_boolean_t recvd = MCAPI_FALSE;
  mcapi_node_attributes_t node_attrs;

  /* this thread will simply listen for any requests from other */
  /* MTAPI instances to create a task, or to wait on a task     */
  /* it will communicate with the other instances via MCAPI     */

#if (__unix__)
  printf("MTAPI: listener_thread initializing MCAPI as <d=%d,n=%d>, PID=%d, TID=%d\n",
         the_domain,the_listener_node,getpid(),(int) pthread_self());
#else  /* !(__unix__) */
  printf("MTAPI: listener_thread initializing MCAPI as <d=%d,n=%d>, PID=%d, TID=%d\n",
         the_domain,the_listener_node,GetCurrentProcessId(),(int) GetCurrentThreadId());
#endif  /* !(__unix__) */

  /* do all the init stuff */

  /* this mtapi impl uses mcapi to communicate with other impls */
  mcapi_initialize(the_domain, 
                   the_listener_node, 
                   &node_attrs,
		   &parms,
                   &mcapi_version, 
                   &mcapi_status);

  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: MTAPI listener <%d,%d> failed to initialize MCAPI: %s\n",
            the_domain, the_listener_node, mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    return NULL;
  }
#if (__unix__)
  printf("MCAPI initialized for PID %d <%d,%d>\n",getpid(),the_domain,the_listener_node);
#else  /* !(__unix__) */
  printf("MCAPI initialized for PID %d <%d,%d>\n",GetCurrentProcessId(),the_domain,the_listener_node);
#endif  /* !(__unix__) */

  /* make a set of endpoints for the listener thread */
  listener_recv_endpt = 
    mcapi_endpoint_create(listener_recv_port, &mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: MTAPI failed to create MCAPI endpoint: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    return NULL;
  }
#if (__unix__)
  printf("PID %d creates listener recv endpt <%d,%d,%d>\n",
         getpid(),the_domain,the_listener_node,listener_recv_port);
#else  /* !(__unix__) */
  printf("PID %d creates listener recv endpt <%d,%d,%d>\n",
         GetCurrentProcessId(),the_domain,the_listener_node,listener_recv_port);
#endif  /* !(__unix__) */

  listener_send_endpt = 
    mcapi_endpoint_create( listener_send_port, &mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: MTAPI failed to create MCAPI endpoint: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    return NULL;
  }
#if (__unix__)
  printf("PID %d creates listener send endpt <%d,%d,%d>\n",
         getpid(),the_domain,the_listener_node,listener_send_port);
#else  /* !(__unix__) */
  printf("PID %d creates listener send endpt <%d,%d,%d>\n",
         GetCurrentProcessId(),the_domain,the_listener_node,listener_send_port);
#endif  /* !(__unix__) */

  mcapi_msg_recv_i(listener_recv_endpt, &cmd, sizeof(cmd), 
                   &request, &mcapi_status);
#if (__unix__)
  printf("listener PID %d issues msg_recv (1) on endpt:%d\n",getpid(),listener_recv_endpt);
#else  /* !(__unix__) */
  printf("listener PID %d issues msg_recv (1) on endpt:%d\n",GetCurrentProcessId(),listener_recv_endpt);
#endif  /* !(__unix__) */

  while (keep_listening) {

    task = NULL;

    /* wait for a command */

    recvd = mcapi_wait(&request, &recv_size, 1000000,&mcapi_status);

    if (recvd == MCAPI_TRUE && mcapi_status == MCAPI_SUCCESS) {

#if (__unix__)
      printf("listener PID %d received from: <%d,%d> [%s] [%d] [%s]\n",
             getpid(), cmd.from_domain, cmd.from_node, cmd.task_name,
             cmd.data_size, cmd.data); 
#else  /* !(__unix__) */
      printf("listener PID %d received from: <%d,%d> [%s] [%d] [%s]\n",
             GetCurrentProcessId(), cmd.from_domain, cmd.from_node, cmd.task_name,
             cmd.data_size, cmd.data); 
#endif  /* !(__unix__) */
      
      switch (cmd.cmd) {
      case MTAPI_CMD_CREATE_TASK:
#if (__unix__)
        printf("PID %d; CMD = MTAPI_CMD_CREATE_TASK\n",getpid());
#else  /* !(__unix__) */
        printf("PID %d; CMD = MTAPI_CMD_CREATE_TASK\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
        task = start_local_task(cmd.task_name, 
                                cmd.data, 
                                cmd.data_size);
        /* respond to requestor */
        if (send_remote_start_response(cmd, task) == MTAPI_SUCCESS) {
#if (__unix__)
          printf("PID %d; DONE WITH MTAPI_CMD_CREATE_TASK\n",getpid());
#else  /* !(__unix__) */
          printf("PID %d; DONE WITH MTAPI_CMD_CREATE_TASK\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
        } else {
          fprintf(stderr,"error sending remote start response!\n");
        }
        break;
      case MTAPI_CMD_WAIT_TASK:
#if (__unix__)
        printf("PID %d; CMD = MTAPI_CMD_WAIT_TASK\n",getpid());
#else  /* !(__unix__) */
        printf("PID %d; CMD = MTAPI_CMD_WAIT_TASK\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
        task_status = wait_local_task((mtapi_task_handle_t) &cmd.data,
                                      &return_data,
                                      &return_data_size,
                                      &mtapi_status);
        if (mtapi_status == MTAPI_SUCCESS) {
          mtapi_status = send_remote_wait_response(MTAPI_REPLY_ACK, 
                                                   cmd, 
                                                   return_data, 
                                                   return_data_size, 
                                                   task_status);
        } else {
          mtapi_status = send_remote_wait_response(MTAPI_REPLY_NACK, 
                                                   cmd, 
                                                   NULL, 
                                                   0, 
                                                   task_status);
        }
        if (mtapi_status == MTAPI_SUCCESS) {
#if (__unix__)
          printf("PID %d; DONE WITH MTAPI_CMD_WAIT_TASK\n",getpid());
#else  /* !(__unix__) */
          printf("PID %d; DONE WITH MTAPI_CMD_WAIT_TASK\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
        } else {
          fprintf(stderr,"error sending remote wait response!\n");
        }
        break;
      case MTAPI_CMD_SHUTDOWN:
#if (__unix__)
        printf("PID %d; CMD = MTAPI_CMD_SHUTDOWN\n",getpid());
#else  /* !(__unix__) */
        printf("PID %d; CMD = MTAPI_CMD_SHUTDOWN\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
        keep_listening = 0;
        if (send_remote_shutdown_response(cmd) == MTAPI_SUCCESS) {
#if (__unix__)
          printf("PID %d shutting down\n",getpid());
#else  /* !(__unix__) */
          printf("PID %d shutting down\n",GetCurrentProcessId());
#endif  /* !(__unix__) */
        } else {
          fprintf(stderr,"error sending shutdown response!\n");
        }
        break;
      default:
        fprintf(stderr,"ERROR: unrecognized listener cmd: %d\n",cmd.cmd);
        break;
      } 

#if (__unix__)
      printf("Listener PID %d issues recv <%d,%d>\n",getpid(),
             the_domain,the_listener_node);
#else  /* !(__unix__) */
      printf("Listener PID %d issues recv <%d,%d>\n",GetCurrentProcessId(),
             the_domain,the_listener_node);
#endif  /* !(__unix__) */
      mcapi_msg_recv_i(listener_recv_endpt, &cmd, sizeof(cmd), 
                       &request, &mcapi_status);

    } else if (mcapi_status == MCAPI_TIMEOUT) {
#if (__unix__)
      printf("Listener PID %d recv timed out <%d,%d>\n",getpid(),
             the_domain,the_listener_node);
#else  /* !(__unix__) */
      printf("Listener PID %d recv timed out <%d,%d>\n",GetCurrentProcessId(),
             the_domain,the_listener_node);
#endif  /* !(__unix__) */
    } else {
      fprintf(stderr,"ERROR: MTAPI listener <%d,%d> failed to recv: %s\n",
              the_domain, the_listener_node, mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
    }

#if (__unix__)
    printf("Listener loop PID %d, recvd=%d\n",getpid(), recvd);
#else  /* !(__unix__) */
    printf("Listener loop PID %d, recvd=%d\n",GetCurrentProcessId(), recvd);
#endif  /* !(__unix__) */

    mthread_yield();
  }

#if (__unix__)
  printf("Listener PID %d shutting down...\n",getpid());
#else  /* !(__unix__) */
  printf("Listener PID %d shutting down...\n",GetCurrentProcessId());
#endif  /* !(__unix__) */

  mcapi_endpoint_delete(listener_send_endpt, &mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: MTAPI failed to delete endpoint: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
  }

  mcapi_endpoint_delete(listener_recv_endpt, &mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: MTAPI failed to delete endpoint: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
  }

  mcapi_finalize(&mcapi_status);
  if (mcapi_status != MCAPI_SUCCESS) {
    fprintf(stderr,"ERROR: MTAPI failed to finalize MCAPI: %s\n",
            mcapi_display_status(mcapi_status,status_buff,sizeof(status_buff)));
  }

#if (__unix__)
  printf("Listener PID %d done\n",getpid());
#else  /* !(__unix__) */
  printf("Listener PID %d done\n",GetCurrentProcessId());
#endif  /* !(__unix__) */

  return NULL;
}



#ifdef __cplusplus
extern } 
#endif /* __cplusplus */
