/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

Port to Windows: #if !(__unix__||__MINGW32__), etc.
*/
#include <mcapi_abb.h>
#include <mrapi_abb.h>
#include <mcapi_impl_spec_abb.h>
#include <transport_sm_abb.h>
#include <stdio.h>
#if (__unix__)
#include <unistd.h> /*for sleep*/
#include <sys/ipc.h> /* for ftok */
#else
#include <mrapi_sys_abb.h>
#endif  /* !(__unix__) */
