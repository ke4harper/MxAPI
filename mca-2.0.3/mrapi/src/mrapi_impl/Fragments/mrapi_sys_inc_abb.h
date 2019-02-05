/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

Port to Windows: #if !(__unix__||__MINGW32__), etc.
*/
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#if (__unix__||__MINGW32__)
#include <unistd.h> /* for memset and getpid */
#include <pthread.h>
#else
#include <tchar.h>
#endif  /* !(__unix__||__MINGW32__) */

#include <mrapi_abb.h>

/* This conditional compile directive enables spinning
   synchronization on Windows that is normally only used
   for Unix, esp. for realtime processes */
#define __atomic_barrier_test__ 0

extern int debug;
