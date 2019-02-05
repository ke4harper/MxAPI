#if !(__unix__)
#include "stdafx.h"
#endif  // !(__unix__)
#include <mrapi_sys_abb.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void atomic_barrier_mode(void* sync,mrapi_atomic_mode_t mode);
mrapi_boolean_t try_atomic_barrier_acquire(void* sync);
mrapi_boolean_t atomic_barrier_release(void* sync);

#ifdef __cplusplus
} 
#endif /* __cplusplus */

typedef struct
{
  uint8_t cValue;
  uint16_t wValue;
  uint32_t lValue;
  uint64_t llValue;
  unsigned index;
  mrapi_msg_t msg[4];
} shmem_data_t;
