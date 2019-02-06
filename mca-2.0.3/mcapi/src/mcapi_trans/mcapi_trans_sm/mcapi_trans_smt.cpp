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
	* Neither the name of ABB, Inc nor the names of its contributors may be
	  used to endorse or promote products derived from this software without
	  specific prior written permission.

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

// mcapi_trans_smt.cpp : Defines the entry point for the console application.
//

#include "Fragments/mcapi_trans_smt_inc.cpp"
#include "Fragments/mcapi_trans_smt_declaration.cpp"
#include "Fragments/mcapi_trans_smt_type.cpp"
#include "Fragments/mcapi_trans_smt_threads.cpp"

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
    mcapi_status_t status = MCAPI_SUCCESS;

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

    xml = argv[1];
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
            case 's': // -m <mode>
                sscanf(param,"%d",&mode);
                break;
            }
#endif  // (__unix__)
        }
    }

	mca_set_debug_level(0);

	domain = d_offset+1;
	node = n_offset+1;

    if(4 >= argc) {
#include "Fragments/mcapi_trans_smt_initialize.cpp"
    }

    //DebugBreak();

	// Initialize runtime for remainder of tests
    assert(mcapi_trans_node_init_attributes(&node_attrs,&status));
	assert(mcapi_trans_initialize(domain,node,&node_attrs));

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
#include "Fragments/mcapi_trans_smt_stress.cpp"

	assert(mcapi_trans_finalize());

	return 0;
}
