#ifdef MRAPI_HAVE_INTTYPES_H
#include <stdint.h>
#endif

#include <inttypes.h>

#include <signal.h>
#if (__unix__)
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <pwd.h>
#endif  /* (__unix__) */
#if (__MINGW32__)
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#endif  /* (__MINGW32__) */
#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if !(__unix__)
#include <windows.h>
#if !(__MINGW32__)
typedef HANDLE pthread_t;
typedef DWORD pid_t;
#endif  /* !(__MINGW32__) */
typedef DWORD uid_t;
#endif  /* !(__unix__) */

#include <mrapi.h>
#include <mca_config.h>
#include <mca_utils.h>
/******************************************************************
           definitions and constants
 ******************************************************************/
#define MRAPI_MAX_SEMS 128  /* we don't currently support different values for max mutex/sem/rwl */
#define MRAPI_MAX_SHMEMS 10
#define MRAPI_MAX_RMEMS 10
#define MRAPI_MAX_REQUESTS MCA_MAX_REQUESTS
#define MRAPI_MAX_SHARED_LOCKS 32

#define MRAPI_RMEM_DEFAULT MRAPI_RMEM_DUMMY

#define mrapi_dprintf mca_dprintf
