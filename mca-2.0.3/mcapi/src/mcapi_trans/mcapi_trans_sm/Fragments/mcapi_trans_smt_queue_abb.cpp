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

    // Buffers
	{
        buffer_entry* db_buff = &mcapi_db->buffers[0];

        assert(0 == db_buff->magic_num);
        assert(NULL != db_buff->buff);
        assert(0 == db_buff->scalar);
        assert(0 == db_buff->size);
    }

	// Queues
	{
        int qindex = 0;
        int index = 0;
        uint16_t d = 0;
        uint16_t n = 0;
        uint16_t e = 0;
        queue* q = NULL;
        mcapi_endpoint_t ep = 0;
        mcapi_uint_t port = 1;
        buffer_entry* db_buff = NULL;
        mcapi_status_t status;

        assert(mcapi_trans_endpoint_create(&ep,port,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep,&d,&n,&e));
        q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue;
        //print_queue(*q);
        assert(MCAPI_ERR_QUEUE_EMPTY == mcapi_trans_queue_status(q));
        assert(0 == q->state.data.send_endpt);
        assert(0 == q->state.data.recv_endpt);
        assert(0 == q->channel_type);
        assert(0 == mcapi_trans_queue_elements(q));
        assert(0 == q->update_counter);
        assert(0 == q->ack_counter);
        assert(0 == mcapi_trans_queue_head(q,&status));
        assert(MCAPI_ERR_QUEUE_EMPTY == status);
        assert(0 == mcapi_trans_queue_tail(q,&status));
        assert(MCAPI_SUCCESS == status);
        qindex = mcapi_trans_push_queue(q,&status);
        assert(MCAPI_SUCCESS == status);
        assert(0 == qindex);
        assert(1 == mcapi_trans_queue_elements(q));
        assert(2 == q->update_counter);
        assert(0 == q->ack_counter);
        db_buff = &mcapi_db->buffers[0];
        db_buff->magic_num = MAGIC_NUM; // Mark buffer in use
#if !(__unix__)
        strcpy_s(db_buff->buff,sizeof(db_buff->buff),"Hello, world!");
#else
        strcpy(db_buff->buff,"Hello, world!");
#endif  // (__unix__)
        db_buff->size = strlen(db_buff->buff);
        // Buffer index allows sharing across address spaces
        q->elements[qindex].buff_index = 0 | MCAPI_VALID_MASK;
        assert(MCAPI_SUCCESS == mcapi_trans_queue_status(q));
        assert(0 == mcapi_trans_queue_head(q,&status));
        assert(MCAPI_SUCCESS == status);
        assert(1 == mcapi_trans_queue_tail(q,&status));
        assert(MCAPI_SUCCESS == status);
        qindex = mcapi_trans_pop_queue(q,&status);
        assert(0 == qindex);
        assert(0 == mcapi_trans_queue_elements(q));
        assert(2 == q->update_counter);
        assert(2 == q->ack_counter);
        index = q->elements[qindex].buff_index &~ MCAPI_VALID_MASK;
        assert(0 <= index);
        db_buff = &mcapi_db->buffers[index];
        assert(0 == strcmp("Hello, world!",db_buff->buff));
        memset(db_buff,0,sizeof(*db_buff));
        q->elements[qindex].buff_index = 0; // Buffer available
        // Full / empty queue
        assert(MCAPI_ERR_QUEUE_EMPTY == mcapi_trans_queue_status(q));
        assert(1 == mcapi_trans_queue_head(q,&status));
        assert(MCAPI_ERR_QUEUE_EMPTY == status);
        assert(1 == mcapi_trans_queue_tail(q,&status));
        assert(MCAPI_SUCCESS == status);
        for(int i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++)
        {
            assert(MCAPI_ERR_QUEUE_FULL != mcapi_trans_queue_status(q));
            qindex = mcapi_trans_push_queue(q,&status);
            db_buff = &mcapi_db->buffers[qindex];
            db_buff->magic_num = MAGIC_NUM;
            q->elements[qindex].buff_index = qindex | MCAPI_VALID_MASK;
            assert((i+2) % MCAPI_MAX_QUEUE_ENTRIES == mcapi_trans_queue_tail(q,&status));
            assert((i < MCAPI_MAX_QUEUE_ENTRIES-1) ? MCAPI_SUCCESS : MCAPI_ERR_QUEUE_FULL == status);
        }
        assert(-1 == mcapi_trans_push_queue(q,&status));
        assert(MCAPI_ERR_QUEUE_FULL == status);
        assert(MCAPI_ERR_QUEUE_FULL == mcapi_trans_queue_status(q));
        for(int i = 0; i < MCAPI_MAX_QUEUE_ENTRIES; i++)
        {
            assert(MCAPI_ERR_QUEUE_EMPTY != mcapi_trans_queue_status(q));
            qindex = mcapi_trans_pop_queue(q,&status);
            index = q->elements[qindex].buff_index &~ MCAPI_VALID_MASK;
            db_buff = &mcapi_db->buffers[index];
            memset(db_buff,0,sizeof(*db_buff));
            q->elements[qindex].buff_index = 0;
            assert((i+2) % MCAPI_MAX_QUEUE_ENTRIES == mcapi_trans_queue_head(q,&status));
            assert((i < MCAPI_MAX_QUEUE_ENTRIES-1) ? MCAPI_SUCCESS : MCAPI_ERR_QUEUE_EMPTY == status);
        }
        assert(-1 == mcapi_trans_pop_queue(q,&status));
        assert(MCAPI_ERR_QUEUE_EMPTY == status);
        assert(MCAPI_ERR_QUEUE_EMPTY == mcapi_trans_queue_status(q));
        mcapi_trans_endpoint_delete(ep);
    }
