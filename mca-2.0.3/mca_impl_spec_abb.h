/*
 * mca_impl_spec.h
 * V1.1, May 2010
 *
 * Note: THE TYPE DEFINTIONS AND TYPES IN THIS FILE ARE REQUIRED.
 * THE SPECIFICE TYPES AND VALUES ARE IMPLEMENTATION DEFINED AS DESCRIBED BELOW.
 * THE TYPES AND VALUES BELOW ARE SPECIFIC TO Poly-Messenger/MCAPI AND INCLUDED TO EXEMPLIFY
 *

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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MCA_IMPL_SPEC_H
#define MCA_IMPL_SPEC_H

#if (__unix__||__MINGW32__)
#define PUBLIC
#else
#define PUBLIC __declspec(dllexport)
#endif  /* !(__unix__||__MINGW32__) */

#include <stdint.h>
#if !(__unix__)
#include <windows.h>
#include <tchar.h>
#include <pdh.h>
#include <pdhmsg.h>
#if !(__MINGW32__)
#define MCA_INLINE
typedef HANDLE pthread_t;
typedef unsigned long sigset_t;
#else
#define MCA_INLINE inline
#endif  /* (__MINGW32__) */

#if (__MINGW32__)
#define	SIG_GET	((__p_sig_fn_t) 2)
#endif  /* (__MINGW32__) */

#if !(__unix__||__MINGW32__)
/* Map signals to bitset */
#define SIG_SETMASK 0	/* set mask with sigprocmask() */
#define SIG_BLOCK 1	/* set of signals to block */
#define SIG_UNBLOCK 2	/* set of signals to, well, unblock */
#endif  /* !(__unix__||__MINGW32__) */

/* Map signals to bitset */
#define sigaddset(what,sig) (*(what) |= (1<<(sig)), 0)
#define sigdelset(what,sig) (*(what) &= ~(1<<(sig)), 0)
#define sigemptyset(what)   (*(what) = 0, 0)
#define sigfillset(what)    (*(what) = ~(0), 0)
#define sigismember(what,sig) (((*(what)) & (1<<(sig))) != 0)
#endif  /* !(__unix__) */

#include "Fragments/mca_impl_spec_inc_abb.h"
#include "Fragments/mca_impl_spec_def_abb.h"

#if !(__unix__)
typedef struct
{
  int dummy;
} siginfo_t;
struct sigaction
{
  void (*sa_handler)(int);
  void (*sa_sigaction)(int, siginfo_t *, void *);
  sigset_t sa_mask;
  int sa_flags;
  void (*sa_restorer)(void);
};
sigset_t* siggetblocked();
int sigprocmask (int how, const sigset_t *set, sigset_t *oset);
#endif  /* !(__unix__) */

#endif /* MCA_IMPL_SPEC_H */

#ifdef __cplusplus
}
#endif /* __cplusplus */
