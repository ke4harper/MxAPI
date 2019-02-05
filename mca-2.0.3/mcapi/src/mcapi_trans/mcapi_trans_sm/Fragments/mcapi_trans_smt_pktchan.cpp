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

        assert(mcapi_trans_endpoint_create(&ep1,port1,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep1,&d,&n,&e1));
        assert(mcapi_trans_endpoint_create(&ep2,port2,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep2,&d,&n,&e2));

        // mcapi_trans_pktchan_connect_i
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        mcapi_trans_connect_channel_have_lock(ep1,ep2,MCAPI_PKT_CHAN);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e1];
        assert(MCAPI_TRUE == entry->connected);
        assert(ep2 == entry->recv_queue.recv_endpt);
        assert(ep1 == entry->recv_queue.send_endpt);
        assert(MCAPI_PKT_CHAN == entry->recv_queue.channel_type);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2];
        assert(MCAPI_TRUE == entry->connected);
        assert(ep1 == entry->recv_queue.send_endpt);
        assert(ep2 == entry->recv_queue.recv_endpt);
        assert(MCAPI_PKT_CHAN == entry->recv_queue.channel_type);
        status = MCAPI_SUCCESS;
        completed = MCAPI_TRUE;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,0,NULL,(mcapi_request_type)CONN_PKTCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        status = MCAPI_SUCCESS;
        mcapi_trans_pktchan_connect_i(ep1,ep2,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_pktchan_recv_open_i
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        mcapi_trans_open_channel_have_lock(d,n,ep2);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2];
        assert(MCAPI_TRUE == entry->open);
        completed = MCAPI_FALSE;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,0,NULL,OPEN_PKTCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == mcapi_db->requests[r].status);
        assert(0 == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(OPEN_PKTCHAN == mcapi_db->requests[r].type);
        assert(ep2 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_pktchan_recv_close_i
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        mcapi_trans_close_channel_have_lock(d,n,ep2);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2];
        assert(MCAPI_FALSE == entry->open);
        completed = MCAPI_FALSE;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,0,NULL,(mcapi_request_type)CLOSE_PKTCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == mcapi_db->requests[r].status);
        assert(0 == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(CLOSE_PKTCHAN == mcapi_db->requests[r].type);
        assert(ep2 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
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
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e1];
        entry->open = MCAPI_TRUE;
        completed = MCAPI_FALSE;
        assert(setup_request_have_lock(&ep1,&request,&status,completed,0,NULL,OPEN_PKTCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == mcapi_db->requests[r].status);
        assert(0 == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(OPEN_PKTCHAN == mcapi_db->requests[r].type);
        assert(ep1 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_pktchan_send_close_i
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        mcapi_trans_close_channel_have_lock(d,n,ep1);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e1];
        assert(MCAPI_FALSE == entry->open);
        completed = MCAPI_FALSE;
        assert(setup_request_have_lock(&ep1,&request,&status,completed,0,NULL,(mcapi_request_type)CLOSE_PKTCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == mcapi_db->requests[r].status);
        assert(0 == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(CLOSE_PKTCHAN == mcapi_db->requests[r].type);
        assert(ep1 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
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
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        snd_buffer[0] = (char)21;
        completed = mcapi_trans_send_have_lock(d,n,e1,d,n,e2,snd_buffer,sizeof(snd_buffer),0);
        assert(MCAPI_TRUE == completed);
        q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2].recv_queue;
        assert(1 == q->num_elements);
        index = q->elements[0].buff_index &~ MCAPI_VALID_MASK;
        db_buff = &mcapi_db->buffers[index];
        assert((char)21 == *(db_buff->buff));
        assert(0 == strcmp(&snd_buffer[1],db_buff->buff+1));
        assert(sizeof(snd_buffer) == db_buff->size);
        status = MCAPI_SUCCESS;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,sizeof(snd_buffer),NULL,SEND,0,0,0,r));
        assert(status == mcapi_db->requests[r].status);
        assert(sizeof(snd_buffer) == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(NULL == mcapi_db->requests[r].buffer);
        assert(SEND == mcapi_db->requests[r].type);
        assert(ep2 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
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
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        completed = mcapi_trans_recv_have_lock(d,n,e2,&buffer,MCAPI_MAX_PKT_SIZE,&size,MCAPI_FALSE,NULL);
        assert(MCAPI_TRUE == completed);
        q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2].recv_queue;
        assert(2 == q->num_elements);
        assert((char)21 == ((char*)buffer)[0]);
        assert(0 == strcmp(&snd_buffer[1],(char*)buffer+1));
        assert(strlen((char*)buffer+1) == size-2);
        assert(0 != db_buff->magic_num); // buffer has not been released
        assert(0 == q->elements[0].buff_index);
        status = MCAPI_SUCCESS;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,MCAPI_MAX_PKT_SIZE,&buffer,RECV,0,0,0,r));
        assert(status == mcapi_db->requests[r].status);
        assert(MCAPI_MAX_PKT_SIZE == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(NULL == mcapi_db->requests[r].buffer);
        assert(RECV == mcapi_db->requests[r].type);
        assert(ep2 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
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
