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
        assert(mcapi_db->domains[d_index].nodes[n_index].valid);
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
        assert(!mcapi_db->domains[0].nodes[0].valid);
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        mcapi_trans_start_have_lock();
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(mcapi_db->domains[0].nodes[0].valid);
        assert(domain == mcapi_db->domains[0].domain_id);
        assert(MCAPI_TRUE == mcapi_db->domains[0].valid);
        assert(1 == mcapi_db->domains[0].num_nodes);
        assert(node == mcapi_db->domains[0].nodes[0].node_num);
        assert(MCAPI_TRUE == mcapi_db->domains[0].nodes[0].valid);
#if !(__unix__)
		assert(GetCurrentProcessId() == mcapi_db->domains[0].nodes[0].pid);
		assert((pthread_t)GetCurrentThreadId() == mcapi_db->domains[0].nodes[0].tid);
#else
		assert(getpid() == mcapi_db->domains[0].nodes[0].pid);
		assert(pthread_self() == mcapi_db->domains[0].nodes[0].tid);
#endif  // (__unix__)
        assert(mcapi_trans_initialized());
		assert(!mcapi_trans_initialize(domain,node,&node_attrs,MCAPI_TRUE));	// Error to initialize duplicate node on same thread
        assert(!mcapi_db->domains[0].nodes[0].rundown);
		assert(mcapi_trans_finalize());
		assert(mcapi_trans_initialize(domain,node,&node_attrs,MCAPI_TRUE));
        assert(!mcapi_db->domains[0].nodes[0].rundown);
        assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
        mcapi_trans_rundown_have_lock();
        assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
        assert(mcapi_db->domains[0].nodes[0].rundown);
		assert(mcapi_trans_finalize());
    }
