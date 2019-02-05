/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

Atomic operations
*/
/************************************************************************
mrapi_barrier_init

DESCRIPTION
This function initializes the structure used to synchronize atomic
operations across processes, supporting spinning on non-Windows platforms.
Barrier spinning works between writers and readers and is only necessary if
the writer and reader are in different processes. The dest PID specifies
the remote process ID. The buffer member references application shared
memory that is organized as an array of entries (possibly only one), where
each entry has mrapi_msg_t as the first element of its structure. The array
size is elems, and the element size is size. The counter is a reference to
an atomic counter that controls which of a finite set of buffers is used
for the next read or write. The timeout determines how long spinning should
wait before failing. These structures form the basis for lock-free data
exchange in the MCAPI layer.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.

NOTE

***********************************************************************/
void mrapi_barrier_init(
    MRAPI_OUT mrapi_atomic_barrier_t* axb,
    MRAPI_IN pid_t dest,
    MRAPI_OUT mrapi_msg_t* buffer,
    MRAPI_IN unsigned elems,
    MRAPI_IN size_t size,
    MRAPI_OUT unsigned* counter,
    MRAPI_IN mca_timeout_t timeout,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_barrier_init(axb,dest,buffer,elems,size,counter,timeout)) {
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_exchange_init

DESCRIPTION
This function initializes the structure used to synchronize atomic
operations across processes, supporting spinning on non-Windows platforms.
Exchange spinning works between a single writer and reader and is only
necessary if the writer and reader are in different processes. The dest
PID specifies the remote process ID. The buffer member references
application shared memory that is organized as an array of entries
(possibly only one), where each entry has mrapi_msg_t as the first element
of its structure. The array size is elems, and the element size is size.
The counter is a reference to an atomic counter that controls which of a 
finite set of buffers is used for the next read or write. The timeout
determines how long spinning should wait before failing. These structures
form the basis for lock-free data exchange in the MCAPI layer.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.

NOTE

***********************************************************************/
void mrapi_exchange_init(
    MRAPI_OUT mrapi_atomic_barrier_t* axb,
    MRAPI_IN pid_t dest,
    MRAPI_OUT mrapi_msg_t* buffer,
    MRAPI_IN unsigned elems,
    MRAPI_IN size_t size,
    MRAPI_OUT unsigned* counter,
    MRAPI_IN mca_timeout_t timeout,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_exchange_init(axb,dest,buffer,elems,size,counter,timeout)) {
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_hold

DESCRIPTION
This function sets whether barrier or exchange synchronization should 
persist across atomic operations, for example updating a complex
structure before allowing read access by a remote process.

RETURN VALUE
The previous hold state for the barrier.

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.

NOTE

***********************************************************************/
mrapi_boolean_t mrapi_atomic_hold(
    MRAPI_OUT mrapi_atomic_barrier_t* axb,
    MRAPI_IN mrapi_boolean_t hold,
    MRAPI_OUT mrapi_status_t* status)
{
  mrapi_boolean_t rc = MRAPI_FALSE;
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else {
    rc = mrapi_impl_atomic_hold(axb,hold);
    *status = MRAPI_SUCCESS;
  }
  return rc;
}

/************************************************************************
mrapi_atomic_read

DESCRIPTION
This function accesses integer values in shared memory. The operation is
valid for memory locations within shared memory, where the synchronization
can be across real-time processes. Different integer widths are supported
based on the platform. The desired size is passed as input and returned
status indicates if the operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_read(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* value,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_read(sync,(void*)dest,(void*)value,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_read_ptr

DESCRIPTION
This function accesses pointer values. The operation is valid for memory
locations within shared memory, where the synchronization can be across
real-time processes. The pointer value at the destination address is
returned. Returned status indicates if the operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_read_ptr(
    MRAPI_OUT void* sync,
    MRAPI_IN uintptr_t* dest,
    MRAPI_OUT uintptr_t* value,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_read_ptr(sync,(uintptr_t*)dest,value,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_add

DESCRIPTION
This function performs atomic integer addition. With a negative addend
value, subtraction is also possible. The operation is only valid for
memory locations within shared memory, where the synchronization can be
across real-time processes. The integer value at destination address
is incremented by the specified integer value and the previous integer
value returned if that argument is non-NULL. Different integer widths
are supported based on the platform. The desired size is passed as input
and returned status indicates if the operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_add(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* value,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_add(sync,(void*)dest,(void*)value,previous,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_inc

DESCRIPTION
This function performs atomic integer increment. Atomic decrement is
provided by mrapi_atomic_dec. The operation is only valid for
memory locations within shared memory, where the synchronization can be
across real-time processes. The integer value at destination address is
incremented by one and the new integer result returned if that argument
is non-NULL. Different integer widths are supported based on the platform.
The desired size is passed as input and returned status indicates if the
operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_inc(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_OUT void* result,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_inc(sync,(void*)dest,result,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_dec

DESCRIPTION
This function performs atomic integer decrement. Atomic increment is
provided by mrapi_atomic_inc. The operation is only valid for
memory locations within shared memory, where the synchronization can be
across real-time processes. The integer value at destination address is
decremented by one and the new integer result returned if that argument
is non-NULL. Different integer widths are supported based on the platform.
The desired size is passed as input and returned status indicates if the
operation succeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_dec(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_OUT void* result,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_dec(sync,(void*)dest,result,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_or

DESCRIPTION
This function performs atomic integer bit-wise union. Atomic bit-wise
intersection is provided by mrapi_atomic_and, and atomic bit-wise
exclusive or is provided by mrapi_atomic_xor. The operation is only valid
for memory locations within shared memory, where the synchronization can be
across real-time processes. The integer value at destination address is
OR'ed with value and the new integer result returned if that argument is
non-NULL. Different integer widths are supported based on the platform.
The desired size is passed as input and returned status indicates if the
operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_or(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* value,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_or(sync,(void*)dest,(void*)value,previous,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_and

DESCRIPTION
This function performs atomic integer bit-wise intersection. Atomic bit-wise
union is provided by mrapi_atomic_or, and atomic bit-wise
exclusive or is provided by mrapi_atomic_xor. The operation is only valid
for memory locations within shared memory, where the synchronization can be
across real-time processes. The integer value at destination address is
AND'ed with value and the new integer result returned if that argument is
non-NULL. Different integer widths are supported based on the platform.
The desired size is passed as input and returned status indicates if the
operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_and(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* value,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_or(sync,(void*)dest,(void*)value,previous,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_xor

DESCRIPTION
This function performs atomic integer bit-wise exclusive or. Atomic bit-wise
union is provided by mrapi_atomic_or, and atomic bit-wise intersection
is provided by mrapi_atomic_and. The operation is only valid for memory
locations within shared memory, where the synchronization can be
across real-time processes. The integer value at destination address is
XOR'ed with value and the new integer result returned if that argument is
non-NULL. Different integer widths are supported based on the platform.
The desired size is passed as input and returned status indicates if the
operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_xor(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* value,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_or(sync,(void*)dest,(void*)value,previous,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_cas

DESCRIPTION
This function performs atomic integer compare and swap. Atomic pointer
compare and swap is provided by mrapi_atomic_cas_ptr. The operation is
only valid for memory locations within shared memory, where the
synchronization can be across real-time processes. The integer value at
destination address is compared with compare, and if they match then
replaced with exchange. The previous value is returned if that argument is
non-NULL. Different integer widths are supported based on the platform.
The desired size is passed as input and returned status indicates if the
operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_cas(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* exchange,
    MRAPI_IN void* compare,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_cas(sync,(void*)dest,(void*)exchange,(void*)compare,previous,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_xchg

DESCRIPTION
This function performs atomic integer exchange. Atomic pointer exchange
is provided by mrapi_atomic_xchg_ptr. The operation is only valid for
memory locations within shared memory, where the synchronization can be
across real-time processes. The integer value at destination address is
replaced by exchange. The previous value is returned if that argument is
non-NULL. Different integer widths are supported based on the platform.
The desired size is passed as input and returned status indicates if the
operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_xchg(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN void* exchange,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_xchg(sync,(void*)dest,(void*)exchange,previous,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_cas_ptr

DESCRIPTION
This function performs atomic pointer compare and swap. Atomic integer
compare and swap is provided by mrapi_atomic_cas. The operation is
only valid for memory locations within shared memory, where the
synchronization can be across real-time processes. The pointer value at
destination address is compared with compare, and if they match then
replaced with exchange. The compare and exchange arguments can be NULL,
otherwise they must be addresses within the shared memory. The previous
value is returned if that argument is non-NULL. Returned status indicates
if the operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_cas_ptr(
    MRAPI_OUT void* sync,
    MRAPI_IN uintptr_t* dest,
    MRAPI_IN uintptr_t exchange,
    MRAPI_IN uintptr_t compare,
    MRAPI_OUT uintptr_t* previous,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_cas_ptr(sync,(uintptr_t*)dest,exchange,compare,previous,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_xchg_ptr

DESCRIPTION
This function performs atomic pointer exchange. Atomic integer exchange
is provided by mrapi_atomic_xchg. The operation is only valid for
memory locations within shared memory, where the synchronization can be
across real-time processes. The pointer value at destination address is
replaced by exchange. The exchange argument can be NULL, otherwise it
must be an address within the shared memory. The previous value is
returned if that argument is non-NULL. Returned status indicates if the
operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_xchg_ptr(
    MRAPI_OUT void* sync,
    MRAPI_IN uintptr_t* dest,
    MRAPI_IN uintptr_t exchange,
    MRAPI_OUT uintptr_t* previous,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_xchg_ptr(sync,(uintptr_t*)dest,exchange,previous,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_sync

DESCRIPTION
This function performs a full memory barrier, flushing all memory reads
and writes before returning. The operation is synchronized across real-
time processes that are connected to the shared memory.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_sync(
    MRAPI_OUT void* sync,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else {
      mrapi_impl_atomic_sync(sync);
      *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_lock

DESCRIPTION
This function performs integer exchange for the purposes of implementing
an atomic lock. Atomic release is provided by mrapi_atomic_release. The
operation is only valid for memory locations within shared memory, where
the synchronization can be across real-time processes. The integer value
at destination address is written with constant 1 and the previous value
returned if that argument is non-NULL. This is an acquire barrier that
does not modify the destination until the integer value is 0. Different
integer widths are supported based on the platform. The desired size is
passed as input and returned status indicates if the operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_lock(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_OUT void* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_lock(sync,(void*)dest,previous,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_release

DESCRIPTION
This function performs integer exchange for the purposes of implementing
an atomic lock. Atomic lock is provided by mrapi_atomic_lock. The
operation is only valid for memory locations within shared memory, where
the synchronization can be across real-time processes. The integer value
at destination address is written with constant 0. This is an release
barrier that does not modify the destination unless it was locked by the
same thread. Different integer widths are supported based on the platform.
The desired size is passed as input and returned status indicates if the
operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_release(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_release(sync,(void*)dest,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_set

DESCRIPTION
This function performs atomic bit set within an integer. Atomic bit clear
is provided by mrapi_atomic_clear, and atomic bit change is provided by
mrapi_atomic_change. The operation is only valid for memory locations
within shared memory, where the synchronization can be across real-time
processes. The integer value at destination address is modified to set
the specified bit, indexed from 0, to be 1 and the previous bit value
returned if that argument is non-NULL. Different integer widths are
supported based on the platform. The desired integer size is passed as
input and returned status indicates if the operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_set(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN int bit,
    MRAPI_OUT int* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_set(sync,(void*)dest,bit,previous,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_clear

DESCRIPTION
This function performs atomic bit clear within an integer. Atomic bit set
is provided by mrapi_atomic_set, and atomic bit change is provided by
mrapi_atomic_change. The operation is only valid for memory locations
within shared memory, where the synchronization can be across real-time
processes. The integer value at destination address is modified to clear
the specified bit, indexed from 0, to be 0 and the previous bit value
returned if that argument is non-NULL. Different integer widths are
supported based on the platform. The desired integer size is passed as
input and returned status indicates if the operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_clear(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN int bit,
    MRAPI_OUT int* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_clear(sync,(void*)dest,bit,previous,size,status)){
    *status = MRAPI_SUCCESS;
  }
}

/************************************************************************
mrapi_atomic_change

DESCRIPTION
This function performs atomic bit change within an integer. Atomic bit
clear is provided by mrapi_atomic_clear, and atomic bit change is provided
by mrapi_atomic_change. The operation is only valid for memory locations
within shared memory, where the synchronization can be across real-time
processes. The integer value at destination address is modified to flip
the specified bit, indexed from 0, to be it complement and the previous
bit value returned if that argument is non-NULL. Different integer widths
are supported based on the platform. The desired integer size is passed as
input and returned status indicates if the operation succeeded.

RETURN VALUE
On success *status is set to MRAPI_SUCCESS. On error, *status is set to
the appropriate error defined below. 

ERRORS
MRAPI_ERR_NODE_NOTINIT      The calling node is not initialized.
MRAPI_ERR_ATOM_OP_FAILED    The local atomic operation failed

NOTE

***********************************************************************/
void mrapi_atomic_change(
    MRAPI_OUT void* sync,
    MRAPI_IN void* dest,
    MRAPI_IN int bit,
    MRAPI_OUT int* previous,
    MRAPI_IN size_t size,
    MRAPI_OUT mrapi_status_t* status)
{
  if  (!mrapi_impl_initialized()) {
    *status = MRAPI_ERR_NODE_NOTINIT;
  } else if (mrapi_impl_atomic_change(sync,(void*)dest,bit,previous,size,status)){
    *status = MRAPI_SUCCESS;
  }
}
