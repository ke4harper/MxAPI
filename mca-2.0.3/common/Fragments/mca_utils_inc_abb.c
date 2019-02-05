#include <stdlib.h>
#if (__unix__||__MINGW32__)
#include <pthread.h>
#else
#include <windows.h>
#endif  /* !(__unix__||__MINGW32__) */
#include <inttypes.h>
#include <stdio.h>
#if (__unix__||__MINGW32__)
#include <unistd.h>
#include <string.h>
#endif  /* (__unix__||__MINGW32__) */
#include <stdarg.h>
#include "mca_config_abb.h"
#include <signal.h>
#include "mca_utils_abb.h"
