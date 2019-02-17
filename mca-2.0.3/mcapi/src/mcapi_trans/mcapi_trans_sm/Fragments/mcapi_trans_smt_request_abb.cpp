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
        int r = -1;
        int r1 = -1;
        int r2 = -1;
        int r3 = -1;
        mcapi_requests* requests = &mcapi_rq;
        indexed_array_header* header = &requests->reserves_header;

        mcapi_trans_init_indexed_array();
        assert(MCAPI_MAX_REQUESTS == header->max_count);
        assert(0 == header->curr_count);

        r = mcapi_trans_request_get_index();
        assert(-1 != r);
        assert(mcapi_trans_request_release_index(r));

        assert(mcapi_trans_reserve_request(&r1));
        assert(REQUEST_VALID == requests->data[r1].state);
        assert(1 == header->curr_count);

        assert(mcapi_trans_reserve_request(&r2));
        assert(REQUEST_VALID == requests->data[r2].state);
        assert(2 == header->curr_count);

        assert(mcapi_trans_reserve_request(&r3));
        assert(REQUEST_VALID == requests->data[r3].state);
        assert(3 == header->curr_count);

        // remove the middle request
        requests->data[r2].state = REQUEST_COMPLETED;
        assert(mcapi_trans_remove_request(r2));
        assert(REQUEST_FREE == requests->data[r2].state);
        assert(2 == header->curr_count);

        // remove the first remaining request
        requests->data[r3].state = REQUEST_COMPLETED;
        assert(mcapi_trans_remove_request(r3));
        assert(REQUEST_FREE == requests->data[r3].state);
        assert(1 == header->curr_count);

        // remove the last remaining request
        requests->data[r1].state = REQUEST_COMPLETED;
        assert(mcapi_trans_remove_request(r1));
        assert(REQUEST_FREE == requests->data[r1].state);
        assert(0 == header->curr_count);
	}
