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

/* atomic sync descriptor */
mrapi_boolean_t sys_atomic_barrier_init(mrapi_atomic_barrier_t* axb,pid_t src,pid_t dest,
    mrapi_msg_t* buffer,unsigned elems,size_t size,unsigned* pindex,mca_timeout_t timeout);
mrapi_boolean_t sys_atomic_exchange_init(mrapi_atomic_barrier_t* axb,pid_t src,pid_t dest,
    mrapi_msg_t* buffer,unsigned elems,size_t size,unsigned* pindex,mca_timeout_t timeout);
mrapi_boolean_t sys_atomic_hold(mrapi_atomic_barrier_t* axb,mrapi_boolean_t hold);
void sys_atomic_override(mrapi_atomic_barrier_t* axb);
/* atomic operations */
mrapi_boolean_t sys_atomic_read(void* sync,void* dest,void* value,size_t size);
mrapi_boolean_t sys_atomic_read_ptr(void* sync,uintptr_t* dest,uintptr_t* value);
mrapi_boolean_t sys_atomic_add(void* sync,void* dest,void* value,void* previous,size_t size);
mrapi_boolean_t sys_atomic_inc(void* sync,void* dest,void* result,size_t size);
mrapi_boolean_t sys_atomic_dec(void* sync,void* dest,void* result,size_t size);
mrapi_boolean_t sys_atomic_or(void* sync,void* dest,void* value,void* previous,size_t size);
mrapi_boolean_t sys_atomic_and(void* sync,void* dest,void* value,void* previous,size_t size);
mrapi_boolean_t sys_atomic_xor(void* sync,void* dest,void* value,void* previous,size_t size);
mrapi_boolean_t sys_atomic_cas(void* sync,void* dest,void* exchange,void* compare,void* previous,size_t size);
void sys_atomic_xchg(void* sync,void* dest,void* exchange,void* previous,size_t size);
mrapi_boolean_t sys_atomic_cas_ptr(void* sync,uintptr_t* dest,uintptr_t exchange,uintptr_t compare,uintptr_t* previous);
void sys_atomic_xchg_ptr(void* sync,uintptr_t* dest,uintptr_t exchange,uintptr_t* previous);
void sys_atomic_sync(void* sync);
mrapi_boolean_t sys_atomic_lock(void* sync,void* dest,void* previous,size_t size);
mrapi_boolean_t sys_atomic_release(void* sync,void* dest,size_t size);
mrapi_boolean_t sys_atomic_set(void* sync,void* dest,int bit,int* previous,size_t size);
mrapi_boolean_t sys_atomic_clear(void* sync,void* dest,int bit,int* previous,size_t size);
mrapi_boolean_t sys_atomic_change(void* sync,void* dest,int bit,int* previous,size_t size);
