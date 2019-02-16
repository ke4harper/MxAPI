#if (__unix__||__MINGW32__)
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <bits/types/sigset_t.h>
#include <bits/sigaction.h>
#else
#endif  /* !(__unix__||__MINGW32__ */
#include <assert.h>
#include <stdarg.h>
#include "mca_config_abb.h"
