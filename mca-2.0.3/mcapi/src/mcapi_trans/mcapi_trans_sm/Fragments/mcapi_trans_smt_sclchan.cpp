	// Scalars
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
        mcapi_sclchan_recv_hndl_t recv_handle = 0;
        mcapi_sclchan_send_hndl_t send_handle = 0;
        uint64_t snd_dataword = 0xdeadbeefdeadbeef;
        uint64_t rcv_dataword = 0;
        queue* q = NULL;
        buffer_entry* db_buff = NULL;

        assert(mcapi_trans_endpoint_create(&ep1,port1,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep1,&d,&n,&e1));
        assert(mcapi_trans_endpoint_create(&ep2,port2,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep2,&d,&n,&e2));

        // mcapi_trans_sclchan_connect_i
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        mcapi_trans_connect_channel_have_lock(ep1,ep2,MCAPI_SCL_CHAN);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e1];
        assert(MCAPI_TRUE == entry->connected);
        assert(ep2 == entry->recv_queue.recv_endpt);
        assert(ep1 == entry->recv_queue.send_endpt);
        assert(MCAPI_SCL_CHAN == entry->recv_queue.channel_type);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2];
        assert(MCAPI_TRUE == entry->connected);
        assert(ep1 == entry->recv_queue.send_endpt);
        assert(ep2 == entry->recv_queue.recv_endpt);
        assert(MCAPI_SCL_CHAN == entry->recv_queue.channel_type);
        status = MCAPI_SUCCESS;
        completed = MCAPI_TRUE;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,0,NULL,(mcapi_request_type)CONN_SCLCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        status = MCAPI_SUCCESS;
        mcapi_trans_sclchan_connect_i(ep1,ep2,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_sclchan_recv_open_i
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        mcapi_trans_open_channel_have_lock(d,n,ep2);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2];
        assert(MCAPI_TRUE == entry->open);
        completed = MCAPI_FALSE;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,0,NULL,OPEN_SCLCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == mcapi_db->requests[r].status);
        assert(0 == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(OPEN_SCLCHAN == mcapi_db->requests[r].type);
        assert(ep2 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_sclchan_recv_close_i
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        mcapi_trans_close_channel_have_lock(d,n,ep2);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2];
        assert(MCAPI_FALSE == entry->open);
        completed = MCAPI_FALSE;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,0,NULL,(mcapi_request_type)CLOSE_SCLCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == mcapi_db->requests[r].status);
        assert(0 == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(CLOSE_SCLCHAN == mcapi_db->requests[r].type);
        assert(ep2 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        mcapi_trans_sclchan_recv_open_i(&recv_handle,ep2,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_sclchan_recv_isopen(recv_handle));
        mcapi_trans_sclchan_recv_close_i(recv_handle,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(!mcapi_trans_sclchan_recv_isopen(recv_handle));

        // mcapi_trans_sclchan_send_open_i
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e1];
        entry->open = MCAPI_TRUE;
        completed = MCAPI_FALSE;
        assert(setup_request_have_lock(&ep1,&request,&status,completed,0,NULL,OPEN_SCLCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == mcapi_db->requests[r].status);
        assert(0 == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(OPEN_SCLCHAN == mcapi_db->requests[r].type);
        assert(ep1 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_sclchan_send_close_i
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        mcapi_trans_close_channel_have_lock(d,n,ep1);
        entry = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e1];
        assert(MCAPI_FALSE == entry->open);
        completed = MCAPI_FALSE;
        assert(setup_request_have_lock(&ep1,&request,&status,completed,0,NULL,(mcapi_request_type)CLOSE_SCLCHAN,0,0,0,r));
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_SUCCESS == mcapi_db->requests[r].status);
        assert(0 == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(CLOSE_SCLCHAN == mcapi_db->requests[r].type);
        assert(ep1 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        mcapi_trans_sclchan_send_open_i(&send_handle,ep1,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_sclchan_send_isopen(send_handle));
        mcapi_trans_sclchan_send_close_i(send_handle,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(!mcapi_trans_sclchan_send_isopen(send_handle));

        mcapi_trans_sclchan_recv_open_i(&recv_handle,ep2,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(0 == mcapi_trans_sclchan_available_i(recv_handle));
        mcapi_trans_sclchan_send_open_i(&send_handle,ep1,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        // mcapi_trans_sclchan_send
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        completed = mcapi_trans_send_have_lock(d,n,e1,d,n,e2,NULL,sizeof(snd_dataword),snd_dataword);
        assert(MCAPI_TRUE == completed);
        q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2].recv_queue;
        assert(1 == q->num_elements);
        index = q->elements[0].buff_index &~ MCAPI_VALID_MASK;
        db_buff = &mcapi_db->buffers[index];
        assert(snd_dataword == db_buff->scalar);
        assert(sizeof(snd_dataword) == db_buff->size);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(1 == mcapi_trans_sclchan_available_i(recv_handle));

        assert(mcapi_trans_sclchan_send(send_handle,snd_dataword,sizeof(snd_dataword)));
        assert(2 == mcapi_trans_sclchan_available_i(recv_handle));

        // mcapi_trans_sclchan_recv
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        completed = mcapi_trans_recv_have_lock(d,n,e2,NULL,sizeof(rcv_dataword),&size,MCAPI_TRUE,&rcv_dataword);
        assert(MCAPI_TRUE == completed);
        q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2].recv_queue;
        assert(1 == q->num_elements);
        assert(snd_dataword == rcv_dataword);
        assert(0 == db_buff->magic_num); // buffer has been released
        assert(0 == q->elements[0].buff_index);
        status = MCAPI_SUCCESS;
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(1 == mcapi_trans_pktchan_available(recv_handle));

        assert(mcapi_trans_sclchan_recv(recv_handle,&rcv_dataword,sizeof(rcv_dataword)));
        assert(snd_dataword == rcv_dataword);
        assert(0 == mcapi_trans_sclchan_available_i(recv_handle));

        mcapi_trans_sclchan_send_close_i(send_handle,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        mcapi_trans_sclchan_recv_close_i(recv_handle,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);

        mcapi_trans_endpoint_delete(ep1);
        mcapi_trans_endpoint_delete(ep2);
    }
