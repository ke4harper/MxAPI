/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

Port to Windows: #if !(__unix__||__MINGW32__), etc.
*/

#if (__unix__)
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#endif  /* (__unix__) */
#include <sys/types.h>
#include <ctype.h>
#include <signal.h> /* for signals */
#if (__MINGW32__)
#define	SIG_GET	((__p_sig_fn_t) 2)
#endif  /* (__MINGW32__) */
                                         
#include <mrapi_abb.h>
#include <mca_utils_abb.h>
#include "mrapi_sys_abb.h"
                                                  
#if (__unix__)
#include <pwd.h>
#endif  /* (__unix__) */
#include <stdlib.h>
#include <string.h>
#if (__unix__||__MINGW32__)
#include <unistd.h>
#endif  /* (__unix__||__MINGW32__) */
#include <errno.h>
#if (__unix__||__MINGW32__)
#include <pthread.h> /* for yield */
#endif  /* (__unix__||__MINGW32__) */

/* Good System V IPC reference: 
  http://beej.us/guide/bgipc/output/html/multipage/semaphores.html#semsamp
*/
