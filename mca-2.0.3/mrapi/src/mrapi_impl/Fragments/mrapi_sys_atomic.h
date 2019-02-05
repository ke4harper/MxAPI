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
