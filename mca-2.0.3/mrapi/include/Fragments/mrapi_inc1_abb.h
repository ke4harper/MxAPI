#include <assert.h>
#include <stddef.h>  /* for size_t */

#include <mca_config_abb.h>

#ifdef HAVE_INTTYPES_H 
#include <inttypes.h>
#endif

#if !(__unix__)
#include <windows.h>
#if !(__MINGW32__)
typedef HANDLE pthread_t;
typedef DWORD pid_t;
#endif  /* !(__MINGW32__) */
typedef DWORD uid_t;
#endif  /* !(__unix__) */

#define MRAPI_NO_OWNER (pid_t)0

#include "mca_abb.h"
