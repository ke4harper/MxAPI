// mrapi_implt.cpp : Defines the entry point for the console application.
//

#include "Fragments/mrapi_implt_inc_abb.cpp"
#include "Fragments/mrapi_implt_type_abb.cpp"
#include "Fragments/mrapi_implt_declaration_abb.cpp"
#include "Fragments/mrapi_implt_threads_abb.cpp"

#if !(__unix__)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif  // (__unix__)
{
    int num_nodes = 4;
	uint32_t n_index = 0;
	uint32_t d_index = 0;
    uint32_t p_index = 0;
	mrapi_domain_t d_num = 0;
	mrapi_domain_t d_offset = 0;
	mrapi_node_t n_num = 0;
	mrapi_node_t n_offset = 0;
	mrapi_status_t status = MRAPI_SUCCESS;

    int start = 0;
#if !(__unix__)
    _TCHAR opt = 0;
    _TCHAR* param = NULL;
#else
    char opt = 0;
    char* param = NULL;
#endif  // (__unix__)
    while(argc > start) {
        if(sys_os_getopt(argc, argv, &start, &opt, &param)) {
#if !(__unix__)
            switch(towlower(opt)) {
            case L'd': // -d <domain-offset>
                swscanf_s(param,L"%d",&d_offset);
                break;
            case L'n': // -n <node_offset>
                swscanf_s(param,L"%d",&n_offset);
                break;
            case L's': // -s <num-nodes>
                swscanf_s(param,L"%d",&num_nodes);
                break;
            }
#else
            switch(tolower(opt)) {
            case 'd': // -d <domain-offset>
                sscanf(param,"%d",&d_offset);
                break;
            case 'n': // -n <node_offset>
                sscanf(param,"%d",&n_offset);
                break;
            case 's': // -s <num-nodes>
                sscanf(param,"%d",&num_nodes);
                break;
            }
#endif  // (__unix__)
        }
    }

	mca_set_debug_level(0);

    if(0 == d_offset) { // Avoid race conditions between process initialization and rundown,
#include "Fragments/mrapi_implt_initialize_abb.cpp"
    }

	// Initialize runtime for remainder of tests
	d_num = d_offset + 1;
	n_num = n_offset + 1;
	assert(mrapi_impl_initialize(d_num,n_num,&status));

    // Who am I?
	{
		n_num = mrapi_impl_node_id_get(&status);
		assert(MRAPI_SUCCESS == status);
		assert(n_offset + 1 == n_num);
		assert(mrapi_impl_get_node_num(&n_num));
		assert(mrapi_impl_valid_node_num(n_num));
		assert(n_offset + 1 == n_num);
		assert(mrapi_impl_get_domain_num(&d_num));
		assert(mrapi_impl_valid_domain_num(d_num));
		assert(d_offset + 1 == d_num);
		assert(mrapi_impl_whoami(&n_num,&n_index,&d_num,&d_index));
		assert(n_offset + 1 == n_num);
		assert(d_offset + 1 == d_num);
	}

	// Encode, decode handle
	{
		uint16_t type = 1;
		uint16_t type_index = 0;
		uint32_t hndl = mrapi_impl_encode_hndl(type);
		assert(0 != hndl);
		assert(mrapi_impl_decode_hndl(hndl,&type_index));
		assert(type_index == type);
	}

    if(0 == d_offset) { // Avoid race conditions between process initialization and rundown,
                        // semaphores and shared memory should be created in parent process
#include "Fragments/mrapi_implt_atomic.cpp"
#include "Fragments/mrapi_implt_sem.cpp"
#include "Fragments/mrapi_implt_mutex.cpp"
#include "Fragments/mrapi_implt_rwl.cpp"
#include "Fragments/mrapi_implt_shmem_abb.cpp"
#include "Fragments/mrapi_implt_rmem.cpp"
#include "Fragments/mrapi_implt_request.cpp"
    }
#include "Fragments/mrapi_implt_stress.cpp"

    assert(mrapi_impl_finalize());

	return 0;
}

