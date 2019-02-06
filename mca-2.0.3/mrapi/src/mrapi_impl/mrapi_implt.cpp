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
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

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

// mrapi_implt.cpp : Defines the entry point for the console application.
//

#include "Fragments/mrapi_implt_inc.cpp"
#include "Fragments/mrapi_implt_type.cpp"
#include "Fragments/mrapi_implt_declaration.cpp"
#include "Fragments/mrapi_implt_threads.cpp"

#if !(__unix__)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif  // (__unix__)
{
    int num_nodes = 4;
	uint32_t n_index = 0;
	uint32_t d_index = 0;
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

    if(1 >= argc) {
#include "Fragments/mrapi_implt_initialize.cpp"
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

    if(1 >= argc) {
#include "Fragments/mrapi_implt_sem.cpp"
#include "Fragments/mrapi_implt_mutex.cpp"
#include "Fragments/mrapi_implt_rwl.cpp"
#include "Fragments/mrapi_implt_shmem.cpp"
#include "Fragments/mrapi_implt_rmem.cpp"
#include "Fragments/mrapi_implt_request.cpp"
    }
#include "Fragments/mrapi_implt_stress.cpp"

    assert(mrapi_impl_finalize());

	return 0;
}

