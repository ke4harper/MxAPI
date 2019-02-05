#define SYNC_BUFFERS 8
#define SYNC_THREADS 8
#define SYNC_PROCESSES 2
#define SPLIT_ITERATIONS 1000

#include "Fragments/mrapi_implt_db_abb.cpp"

typedef enum
{
    MRAPI_TEST_WRITE = 0,
    MRAPI_TEST_READ
} mrapi_test_role_t;

typedef struct
{
    int bproc;
    int affinity;
    int num_nodes;
	mrapi_domain_t domain;
	mrapi_node_t node;
	int mutex_key;
    mrapi_shmem_hndl_t shmem_id;
    mrapi_test_db_t* db;
} mrapi_test_args_t;
