/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of ABB, Inc nor the names of its contributors may be
	  used to endorse or promote products derived from this software without
	  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Port to Windows: #if !(__unix__||__MINGW32__), etc.
*/
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#define MRAPI_SEM_OBJ_NAME_LEN 100
#if (__unix__||__MINGW32__)
#include <unistd.h> /* for memset and getpid */
#include <pthread.h>
#define MRAPI_SEM_OBJ_NAME_TEMPLATE "Local\\mca_%u_%u"
#else
#include <tchar.h>
/* Local session prefix allows sharing across processes */
#define MRAPI_SEM_OBJ_NAME_TEMPLATE L"Local\\mca_%u_%u"
#endif  /* !(__unix__||__MINGW32__) */

#include <mrapi_abb.h>

#if !(__unix__)
// Internal semaphore set representation
typedef struct {
	int key;
	int num_locks;
	HANDLE* sem;
} sem_set_t;
#endif  // !(__unix__)

/* This conditional compile directive enables spinning
   synchronization on Windows that is normally only used
   for Unix, esp. for realtime processes */
#define __atomic_barrier_test__ 0

extern int debug;
