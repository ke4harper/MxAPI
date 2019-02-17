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
                                         
#include <mrapi.h>
#include <mca_utils.h>
#include "mrapi_sys.h"
                                                  
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
