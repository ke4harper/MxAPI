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

    // Packets
	{
        int r = -1;
        int index = 0;
        uint16_t d = 0;
        uint16_t n = 0;
        uint16_t e1 = 0;
        uint16_t e2 = 0;
        size_t size = 0;
        mcapi_uint_t port1 = 1;
        mcapi_uint_t port2 = 2;
        mcapi_endpoint_t ep1 = 0;
        mcapi_endpoint_t ep2 = 0;
        mcapi_request_t request = 0;
        mcapi_boolean_t completed = MCAPI_FALSE;
        endpoint_entry* entry = NULL;
        mcapi_pktchan_recv_hndl_t recv_handle = 0;
        mcapi_pktchan_send_hndl_t send_handle = 0;
        char snd_buffer[] = " Hello, world!";
        void* buffer = NULL;
        queue* q = NULL;
        buffer_entry* db_buff = NULL;
        mcapi_requests* requests = &mcapi_rq;

        assert(mcapi_trans_endpoint_create(&ep1,port1,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep1,&d,&n,&e1));
        assert(mcapi_trans_endpoint_create(&ep2,port2,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep2,&d,&n,&e2));

        // mcapi_trans_pktchan_connect_i
        assert(mcapi_trans_reserve_request(&r));
        mcapi_trans_connect_channel(ep1,ep2,MCAPI_PKT_CHAN);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e1];
        assert(MCAPI_TRUE == entry->state.data.connected);
        assert(ep2 == entry->recv_queue.state.data.recv_endpt);
        assert(ep1 == entry->recv_queue.state.data.send_endpt);
        assert(MCAPI_PKT_CHAN == entry->recv_queue.channel_type);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2];
        assert(MCAPI_TRUE == entry->state.data.connected);
        assert(ep1 == entry->recv_queue.state.data.send_endpt);
        assert(ep2 == entry->recv_queue.state.data.recv_endpt);
        assert(MCAPI_PKT_CHAN == entry->recv_queue.channel_type);
        status = MCAPI_SUCCESS;
        completed = MCAPI_TRUE;
        assert(setup_request(&ep2,&request,&status,completed,0,NULL,(mcapi_request_type)CONN_PKTCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        status = MCAPI_SUCCESS;
        mcapi_trans_pktchan_connect_i(ep1,ep2,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_pktchan_recv_open_i
        assert(mcapi_trans_reserve_request(&r));
        mcapi_trans_open_channel(d,n,ep2);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2];
        assert(MCAPI_TRUE == entry->state.data.open);
        completed = MCAPI_FALSE;
        assert(setup_request(&ep2,&request,&status,completed,0,NULL,OPEN_PKTCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == requests->data[r].status);
        assert(0 == requests->data[r].size);
        assert(REQUEST_VALID == requests->data[r].state);
        assert(OPEN_PKTCHAN == requests->data[r].type);
        assert(ep2 == requests->data[r].handle);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_pktchan_recv_close_i
        assert(mcapi_trans_reserve_request(&r));
        mcapi_trans_close_channel(d,n,ep2);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2];
        assert(MCAPI_FALSE == entry->state.data.open);
        completed = MCAPI_FALSE;
        assert(setup_request(&ep2,&request,&status,completed,0,NULL,(mcapi_request_type)CLOSE_PKTCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == requests->data[r].status);
        assert(0 == requests->data[r].size);
        assert(REQUEST_VALID == requests->data[r].state);
        assert(CLOSE_PKTCHAN == requests->data[r].type);
        assert(ep2 == requests->data[r].handle);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        mcapi_trans_pktchan_recv_open_i(&recv_handle,ep2,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_pktchan_recv_isopen(recv_handle));
        mcapi_trans_pktchan_recv_close_i(recv_handle,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(!mcapi_trans_pktchan_recv_isopen(recv_handle));

        // mcapi_trans_pktchan_send_open_i
        assert(mcapi_trans_reserve_request(&r));
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e1];
        entry->state.data.open = MCAPI_TRUE;
        completed = MCAPI_FALSE;
        assert(setup_request(&ep1,&request,&status,completed,0,NULL,OPEN_PKTCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == requests->data[r].status);
        assert(0 == requests->data[r].size);
        assert(REQUEST_VALID == requests->data[r].state);
        assert(OPEN_PKTCHAN == requests->data[r].type);
        assert(ep1 == requests->data[r].handle);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_pktchan_send_close_i
        assert(mcapi_trans_reserve_request(&r));
        mcapi_trans_close_channel(d,n,ep1);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e1];
        assert(MCAPI_FALSE == entry->state.data.open);
        completed = MCAPI_FALSE;
        assert(setup_request(&ep1,&request,&status,completed,0,NULL,(mcapi_request_type)CLOSE_PKTCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == requests->data[r].status);
        assert(0 == requests->data[r].size);
        assert(REQUEST_VALID == requests->data[r].state);
        assert(CLOSE_PKTCHAN == requests->data[r].type);
        assert(ep1 == requests->data[r].handle);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        mcapi_trans_pktchan_send_open_i(&send_handle,ep1,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_pktchan_send_isopen(send_handle));
        mcapi_trans_pktchan_send_close_i(send_handle,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(!mcapi_trans_pktchan_send_isopen(send_handle));

        mcapi_trans_pktchan_recv_open_i(&recv_handle,ep2,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(0 == mcapi_trans_pktchan_available(recv_handle));
        mcapi_trans_pktchan_send_open_i(&send_handle,ep1,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_pktchan_send_i
        assert(mcapi_trans_reserve_request(&r));
        snd_buffer[0] = (char)21;
        completed = mcapi_trans_send(d,n,e1,d,n,e2,snd_buffer,sizeof(snd_buffer),0);
        assert(MCAPI_TRUE == completed);
        q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2].recv_queue;
        assert(1 == mcapi_trans_queue_elements(q));
        index = q->elements[0].buff_index &~ MCAPI_VALID_MASK;
        db_buff = &mcapi_db->buffers[index];
        assert((char)21 == *(db_buff->buff));
        assert(0 == strcmp(&snd_buffer[1],db_buff->buff+1));
        assert(sizeof(snd_buffer) == db_buff->size);
        status = MCAPI_SUCCESS;
        assert(setup_request(&ep2,&request,&status,completed,sizeof(snd_buffer),NULL,SEND,0,0,0,r));
        assert(status == requests->data[r].status);
        assert(sizeof(snd_buffer) == requests->data[r].size);
        assert(REQUEST_COMPLETED == requests->data[r].state);
        assert(NULL == requests->data[r].buffer);
        assert(SEND == requests->data[r].type);
        assert(ep2 == requests->data[r].handle);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(1 == mcapi_trans_pktchan_available(recv_handle));

        // mcapi_trans_pktchan_send
        status = MCAPI_SUCCESS;
        snd_buffer[0] = (char)22;
        mcapi_trans_pktchan_send_i(send_handle,snd_buffer,sizeof(snd_buffer),&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(2 == mcapi_trans_pktchan_available(recv_handle));

        snd_buffer[0] = (char)23;
        assert(mcapi_trans_pktchan_send(send_handle,(void*)snd_buffer,sizeof(snd_buffer)));
        assert(3 == mcapi_trans_pktchan_available(recv_handle));

        // mcapi_trans_pktchan_recv_i
        assert(mcapi_trans_reserve_request(&r));
        completed = mcapi_trans_recv(d,n,e2,&buffer,MCAPI_MAX_PKT_SIZE,&size,MCAPI_FALSE,NULL);
        assert(MCAPI_TRUE == completed);
        q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2].recv_queue;
        assert(2 == mcapi_trans_queue_elements(q));
        assert((char)21 == ((char*)buffer)[0]);
        assert(0 == strcmp(&snd_buffer[1],(char*)buffer+1));
        assert(strlen((char*)buffer+1) == size-2);
        assert(0 != db_buff->magic_num); // buffer has not been released
        assert(0 == q->elements[0].buff_index);
        status = MCAPI_SUCCESS;
        assert(setup_request(&ep2,&request,&status,completed,MCAPI_MAX_PKT_SIZE,&buffer,RECV,0,0,0,r));
        assert(status == requests->data[r].status);
        assert(MCAPI_MAX_PKT_SIZE == requests->data[r].size);
        assert(REQUEST_COMPLETED == requests->data[r].state);
        assert(NULL == requests->data[r].buffer);
        assert(RECV == requests->data[r].type);
        assert(ep2 == requests->data[r].handle);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_pktchan_free((void*)buffer));
        assert(2 == mcapi_trans_pktchan_available(recv_handle));

        // mcapi_trans_pktchan_recv
        status = MCAPI_SUCCESS;
        mcapi_trans_pktchan_recv_i(recv_handle,&buffer,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert((char)22 == ((char*)buffer)[0]);
        assert(0 == strcmp(&snd_buffer[1],(char*)buffer+1));
        assert(strlen((char*)buffer+1) == size-2);
        assert(mcapi_trans_pktchan_free((void*)buffer));
        assert(1 == mcapi_trans_pktchan_available(recv_handle));

        assert(mcapi_trans_pktchan_recv(recv_handle,&buffer,&size));
        assert((char)23 == ((char*)buffer)[0]);
        assert(0 == strcmp(&snd_buffer[1],(char*)buffer+1));
        assert(strlen((char*)buffer+1) == size-2);
        assert(mcapi_trans_pktchan_free((void*)buffer));
        assert(0 == mcapi_trans_pktchan_available(recv_handle));

        mcapi_trans_pktchan_send_close_i(send_handle,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        mcapi_trans_pktchan_recv_close_i(recv_handle,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        mcapi_trans_endpoint_delete(ep1);
        mcapi_trans_endpoint_delete(ep2);
    }
