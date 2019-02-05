/* shared memory management */
uint32_t sys_shmem_create(uint32_t shmkey,int sizeOfShmSeg);
uint32_t sys_shmem_get(uint32_t shmkey,int sizeOfShmSeg);
mrapi_boolean_t sys_shmem_duplicate(uint32_t shmid,int tprocid,uint32_t* tshmid);
void* sys_shmem_attach(int shmid);
mrapi_boolean_t sys_shmem_detach(void *shm_address);
mrapi_boolean_t sys_shmem_release(uint32_t shmid);
mrapi_boolean_t sys_shmem_delete(uint32_t shmid);
