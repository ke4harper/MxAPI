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

Port to Windows: #if !(__unix||__MINGW32__), etc.

*/
#include <mca_utils.h>
#include <mrapi.h>

char status_buff[MRAPI_MAX_STATUS_SIZE];
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>
#include <signal.h>

#ifndef NODE
#define NODE 1
#endif

#ifndef DOMAIN
#define DOMAIN 1
#endif


#define MAX_SIGNALS 24
// Stores old signal handlers for propagation purposes.
struct sigaction signals [MAX_SIGNALS];
int rc;

void block_signals() {
  sigset_t block_alarm;
  
  /* Initialize the signal mask. */
  sigemptyset (&block_alarm);
#if (__unix)
  sigaddset (&block_alarm, SIGALRM);
#endif  /* (__unix) */
  sigaddset (&block_alarm, SIGINT);
#if (__unix)
  sigaddset (&block_alarm, SIGHUP);
#endif  /* (__unix) */
  sigaddset (&block_alarm, SIGILL);
  sigaddset (&block_alarm, SIGSEGV);
  sigaddset (&block_alarm, SIGTERM);
  sigaddset (&block_alarm, SIGFPE);
  sigaddset (&block_alarm, SIGABRT);

  sigprocmask (SIG_BLOCK, &block_alarm, NULL);
  
}

void unblock_signals() {
  sigset_t block_alarm;

  /* Initialize the signal mask. */
  sigemptyset (&block_alarm);
#if (__unix)
  sigaddset (&block_alarm, SIGALRM);
#endif  /* (__unix) */
  sigaddset (&block_alarm, SIGINT);
#if (__unix)
  sigaddset (&block_alarm, SIGHUP);
#endif  /* (__unix) */
  sigaddset (&block_alarm, SIGILL);
  sigaddset (&block_alarm, SIGSEGV);
  sigaddset (&block_alarm, SIGTERM);
  sigaddset (&block_alarm, SIGFPE);
  sigaddset (&block_alarm, SIGABRT);
  
  sigprocmask (SIG_UNBLOCK, &block_alarm, NULL);
}


void signal_handler ( int sig )
{
  struct sigaction new_action, old_action;
  
  block_signals();
  
  /* restore the old action */
  new_action = signals[sig];
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  
#if (__unix)
  sigaction (sig, NULL, &old_action);
#else
  old_action.sa_handler = signal(sig,SIG_GET);
#endif  /* !(__unix) */
  new_action = signals[sig];
  if (old_action.sa_handler != SIG_IGN)
#if (__unix)
    sigaction (sig, &new_action, NULL);
#else
      (void)signal(sig,new_action.sa_handler);
#endif  /* !(__unix) */
  
  rc = 1;
  
  /* print info on which signal was caught */
  printf("\nsig1: received signal, freeing semaphore and shared memory\n");
  switch (sig) {
#if (__unix)
  case (SIGALRM):{printf( "\nsignal_handler: app caught SIGALRM\n"); }
    break;
#endif  /* (__unix) */
  case (SIGINT):{ printf( "\nsignal_handler: app caught SIGINT\n"); }
    break;
#if (__unix)
  case (SIGHUP):{ printf( "\nsignal_handler: app caught SIGHUP\n"); }
    break;
#endif  /* (__unix) */
  case (SIGILL):{ printf( "\nsignal_handler: app caught SIGILL\n"); }
    break;
  case (SIGSEGV):{printf( "\nsignal_handler: app caught SIGSEGV\n"); }
    break;
  case (SIGTERM):{printf( "\nsignal_handler: app caught SIGTERM\n"); }
    break;
  case (SIGFPE):{ printf( "\nsignal_handler: app caught SIGFPE\n"); }
    break;
  case (SIGABRT):{printf( "\nsignal_handler: app aught SIGABRT\n"); }
    break;
  default: { printf( "\nsignal_handler: app caught unknown signal\n"); }
    break;
  };

  unblock_signals();

  if (rc) {
    printf("PASSED");
    
    raise(sig);
    //exit(0);
  }
}


void register_handlers() {
    
  /* register signal handlers so that we can still clean up resources
     if an interrupt occurs
     http://www.gnu.org/software/libtool/manual/libc/Sigaction-Function-Example.html
  */
  struct sigaction new_action, old_action;
  
  /* Set up the structure to specify the new action. */
  new_action.sa_handler = signal_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  
#if (__unix)
  sigaction (SIGINT, NULL, &old_action);
#else
  old_action.sa_handler = signal(SIGINT,SIG_GET);
#endif  /* !(__unix) */
  signals[SIGINT] = old_action;
  if (old_action.sa_handler != SIG_IGN)
#if (__unix)
    sigaction (SIGINT, &new_action, NULL);
#else
    (void)signal(SIGINT,new_action.sa_handler);
#endif  /* !(__unix) */
  
#if (__unix)
  sigaction (SIGHUP, NULL, &old_action);
  signals[SIGHUP] = old_action;
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGHUP, &new_action, NULL);
#endif  /* (__unix) */
  
#if (__unix)
  sigaction (SIGILL, NULL, &old_action);
#else
  old_action.sa_handler = signal(SIGILL,SIG_GET);
#endif  /* !(__unix) */
  signals[SIGILL] = old_action;
  if (old_action.sa_handler != SIG_IGN)
#if (__unix)
    sigaction (SIGILL, &new_action, NULL);
#else
    (void)signal(SIGILL,new_action.sa_handler);
#endif  /* !(__unix) */
  
#if (__unix)
  sigaction (SIGSEGV, NULL, &old_action);
#else
  old_action.sa_handler = signal(SIGSEGV,SIG_GET);
#endif  /* !(__unix) */
  signals[SIGSEGV] = old_action;
  if (old_action.sa_handler != SIG_IGN)
#if (__unix)
    sigaction (SIGSEGV, &new_action, NULL);
#else
    (void)signal(SIGSEGV,new_action.sa_handler);
#endif  /* !(__unix) */
  
#if (__unix)
  sigaction (SIGTERM, NULL, &old_action);
#else
  old_action.sa_handler = signal(SIGTERM,SIG_GET);
#endif  /* !(__unix) */
  signals[SIGTERM] = old_action;
  if (old_action.sa_handler != SIG_IGN)
#if (__unix)
    sigaction (SIGTERM, &new_action, NULL);
#else
    (void)signal(SIGTERM,new_action.sa_handler);
#endif  /* !(__unix) */
  
#if (__unix)
  sigaction (SIGFPE, NULL, &old_action);
#else
  old_action.sa_handler = signal(SIGFPE,SIG_GET);
#endif  /* !(__unix) */
  signals[SIGFPE] = old_action;
  if (old_action.sa_handler != SIG_IGN)
#if (__unix)
    sigaction (SIGFPE, &new_action, NULL);
#else
    (void)signal(SIGFPE,new_action.sa_handler);
#endif  /* !(__unix) */
  
#if (__unix)
  sigaction (SIGABRT, NULL, &old_action);
#else
  old_action.sa_handler = signal(SIGABRT,SIG_GET);
#endif  /* !(__unix) */
  signals[SIGABRT] = old_action;
  if (old_action.sa_handler != SIG_IGN)
#if (__unix)
    sigaction (SIGABRT, &new_action, NULL);
#else
    (void)signal(SIGABRT,new_action.sa_handler);
#endif  /* !(__unix) */
}

int main () {

  mca_status_t status;
  mrapi_info_t version;
  mrapi_parameters_t parms = 0;

  mrapi_set_debug_level(6);

 
  /* create a node */
  mrapi_initialize(DOMAIN,NODE,parms,&version,&status);
  if (status != MRAPI_SUCCESS) {
    fprintf(stderr,"FAILED 1: mrapi_initialize NODE=%i, status=%s",NODE,mrapi_display_status(status,status_buff,sizeof(status_buff)));
    return -1;
  }
  

  /* register my handlers after calling mrapi_initialize */
  register_handlers();


  raise(SIGINT);
  
  
 
 return 0;
}

