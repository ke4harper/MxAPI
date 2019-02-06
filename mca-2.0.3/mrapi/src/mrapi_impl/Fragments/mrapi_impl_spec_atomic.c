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

Port to Windows: #if !(__unix__||__MINGW32__), etc.
Atomic operations
*/
/***************************************************************************
  Function: mrapi_impl_atomic_forward

  Description: Synchronize atomic operation in remote processes

  Parameters: s - shared memory index
              op - atomic operation and parameters
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_NOFORWARD	The atomic operation was not forwarded to other processes.

  Note: Windows shared memory supports atomic operations across processes

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_forward(uint16_t s,mrapi_atomic_op *op,mrapi_status_t* status) {
    mrapi_boolean_t rc = MRAPI_FALSE;
    int p = 0;
    int lock = 1, released = 0;
    int previous = 0;

    *status = MRAPI_ERR_ATOM_OP_NOFORWARD;
    op->spindex = mrapi_pindex;
    op->shmem = s;
    op->valid = MRAPI_TRUE;

    /* Validate operation for platform */
#if (__unix__)
    switch(op->type) {
    case MRAPI_ATOM_SHMDUP:
      break;
    default:
      return rc;
      break;
    }
#else
    switch(op->type) {
    case MRAPI_ATOM_OPENPROC:
    case MRAPI_ATOM_CLOSEPROC:
    case MRAPI_ATOM_SHMDUP:
      break;
    default:
      return rc;
      break;
    }
#endif  /* !(__unix__) */

    /* Fire events */
    switch(op->type) {
    case MRAPI_ATOM_OPENPROC:
    case MRAPI_ATOM_CLOSEPROC:
      for(p = 0; p < MRAPI_MAX_PROCESSES; p++) {
        if(mrapi_pindex != p &&
            mrapi_db->processes[p].state.data.valid) {

          mrapi_dprintf(2,"mrapi_impl_atomic_forward (%d /*s*/,%d /*op->type*/); %d /*target*/",
                        s,op->type,mrapi_db->processes[p].state.data.pid);

          /* Spin in this process while operation is in use */
          while(sys_atomic_cas(NULL,&mrapi_db->processes[p].op.valid,&lock,&released,&previous,
                               sizeof(mrapi_db->processes[p].op.valid)) && previous) {
            sys_os_yield();
          }

          mrapi_db->processes[p].op = *op;

          /* Signal remote process */
#if (__unix__)
          kill(mrapi_db->processes[p].state.data.pid,SIGUSR1);
#else
          SetEvent(mrapi_atomic_evt[p]);
#endif  /* !(__unix__) */

          /* Force memory synchronization */
          sys_atomic_sync(NULL);

          switch(op->type) {
          case MRAPI_ATOM_OPENPROC:
          case MRAPI_ATOM_CLOSEPROC:
            /* Spin waiting for link update */
            while((op->type - MRAPI_ATOM_OPENPROC) == mrapi_db->processes[p].link[mrapi_pindex]) {
              sys_os_yield();
            }
            break;
          default:
            break;
          }
        }
      }
      rc = MRAPI_TRUE;
      break;
    case MRAPI_ATOM_SHMDUP:
      p = op->u.dup.source;
      if(mrapi_pindex != p) {

        mrapi_dprintf(2,"mrapi_impl_atomic_forward (%d /*s*/,%d /*op->type*/); %d /*target*/",
                      s,op->type,mrapi_db->processes[p].state.data.pid);

        /* spin until source process is valid */
        while(!mrapi_db->processes[p].state.data.valid) {
          sys_os_yield();
        }

        /* Spin in this process while operation is in use */
        while(sys_atomic_cas(NULL,&mrapi_db->processes[p].op.valid,&lock,&released,&previous,
                             sizeof(mrapi_db->processes[p].op.valid)) && previous) {
          sys_os_yield();
        }

        mrapi_db->processes[p].op = *op;

        /* Signal remote process */
#if (__unix__)
        kill(mrapi_db->processes[p].state.data.pid,SIGUSR1);
#else
        SetEvent(mrapi_atomic_evt[p]);
#endif  /* !(__unix__) */

        /* Force memory synchronization */
        sys_atomic_sync(NULL);
      }
      rc = MRAPI_TRUE;
      break;
    default:

      mrapi_dprintf(2,"mrapi_impl_atomic_forward (%d /*s*/,%d /*op->type*/);",
                    s,op->type);

      return rc;
    }

    return rc;
  }

/***************************************************************************
  Function: mrapi_impl_atomic_barrier_init

  Description: Initialize sync descriptor for cross-process read/write spinning
    Shared memory is marked as invalid when being written, readers spin waiting
    for valid access until timeout expires.

  Parameters: axb - sync object
              dest - destination process ID
              buffer - address of buffer
              elems - number of buffer elements
              pindex - address of buffer index
              timeout - read access timeout

  Returns: MRAPI_TRUE on success, MRAPI_FALSE otherwise

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_barrier_init(mrapi_atomic_barrier_t* axb,pid_t dest,
    mrapi_msg_t* buffer,unsigned elems,size_t size,unsigned* pindex,mca_timeout_t timeout) {
    return sys_atomic_barrier_init(axb,mrapi_pid,dest,buffer,elems,size,pindex,timeout);
  }

/***************************************************************************
  Function: mrapi_impl_atomic_exchange_init

  Description: Initialize sync descriptor for cross-process exchange spinning
    Shared memory is marked with process ID to indicate ownership. Read and write
    operations transfer ownership to destination after completion.

  Parameters: axb - sync object
              dest - destination process ID
              buffer - address of buffer
              elems - number of buffer elements
              pindex - address of buffer index
              timeout - read access timeout

  Returns: MRAPI_TRUE on success, MRAPI_FALSE otherwise

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_exchange_init(mrapi_atomic_barrier_t* axb,pid_t dest,
    mrapi_msg_t* buffer,unsigned elems,size_t size,unsigned* pindex,mca_timeout_t timeout) {
    return sys_atomic_exchange_init(axb,mrapi_pid,dest,buffer,elems,size,pindex,timeout);
  }

/***************************************************************************
  Function: mrapi_impl_atomic_hold

  Description: Set sync descriptor to reserve access across atomic operations

  Parameters: axb - sync object
              hold - MRAPI_TRUE to reserve, MRAPI_FALSE to clear

  Returns: Previous hold state

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_hold(mrapi_atomic_barrier_t* axb,mrapi_boolean_t hold) {
    return sys_atomic_hold(axb,hold);
  }

/***************************************************************************
  Function: mrapi_impl_atomic_read

  Description: Exclusive memory address accessor.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be read
              value - destination value
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_read(void* sync,void* dest,void* value,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_read(sync,dest,value,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_read_ptr

  Description: Exclusive memory address accessor for pointer.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be read
              value - pointer value
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_read_ptr(void* sync,uintptr_t* dest,uintptr_t* value,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_read_ptr(sync,dest,value);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

/***************************************************************************
  Function: mrapi_impl_atomic_add

  Description: Exclusive memory address mutator for integer addition.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              value - amount to add
              previous (optional) - original value
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_add(void* sync,void* dest,void* value,void* previous,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_add(sync,dest,value,previous,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_inc

  Description: Exclusive memory address mutator for integer increment.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              result (optional) - new value
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_inc(void* sync,void* dest,void* result,size_t size,mrapi_status_t* status){
    mrapi_boolean_t rc = sys_atomic_inc(sync,dest,result,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_dec

  Description: Exclusive memory address mutator for integer decrement.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              result (optional) - new value
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_dec(void* sync,void* dest,void* result,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_dec(sync,dest,result,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_or

  Description: Exclusive memory address mutator for integer bitwise union.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              value - union mask
              previous (optional) - original value
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_or(void* sync,void* dest,void* value,void* previous,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_or(sync,dest,value,previous,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_and

  Description: Exclusive memory address mutator for integer bitwise intersection.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              value - union mask
              previous (optional) - original value
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_and(void* sync,void* dest,void* value,void* previous,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_and(sync,dest,value,previous,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_xor

  Description: Exclusive memory address mutator for integer bitwise exclusive union.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              value - union mask
              previous (optional) - original value
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_xor(void* sync,void* dest,void* value,void* previous,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_xor(sync,dest,value,previous,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_cas

  Description: Exclusive memory address mutator for integer compare and exchange.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              exchange - replacement value
              compare - test value
              previous - original dest value
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_cas(void* sync,void* dest,void* exchange,void* compare,void* previous,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_cas(sync,dest,exchange,compare,previous,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_xchg

  Description: Exclusive memory address mutator for integer exchange.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              exchange - replacement value
              previous (optional) - original value
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_xchg(void* sync,void* dest,void* exchange,void* previous,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = MRAPI_TRUE;
    (void)sys_atomic_xchg(sync,dest,exchange,previous,size);
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_cas_ptr

  Description: Exclusive memory address mutator for pointer compare and exchange.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              exchange - replacement pointer
              comparand - test pointer value
              previous - original pointer value
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_cas_ptr(void* sync,uintptr_t* dest,uintptr_t exchange,uintptr_t compare,uintptr_t* previous,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_cas_ptr(sync,dest,exchange,compare,previous);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_xchg_ptr

  Description: Exclusive memory address mutator for pointer exchange.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              exchange - replacement pointer value
              previous (optional) - original pointer value
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_xchg_ptr(void* sync,uintptr_t* dest,uintptr_t exchange,uintptr_t* previous,mrapi_status_t* status) {
    mrapi_boolean_t rc = MRAPI_TRUE;
    (void)sys_atomic_xchg_ptr(sync,dest,exchange,previous);
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_sync

  Description: Full memory barrier, ensure all reads and writes before next access

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)

  Returns: None

  Errors:

  ***************************************************************************/
  void mrapi_impl_atomic_sync(void* sync) {
    sys_atomic_sync(sync);
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_lock

  Description: Exclusive memory address mutator for integer exchange. Successful
               operation writes constant 1 to destination. This is an acquire
               barrier, released by sys_atomic_release.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              previous (optional) - original value
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_lock(void* sync,void* dest,void* previous,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_lock(sync,dest,previous,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_release

  Description: Exclusive memory address mutator for integer exchange. Successful
               operation writes constant 0 to destination. This is a release
               barrier, releasing the lock obtained by sys_atomic_lock.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_release(void* sync,void* dest,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_release(sync,dest,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_set

  Description: Exclusive memory address mutator for bit state within integer.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              bit - bit to set, indexed from zero as LSB
              previous (optional) - original state
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_set(void* sync,void* dest,int bit,int* previous,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_set(sync,dest,bit,previous,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_clear

  Description: Exclusive memory address mutator for bit state within integer.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              bit - bit to clear, indexed from zero as LSB
              previous (optional) - original state
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_clear(void* sync,void* dest,int bit,int* previous,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_clear(sync,dest,bit,previous,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_atomic_change

  Description: Exclusive memory address mutator for bit state within integer.

  Parameters: sync (optional) - sync descriptor for cross-process (non-Windows)
              dest - address to be modified
              bit - bit to complement, indexed from zero as LSB
              previous (optional) - original state
              size - integer type discriminator
              status - status code if failure

  Returns: boolean indicating success or failure

  Errors:
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_atomic_change(void* sync,void* dest,int bit,int* previous,size_t size,mrapi_status_t* status) {
    mrapi_boolean_t rc = sys_atomic_change(sync,dest,bit,previous,size);
    if(!rc) {
      *status = MRAPI_ERR_ATOM_OP_FAILED;
    }
    return rc;
  }
