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
        int r0 = -1;
        int r1 = -1;
        int r2 = -1;
        indexed_array_header* header = &mcapi_db->request_reserves_header;

        mcapi_trans_init_indexed_array_have_lock();
        assert(MCAPI_MAX_REQUESTS == header->max_count);
        assert(0 == header->curr_count);
        // available list contains all the requests:
        // 0 -> 1 -> 2 -> 3 ...
        assert(0 == header->empty_head_index);
        assert(1 == header->array[0].next_index);
        assert(-1 == header->array[0].prev_index);
        assert(2 == header->array[1].next_index);
        assert(0 == header->array[1].prev_index);
        assert(3 == header->array[2].next_index);
        assert(1 == header->array[2].prev_index);
        assert(4 == header->array[3].next_index);
        assert(2 == header->array[3].prev_index);
        // full list is empty
        assert(-1 == header->full_head_index);
        assert(-1 == header->array[MCAPI_MAX_REQUESTS-1].next_index);
        // etc.

        assert(mcapi_trans_reserve_request_have_lock(&r0));
        assert(0 == r0);
        assert(MCAPI_TRUE == mcapi_db->requests[r0].valid);
        assert(1 == header->curr_count);
        // available list contains remaining requests:
        // 1 -> 2 -> 3 ...
        assert(1 == header->empty_head_index);
        assert(2 == header->array[1].next_index);
        assert(-1 == header->array[1].prev_index);
        assert(3 == header->array[2].next_index);
        assert(1 == header->array[2].prev_index);
        assert(4 == header->array[3].next_index);
        assert(2 == header->array[3].prev_index);
        // etc.
        // full list has one member:
        // 0
        assert(0 == header->full_head_index);
        assert(-1 == header->array[r0].next_index);
        assert(-1 == header->array[r0].prev_index);

        assert(mcapi_trans_reserve_request_have_lock(&r1));
        assert(1 == r1);
        assert(MCAPI_TRUE == mcapi_db->requests[r1].valid);
        assert(2 == header->curr_count);
        // available list contains remaining requests:
        // 2 -> 3 ...
        assert(2 == header->empty_head_index);
        assert(3 == header->array[2].next_index);
        assert(-1 == header->array[2].prev_index);
        assert(4 == header->array[3].next_index);
        assert(2 == header->array[3].prev_index);
        // etc.
        // full list has two members:
        // 1 -> 0
        assert(1 == header->full_head_index);
        assert(r0 == header->array[r1].next_index);
        assert(-1 == header->array[r1].prev_index);
        assert(-1 == header->array[r0].next_index);
        assert(r1 == header->array[r0].prev_index);

        assert(mcapi_trans_reserve_request_have_lock(&r2));
        assert(2 == r2);
        assert(MCAPI_TRUE == mcapi_db->requests[r2].valid);
        assert(3 == header->curr_count);
        // available list contains remaining requests:
        // 3 ...
        assert(3 == header->empty_head_index);
        assert(4 == header->array[3].next_index);
        assert(-1 == header->array[3].prev_index);
        // etc.
        // full list has three members:
        // 2 -> 1 -> 0
        assert(2 == header->full_head_index);
        assert(r1 == header->array[r2].next_index);
        assert(-1 == header->array[r2].prev_index);
        assert(r0 == header->array[r1].next_index);
        assert(r2 == header->array[r1].prev_index);
        assert(-1 == header->array[r0].next_index);
        assert(r1 == header->array[r0].prev_index);

        // remove the middle request
        assert(mcapi_trans_remove_request_have_lock(r1));
        assert(MCAPI_TRUE == mcapi_db->requests[r1].valid); // request is not set to invalid?
        // mcapi_trans_test_i clears request
        memset(&mcapi_db->requests[r1],0,sizeof(mcapi_request_data));
        assert(2 == header->curr_count);
        // available list has r1 as its head:
        // r1 -> 3 ...
        assert(r1 == header->empty_head_index);
        assert(3 == header->array[r1].next_index);
        assert(-1 == header->array[r1].prev_index);
        assert(4 == header->array[3].next_index);
        assert(1 == header->array[3].prev_index);
        // etc.
        // full list has two members:
        // 2 -> 0
        assert(r2 == header->full_head_index);
        assert(0 == header->array[r2].next_index);
        assert(-1 == header->array[r2].prev_index);
        assert(-1 == header->array[r0].next_index);
        assert(2 == header->array[r0].prev_index);

        assert(mcapi_trans_remove_request_have_lock(r2));
        assert(MCAPI_TRUE == mcapi_db->requests[r2].valid); // request is not set to invalid?
        // mcapi_trans_test_i clears request
        memset(&mcapi_db->requests[r2],0,sizeof(mcapi_request_data));
        assert(1 == header->curr_count);
        // available list has r2 as its head:
        // r2 -> r1 -> 3 ...
        assert(2 == header->empty_head_index);
        assert(1 == header->array[r2].next_index);
        assert(-1 == header->array[r2].prev_index);
        assert(3 == header->array[r1].next_index);
        assert(2 == header->array[r1].prev_index);
        assert(4 == header->array[3].next_index);
        assert(1 == header->array[3].prev_index);
        // etc.
        // full list has one member:
        // 0
        assert(0 == header->full_head_index);
        assert(-1 == header->array[r0].next_index);
        assert(-1 == header->array[r0].prev_index);

        assert(mcapi_trans_remove_request_have_lock(r0));
        assert(MCAPI_TRUE == mcapi_db->requests[r0].valid); // request is not set to invalid?
        // mcapi_trans_test_i clears request
        memset(&mcapi_db->requests[r0],0,sizeof(mcapi_request_data));
        assert(0 == header->curr_count);
        // available list has r0 as its head:
        // r0 -> r2 -> r1 -> 3 ...
        assert(0 == header->empty_head_index);
        assert(2 == header->array[r0].next_index);
        assert(-1 == header->array[r0].prev_index);
        assert(1 == header->array[r2].next_index);
        assert(0 == header->array[r2].prev_index);
        assert(3 == header->array[r1].next_index);
        assert(2 == header->array[r1].prev_index);
        assert(4 == header->array[3].next_index);
        assert(1 == header->array[3].prev_index);
        // etc.
        // full list is empty
        assert(-1 == header->full_head_index);
	}
