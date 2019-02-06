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

#if (__unix__)
  /***************************************************************************
  Function: test_and_set_bit

  Description: Inline assembly access to x86 bts instruction

  Parameters: offset - bit to set, indexed from LSB
              mem - address to be modified

  Returns: Previous bit value

  ***************************************************************************/
  int test_and_set_bit(int offset,volatile void* mem) {
    int old;
	__asm__ volatile (
      "lock; bts %1, (%2)\n\t"
      "setc %%al\n\t"
      "movzb %%al, %0\n\t"
      : "=r" (old)
      : "r" (offset), "r" (mem)
      : "memory", "cc", "%eax");
	return old;
  }

  /***************************************************************************
  Function: test_and_clear_bit

  Description: Inline assembly access to x86 btr instruction

  Parameters: offset - bit to clear, indexed from LSB
              mem - address to be modified

  Returns: Previous bit value

  ***************************************************************************/
  int test_and_clear_bit(int offset,volatile void* mem) {
    int old;
	__asm__ volatile (
      "lock; btr %1, (%2)\n\t"
      "setc %%al\n\t"
      "movzb %%al, %0\n\t"
      : "=r" (old)
      : "r" (offset), "r" (mem)
      : "memory", "cc", "%eax");
	return old;
  }

  /***************************************************************************
  Function: test_and_change_bit

  Description: Inline assembly access to x86 btc instruction

  Parameters: offset - bit to change, indexed from LSB
              mem - address to be modified

  Returns: Previous bit value

  ***************************************************************************/
  int test_and_change_bit(int offset,volatile void* mem) {
    int old;
	__asm__ volatile (
      "lock; btc %1, (%2)\n\t"
      "setc %%al\n\t"
      "movzb %%al, %0\n\t"
      : "=r" (old)
      : "r" (offset), "r" (mem)
      : "memory", "cc", "%eax");
	return old;
  }
#endif  /* (__unix__) */

#if (__unix__||__atomic_barrier_test__)
  /***************************************************************************
  Function: atomic_barrier_mode

  Description: Set barrier mode

  Parameters: sync - sync object

  Returns: None

  ***************************************************************************/
  void atomic_barrier_mode(void* sync,mrapi_atomic_mode_t mode) {
    if(NULL != sync) {
      mrapi_atomic_barrier_t* axb = (mrapi_atomic_barrier_t*)sync;
      axb->mode = mode;
    }
  }

  /***************************************************************************
  Function: try_atomic_barrier_acquire

  Description: Spin waiting for shared memory access in remote process

  Parameters: sync - sync object

  Returns: MRAPI_TRUE on success, MRAPI_FALSE otherwise

  ***************************************************************************/
  mrapi_boolean_t try_atomic_barrier_acquire(void* sync) {
    mrapi_boolean_t rc = MRAPI_TRUE;
    if(NULL != sync) {
      mrapi_atomic_barrier_t* axb = (mrapi_atomic_barrier_t*)sync;
      if(MRAPI_ATOMIC_NONE != axb->mode &&
#if !(__atomic_barrier_test__)
         /* barrier sync only required if destination PID is different than source PID;
            exchange sync required regardless */
		 (axb->xchg ||
          axb->dest != axb->src) &&
#endif  /* !(__atomic_barrier_test__) */
         !axb->sync.active) {
        mca_timeout_t timeout = axb->timeout;
        mrapi_msg_t* elem = NULL;
        unsigned idx = 0;
        if(NULL != axb->sync.pindex) {
          idx = *(axb->sync.pindex);
        }
        assert(axb->elems > idx);
        elem = (mrapi_msg_t*)((char*)axb->buffer + (idx * axb->size));
        if(axb->xchg) {
          /* spin waiting for access;
             override allows multiple operations with the same mode */
          while(!axb->sync.override) {
            mrapi_boolean_t valid;
            sys_atomic_read(NULL,&elem->valid,&valid,sizeof(elem->valid));
            if(!(axb->mode ^ elem->valid)) {
              break;
            }
            else if(MCA_INFINITE != timeout &&
                (0 == timeout ||
                 0 >= --timeout)) {
              rc = MRAPI_FALSE;
              break;
            }
            sys_atomic_sync(NULL);
            sys_os_yield();
          }
          if(axb->sync.override) {
            elem->valid = (mrapi_boolean_t)axb->mode;
          }
        }
        else {
          /* message counter is odd when writing */
          switch(axb->mode) {
          case MRAPI_ATOMIC_NONE:
            break;
          case MRAPI_ATOMIC_READ:
            /* read override could produce corrupted result */
            while(!axb->sync.override) {
              sys_atomic_read(NULL,&elem->counter,&axb->sync.last_counter,sizeof(elem->counter));
              if(0 == axb->sync.last_counter % 2) {
                /* no write in progress */
                break;
              }
              else if(MCA_INFINITE != timeout &&
                  (0 == timeout ||
                   0 >= --timeout)) {
                rc = MRAPI_FALSE;
                break;
              }
              sys_atomic_sync(NULL);
              sys_os_yield();
            }
            break;
          case MRAPI_ATOMIC_WRITE:
            /* write override could produce corrupted result */
            while(!axb->sync.override) {
              uint16_t counter;
              sys_atomic_read(NULL,&elem->counter,&axb->sync.last_counter,sizeof(elem->counter));
              counter = axb->sync.last_counter +1;
              if(1 == (counter = axb->sync.last_counter +1) % 2 &&
                  sys_atomic_cas(NULL,&elem->counter,&counter,&axb->sync.last_counter,NULL,sizeof(elem->counter))) {
                /* no other write in progress */
                axb->sync.last_counter = counter;
                break;
              }
              else if(MCA_INFINITE != timeout &&
                  (0 == timeout ||
                   0 >= --timeout)) {
                /* this cannot happen unless acquire was skipped */
                rc = MRAPI_FALSE;
                break;
              }
              sys_atomic_sync(NULL);
              sys_os_yield();
            }
          }
        }
      }
      axb->sync.active = axb->sync.hold;
    }
    return rc;
  }

  /***************************************************************************
  Function: atomic_barrier_release

  Description: Release ownership of shared memory referenced by sync object

  Parameters: sync - sync object

  Returns: MRAPI_TRUE if access succeeded, MRAPI_FALSE otherwise

  ***************************************************************************/
  mrapi_boolean_t atomic_barrier_release(void* sync) {
    mrapi_boolean_t rc = MRAPI_TRUE;
    if(NULL != sync) {
      mrapi_atomic_barrier_t* axb = (mrapi_atomic_barrier_t*)sync;
      if(MRAPI_ATOMIC_NONE != axb->mode &&
#if !(__atomic_barrier_test__)
         /* barrier sync only required if destination PID is different than source PID;
            exchange sync required regardless */
		 (axb->xchg ||
          axb->dest != axb->src) &&
#endif  /* !(__atomic_barrier_test__) */
         !axb->sync.active) {
        mrapi_msg_t* elem = NULL;
        unsigned idx = 0;
        if(NULL != axb->sync.pindex) {
          idx = *(axb->sync.pindex);
        }
        assert(axb->elems > idx);
        elem = (mrapi_msg_t*)((char*)axb->buffer + (idx * axb->size));
        if(axb->xchg) {
          mrapi_boolean_t valid = !elem->valid;
          /* flip valid status */
          sys_atomic_xchg(NULL,&elem->valid,&valid,NULL,sizeof(elem->valid));
          sys_atomic_sync(NULL);
        }
        else {
          uint16_t counter;
          switch(axb->mode) {
          case MRAPI_ATOMIC_NONE:
            break;
          case MRAPI_ATOMIC_READ:
            sys_atomic_sync(NULL);
            sys_atomic_read(NULL,&elem->counter,&counter,sizeof(elem->counter));
            if(axb->sync.last_counter != counter) {
              /* element was written during attempted read operation */
              rc = MRAPI_FALSE;
            }
            break;
          case MRAPI_ATOMIC_WRITE:
            counter = axb->sync.last_counter +1;
            sys_atomic_sync(NULL);
            if(!sys_atomic_cas(NULL,&elem->counter,&counter,&axb->sync.last_counter,NULL,sizeof(elem->counter))) {
              /* element was written during attempted write operation */
              rc = MRAPI_FALSE;
            }
            else {
              sys_atomic_sync(NULL);
            }
            break;
          }
        }
      }
      axb->sync.override = MRAPI_FALSE;
    }
    return rc;
  }
#endif  /* (__unix__||__atomic_barrier_test__) */

  /***************************************************************************
  Function: sys_atomic_barrier_init

  Description: Initialize sync descriptor for cross-process read/write spinning
    Shared memory is marked as invalid when being written, readers spin waiting
    for valid access until timeout expires.

  Parameters: axb - sync object
              src - owner process ID
              dest - destination process ID
              buffer - address of buffer
              elems - number of buffer elements
			  size - buffer element size
              pindex - address of buffer index
              timeout - read access timeout

  Returns: MRAPI_TRUE on success, MRAPI_FALSE otherwise

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_barrier_init(mrapi_atomic_barrier_t* axb,pid_t src,pid_t dest,
      mrapi_msg_t* buffer,unsigned elems,size_t size,unsigned* pindex,mca_timeout_t timeout) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    if(NULL != axb) {
      memset(axb,0,sizeof(mrapi_atomic_barrier_t));
      axb->mode = MRAPI_ATOMIC_NONE;
      axb->xchg = MRAPI_FALSE;
      axb->timeout = timeout;
      axb->src = src;
      axb->dest = dest;
      axb->buffer = buffer;
      axb->elems = elems;
      axb->size = size;
      axb->sync.pindex = pindex;
      axb->sync.last_counter = 0;
      axb->sync.hold = MRAPI_FALSE;
      rc = MRAPI_TRUE;
    }
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_exchange_init

  Description: Initialize sync descriptor for cross-process exchange spinning
    Shared memory is marked with process ID to indicate ownership. Read and write
    operations transfer ownership to destination after completion.

  Parameters: axb - sync object
              src - owner process ID
              dest - destination process ID
              buffer - address of buffer
              elems - number of buffer elements
			  size - buffer element size
              pindex - address of buffer index
              timeout - read access timeout

  Returns: MRAPI_TRUE on success, MRAPI_FALSE otherwise

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_exchange_init(mrapi_atomic_barrier_t* axb,pid_t src,pid_t dest,
      mrapi_msg_t* buffer,unsigned elems,size_t size,unsigned* pindex,mca_timeout_t timeout) {
    mrapi_boolean_t rc = sys_atomic_barrier_init(axb,src,dest,buffer,elems,size,pindex,timeout);
    if(rc) {
      axb->xchg = MRAPI_TRUE;
    }
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_hold

  Description: Set sync descriptor to reserve access across atomic operations

  Parameters: axb - sync object
              hold - MRAPI_TRUE to reserve, MRAPI_FALSE to clear

  Returns: Previous hold state

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_hold(mrapi_atomic_barrier_t* axb,mrapi_boolean_t hold) {
    mrapi_boolean_t prev = MRAPI_FALSE;
    if(NULL != axb) {
      prev = axb->sync.hold;
      axb->sync.hold = hold;
    }
    return prev;
  }

  /***************************************************************************
  Function: sys_atomic_override

  Description: Set sync descriptor for one-time override allowing selected mode
   operation to complete. Flag is reset after barrier release.

  Parameters: axb - sync object

  Returns: None

  ***************************************************************************/
  void sys_atomic_override(mrapi_atomic_barrier_t* axb) {
    if(NULL != axb) {
      axb->sync.override = MRAPI_TRUE;
    }
  }

  /***************************************************************************
  Function: sys_atomic_read

  Description: Exclusive memory address accessor.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be read
              value - address value
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint8_t, uint16_t, uint32_t, uint64_t
    Unix, minGW - uint8_t, uint16_t, uint32_t, uint64_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_read(void* sync,void* dest,void* value,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    assert(NULL != dest);
    assert(NULL != value);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_READ);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
    case sizeof(uint8_t):
      *(uint8_t*)value = *(uint8_t*)dest;
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      *(uint16_t*)value = *(uint16_t*)dest;
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      *(uint32_t*)value = *(uint32_t*)dest;
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      *(uint64_t*)value = *(uint64_t*)dest;
      rc = MRAPI_TRUE;
      break;
    default:
      printf("sys_atomic_read: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_read_ptr

  Description: Exclusive memory address accessor for pointer type.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be read
              value - address value

  Returns: boolean indicating success or failure

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_read_ptr(void* sync,uintptr_t* dest,uintptr_t* value) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    assert((uintptr_t*)NULL != dest);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_READ);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    *value = *dest;
    rc = MRAPI_TRUE;
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_add

  Description: Exclusive memory address mutator for integer addition.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              value - amount to add
              previous (optional) - original value
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint32_t, uint64_t
    Unix, minGW - uint8_t, uint16_t, uint32_t, uint64_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_add(void* sync,void* dest,void* value,void* previous,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    uint8_t cValue = 0;
    uint16_t wValue = 0;
    uint32_t lValue = 0;
    uint64_t llValue = 0;
    assert(NULL != dest);
    assert(NULL != value);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint8_t):
      cValue = __sync_fetch_and_add((uint8_t*)dest,*(uint8_t*)value);
      if(NULL != previous) {
        *(uint8_t*)previous = cValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      wValue = __sync_fetch_and_add((uint16_t*)dest,*(uint16_t*)value);
      if(NULL != previous) {
        *(uint16_t*)previous = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = __sync_fetch_and_add((uint32_t*)dest,*(uint32_t*)value);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = __sync_fetch_and_add((uint64_t*)dest,*(uint64_t*)value);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#else
    case sizeof(uint32_t):
      lValue = InterlockedExchangeAdd((uint32_t*)dest,*(uint32_t*)value);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = InterlockedExchangeAdd64((uint64_t*)dest,*(uint64_t*)value);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_add: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_inc

  Description: Exclusive memory address mutator for integer increment.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              result (optional) - new value
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint16_t, uint32_t, uint64_t
    Unix, minGW - uint8_t, uint16_t, uint32_t, uint64_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_inc(void* sync,void* dest,void* result,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    uint8_t cValue = 0;
    uint16_t wValue = 0;
    uint32_t lValue = 0;
    uint64_t llValue = 0;
    assert(NULL != dest);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint8_t):
      cValue = __sync_add_and_fetch((uint8_t*)dest,1);
      if(NULL != result) {
        *(uint8_t*)result = cValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      wValue = __sync_add_and_fetch((uint16_t*)dest,1);
      if(NULL != result) {
        *(uint16_t*)result = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = __sync_add_and_fetch((uint32_t*)dest,1);
      if(NULL != result) {
        *(uint32_t*)result = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = __sync_add_and_fetch((uint64_t*)dest,1);
      if(NULL != result) {
        *(uint64_t*)result = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#else
    case sizeof(uint16_t):
      wValue = _InterlockedIncrement16((uint16_t*)dest);
      if(NULL != result) {
        *(uint16_t*)result = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = InterlockedIncrement((uint32_t*)dest);
      if(NULL != result) {
        *(uint32_t*)result = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = InterlockedIncrement64((uint64_t*)dest);
      if(NULL != result) {
        *(uint64_t*)result = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_inc: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_dec

  Description: Exclusive memory address mutator for integer decrement.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              result (optional) - new value
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint16_t, uint32_t, uint64_t
    Unix, minGW - uint8_t, uint16_t, uint32_t, uint64_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_dec(void* sync,void* dest,void* result,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    uint8_t cValue = 0;
    uint16_t wValue = 0;
    uint32_t lValue = 0;
    uint64_t llValue = 0;
    assert(NULL != dest);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint8_t):
      cValue = __sync_sub_and_fetch((uint8_t*)dest,1);
      if(NULL != result) {
        *(uint8_t*)result = cValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      wValue = __sync_sub_and_fetch((uint16_t*)dest,1);
      if(NULL != result) {
        *(uint16_t*)result = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = __sync_sub_and_fetch((uint32_t*)dest,1);
      if(NULL != result) {
        *(uint32_t*)result = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = __sync_sub_and_fetch((uint64_t*)dest,1);
      if(NULL != result) {
        *(uint64_t*)result = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#else
    case sizeof(uint16_t):
      wValue = _InterlockedDecrement16((uint16_t*)dest);
      if(NULL != result) {
        *(uint16_t*)result = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = InterlockedDecrement((uint32_t*)dest);
      if(NULL != result) {
        *(uint32_t*)result = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = InterlockedDecrement64((uint64_t*)dest);
      if(NULL != result) {
        *(uint64_t*)result = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_dec: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_or

  Description: Exclusive memory address mutator for integer bitwise union.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              value - union mask
              previous (optional) - original value
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint8_t, uint16_t, uint32_t, uint64_t
    Unix, minGW - uint8_t, uint16_t, uint32_t, uint64_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_or(void* sync,void* dest,void* value,void* previous,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    uint8_t cValue = 0;
    uint16_t wValue = 0;
    uint32_t lValue = 0;
    uint64_t llValue = 0;
    assert(NULL != dest);
    assert(NULL != value);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint8_t):
      cValue = __sync_fetch_and_or((uint8_t*)dest,*(uint8_t*)value);
      if(NULL != previous) {
        *(uint8_t*)previous = cValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      wValue = __sync_fetch_and_or((uint16_t*)dest,*(uint16_t*)value);
      if(NULL != previous) {
        *(uint16_t*)previous = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = __sync_fetch_and_or((uint32_t*)dest,*(uint32_t*)value);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = __sync_fetch_and_or((uint64_t*)dest,*(uint64_t*)value);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#else
    case sizeof(uint8_t):
      cValue = _InterlockedOr8((uint8_t*)dest,*(uint8_t*)value);
      if(NULL != previous) {
        *(uint8_t*)previous = cValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      wValue = _InterlockedOr16((uint16_t*)dest,*(uint16_t*)value);
      if(NULL != previous) {
        *(uint16_t*)previous = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = _InterlockedOr((uint32_t*)dest,*(uint32_t*)value);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = InterlockedOr64((uint64_t*)dest,*(uint64_t*)value);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_or: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_and

  Description: Exclusive memory address mutator for integer bitwise intersection.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              value - intersection mask
              previous (optional) - original value
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint8_t, uint16_t, uint32_t, uint64_t
    Unix, minGW - uint8_t, uint16_t, uint32_t, uint64_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_and(void* sync,void* dest,void* value,void* previous,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    uint8_t cValue = 0;
    uint16_t wValue = 0;
    uint32_t lValue = 0;
    uint64_t llValue = 0;
    assert(NULL != dest);
    assert(NULL != value);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint8_t):
      cValue = __sync_fetch_and_and((uint8_t*)dest,*(uint8_t*)value);
      if(NULL != previous) {
        *(uint8_t*)previous = cValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      wValue = __sync_fetch_and_and((uint16_t*)dest,*(uint16_t*)value);
      if(NULL != previous) {
        *(uint16_t*)previous = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = __sync_fetch_and_and((uint32_t*)dest,*(uint32_t*)value);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = __sync_fetch_and_and((uint64_t*)dest,*(uint64_t*)value);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#else
    case sizeof(uint8_t):
      cValue = _InterlockedAnd8((uint8_t*)dest,*(uint8_t*)value);
      if(NULL != previous) {
        *(uint8_t*)previous = cValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      wValue = _InterlockedAnd16((uint16_t*)dest,*(uint16_t*)value);
      if(NULL != previous) {
        *(uint16_t*)previous = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = _InterlockedAnd((uint32_t*)dest,*(uint32_t*)value);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = InterlockedAnd64((uint64_t*)dest,*(uint64_t*)value);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_and: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_xor

  Description: Exclusive memory address mutator for integer bitwise exclusive union.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              value - exclusive union mask
              previous (optional) - original value
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint8_t, uint16_t, uint32_t, uint64_t
    Unix, minGW - uint8_t, uint16_t, uint32_t, uint64_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_xor(void* sync,void* dest,void* value,void* previous,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    uint8_t cValue = 0;
    uint16_t wValue = 0;
    uint32_t lValue = 0;
    uint64_t llValue = 0;
    assert(NULL != dest);
    assert(NULL != value);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint8_t):
      cValue = __sync_fetch_and_xor((uint8_t*)dest,*(uint8_t*)value);
      if(NULL != previous) {
        *(uint8_t*)previous = cValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      wValue = __sync_fetch_and_xor((uint16_t*)dest,*(uint16_t*)value);
      if(NULL != previous) {
        *(uint16_t*)previous = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = __sync_fetch_and_xor((uint32_t*)dest,*(uint32_t*)value);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = __sync_fetch_and_xor((uint64_t*)dest,*(uint64_t*)value);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#else
    case sizeof(uint8_t):
      cValue = _InterlockedXor8((uint8_t*)dest,*(uint8_t*)value);
      if(NULL != previous) {
        *(uint8_t*)previous = cValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      wValue = _InterlockedXor16((uint16_t*)dest,*(uint16_t*)value);
      if(NULL != previous) {
        *(uint16_t*)previous = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = _InterlockedXor((uint32_t*)dest,*(uint32_t*)value);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = InterlockedXor64((uint64_t*)dest,*(uint64_t*)value);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_xor: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_cas

  Description: Exclusive memory address mutator for integer compare and exchange.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              exchange - replacement value
              compare - test value
              previous (optional) - original dest value
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint8_t, uint16_t, uint32_t, uint64_t
    Unix, minGW - uint8_t, uint16_t, uint32_t, uint64_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_cas(void* sync,void* dest,void* exchange,void* compare,void* previous,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    uint8_t cComp,cValue = 0;
    uint16_t wComp,wValue = 0;
    uint32_t lComp,lValue = 0;
    uint64_t llComp,llValue = 0;
    assert(NULL != dest);
    assert(NULL != exchange);
    assert(NULL != compare);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint8_t):
      cComp = *(uint8_t*)compare;
      cValue = __sync_val_compare_and_swap((uint8_t*)dest,*(uint8_t*)compare,*(uint8_t*)exchange);
      if(NULL != previous) {
        *(uint8_t*)previous = cValue;
      }
      if(cComp == cValue) {
        rc = MRAPI_TRUE;
      }
      break;
    case sizeof(uint16_t):
      wComp = *(uint16_t*)compare;
      wValue = __sync_val_compare_and_swap((uint16_t*)dest,*(uint16_t*)compare,*(uint16_t*)exchange);
      if(NULL != previous) {
        *(uint16_t*)previous = wValue;
      }
      if(wComp == wValue) {
        rc = MRAPI_TRUE;
      }
      break;
    case sizeof(uint32_t):
      lComp = *(uint32_t*)compare;
      lValue = __sync_val_compare_and_swap((uint32_t*)dest,*(uint32_t*)compare,*(uint32_t*)exchange);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      if(lComp == lValue) {
        rc = MRAPI_TRUE;
      }
      break;
    case sizeof(uint64_t):
      llComp = *(uint64_t*)compare;
      llValue = __sync_val_compare_and_swap((uint64_t*)dest,*(uint64_t*)compare,*(uint64_t*)exchange);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      if(llComp == llValue) {
        rc = MRAPI_TRUE;
      }
      break;
#else
    case sizeof(uint8_t):
      cComp = *(uint8_t*)compare;
      cValue = _InterlockedCompareExchange8((uint8_t*)dest,*(uint8_t*)exchange,*(uint8_t*)compare);
      if(NULL != previous) {
        *(uint8_t*)previous = cValue;
      }
      if(cComp == cValue) {
        rc = MRAPI_TRUE;
      }
      break;
    case sizeof(uint16_t):
      wComp = *(uint16_t*)compare;
      wValue = _InterlockedCompareExchange16((uint16_t*)dest,*(uint16_t*)exchange,*(uint16_t*)compare);
      if(NULL != previous) {
        *(uint16_t*)previous = wValue;
      }
      if(wComp == wValue) {
        rc = MRAPI_TRUE;
      }
      break;
    case sizeof(uint32_t):
      lComp = *(uint32_t*)compare;
      lValue = _InterlockedCompareExchange((uint32_t*)dest,*(uint32_t*)exchange,*(uint32_t*)compare);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      if(lComp == lValue) {
        rc = MRAPI_TRUE;
      }
      break;
    case sizeof(uint64_t):
      llComp = *(uint64_t*)compare;
      llValue = InterlockedCompareExchange64((uint64_t*)dest,*(uint64_t*)exchange,*(uint64_t*)compare);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      if(llComp == llValue) {
        rc = MRAPI_TRUE;
      }
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_cas: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_xchg

  Description: Exclusive memory address mutator for integer exchange.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              exchange - replacement value
              previous (optional) - original value
              size - integer type discriminator

  Supported integer types:

    Windows - uint8_t, uint16_t, uint32_t, uint64_t
    Unix, minGW - uint8_t, uint16_t, uint32_t, uint64_t

  ***************************************************************************/
  void sys_atomic_xchg(void* sync,void* dest,void* exchange,void* previous,size_t size) {
    uint8_t cPrevious = 0;
    uint16_t wPrevious = 0;
    uint32_t lPrevious = 0;
    uint64_t llPrevious = 0;
    switch(size) {
    case sizeof(uint8_t):
      cPrevious = *(uint8_t*)dest;
      (void)sys_atomic_cas(sync,dest,exchange,&cPrevious,&cPrevious,size);
      if(NULL != previous) {
        *(uint8_t*)previous = cPrevious;
      }
      break;
    case sizeof(uint16_t):
      wPrevious = *(uint16_t*)dest;
      (void)sys_atomic_cas(sync,dest,exchange,&wPrevious,&wPrevious,size);
      if(NULL != previous) {
        *(uint16_t*)previous = wPrevious;
      }
      break;
    case sizeof(uint32_t):
      lPrevious = *(uint32_t*)dest;
      (void)sys_atomic_cas(sync,dest,exchange,&lPrevious,&lPrevious,size);
      if(NULL != previous) {
        *(uint32_t*)previous = lPrevious;
      }
      break;
    case sizeof(uint64_t):
      llPrevious = *(uint64_t*)dest;
      (void)sys_atomic_cas(sync,dest,exchange,&llPrevious,&llPrevious,size);
      if(NULL != previous) {
        *(uint64_t*)previous = llPrevious;
      }
      break;
    default:
      printf("sys_atomic_xchg: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
  }

  /***************************************************************************
  Function: sys_atomic_cas_ptr

  Description: Exclusive memory address mutator for pointer compare and exchange.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              exchange - replacement pointer
              comparand - test pointer value
              previous (optional) - original pointer value

  Returns: boolean indicating success or failure

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_cas_ptr(void* sync,uintptr_t* dest,uintptr_t exchange,uintptr_t compare,uintptr_t* previous) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    uintptr_t pComp,pValue = 0;
    assert((uintptr_t*)NULL != dest);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
#if (__unix__||__MINGW32__)
    pComp = (uintptr_t)compare;
    pValue = __sync_val_compare_and_swap(dest,compare,exchange);
    if(NULL != previous) {
      *previous = pValue;
    }
    if(pComp == pValue) {
      rc = MRAPI_TRUE;
    }
#else
    pComp = (uintptr_t)compare;
    pValue = (uintptr_t)InterlockedCompareExchangePointer((PVOID*)dest,(PVOID)exchange,(PVOID)compare);
    if(NULL != previous) {
      *previous = pValue;
    }
    if(pComp == pValue) {
      rc = MRAPI_TRUE;
    }
#endif  /* !(__unix__||__MINGW32__) */
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_xchg_ptr

  Description: Exclusive memory address mutator for pointer exchange.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - pointer to be modified
              exchange - replacement pointer value
              previous (optional) - original pointer value

  ***************************************************************************/
  void sys_atomic_xchg_ptr(void* sync,uintptr_t* dest,uintptr_t exchange,uintptr_t* previous) {
    uintptr_t orig = *dest;
    (void)sys_atomic_cas_ptr(sync,dest,exchange,orig,&orig);
    if(NULL != previous) {
      *previous = orig;
    }
  }

  /***************************************************************************
  Function: sys_atomic_sync

  Description: Full memory barrier, ensure all reads and writes before next access

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)

  Returns: None

  ***************************************************************************/
  void sys_atomic_sync(void* sync) {
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
#if (__unix__||__MINGW32__)
    __sync_synchronize();
#else
    MemoryBarrier();
#endif  /* !(__unix__||__MINGW32__) */
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
  }

  /***************************************************************************
  Function: sys_atomic_lock

  Description: Exclusive memory address mutator for integer exchange. Successful
               operation writes constant 1 to destination. This is an acquire
               barrier, released by sys_atomic_release.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              previous (optional) - original value
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint32_t
    Unix, minGW - uint8_t, uint16_t, uint32_t, uint64_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_lock(void* sync,void* dest,void* previous,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    uint8_t cValue = 0;
    uint16_t wValue = 0;
    uint32_t lValue = 0;
    uint64_t llValue = 0;
    assert(NULL != dest);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint8_t):
      cValue = __sync_lock_test_and_set((uint8_t*)dest,1);
      if(NULL != previous) {
        *(uint8_t*)previous = cValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      wValue = __sync_lock_test_and_set((uint16_t*)dest,1);
      if(NULL != previous) {
        *(uint16_t*)previous = wValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      lValue = __sync_lock_test_and_set((uint32_t*)dest,1);
      if(NULL != previous) {
        *(uint32_t*)previous = lValue;
      }
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      llValue = __sync_lock_test_and_set((uint64_t*)dest,1);
      if(NULL != previous) {
        *(uint64_t*)previous = llValue;
      }
      rc = MRAPI_TRUE;
      break;
#else
    case sizeof(uint32_t):
      if(NULL != previous) {
        *(uint32_t*)previous = *(uint32_t*)dest;
      }
      lValue = InterlockedIncrementAcquire((uint32_t*)dest);
      rc = MRAPI_TRUE;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_lock: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_release

  Description: Exclusive memory address mutator for integer exchange. Successful
               operation writes constant 0 to destination. This is a release
               barrier, releasing the lock obtained by sys_atomic_lock.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              size - integer type discriminator

  Returns: boolean indicating success or failure

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_release(void* sync,void* dest,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    assert(NULL != dest);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint8_t):
      __sync_lock_release((uint8_t*)dest);
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint16_t):
      __sync_lock_release((uint16_t*)dest);
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint32_t):
      __sync_lock_release((uint32_t*)dest);
      rc = MRAPI_TRUE;
      break;
    case sizeof(uint64_t):
      __sync_lock_release((uint64_t*)dest);
      rc = MRAPI_TRUE;
      break;
#else
    case sizeof(uint32_t):
      InterlockedDecrementRelease((uint32_t*)dest);
      rc = MRAPI_TRUE;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_release: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_set

  Description: Exclusive memory address mutator for bit state within integer.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              bit - bit to set, indexed from zero as LSB
              previous (optional) - original state
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint32_t
    Unix, minGW - uint32_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_set(void* sync,void* dest,int bit,int* previous,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    int orig = 0;
    assert(NULL != dest);
    assert(0 <= bit && 8*(int)size > bit);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint32_t):
      orig = test_and_set_bit(bit,dest);
      if(NULL != previous) {
        *previous = orig;
      }
      rc = !orig;
      break;
#else
    case sizeof(uint32_t):
      orig = _bittestandset((uint32_t*)dest,bit);
      if(NULL != previous) {
        *previous = orig;
      }
      rc = !orig;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_set: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_clear

  Description: Exclusive memory address mutator for bit state within integer.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              bit - bit to clear, indexed from zero as LSB
              previous (optional) - original state
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint32_t
    Unix, minGW - uint32_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_clear(void* sync,void* dest,int bit,int* previous,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    int orig = 0;
    assert(NULL != dest);
    assert(0 <= bit && 8*(int)size > bit);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint32_t):
      orig = test_and_clear_bit(bit,(uint32_t*)dest);
      if(NULL != previous) {
        *previous = orig;
      }
      rc = orig;
      break;
#else
    case sizeof(uint32_t):
      orig = _bittestandreset((uint32_t*)dest,bit);
      if(NULL != previous) {
        *previous = orig;
      }
      rc = orig;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_clear: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }

  /***************************************************************************
  Function: sys_atomic_change

  Description: Exclusive memory address mutator for bit state within integer.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              bit - bit to complement, indexed from zero as LSB
              previous (optional) - original state
              size - integer type discriminator

  Returns: boolean indicating success or failure

  Supported integer types:

    Windows - uint32_t
    Unix, minGW - uint32_t

  ***************************************************************************/
  mrapi_boolean_t sys_atomic_change(void* sync,void* dest,int bit,int* previous,size_t size) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    int orig = 0;
    assert(NULL != dest);
    assert(0 <= bit && (int)size > bit);
#if (__unix__||__atomic_barrier_test__)
    atomic_barrier_mode(sync,MRAPI_ATOMIC_WRITE);
    while(1) {
      if(try_atomic_barrier_acquire(sync)) {
#endif  /* (__unix__||__atomic_barrier_test__) */
    switch(size) {
#if (__unix__||__MINGW32__)
    case sizeof(uint32_t):
      orig = test_and_change_bit(bit,(uint32_t*)dest);
      if(NULL != previous) {
        *previous = orig;
      }
      rc = MRAPI_TRUE;
      break;
#else
    case sizeof(uint32_t):
      orig = _bittestandcomplement((uint32_t*)dest,bit);
      if(NULL != previous) {
        *previous = orig;
      }
      rc = MRAPI_TRUE;
      break;
#endif  /* !(__unix__||__MINGW32__) */
    default:
      printf("sys_atomic_change: size %d not supported\n",(int)size);
      assert(MRAPI_FALSE);
    }
#if (__unix__||__atomic_barrier_test__)
      }
      if(atomic_barrier_release(sync)) {
        break;
      }
      sys_os_yield();
    }
#endif  /* (__unix__||__atomic_barrier_test__) */
    return rc;
  }
