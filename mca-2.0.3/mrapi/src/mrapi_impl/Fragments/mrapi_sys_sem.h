/* semaphore management */
mrapi_boolean_t sys_sem_create(int key,int num_locks,int* semid);
mrapi_boolean_t sys_sem_get(int key,int num_locks,int* semid);
mrapi_boolean_t sys_sem_duplicate(int pproc,int psemid,int* semid);
mrapi_boolean_t sys_sem_lock (int semid,int member);
mrapi_boolean_t sys_sem_trylock (int semid,int member);
mrapi_boolean_t sys_sem_unlock (int semid,int member);
mrapi_boolean_t sys_sem_release(int semid);
mrapi_boolean_t sys_sem_delete(int semid);
