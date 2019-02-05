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
        mcapi_requests* requests = &mcapi_rq;

		assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));
        port = 1;
        assert(mcapi_trans_endpoint_create(&ep1,port,MCAPI_FALSE));
        assert(mcapi_trans_decode_handle(ep1,&d,&n,&e));
        // Receive queue connectionless by default
        assert(MCAPI_NO_CHAN == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type);
        assert(MCAPI_TRUE == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.valid);
        assert(port == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.port_num);
        assert(MCAPI_FALSE == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.open);
        assert(MCAPI_FALSE == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].anonymous);
        assert(0 == mcapi_db->domains[d].nodes[n].node_d.endpoints[e].num_attributes);
        assert(1 == mcapi_db->domains[d].nodes[n].node_d.num_endpoints);

        // mcapi_trans_endpoint_get_
        assert(mcapi_trans_endpoint_get_(&ep2,domain,node,port));
        assert(ep2 == ep1);

        mcapi_trans_endpoint_get(&ep2,domain,node,port);
        assert(ep2 == ep1);

        // mcapi_trans_endpoint_get_i
        assert(mcapi_trans_reserve_request(&r));
        assert(mcapi_trans_endpoint_get_(&ep2,domain,node,port));
        completed = MCAPI_FALSE;
        assert(setup_request(&ep2,&request,&status,completed,0,NULL,GET_ENDPT,node,port,domain,r));
        assert(status == requests->data[r].status);
        assert(size == requests->data[r].size);
        assert(REQUEST_VALID == requests->data[r].state);
        assert(NULL == requests->data[r].buffer);
        assert(GET_ENDPT == requests->data[r].type);
        assert(ep2 == requests->data[r].handle);
        assert(&ep2 == requests->data[r].ep_endpoint);
        assert(node == requests->data[r].ep_node_num);
        assert(port == requests->data[r].ep_port_num);
        assert(domain == requests->data[r].ep_domain_num);
        check_get_endpt_request(&request);
        assert(REQUEST_COMPLETED == requests->data[r].state);
        assert(MCAPI_SUCCESS == requests->data[r].status);
        assert(mcapi_trans_remove_request(r));
        memset(&requests->data[r],0,sizeof(mcapi_request_data));

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
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // type set by create policy
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_NO_CHAN == attribute);
        assert(MCAPI_SUCCESS == status);

        // MCAPI_ENDP_ATTR_GET_COUNT - Number of gets outstanding
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_GET_COUNT,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_GET_COUNT,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented

        // MCAPI_ENDP_ATTR_GET_SOURCE - Source id (domain, node) of endpoint "getter"
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_GET_SOURCE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_GET_SOURCE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented

        // MCAPI_ENDP_ATTR_MAX_PAYLOAD_SIZE - Maximum payload size - Channel compatibility attribute
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_MAX_PAYLOAD_SIZE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_MAX_PAYLOAD_SIZE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented

        // MCAPI_ENDP_ATTR_BUFFER_TYPE - Buffer type, FIFO, STATE - Channel compatibility attribute
        attribute = MCAPI_ENDP_ATTR_FIFO_BUFFER;
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_BUFFER_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_SUCCESS == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_BUFFER_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_ENDP_ATTR_FIFO_BUFFER == attribute);
        assert(MCAPI_SUCCESS == status);
        attribute = MCAPI_ENDP_ATTR_STATE_BUFFER;
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_BUFFER_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_SUCCESS == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_BUFFER_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_ENDP_ATTR_STATE_BUFFER == attribute);
        assert(MCAPI_SUCCESS == status);

        // MCAPI_ENDP_ATTR_MEMORY_TYPE - Shared/local (0-copy), blocking or non-blocking on limit - Channel compatibility attribute
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_MEMORY_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_MEMORY_TYPE,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented

        // MCAPI_ENDP_ATTR_NUM_PRIORITIES - Number of priorities - Channel compatibility attribute
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_NUM_PRIORITIES,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_NUM_PRIORITIES,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented

        // MCAPI_ENDP_ATTR_PRIORITY - Priority on connected endpoint - Channel compatibility attribute
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_PRIORITY,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_PRIORITY,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented

        // MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS - Number of send buffers at the current endpoint priority level
        attribute = 4;
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS,&attribute,sizeof(int),&status);
        assert(MCAPI_SUCCESS == status);
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS,&attribute,sizeof(int),&status);
        assert(MCAPI_SUCCESS == status);
        assert(4 == attribute);

        // MCAPI_ENDP_ATTR_NUM_RECV_BUFFERS - Number of available receive buffers
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_NUM_RECV_BUFFERS,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_NUM_RECV_BUFFERS,&attribute,sizeof(int),&status);
        assert(MCAPI_SUCCESS == status);
        assert((int)attribute == MCAPI_MAX_QUEUE_ENTRIES -
            mcapi_trans_queue_elements(&mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue));

        // MCAPI_ENDP_ATTR_ENDP_STATUS - Endpoint status, connected, open etc.
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_ENDP_STATUS,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_ENDP_STATUS,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented

        // MCAPI_ENDP_ATTR_TIMEOUT - Timeout
        mcapi_trans_endpoint_set_attribute(ep,MCAPI_ENDP_ATTR_TIMEOUT,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented
        mcapi_trans_endpoint_get_attribute(ep,MCAPI_ENDP_ATTR_TIMEOUT,&attribute,sizeof(int),&status);
        assert(MCAPI_ERR_ATTR_NUM == status); // not implemented

        mcapi_trans_endpoint_delete(ep);
	}
