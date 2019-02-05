typedef struct
{
    int valid;
    int txn;
    mca_timestamp_t start_xfr;
} mrapi_msg_t;

typedef struct
{
    long iterations;
    double run;
    double rundown;
    double util;
} mrapi_elapsed_t;

typedef struct
{
    int mode;
    int benable;
    int nread;
    int nwrite;
    mca_timestamp_t start;
    mca_timestamp_t start_sync;
    mca_timestamp_t start_read;
    mca_timestamp_t start_write;
    mca_timestamp_t start_rundown;
    mrapi_elapsed_t elapsed;
    mca_cpu_t cpu;
    mrapi_msg_t buffer[SYNC_BUFFERS];
} mrapi_sync_t;

typedef struct
{
    pthread_t tid;
    mrapi_sync_t sync[3];
} mrapi_thread_t;

typedef struct
{
    pid_t pid;
    int nthread;
    int nmode[3];
    mrapi_thread_t thread[SYNC_THREADS];
} mrapi_proc_t;

typedef struct
{
    int nprocess;
    mrapi_proc_t process[SYNC_PROCESSES];
} mrapi_test_db_t;
