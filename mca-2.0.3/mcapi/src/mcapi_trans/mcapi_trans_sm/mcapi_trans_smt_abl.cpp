// mcapi_trans_smt_abb.cpp : Defines the entry point for the console application.
//

#include "Fragments/mcapi_trans_smt_inc_abl.cpp"
#include "Fragments/mcapi_trans_smt_declaration.cpp"
#include "Fragments/mcapi_trans_smt_type_abb.cpp"
#include "Fragments/mcapi_trans_smt_threads_abl.cpp"

#if !(__unix__)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif  // (__unix__)
{
#if (__unix||__MINGW32__)
    int rc = 0;
#endif  // (__unix__)||(__MINGW32__)
    mcapi_uint_t mode = 0x7;
	uint32_t n_index = 0;
	uint32_t d_index = 0;
	mcapi_domain_t d_id = 0;
	mcapi_domain_t domain = 0;
	mrapi_node_t n_id = 0;
	mrapi_node_t node = 0;
	mrapi_domain_t d_offset = 0;
	mrapi_node_t n_offset = 0;
    mcapi_node_attributes_t node_attrs = { { { 0 } } };
    mcapi_status_t status = MCAPI_FALSE;

    int start = 0;
#if !(__unix__)
    _TCHAR opt = 0;
    _TCHAR* param = NULL;
    _TCHAR* xml = NULL;
#else
    char opt = 0;
    char* param = NULL;
    char* xml = NULL;
#endif  // (__unix__)

    if(1 < argc) {
        xml = argv[1];
    }
    else {
        printf("missing xml file\n");
        exit(-1);
    }

    start = 1;
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
            case L'm': // -m <mode>
                swscanf_s(param,L"%x",&mode);
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
            case 'm': // -m <mode>
                sscanf(param,"%x",&mode);
                break;
            }
#endif  // (__unix__)
        }
    }

	mca_set_debug_level(0);

#if (__unix__)
	printf("test %s (%d %d), mode 0x%x\n",xml,d_offset,n_offset,mode);
#else
    wprintf(L"test %s (%d %d), mode 0x%x\n",xml,d_offset,n_offset,mode);
#endif  // !(__unix__)

	domain = d_offset+1;
	node = n_offset+1;

    if(4 >= argc) {
#include "Fragments/mcapi_trans_smt_initialize_abl.cpp"
    }

    //DebugBreak();

	// Initialize runtime for remainder of tests
    assert(mcapi_trans_node_init_attributes(&node_attrs,&status));
	assert(mcapi_trans_initialize(domain,node,&node_attrs,MCAPI_TRUE));

	// Who am I?
	{
		assert(mcapi_trans_get_node_num(&n_id));
		assert(mcapi_trans_valid_node(n_id));
        assert(n_id == node);
		assert(mcapi_trans_get_domain_num(&d_id));
        assert(d_id == domain);
		assert(mcapi_trans_whoami(&n_id,&n_index,&d_id,&d_index));
		assert(n_id == node);
		assert(d_id == domain);
	}

	// Encode, decode handle
	{
        uint16_t n = 0;
        uint16_t d = 0;
		uint16_t endpoint = 1;
		uint16_t endpoint_index = 0;

		assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));
		uint32_t hndl = mcapi_trans_encode_handle(d_index,n_index,endpoint);
		assert(0 != hndl);
		assert(mcapi_trans_decode_handle(hndl,&d,&n,&endpoint_index));
		assert(endpoint_index == endpoint);
	}

    if(4 >= argc) {
#include "Fragments/mcapi_trans_smt_request.cpp"
#include "Fragments/mcapi_trans_smt_endpoint.cpp"
#include "Fragments/mcapi_trans_smt_queue.cpp"
#include "Fragments/mcapi_trans_smt_msg.cpp"
#include "Fragments/mcapi_trans_smt_pktchan.cpp"
#include "Fragments/mcapi_trans_smt_sclchan.cpp"
    }
#include "Fragments/mcapi_trans_smt_stress_abl.cpp"

	assert(mcapi_trans_finalize());

	return 0;
}
