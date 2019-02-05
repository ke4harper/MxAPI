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

#include <mrapi.h>

extern int debug;
