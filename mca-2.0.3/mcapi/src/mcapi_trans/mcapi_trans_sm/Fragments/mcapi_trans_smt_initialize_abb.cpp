	// Node attributes
	{
        int i = 0;
        mcapi_node_attributes_t na = { { { 0 } } };
    	mcapi_uint_t attribute1 = 0;
    	mcapi_uint_t attribute2 = 0;

        status = MCAPI_SUCCESS;
		assert(mcapi_trans_node_init_attributes(&na,&status));
        assert(MCAPI_SUCCESS == status); // status not used
        for(i = 0; i < MAX_NUM_ATTRIBUTES; i++)
        {
            assert(MCAPI_FALSE == na.entries[i].valid);
        }
        attribute1 = MCAPI_NODE_ATTR_TYPE_REGULAR;
        mcapi_trans_node_set_attribute(&na,MCAPI_NODE_ATTR_TYPE,&attribute1,sizeof(mcapi_uint_t),&status);
        // Attribute has to fit in size of pointer (void*)
        assert(MCAPI_SUCCESS == status);
        assert(MCAPI_NODE_ATTR_TYPE_REGULAR == na.entries[MCAPI_NODE_ATTR_TYPE].attribute_d.type);
        assert(mcapi_trans_initialize(domain,node,&na,MCAPI_TRUE));
		assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));
        assert(mcapi_db->domains[d_index].nodes[n_index].state.data.valid);
        // Attribute valid property not used
        assert(sizeof(mcapi_uint_t) ==
            mcapi_db->domains[d_index].nodes[n_index].attributes.entries[MCAPI_NODE_ATTR_TYPE].bytes);
        assert(MCAPI_NODE_ATTR_TYPE_REGULAR ==
               mcapi_db->domains[d_index].nodes[n_index].attributes.entries[MCAPI_NODE_ATTR_TYPE].attribute_d.type);
        mcapi_trans_node_get_attribute(domain,node,MCAPI_NODE_ATTR_TYPE,&attribute2,sizeof(mcapi_uint_t),&status);
        assert(MCAPI_SUCCESS == status);
        assert(attribute2 == attribute1);
		assert(mcapi_trans_finalize());
	}

    // Runtime initialization
	{
        // One node
        assert(mcapi_trans_node_init_attributes(&node_attrs,&status));
        assert(mcapi_trans_initialize(domain,node,&node_attrs,MCAPI_FALSE));
        assert(!mcapi_db->domains[0].nodes[0].state.data.valid);
        mcapi_trans_start();
        assert(mcapi_db->domains[0].nodes[0].state.data.valid);
        assert(domain == mcapi_db->domains[0].state.data.domain_id);
        assert(MCAPI_TRUE == mcapi_db->domains[0].state.data.valid);
        assert(1 == mcapi_db->domains[0].num_nodes);
        assert(node == mcapi_db->domains[0].nodes[0].state.data.node_num);
        assert(MCAPI_TRUE == mcapi_db->domains[0].nodes[0].state.data.valid);
#if !(__unix__)
		assert(GetCurrentProcessId() == mcapi_db->domains[0].nodes[0].pid);
		assert((pthread_t)GetCurrentThreadId() == mcapi_db->domains[0].nodes[0].tid);
#else
		assert(getpid() == mcapi_db->domains[0].nodes[0].pid);
		assert(pthread_self() == mcapi_db->domains[0].nodes[0].tid);
#endif  // (__unix__)
        assert(mcapi_trans_initialized());
		assert(!mcapi_trans_initialize(domain,node,&node_attrs,MCAPI_TRUE));	// Error to initialize duplicate node on same thread
        assert(!mcapi_db->domains[0].nodes[0].state.data.rundown);
		assert(mcapi_trans_finalize());
		assert(mcapi_trans_initialize(domain,node,&node_attrs,MCAPI_TRUE));
        assert(!mcapi_db->domains[0].nodes[0].state.data.rundown);
        mcapi_trans_rundown();
        assert(mcapi_db->domains[0].nodes[0].state.data.rundown);
		assert(mcapi_trans_finalize());
    }
