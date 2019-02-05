#if !(__unix__)
// Internal semaphore set representation
typedef struct {
  int key;
  int num_locks;
  HANDLE* sem;
} sem_set_t;
#endif  // !(__unix__)

typedef struct
{
	int key;
	int multicore;
	int cpu;
} sem_args_t;

typedef struct
{
    int benable;
    int ncount;
} shmem_sync_t;

typedef struct
{
    shmem_sync_t sync;
    pthread_t tid;
} shmem_thread_t;

typedef struct
{
    mrapi_msg_t msg;  /* must be first member */
    shmem_sync_t sync;
    pid_t pid;
    int nthread;
    int nmode[3];
    shmem_thread_t thread[2];
} shmem_proc_t;

typedef struct
{
    int nprocess;
    shmem_proc_t process[2];
} shmem_db_t;

typedef struct
{
    int proc[2];
    uint32_t shmid[2];
} shmem_xchg_t;

typedef struct
{
    int bproc;
    int nproc;
    int multicore;
    int cpu;
	void* db;
} shmem_args_t;
