/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

Port to Windows: #if !(__unix__||__MINGW32__), etc.
*/
#include <mcapi.h>
#include <mrapi.h>
#include <mcapi_impl_spec.h>
#include <transport_sm.h>
#include <stdio.h>
#if (__unix__)
#include <unistd.h> /*for sleep*/
#include <sys/ipc.h> /* for ftok */
#endif  /* (__unix__) */
#include <mrapi_sys.h>
