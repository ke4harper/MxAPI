typedef enum
{
    MCAPI_MSG_MODE,
    MCAPI_PKTCHAN_MODE,
    MCAPI_SCLCHAN_MODE
} mcapi_test_mode_t;

typedef union
{
    uint64_t buf;
    struct {
        uint32_t tcount;
        uint32_t marker;
    } data;
} mcapi_test_scldata_t;

typedef struct
{
    int bproc;
#if (__unix__)
    pthread_cond_t* pcv;
    pthread_mutex_t* psync;
    int *prun;
#endif  // (__unix__)
    mcapi_uint_t mode;
    mxml_node_t* root;
	mcapi_domain_t domain;
	mcapi_node_t node;
    int affinity;
    int multicore;
    int iteration;
    int sample;
} mcapi_test_args_t;
