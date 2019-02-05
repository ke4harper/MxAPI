const int MAX_TRANSACTIONS = 1000;

void client_session(mcapi_test_args_t* mta)
{
#if (__unix__)
    int rc = 0;
#endif  // (__unix__)
    int i = 0;
    int j = 0;
    int cactive = 0;
    int active[MCAPI_MAX_ENDPOINTS] = { 0 };
    int tcount[MCAPI_MAX_ENDPOINTS] = { 0 };
    int t[MCAPI_MAX_ENDPOINTS][MAX_TRANSACTIONS] = { { 0 } };
    uint32_t n_index = 0;
    uint32_t d_index = 0;
    mcapi_node_t node = 0;
    mcapi_domain_t domain = 0;
    mcapi_config_t config = { 0 };
    mcapi_boolean_t done = MCAPI_TRUE;

    char snd_msg_buffer[] = "  Hello, MCAPI msg!";
    char snd_pktchan_buffer[] = "  Hello, MCAPI pktchan!";
    char rcv_msg_buffer[30] = "";
    mcapi_test_scldata_t snd_sclchan = { 0 };
    mcapi_test_scldata_t rcv_sclchan = { 0 };
    char* buffer = NULL;
    size_t size = 0;
    int rundown = 0;
#if !(__unix__)
    pthread_t tid = (pthread_t)GetCurrentThreadId();
#else
    pthread_t tid = pthread_self();
#endif  // (__unix__)
    mca_timeout_t connect_timeout = (mta->bproc) ? MCA_INFINITE : 1000000;
    mca_timestamp_t start;
    double elapsed = 0.0;
    double split_run = 0.0;

    mcapi_status_t status = MCAPI_FALSE;
	mcapi_node_attributes_t node_attrs = { { { 0 } } };

#ifdef NOTUSED
#if (__unix__)
    cpu_set_t mask = { { 0 } };
    CPU_ZERO(&mask);
    CPU_SET(mta->affinity,&mask);
    assert(0 == sched_setaffinity(0,sizeof(cpu_set_t),&mask));
#endif  // (__unix__)
#endif  // NOTUSED

    //printf("start client %d, thread %d, run (%d %d %d)\n",mta->node,tid,mta->multicore,mta->iteration,mta->sample);

    mca_set_debug_level(0);

    assert(mcapi_trans_node_init_attributes(&node_attrs,&status));
    assert(mcapi_trans_initialize(mta->domain,mta->node,&node_attrs,MCAPI_FALSE));
	assert(mcapi_trans_initialized());
    assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));

    //DebugBreak();

    mca_set_debug_level(0);

    assert(mcapi_trans_collect(domain,node,mta->root,&config,connect_timeout,&status));
    assert(MCAPI_SUCCESS == status);

    mca_set_debug_level(0);

    mcapi_trans_start();

#if (__unix__)
    rc = rc;
    rc = pthread_mutex_lock(mta->psync);
    while(!*(mta->prun)) {
        pthread_cond_wait(mta->pcv,mta->psync);
    }
    rc = pthread_mutex_unlock(mta->psync);
#endif  // (__unix__)

    /* wait for all nodes to be initialized */
    done = MCAPI_FALSE;
    while(!done) {
        done = MCAPI_TRUE;
        for(i = 0; i < config.nep; i++) {
            mcapi_endpoint_tuple_t* t = &config.tuple[i];
            if(!config.endpoint[i].initialized) {
              continue;
            }
            mcapi_node_state state;
            mrapi_atomic_read(NULL,&mcapi_db->domains[t->d_index].nodes[t->n_index].state,&state,
                sizeof(mcapi_db->domains[t->d_index].nodes[t->n_index].state),&status);
            assert(MRAPI_SUCCESS == status);
            if(!state.data.valid) {
                done = MCAPI_FALSE;
                sys_os_yield();
                break;
            }
        }
    }

    mca_set_debug_level(0);

    assert(mcapi_trans_connect(domain,node,&config,connect_timeout,&status));
    assert(MCAPI_SUCCESS == status);

    mca_set_debug_level(0);

    // Filter active channels
    for(i = 0; i < config.nchans; i++) {
        mcapi_chan_cfg_t* c = &config.chan[i];
        // Only process the channels and modes that have been enabled
        // Do not enable state communcation channels on single core
        if(c->valid && c->from_ep->valid && c->to_ep->valid &&
            (mta->mode & (1<<(int)c->type)) &&
            MCAPI_ACTIVE_STATE == c->state &&
            (mta->multicore || MCAPI_ENDP_ATTR_FIFO_BUFFER == c->buffer_type)) {
            mcapi_endpoint_index_t* fpath = &c->from_ep->tuple->path;
            mcapi_endpoint_index_t* tpath = &c->to_ep->tuple->path;
            active[j++] = i;
            mcapi_trans_decode_handle(c->from_ep->endpoint,&fpath->domain,&fpath->node,&fpath->ep);
            mcapi_trans_decode_handle(c->to_ep->endpoint,&tpath->domain,&tpath->node,&tpath->ep);
        }
    }
    cactive = j;

    if(0 >= cactive) {
        mcapi_trans_rundown();

        // Wait for all nodes to rundown
        done = MCAPI_FALSE;
        while(!done) {
            done = MCAPI_TRUE;
            for(i = 0; i < config.nep; i++) {
                mcapi_endpoint_tuple_t* t = &config.tuple[i];
                if(!config.endpoint[i].initialized) {
                  continue;
                }
                mcapi_node_state state;
                mrapi_atomic_read(NULL,&mcapi_db->domains[t->d_index].nodes[t->n_index].state,&state,
                    sizeof(mcapi_db->domains[t->d_index].nodes[t->n_index].state),&status);
                assert(MRAPI_SUCCESS == status);
                if(!state.data.rundown) {
                    done = MCAPI_FALSE;
                    sys_os_yield();
                    break;
                }
            }
        }

        assert(mcapi_trans_disconnect(domain,node,&config,100000,&status));
        assert(MCAPI_SUCCESS == status);
    	assert(mcapi_trans_finalize());
        return;
    }

    // Save beginning time
    mca_begin_ts(&start);

    while(1) {
        for(j = 0; j < cactive; j++) {
            mcapi_chan_cfg_t* c = &config.chan[(i = active[j])];
            if(MCAPI_ACTIVE_STATE == c->state) {
                switch(c->direction) {
                case MCAPI_SEND_DIRECTION:
                    switch(c->type) {
                    case MCAPI_NO_CHAN:
                        status = MCAPI_SUCCESS;
                        if(0 == c->request) {
                            // Send next message
                            *(mcapi_uint16_t*)snd_msg_buffer = tcount[i]+1;
                            mcapi_trans_msg_send_i(c->from_ep->endpoint,c->to_ep->endpoint,snd_msg_buffer,sizeof(snd_msg_buffer),&c->request,&status);
                            assert(MCAPI_SUCCESS == status);
                        }
                        if(mcapi_trans_wait(&c->request,&size,&status,0)) {
                            if(MCAPI_ERR_MEM_LIMIT == status ||
                               MCAPI_TIMEOUT == status) {
                                // Give receiver a chance to catch up
                                sys_os_yield();
                            }
                            else {
                                assert(MCAPI_SUCCESS == status);
                                t[i][tcount[i]] = tcount[i]+1;
                                if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                    c->state = MCAPI_RUNDOWN_STATE;
                                }
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    case MCAPI_PKT_CHAN:
                        status = MCAPI_SUCCESS;
                        if(0 == c->request) {
                            // Send next message
                            *(mcapi_uint16_t*)snd_pktchan_buffer = tcount[i]+1;
                            mcapi_trans_pktchan_send_i(c->send.pkt_hndl,(void*)snd_pktchan_buffer,sizeof(snd_pktchan_buffer),&c->request,&status);
                            assert(MCAPI_SUCCESS == status);
                        }
                        if(mcapi_trans_wait(&c->request,&size,&status,0)) {
                            if(MCAPI_ERR_MEM_LIMIT == status ||
                               MCAPI_TIMEOUT == status) {
                                // Give receiver a chance to catch up
                                sys_os_yield();
                            }
                            else {
                                t[i][tcount[i]] = tcount[i]+1;
                                if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                    c->state = MCAPI_RUNDOWN_STATE;
                                }
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    case MCAPI_SCL_CHAN:
                        // Send next message
                        snd_sclchan.data.tcount = tcount[i]+1;
                        snd_sclchan.data.marker = 0xdeadbeef;
                        if(mcapi_trans_sclchan_send(c->send.scl_hndl,snd_sclchan.buf,sizeof(snd_sclchan))) {
                            t[i][tcount[i]] = tcount[i]+1;
                            if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                c->state = MCAPI_RUNDOWN_STATE;
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    }
                    break;
                case MCAPI_RECV_DIRECTION:
                    switch(c->type) {
                    case MCAPI_NO_CHAN:
                        status = MCAPI_SUCCESS;
                        if(0 == c->request) {
                            // Receive next message
                            mcapi_trans_msg_recv_i(c->to_ep->endpoint,rcv_msg_buffer,sizeof(rcv_msg_buffer),&c->request,&status);
                            assert(MCAPI_SUCCESS == status);
                        }
                        if(mcapi_trans_wait(&c->request,&size,&status,0) &&
                           MCAPI_TIMEOUT != status) {
                            mcapi_uint16_t tx = *(mcapi_uint16_t*)rcv_msg_buffer;
                            assert(MCAPI_SUCCESS == status);
                            if(0 < tcount[i] &&
                                MCAPI_ENDP_ATTR_FIFO_BUFFER == c->buffer_type) {
                                if(t[i][tcount[i]-1]+1 != tx) {
                                    mcapi_endpoint_index_t* fpath = &c->from_ep->tuple->path;
                                    mcapi_endpoint_index_t* tpath = &c->to_ep->tuple->path;
                                    printf("node %d recv (%d,%d,%d) -> (%d,%d,%d) %s tx %d -> %d\n",node,
                                        fpath->domain,fpath->node,fpath->ep,
                                        tpath->domain,tpath->node,tpath->ep,
                                        c->name,t[i][tcount[i]-1],tx);
                                }
                            }
                            assert(0 == strcmp("Hello, world msg!",&rcv_msg_buffer[2]));
                            t[i][tcount[i]] = tx;
                            if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                c->state = MCAPI_RUNDOWN_STATE;
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    case MCAPI_PKT_CHAN:
                        status = MCAPI_SUCCESS;
                        if(0 == c->request) {
                            // Receive next message
                            mcapi_trans_pktchan_recv_i(c->recv.pkt_hndl,(void**)&buffer,&c->request,&status);
                            assert(MCAPI_SUCCESS == status);
                        }
                        if(mcapi_trans_wait(&c->request,&size,&status,0)) {
                            if(MCAPI_ERR_MEM_LIMIT == status ||
                               MCAPI_TIMEOUT == status) {
                                // Give sender a chance to catch up
                                sys_os_yield();
                            }
                            else {
                                mcapi_uint16_t tx = *(mcapi_uint16_t*)buffer;
                                if(0 < tcount[i]) {
                                    if(t[i][tcount[i]-1]+1 != tx) {
                                        mcapi_endpoint_index_t* fpath = &c->from_ep->tuple->path;
                                        mcapi_endpoint_index_t* tpath = &c->to_ep->tuple->path;
                                        printf("node %d recv (%d,%d,%d) -> (%d,%d,%d) %s tx %d -> %d\n",node,
                                            fpath->domain,fpath->node,fpath->ep,
                                            tpath->domain,tpath->node,tpath->ep,
                                            c->name,t[i][tcount[i]-1],tx);
                                    }
                                }
                                assert(0 == strcmp("Hello, world pktchan!",&buffer[2]));
                                t[i][tcount[i]] = tx;
                                assert(mcapi_trans_pktchan_free((void*)buffer));
                                if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                    c->state = MCAPI_RUNDOWN_STATE;
                                }
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    case MCAPI_SCL_CHAN:
                        // Receive next message
                        if(mcapi_trans_sclchan_recv(c->recv.pkt_hndl,&rcv_sclchan.buf,sizeof(rcv_sclchan))) {
                            mcapi_uint16_t tx = rcv_sclchan.data.tcount;
                            if(0 < tcount[i] &&
                                MCAPI_ENDP_ATTR_FIFO_BUFFER == c->buffer_type) {
                                if(t[i][tcount[i]-1]+1 != tx) {
                                    mcapi_endpoint_index_t* fpath = &c->from_ep->tuple->path;
                                    mcapi_endpoint_index_t* tpath = &c->to_ep->tuple->path;
                                    printf("node %d recv (%d,%d,%d) -> (%d,%d,%d) %s tx %d -> %d\n",node,
                                        fpath->domain,fpath->node,fpath->ep,
                                        tpath->domain,tpath->node,tpath->ep,
                                        c->name,t[i][tcount[i]-1],tx);
                                }
                            }
                            assert(0xdeadcafe == rcv_sclchan.data.marker);
                            t[i][tcount[i]] = tx;
                            if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                c->state = MCAPI_RUNDOWN_STATE;
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    }
                    break;
                }
            }
        }
        rundown = 1;
        for(j = 0; j < cactive; j++) {
            mcapi_chan_cfg_t* c = &config.chan[(i = active[j])];
            if(MCAPI_ACTIVE_STATE == c->state) {
                rundown = 0;
                break;
            }
        }
        if(rundown) {
            break;
        }
    }

    // Collect elapsed time
    elapsed = mca_end_ts(&start);

    // Verify FIFO monotonically increasing transaction IDs
    for(j = 0; j < cactive; j++) {
        int k = 0;
        mcapi_chan_cfg_t* c = &config.chan[(i = active[j])];
        if(MCAPI_ENDP_ATTR_FIFO_BUFFER == c->buffer_type) {
            for(k = 0; k < MAX_TRANSACTIONS; k++) {
                assert(k+1 == t[i][k]);
            }
        }
    }

    split_run = elapsed/MAX_TRANSACTIONS;
    printf("client %3d, thread 0x%-16lx,\trun (%d %2d %d),\t%12.2f\t%7.2f\n",
           mta->node,tid,mta->multicore,mta->iteration,mta->sample,1.0E6/split_run,split_run);

    mca_set_debug_level(0);

    mcapi_trans_rundown();

    /* wait for all nodes to rundown */
    done = MCAPI_FALSE;
    while(!done) {
        done = MCAPI_TRUE;
        for(i = 0; i < config.nep; i++) {
            mcapi_endpoint_tuple_t* t = &config.tuple[i];
            if(!config.endpoint[i].initialized) {
              continue;
            }
            mcapi_node_state state;
            mrapi_atomic_read(NULL,&mcapi_db->domains[t->d_index].nodes[t->n_index].state,&state,
                sizeof(mcapi_db->domains[t->d_index].nodes[t->n_index].state),&status);
            assert(MRAPI_SUCCESS == status);
            if(!state.data.rundown) {
                done = MCAPI_FALSE;
                sys_os_yield();
                break;
            }
        }
    }

    mcapi_trans_disconnect(domain,node,&config,MCA_INFINITE,&status);
    assert(MCAPI_SUCCESS == status);
    //printf("rundown client %d, thread %d, run (%d %d %d)\n",mta->node,tid,mta->multicore,mta->iteration,mta->sample);

    mca_set_debug_level(0);

	assert(mcapi_trans_finalize());
    //printf("end client %d, thread %d, run (%d %d %d)\n", mta->node,tid,mta->multicore,mta->iteration,mta->sample);

    mca_set_debug_level(0);

    /* wait for all nodes to finalize */
    done = MCAPI_FALSE;
    while(!done) {
        done = MCAPI_TRUE;
        for(i = 0; i < config.nep; i++) {
            mcapi_endpoint_tuple_t* t = &config.tuple[i];
            mcapi_node_state state;
            if(!config.endpoint[i].initialized) {
              continue;
            }
            mrapi_atomic_read(NULL,&mcapi_db->domains[t->d_index].nodes[t->n_index].state,&state,
                sizeof(mcapi_db->domains[t->d_index].nodes[t->n_index].state),&status);
            assert(MRAPI_SUCCESS == status);
            if(state.data.valid) {
                done = MCAPI_FALSE;
                sys_os_yield();
                break;
            }
        }
    }
}

void server_session(mcapi_test_args_t* mta)
{
#if (__unix__)
    int rc = 0;
#endif  // (__unix__)
    int i = 0;
    int j = 0;
    int cactive = 0;
    int active[MCAPI_MAX_ENDPOINTS] = { 0 };
    int tcount[MCAPI_MAX_ENDPOINTS] = { 0 };
    int t[MCAPI_MAX_ENDPOINTS][MAX_TRANSACTIONS] = { { 0 } };
    uint32_t n_index = 0;
    uint32_t d_index = 0;
    mcapi_node_t node = 0;
    mcapi_domain_t domain = 0;
    mcapi_config_t config = { 0 };
    mcapi_boolean_t done = MCAPI_FALSE;

    char snd_msg_buffer[] = "  Hello, world msg!";
    char snd_pktchan_buffer[] = "  Hello, world pktchan!";
    char rcv_msg_buffer[30] = "";
    char* buffer = NULL;
    mcapi_test_scldata_t snd_sclchan = { 0 };
    mcapi_test_scldata_t rcv_sclchan = { 0 };
    size_t size = 0;
    int rundown = 0;
#if !(__unix__)
    pthread_t tid = (pthread_t)GetCurrentThreadId();
#else
    pthread_t tid = pthread_self();
#endif  // (_unix__)
    mca_timeout_t connect_timeout = (mta->bproc) ? MCA_INFINITE : 1000000;
    mca_timestamp_t start;
    double elapsed = 0.0;
    double split_run = 0.0;

    mcapi_status_t status = MCAPI_FALSE;
	mcapi_node_attributes_t node_attrs = { { { 0 } } };

#ifdef NOTUSED
#if (__unix__)
    cpu_set_t mask = { { 0 } };
    CPU_ZERO(&mask);
    CPU_SET(mta->affinity,&mask);
    assert(0 == sched_setaffinity(0,sizeof(cpu_set_t),&mask));
#endif  // (__unix__)
#endif  // NOTUSED

    //printf("start server, thread %d, run (%d %d %d), mode 0x%x\n",tid,mta->multicore,mta->iteration,mta->sample,mta->mode);

    mca_set_debug_level(0);

    assert(mcapi_trans_node_init_attributes(&node_attrs,&status));
    assert(mcapi_trans_initialize(mta->domain,mta->node,&node_attrs,MCAPI_FALSE));
	assert(mcapi_trans_initialized());
    assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));

    //DebugBreak();

    mca_set_debug_level(0);

    assert(mcapi_trans_collect(domain,node,mta->root,&config,connect_timeout,&status));
    assert(MCAPI_SUCCESS == status);

    mcapi_trans_start();

#if (__unix__)
    rc = rc;
    rc = pthread_mutex_lock(mta->psync);
    while(!*(mta->prun)) {
        pthread_cond_wait(mta->pcv,mta->psync);
    }
    rc = pthread_mutex_unlock(mta->psync);
#endif  // (__unix__)

    // Wait for all nodes to be initialized
    done = MCAPI_FALSE;
    while(!done) {
        done = MCAPI_TRUE;
        for(i = 0; i < config.nep; i++) {
            mcapi_endpoint_tuple_t* t = &config.tuple[i];
            if(!config.endpoint[i].initialized) {
              continue;
            }
            mcapi_node_state state;
            mrapi_atomic_read(NULL,&mcapi_db->domains[t->d_index].nodes[t->n_index].state,&state,
                sizeof(mcapi_db->domains[t->d_index].nodes[t->n_index].state),&status);
            assert(MRAPI_SUCCESS == status);
            if(!state.data.valid) {
                done = MCAPI_FALSE;
                sys_os_yield();
                break;
            }
        }
    }

    mca_set_debug_level(0);

    assert(mcapi_trans_connect(domain,node,&config,connect_timeout,&status));
    assert(MCAPI_SUCCESS == status);

    mca_set_debug_level(0);

    // Filter active channels
    for(i = 0; i < config.nchans; i++) {
        mcapi_chan_cfg_t* c = &config.chan[i];
        // Only process the channels and modes that have been enabled
        // Do not enable state communcation channels on single core
        if(c->valid && c->from_ep->valid && c->to_ep->valid &&
            (mta->mode & (1<<(int)c->type)) &&
            MCAPI_ACTIVE_STATE == c->state &&
            (mta->multicore || MCAPI_ENDP_ATTR_FIFO_BUFFER == c->buffer_type)) {
            mcapi_endpoint_index_t* fpath = &c->from_ep->tuple->path;
            mcapi_endpoint_index_t* tpath = &c->to_ep->tuple->path;
            active[j++] = i;
            mcapi_trans_decode_handle(c->from_ep->endpoint,&fpath->domain,&fpath->node,&fpath->ep);
            mcapi_trans_decode_handle(c->to_ep->endpoint,&tpath->domain,&tpath->node,&tpath->ep);
        }
    }
    cactive = j;

    if(0 >= cactive) {
        mcapi_trans_rundown();

        // Wait for all nodes to rundown
        done = MCAPI_FALSE;
        while(!done) {
            done = MCAPI_TRUE;
            for(i = 0; i < config.nep; i++) {
                mcapi_endpoint_tuple_t* t = &config.tuple[i];
                if(!config.endpoint[i].initialized) {
                  continue;
                }
                mcapi_node_state state;
                mrapi_atomic_read(NULL,&mcapi_db->domains[t->d_index].nodes[t->n_index].state,&state,
                    sizeof(mcapi_db->domains[t->d_index].nodes[t->n_index].state),&status);
                assert(MRAPI_SUCCESS == status);
                if(!state.data.rundown) {
                    done = MCAPI_FALSE;
                    sys_os_yield();
                    break;
                }
            }
        }

        assert(mcapi_trans_disconnect(domain,node,&config,100000,&status));
        assert(MCAPI_SUCCESS == status);
    	assert(mcapi_trans_finalize());
        return;
    }

    // Save beginning time
    mca_begin_ts(&start);

    while(1) {
        for(j = 0; j < cactive; j++) {
            mcapi_chan_cfg_t* c = &config.chan[(i = active[j])];
            if(MCAPI_ACTIVE_STATE == c->state) {
                switch(c->direction) {
                case MCAPI_SEND_DIRECTION:
                    switch(c->type) {
                    case MCAPI_NO_CHAN:
                        status = MCAPI_SUCCESS;
                        if(0 == c->request) {
                            // Send next message
                            *(mcapi_uint16_t*)snd_msg_buffer = tcount[i]+1;
                            mcapi_trans_msg_send_i(c->from_ep->endpoint,c->to_ep->endpoint,snd_msg_buffer,sizeof(snd_msg_buffer),&c->request,&status);
                            assert(MCAPI_SUCCESS == status);
                        }
                        if(mcapi_trans_wait(&c->request,&size,&status,0)) {
                            if(MCAPI_ERR_MEM_LIMIT == status ||
                               MCAPI_TIMEOUT == status) {
                                // Give receiver a change to catch up
                                sys_os_yield();
                            }
                            else {
                                assert(MCAPI_SUCCESS == status);
                                t[i][tcount[i]] = tcount[i]+1;
                                if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                    c->state = MCAPI_RUNDOWN_STATE;
                                }
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    case MCAPI_PKT_CHAN:
                        status = MCAPI_SUCCESS;
                        if(0 == c->request) {
                            // Send next message
                            *(mcapi_uint16_t*)snd_pktchan_buffer = tcount[i]+1;
                            mcapi_trans_pktchan_send_i(c->send.pkt_hndl,(void*)snd_pktchan_buffer,sizeof(snd_pktchan_buffer),&c->request,&status);
                            assert(MCAPI_SUCCESS == status);
                        }
                        if(mcapi_trans_wait(&c->request,&size,&status,0)) {
                            if(MCAPI_ERR_MEM_LIMIT == status ||
                               MCAPI_TIMEOUT == status) {
                                // Give receiver a chance to catch up
                                sys_os_yield();
                            }
                            else {
                                t[i][tcount[i]] = tcount[i]+1;
                                if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                    c->state = MCAPI_RUNDOWN_STATE;
                                }
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    case MCAPI_SCL_CHAN:
                        // Send next message
                        snd_sclchan.data.tcount = tcount[i]+1;
                        snd_sclchan.data.marker = 0xdeadcafe;
                        if(mcapi_trans_sclchan_send(c->send.scl_hndl,snd_sclchan.buf,sizeof(snd_sclchan))) {
                            t[i][tcount[i]] = tcount[i]+1;
                            if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                c->state = MCAPI_RUNDOWN_STATE;
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    }
                    break;
                case MCAPI_RECV_DIRECTION:
                    switch(c->type) {
                    case MCAPI_NO_CHAN:
                        status = MCAPI_SUCCESS;
                        if(0 == c->request) {
                            // Receive next message
                            mcapi_trans_msg_recv_i(c->to_ep->endpoint,rcv_msg_buffer,sizeof(rcv_msg_buffer),&c->request,&status);
                            assert(MCAPI_SUCCESS == status);
                        }
                        if(mcapi_trans_wait(&c->request,&size,&status,0) &&
                           MCAPI_TIMEOUT != status) {
                            mcapi_uint16_t tx = *(mcapi_uint16_t*)rcv_msg_buffer;
                            assert(MCAPI_SUCCESS == status);
                            if(0 < tcount[i] &&
                                MCAPI_ENDP_ATTR_FIFO_BUFFER == c->buffer_type) {
                                if(t[i][tcount[i]-1]+1 != tx) {
                                    mcapi_endpoint_index_t* fpath = &c->from_ep->tuple->path;
                                    mcapi_endpoint_index_t* tpath = &c->to_ep->tuple->path;
                                    printf("node %d recv (%d,%d,%d) -> (%d,%d,%d) %s tx %d -> %d\n",node,
                                        fpath->domain,fpath->node,fpath->ep,
                                        tpath->domain,tpath->node,tpath->ep,
                                        c->name,t[i][tcount[i]-1],tx);
                                }
                            }
                            assert(0 == strcmp("Hello, MCAPI msg!",&rcv_msg_buffer[2]));
                            t[i][tcount[i]] = tx;
                            if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                c->state = MCAPI_RUNDOWN_STATE;
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    case MCAPI_PKT_CHAN:
                        status = MCAPI_SUCCESS;
                        if(0 == c->request) {
                            // Receive next message
                            mcapi_trans_pktchan_recv_i(c->recv.pkt_hndl,(void**)&buffer,&c->request,&status);
                            assert(MCAPI_SUCCESS == status);
                        }
                        if(mcapi_trans_wait(&c->request,&size,&status,0)) {
                            if(MCAPI_ERR_MEM_LIMIT == status ||
                               MCAPI_TIMEOUT == status) {
                                // Give sender a chance to catch up
                                sys_os_yield();
                            }
                            else {
                                mcapi_uint16_t tx = *(mcapi_uint16_t*)buffer;
                                if(0 < tcount[i] &&
                                    MCAPI_ENDP_ATTR_FIFO_BUFFER == c->buffer_type) {
                                    if(t[i][tcount[i]-1]+1 != tx) {
                                        mcapi_endpoint_index_t* fpath = &c->from_ep->tuple->path;
                                        mcapi_endpoint_index_t* tpath = &c->to_ep->tuple->path;
                                        printf("node %d recv (%d,%d,%d) -> (%d,%d,%d) %s tx %d -> %d\n",node,
                                            fpath->domain,fpath->node,fpath->ep,
                                            tpath->domain,tpath->node,tpath->ep,
                                            c->name,t[i][tcount[i]-1],tx);
                                    }
                                }
                                assert(0 == strcmp("Hello, MCAPI pktchan!",&buffer[2]));
                                t[i][tcount[i]] = tx;
                                assert(mcapi_trans_pktchan_free((void*)buffer));
                                if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                    c->state = MCAPI_RUNDOWN_STATE;
                                }
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    case MCAPI_SCL_CHAN:
                        if(mcapi_trans_sclchan_recv(c->recv.scl_hndl,&rcv_sclchan.buf,sizeof(rcv_sclchan))) {
                            mcapi_uint32_t tx = rcv_sclchan.data.tcount;
                            if(0 < tcount[i] &&
                                MCAPI_ENDP_ATTR_FIFO_BUFFER == c->buffer_type) {
                                if(t[i][tcount[i]-1]+1 != (int)tx) {
                                    mcapi_endpoint_index_t* fpath = &c->from_ep->tuple->path;
                                    mcapi_endpoint_index_t* tpath = &c->to_ep->tuple->path;
                                    printf("node %d recv (%d,%d,%d) -> (%d,%d,%d) %s tx %d -> %d\n",node,
                                        fpath->domain,fpath->node,fpath->ep,
                                        tpath->domain,tpath->node,tpath->ep,
                                        c->name,t[i][tcount[i]-1],tx);
                                }
                            }
                            assert(0xdeadbeef == rcv_sclchan.data.marker);
                            t[i][tcount[i]] = tx;
                            if(MAX_TRANSACTIONS <= ++(tcount[i])) {
                                c->state = MCAPI_RUNDOWN_STATE;
                            }
                        }
                        else {
                            sys_os_yield();
                        }
                        break;
                    }
                    break;
                }
            }
        }
        // Check for rundown
        rundown = 1;
        for(j = 0; j < cactive; j++) {
            mcapi_chan_cfg_t* c = &config.chan[(i = active[j])];
            if(MCAPI_ACTIVE_STATE == c->state) {
                rundown = 0;
                break;
            }
        }
        if(rundown) {
            break;
        }
    }

    // Collect elapsed time
    elapsed = mca_end_ts(&start);

    // Verify FIFO monotonically increasing transaction IDs
    for(j = 0; j < cactive; j++) {
        int k = 0;
        mcapi_chan_cfg_t* c = &config.chan[(i = active[j])];
        if(MCAPI_ENDP_ATTR_FIFO_BUFFER == c->buffer_type) {
            for(k = 0; k < MAX_TRANSACTIONS; k++) {
                assert(k+1 == t[i][k]);
            }
        }
    }

    split_run = elapsed/MAX_TRANSACTIONS;
    printf("server %3d, thread 0x%-16lx,\trun (%d %2d %d),\t%12.2f\t%7.2f\n",
           mta->node,tid,mta->multicore,mta->iteration,mta->sample,1.0E6/split_run,split_run);

    mca_set_debug_level(0);

    mcapi_trans_rundown();

    /* wait for all nodes to rundown */
    done = MCAPI_FALSE;
    while(!done) {
        done = MCAPI_TRUE;
        for(i = 0; i < config.nep; i++) {
            mcapi_endpoint_tuple_t* t = &config.tuple[i];
            mcapi_node_state state;
            if(!config.endpoint[i].initialized) {
              continue;
            }
            mrapi_atomic_read(NULL,&mcapi_db->domains[t->d_index].nodes[t->n_index].state,&state,
                sizeof(mcapi_db->domains[t->d_index].nodes[t->n_index].state),&status);
            assert(MRAPI_SUCCESS == status);
            if(!state.data.rundown) {
                done = MCAPI_FALSE;
                sys_os_yield();
                break;
            }
        }
    }

    mcapi_trans_disconnect(domain,node,&config,MCA_INFINITE,&status);
    assert(MCAPI_SUCCESS == status);
    //printf("rundown server, thread %d, run (%d %d %d)\n",tid,mta->multicore,mta->iteration,mta->sample);

    mca_set_debug_level(0);

	assert(mcapi_trans_finalize());
    //printf("end server, thread %d, run (%d %d %d)\n",tid,mta->multicore,mta->iteration,mta->sample);

    mca_set_debug_level(0);

    /* wait for all nodes to finalize */
    done = MCAPI_FALSE;
    while(!done) {
        done = MCAPI_TRUE;
        for(i = 0; i < config.nep; i++) {
            mcapi_endpoint_tuple_t* t = &config.tuple[i];
            mcapi_node_state state;
            if(!config.endpoint[i].initialized) {
              continue;
            }
            mrapi_atomic_read(NULL,&mcapi_db->domains[t->d_index].nodes[t->n_index].state,&state,
                sizeof(mcapi_db->domains[t->d_index].nodes[t->n_index].state),&status);
            assert(MRAPI_SUCCESS == status);
            if(state.data.valid) {
                done = MCAPI_FALSE;
                sys_os_yield();
                break;
            }
        }
    }
}

void* node1_init(void* args)
{
    mcapi_test_args_t* mta = (mcapi_test_args_t*)args;
    client_session(mta);
	return 0;
}

void* node2_init(void* args)
{
	mcapi_test_args_t* mta = (mcapi_test_args_t*)args;
    client_session(mta);
	return 0;
}

void* node3_init(void* args)
{
	mcapi_test_args_t* mta = (mcapi_test_args_t*)args;
    server_session(mta);
	return 0;
}
