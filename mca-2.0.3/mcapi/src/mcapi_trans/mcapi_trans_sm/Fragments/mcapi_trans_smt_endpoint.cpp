	// Endpoints
	{
        int r = -1;
        uint16_t d = 0;
        uint16_t n = 0;
        uint16_t e = 0;
        size_t size = 0;
        mcapi_endpoint_t ep1 = 0;
        mcapi_endpoint_t ep2 = 0;
        mcapi_uint_t port = 0;
        mcapi_request_t request = 0;
        mcapi_boolean_t completed = MCAPI_FALSE;

		assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));
        port = 1;
        assert(mcapi_trans_endpoint_create(&ep1,port,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep1,&d,&n,&e));
        // Receive queue connectionless by default
        assert(MCAPI_NO_CHAN == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type);
        assert(MCAPI_TRUE == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].valid);
        assert(port == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].port_num);
        assert(MCAPI_FALSE == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].open);
        assert(MCAPI_FALSE == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].anonymous);
        assert(0 == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].num_attributes);
        assert(1 == mcapi_db->domains[d].nodes[n].node_d.num_endpoints);

        // mcapi_trans_endpoint_get
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_endpoint_get_have_lock(&ep2,domain,node,port));
        assert(ep2 == ep1);
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));

        mcapi_trans_endpoint_get(&ep2,domain,node,port);
        assert(ep2 == ep1);

        // mcapi_trans_endpoint_get_i
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        assert(mcapi_trans_reserve_request_have_lock(&r));
        assert(mcapi_trans_endpoint_get_have_lock(&ep2,domain,node,port));
        completed = MCAPI_FALSE;
        assert(setup_request_have_lock(&ep2,&request,&status,completed,0,NULL,GET_ENDPT,node,port,domain,r));
        assert(status == mcapi_db->requests[r].status);
        assert(size == mcapi_db->requests[r].size);
        assert(MCAPI_FALSE == mcapi_db->requests[r].cancelled);
        assert(completed == mcapi_db->requests[r].completed);
        assert(NULL == mcapi_db->requests[r].buffer);
        assert(GET_ENDPT == mcapi_db->requests[r].type);
        assert(ep2 == mcapi_db->requests[r].handle);
        assert(&ep2 == mcapi_db->requests[r].ep_endpoint);
        assert(node == mcapi_db->requests[r].ep_node_num);
        assert(port == mcapi_db->requests[r].ep_port_num);
        assert(domain == mcapi_db->requests[r].ep_domain_num);
        check_get_endpt_request_have_lock(&request);
        assert(MCAPI_TRUE == mcapi_db->requests[r].completed);
        assert(MCAPI_SUCCESS == mcapi_db->requests[r].status);
        assert(mcapi_trans_remove_request_have_lock(r));
        memset(&mcapi_db->requests[r],0,sizeof(mcapi_request_data));
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));

        mcapi_trans_endpoint_get_i(&ep2,domain,node,port,&request,&status);
        assert(MCAPI_SUCCESS == status);
        assert(mcapi_trans_test_i(&request,&size,&status));
        mcapi_trans_endpoint_delete(ep1);
	}

	// Endpoint attributes
	{
        uint16_t d = 0;
        uint16_t n = 0;
        uint16_t e = 0;
        unsigned int attribute = 0;
        mcapi_uint_t port = 1;
        mcapi_endpoint_t ep = 0;

        assert(mcapi_trans_endpoint_create(&ep,port,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep,&d,&n,&e));

        // MCAPI_ENDP_ATTR_TYPE - Endpoint type, message, packet channel or scalar channel
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_TYPE,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        // MCAPI_ENDP_ATTR_GET_COUNT - Number of gets outstanding
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_GET_COUNT,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_GET_COUNT,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        // MCAPI_ENDP_ATTR_GET_SOURCE - Source id (domain, node) of endpoint "getter"
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_GET_SOURCE,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_GET_SOURCE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        // MCAPI_ENDP_ATTR_MAX_PAYLOAD_SIZE - Maximum payload size - Channel compatibility attribute
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_MAX_PAYLOAD_SIZE,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_MAX_PAYLOAD_SIZE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        // MCAPI_ENDP_ATTR_BUFFER_TYPE - Buffer type, FIFO - Channel compatibility attribute
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_BUFFER_TYPE,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_BUFFER_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        // MCAPI_ENDP_ATTR_MEMORY_TYPE - Shared/local (0-copy), blocking or non-blocking on limit - Channel compatibility attribute
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_MEMORY_TYPE,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_MEMORY_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        // MCAPI_ENDP_ATTR_NUM_PRIORITIES - Number of priorities - Channel compatibility attribute
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_NUM_PRIORITIES,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_NUM_PRIORITIES,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        // MCAPI_ENDP_ATTR_PRIORITY - Priority on connected endpoint - Channel compatibility attribute
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_PRIORITY,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_PRIORITY,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        // MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS - Number of send buffers at the current endpoint priority level
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        // MCAPI_ENDP_ATTR_NUM_RECV_BUFFERS - Number of available receive buffers
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_NUM_RECV_BUFFERS,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_NUM_RECV_BUFFERS,&attribute,sizeof(int),&status);
        assert(MCAPI_SUCCESS == status);
        assert(attribute == MCAPI_MAX_QUEUE_ENTRIES -
            mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.num_elements);

        // MCAPI_ENDP_ATTR_ENDP_STATUS - Endpoint status, connected, open etc.
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_ENDP_STATUS,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_ENDP_STATUS,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        // MCAPI_ENDP_ATTR_TIMEOUT - Timeout
        status = -1; // set_attribute not implemented
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_TIMEOUT,&attribute,sizeof(int),&status);
        assert((mcapi_status_t)-1 == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_TIMEOUT,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status);

        mcapi_trans_endpoint_delete(ep);
	}
