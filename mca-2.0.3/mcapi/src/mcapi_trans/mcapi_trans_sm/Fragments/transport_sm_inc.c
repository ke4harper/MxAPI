/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

Port to Windows: #if !(__unix__||__MINGW32__), etc.
*/
#include <mcapi.h> /*for cached_domain */
#include <mcapi_trans.h> /* the transport API */
#include "transport_sm.h"
#include <mca_utils.h> /*for crc32 */

#include <string.h> /* for memcpy */
#if (__unix__)
#include <sys/ipc.h> /* for ftok */
#include <pwd.h> /*for get uid */
#endif  /* (__unix__) */
#include <mrapi_sys.h>
#include <assert.h> /* for assertions */
#if (__unix__)
#include <sched.h> /* for sched_yield */
#endif  /* (_unix__) */
#include <stdlib.h> /* for exit */
