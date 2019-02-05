	// Send / receive
	{
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
        char snd_buffer[] = " Hello, world!";
        char rcv_buffer[20] = "";
        void* buffer = NULL;
        queue* q = NULL;
        buffer_entry* db_buff = NULL;

        assert(mcapi_trans_endpoint_create(&ep1,port1,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep1,&d,&n,&e1));
        assert(mcapi_trans_endpoint_create(&ep2,port2,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep1,&d,&n,&e2));

        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_send_have_lock(d,n,e1,d,n,e2,snd_buffer,sizeof(snd_buffer),MCAPI_FALSE));
        q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2].recv_queue;
        index = q->elements[0].buff_index &~ MCAPI_VALID_MASK;
        db_buff = &mcapi_db->buffers[index];
        assert(0 == strcmp(snd_buffer,db_buff->buff));
        buffer = (void*)rcv_buffer;
        assert(mcapi_trans_recv_have_lock(d,n,e2,(void**)&buffer,sizeof(rcv_buffer),&size,MCAPI_TRUE,MCAPI_FALSE));
        assert(0 == strcmp(snd_buffer,rcv_buffer));
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));

        mcapi_trans_endpoint_delete(ep1);
        mcapi_trans_endpoint_delete(ep2);
    }

	// Messages
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
        char snd_buffer[] = " Hello, world!";
        char rcv_buffer[20] = "";
        void* buffer = NULL;
        queue* q = NULL;
        buffer_entry* db_buff = NULL;

        assert(mcapi_trans_endpoint_create(&ep1,port1,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep1,&d,&n,&e1));
        assert(mcapi_trans_endpoint_create(&ep2,port2,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep2,&d,&n,&e2));

        // mcapi_trans_msg_send_i / mcapi_trans_wait
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        snd_buffer[0] = (char)11;
        assert(mcapi_trans_send_have_lock(d,n,e1,d,n,e2,snd_buffer,sizeof(snd_buffer),0));
        q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2].recv_queue;
        assert(1 == q->num_elements);
        index = q->elements[0].buff_index &~ MCAPI_VALID_MASK;
        db_buff = &mcapi_db->buffers[index];
        assert((char)11 == *(db_buff->buff));
        assert(0 == strcmp(&snd_buffer[1],db_buff->buff+1));
        assert(sizeof(snd_buffer) == db_buff->size);
        status = MCAPI_SUCCESS;
        completed = MCAPI_FALSE;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,sizeof(snd_buffer),NULL,SEND,0,0,0,r));
        assert(status == mcapi_db->requests[r].status);
        assert(sizeof(snd_buffer) == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(NULL == mcapi_db->requests[r].buffer);
        assert(SEND == mcapi_db->requests[r].type);
        assert(ep2 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_remove_request_have_lock(r));
        memset(&mcapi_db->requests[r],0,sizeof(mcapi_request_data));
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(1 == mcapi_trans_msg_available(ep2));

        // mcapi_trans_msg_send
        status = MCAPI_SUCCESS;
        snd_buffer[0] = (char)12;
        mcapi_trans_msg_send_i(ep1,ep2,snd_buffer,sizeof(snd_buffer),&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_TRUE == mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert(2 == mcapi_trans_msg_available(ep2));

        snd_buffer[0] = (char)13;
        assert(mcapi_trans_msg_send(ep1,ep2,snd_buffer,sizeof(snd_buffer)));
        assert(3 == mcapi_trans_msg_available(ep2));

        // mcapi_trans_msg_recv_i / mcapi_trans_wait
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        index = q->elements[0].buff_index &~ MCAPI_VALID_MASK;
        db_buff = &mcapi_db->buffers[index];
        buffer = (void*)rcv_buffer; // messsage copied to application buffer
        completed = mcapi_trans_recv_have_lock(d,n,e2,&buffer,sizeof(rcv_buffer),&size,MCAPI_TRUE,NULL);
        assert(MCAPI_TRUE == completed);
        q = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e2].recv_queue;
        assert(2 == q->num_elements);
        assert((char)11 == rcv_buffer[0]);
        assert(0 == strcmp(&snd_buffer[1],&rcv_buffer[1]));
        assert(strlen(&rcv_buffer[1]) == size-2); // size includes terminating NULL
        assert(0 == q->elements[0].buff_index);
        assert(0 == db_buff->magic_num); // buffer has been released
        status = MCAPI_SUCCESS;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,sizeof(rcv_buffer),&buffer,RECV,0,0,0,r));
        assert(status == mcapi_db->requests[r].status);
        assert(sizeof(rcv_buffer) == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(NULL == mcapi_db->requests[r].buffer); // buffer zeroed when complete
        assert(RECV == mcapi_db->requests[r].type);
        assert(ep2 == mcapi_db->requests[r].handle);
        assert(mcapi_trans_remove_request_have_lock(r));
        memset(&mcapi_db->requests[r],0,sizeof(mcapi_request_data));
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(2 == mcapi_trans_msg_available(ep2));

        // mcapi_trans_msg_recv
        status = MCAPI_SUCCESS;
        mcapi_trans_msg_recv_i(ep2,rcv_buffer,sizeof(rcv_buffer),&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_TRUE == mcapi_trans_wait(&request,&size,&status,MCA_INFINITE));
        assert(MCAPI_SUCCESS == status);
        assert((char)12 == rcv_buffer[0]);
        assert(0 == strcmp(&snd_buffer[1],&rcv_buffer[1]));
        assert(strlen(&rcv_buffer[1]) == size-2); // size includes terminating NULL
        assert(1 == mcapi_trans_msg_available(ep2));

        assert(mcapi_trans_msg_recv(ep2,rcv_buffer,sizeof(rcv_buffer),&size));
        assert((char)13 == rcv_buffer[0]);
        assert(0 == strcmp(&snd_buffer[1],&rcv_buffer[1]));
        assert(strlen(&rcv_buffer[1]) == size-2); // size includes terminating NULL
        assert(0 == mcapi_trans_msg_available(ep2));

        mcapi_trans_endpoint_delete(ep1);
        mcapi_trans_endpoint_delete(ep2);
    }
