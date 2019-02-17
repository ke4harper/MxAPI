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

// Requests
	{
		const int max = 10;
		int i = 0;
		uint32_t r[max] = { 0 };
		mrapi_request_t request[max] = { 0 };

		assert(mrapi_impl_get_domain_num(&d_num));
		assert(mrapi_impl_get_node_num(&n_num));
		for(i = 0; i < max; i++)
		{
			r[i] = mrapi_impl_setup_request();
			assert(MRAPI_TRUE == mrapi_db->requests[r[i]].valid);
			assert(d_num == mrapi_db->requests[r[i]].domain_id);
			assert(n_num == mrapi_db->requests[r[i]].node_num);
			assert(MRAPI_FALSE == mrapi_db->requests[r[i]].cancelled);
			assert(MRAPI_FALSE == mrapi_db->requests[r[i]].completed);
			mrapi_db->requests[r[i]].completed = MRAPI_TRUE;	// Non-blocking MRAPI operations complete immediately?
			request[i] = mrapi_impl_encode_hndl(r[i]);
			assert(0 != request[i]);
			assert(mrapi_impl_valid_request_hndl(&request[i]));
			assert(!mrapi_impl_canceled_request(&request[i]));
			assert(mrapi_impl_test(&request[i],&status));	// Releases request regardless of state
			assert(MRAPI_FALSE == mrapi_db->requests[r[i]].valid);
		}
	}
