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
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

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

*/

/***************************************************************************
  Function: sys_os_getopt

  Description:

  Parameters:

  Returns: boolean indicating success or failure

 ***************************************************************************/
#if !(__unix__)
  mrapi_boolean_t sys_os_getopt(int argc, _TCHAR* argv[], int* start, _TCHAR* opt, _TCHAR** param) {
#else
  mrapi_boolean_t sys_os_getopt(int argc, char* argv[], int* start, char* opt, char** param) {
#endif  // (__unix__)
    mrapi_boolean_t rc = MRAPI_FALSE;
    int next = 0;
    for(next = *start; next < argc; next++) {
      if(L'-' == argv[next][0]) {
        *opt = argv[next++][1];
        *param = argv[next++];
        rc = MRAPI_TRUE;
        break;
      }
    }
    *start = next;
    return rc;
  }

  /***************************************************************************
  Function: sys_os_yield

  Description:

  Parameters:

  Returns: None

 ***************************************************************************/
  void sys_os_yield(void) {
#if !(__unix__)
    SleepEx(0,0);
#else
    sched_yield();
#endif  /* (__unix__) */
  }

  /***************************************************************************
  Function: sys_os_usleep

  Description:

  Parameters:

  Returns: None

 ***************************************************************************/
  void sys_os_usleep(int usec) {
#if !(__unix__)
    SleepEx(usec/1000,0);
#else
    usleep(usec);
#endif  /* (__unix__) */
  }

    /***************************************************************************
  Function: sys_os_srand

  Description: Seed pseudo random number generator

  Parameters:
  seed - seed for random number generation

  Returns: None

 ***************************************************************************/
  void sys_os_srand(unsigned int seed) {
#if !(__unix__)
    srand(seed);
#else
    srand(seed);
#endif  /* (__unix__) */
  }

  /***************************************************************************
  Function: sys_os_rand

  Description: Generate pseudo random number 0 <= r <= RAND_MAX

  Parameters: None

  Returns: Generated random number

 ***************************************************************************/
  int sys_os_rand(void) {
#if !(__unix__)
    return rand();
#else
    return rand();
#endif  /* (__unix__) */
  }
