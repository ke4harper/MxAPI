/*
Copyright (c) 2012, ABB, Inc
All rights reserved.
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
