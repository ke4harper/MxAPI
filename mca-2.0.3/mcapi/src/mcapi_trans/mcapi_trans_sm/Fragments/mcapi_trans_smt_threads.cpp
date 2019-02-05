void connect_init(mcapi_domain_t domain,mcapi_node_t node,mxml_node_t* root,mcapi_config_t* config)
{
    int i = 0;
    int nep = 0;
    int nl = 0;
    size_t size = 0;
    mcapi_status_t status = MCAPI_SUCCESS;
    mxml_node_t* top = mxmlFindElement(root,root,"topology",NULL,NULL,MXML_DESCEND);
    mxml_node_t* xnode = NULL;
    mcapi_endpoint_data_t* ed = NULL;

    memset(config,0,sizeof(mcapi_config_t));

    // Configure endpoints
    xnode = mxmlGetFirstChild(top);
    while(NULL != xnode) {
        if(MXML_ELEMENT != mxmlGetType(xnode)) {
            xnode = mxmlGetNextSibling(xnode);
            continue;
        }
        if(!strcmp("endpoint",mxmlGetElement(xnode))) {
            const char* s_name = mxmlElementGetAttr(xnode,"name");
            const char* s_d = mxmlElementGetAttr(xnode,"domain");
            const char* s_n = mxmlElementGetAttr(xnode,"node");
            const char* s_p = mxmlElementGetAttr(xnode,"port");
            if(NULL == s_name) {
                printf("(%d %d) missing name for endpoint\n",domain,node);
                xnode = mxmlGetNextSibling(xnode);
                continue;
            }
            else if(MCAPI_MAX_ENDPOINTS <= nep) {
                printf("(%d %d) maximum number of endpoints (%d) exceeded\n",domain,node,MCAPI_MAX_ENDPOINTS);
                xnode = mxmlGetNextSibling(xnode);
                continue;
            }
            else {
                if(MAX_CFG_NAME_LEN < strlen(s_name)) {
                    printf("(%d %d) endpoint name %s truncated to %d characters\n",domain,node,s_name,MAX_CFG_NAME_LEN);
                }
                if(NULL == s_d) {
                    printf("(%d %d) missing domain for endpoint %s\n",domain,node,s_name);
                    xnode = mxmlGetNextSibling(xnode);
                    continue;
                }
                if(NULL == s_n) {
                    printf("(%d %d) missing node for endpoint %s\n",domain,node,s_name);
                    xnode = mxmlGetNextSibling(xnode);
                    continue;
                }
                if(NULL == s_p) {
                    printf("(%d %d) missing port for endpoint %s\n",domain,node,s_name);
                    xnode = mxmlGetNextSibling(xnode);
                    continue;
                }
            }
            mcapi_endpoint_cfg_t* ep = &config->endpoint[nep++];
            memset(ep,0,sizeof(mcapi_endpoint_cfg_t));
            ep->endpoint = -1;
            ep->index = nep;
            config->tuple[nep-1].parent = ep;
            ed = &config->tuple[nep-1].u.data;
            ed->domain = atoi(s_d);
            ed->node = atoi(s_n);
            ed->port = atoi(s_p);
            ep->tuple = &config->tuple[nep-1];
#if !(__unix__)
            strncpy_s(ep->name,MAX_CFG_NAME_LEN,s_name,MAX_CFG_NAME_LEN);
#else
            strncpy(ep->name,s_name,MAX_CFG_NAME_LEN);
#endif  // (__unix__)
            mxmlSetUserData(xnode,(void*)ep->index);
        }
        xnode = mxmlGetNextSibling(xnode);
    }

    // Establish connections originating and terminating from this node
    xnode = mxmlGetFirstChild(top);
    while(NULL != xnode) {
        if(MXML_ELEMENT != mxmlGetType(xnode)) {
            xnode = mxmlGetNextSibling(xnode);
            continue;
        }
        if(!strcmp("channel",mxmlGetElement(xnode))) {
            uintptr_t fei = 0;
            uintptr_t tei = 0;
            const char* s_name = mxmlElementGetAttr(xnode,"name");
            const char* s_from_name = NULL;
            const char* s_to_name = NULL;
            const char* s_t = mxmlElementGetAttr(xnode,"type");
            channel_type ct;
            mxml_node_t* from_ref = NULL;
            mxml_node_t* to_ref = NULL;
            mxml_node_t* from_node = NULL;
            mxml_node_t* to_node = NULL;
            mcapi_endpoint_data_t* fed = NULL;
            mcapi_endpoint_data_t* ted = NULL;
            mcapi_endpoint_cfg_t* from_ep = NULL;
            mcapi_endpoint_cfg_t* to_ep = NULL;
            mcapi_chan_cfg_t* c = NULL;

            if(NULL == s_name) {
                printf("(%d %d) missing name for channel\n",domain,node);
                xnode = mxmlGetNextSibling(xnode);
                continue;
            }
            else if(MCAPI_MAX_ENDPOINTS <= config->nchans) {
                printf("(%d %d) maximum number (%d) of channels exceeded\n",domain,node,MCAPI_MAX_ENDPOINTS);
                xnode = mxmlGetNextSibling(xnode);
                continue;
            }
            else {
                if(MAX_CFG_NAME_LEN < strlen(s_name)) {
                    printf("(%d %d) channel name %s truncated to %d characters\n",domain,node,s_name,MAX_CFG_NAME_LEN);
                }
                if(NULL == s_t) {
                    printf("(%d %d) missing type for channel %s\n",domain,node,s_t);
                    xnode = mxmlGetNextSibling(xnode);
                    continue;
                }
                from_ref = mxmlFindElement(xnode,top,"from",NULL,NULL,MXML_DESCEND);
                to_ref = mxmlFindElement(xnode,top,"to",NULL,NULL,MXML_DESCEND);
                assert(NULL != from_ref);
                s_from_name = mxmlElementGetAttr(from_ref,"endpoint");
                from_node = mxmlFindElement(top,top,"endpoint","name",s_from_name,MXML_DESCEND);
                assert(NULL != from_node);
                fei = (uintptr_t)mxmlGetUserData(from_node);
                assert(0 < fei);
                from_ep = &config->endpoint[fei-1];
                fed = &from_ep->tuple->u.data;
                assert(NULL != to_ref);
                s_to_name = mxmlElementGetAttr(to_ref,"endpoint");
                to_node = mxmlFindElement(top,top,"endpoint","name",s_to_name,MXML_DESCEND);
                assert(NULL != to_node);
                tei = (uintptr_t)mxmlGetUserData(to_node);
                assert(0 < tei);
                to_ep = &config->endpoint[tei-1];
                ted = &to_ep->tuple->u.data;

                if(!strcmp("msg",s_t)) {
                    ct = MCAPI_NO_CHAN;
                }
                else if(!strcmp("pkt",s_t)) {
                    ct = MCAPI_PKT_CHAN;
                }
                else if(!strcmp("scl",s_t)) {
                    ct = MCAPI_SCL_CHAN;
                }
                else {
                    printf("(%d %d) invalid channel type %s\n",domain,node,s_t);
                    xnode = mxmlGetNextSibling(xnode);
                    continue;
                }
            }
            switch(ct) {
            case MCAPI_NO_CHAN:
            case MCAPI_PKT_CHAN:
            case MCAPI_SCL_CHAN:
                c = &config->chan[config->nchans++];
                memset(c,0,sizeof(mcapi_chan_cfg_t));
                c->type = ct;
                c->valid = 0;
                c->index = config->nchans;
                c->state = MCAPI_INVALID_STATE;
                c->from = from_node;
                c->from_ep = from_ep;
                c->to = to_node;
                c->to_ep = to_ep;
#if !(__unix__)
                strncpy_s(c->name,MAX_CFG_NAME_LEN,s_name,MAX_CFG_NAME_LEN);
#else
                strncpy(c->name,s_name,MAX_CFG_NAME_LEN);
#endif  // (__unix__)
                mxmlSetUserData(xnode,(void*)c->index);

                if(domain == fed->domain && node == fed->node) {
                    c->direction = MCAPI_SEND_DIRECTION;
                }
                else if(domain == ted->domain && node == ted->node) {
                    c->direction = MCAPI_RECV_DIRECTION;
                }
                else {
                    xnode = mxmlGetNextSibling(xnode);
                    continue;
                }

                switch(c->direction) {
                case MCAPI_SEND_DIRECTION:
                    // Create local endpoint
                    for(i = 0; i < nep; i++) {
                        if(config->tuple[i].u.buf == from_ep->tuple->u.buf &&
                            config->tuple[i].used) {
                            mcapi_endpoint_cfg_t* ep = (mcapi_endpoint_cfg_t*)config->tuple[i].parent;
                            printf("(%d %d) endpoint %s (%ld) tuple (%d %d %d) already in use by %s\n",
                                domain,node,s_from_name,from_ep->index,fed->domain,fed->node,fed->port,ep->name);
                            break;
                        }
                    }
                    if(i < nep) {
                        xnode = mxmlGetNextSibling(xnode);
                        continue;
                    }

                    assert(mcapi_trans_endpoint_create(&from_ep->endpoint,fed->port,MCAPI_FALSE));
                    from_ep->tuple->used = 1;
                    from_ep->valid = 1;
#ifdef NOTUSED
                    printf("(%d %d) endpoint (local) %s (%d 0x%x) - domain: %d, node: %d, port: %d\n",
                        domain,node,s_from_name,from_ep->index,from_ep->endpoint,fed->domain,fed->node,fed->port);
#endif  // NOTUSED
                    // Get destination endpoint from remote node, wait for creation
                    switch(ct) {
                    case MCAPI_NO_CHAN:
                    case MCAPI_PKT_CHAN:
                    case MCAPI_SCL_CHAN:
                        for(i = 0; i < nep; i++) {
                            if(config->tuple[i].u.buf == to_ep->tuple->u.buf &&
                                config->tuple[i].used) {
                                mcapi_endpoint_cfg_t* ep = (mcapi_endpoint_cfg_t*)config->tuple[i].parent;
                                printf("(%d %d) endpoint %s (%ld) tuple (%d %d %d) already in use by %s\n",
                                    domain,node,s_to_name,to_ep->index,ted->domain,ted->node,ted->port,ep->name);
                                break;
                            }
                        }
                        if(i < nep) {
                            xnode = mxmlGetNextSibling(xnode);
                            continue;
                        }

                        mcapi_trans_endpoint_get(&to_ep->endpoint,ted->domain,ted->node,ted->port);
                        to_ep->tuple->used = 1;
                        to_ep->valid = 1;
                        break;
                    }
#ifdef NOTUSED
                    printf("(%d %d) endpoint (remote) %s (%d 0x%x) - domain: %d, node: %d, port: %d\n",
                        domain,node,s_to_name,to_ep->index,to_ep->endpoint,ted->domain,ted->node,ted->port);
#endif  // NOTUSED
                    break;
                case MCAPI_RECV_DIRECTION:
                    // Create local endpoint
                    for(i = 0; i < nep; i++) {
                        if(config->tuple[i].u.buf == to_ep->tuple->u.buf &&
                            config->tuple[i].used) {
                            mcapi_endpoint_cfg_t* ep = (mcapi_endpoint_cfg_t*)config->tuple[i].parent;
                            printf("(%d %d) endpoint %s (%ld) tuple (%d %d %d) already in use by %s\n",
                                domain,node,s_to_name,to_ep->index,ted->domain,ted->node,ted->port,ep->name);
                            break;
                        }
                    }
                    if(i < nep) {
                        xnode = mxmlGetNextSibling(xnode);
                        continue;
                    }

                    assert(mcapi_trans_endpoint_create(&to_ep->endpoint,ted->port,MCAPI_FALSE));
                    to_ep->tuple->used = 1;
                    to_ep->valid = 1;
#ifdef NOTUSED
                    printf("(%d %d) endpoint (local) %s (%d 0x%x) - domain: %d, node: %d, port: %d\n",
                        domain,node,s_to_name,to_ep->index,to_ep->endpoint,ted->domain,ted->node,ted->port);
#endif  // NOTUSED
                    // Get source endpoint from remote node, wait for creation
                    switch(ct) {
                    case MCAPI_NO_CHAN:
                    case MCAPI_PKT_CHAN:
                    case MCAPI_SCL_CHAN:
                        for(i = 0; i < nep; i++) {
                            if(config->tuple[i].u.buf == from_ep->tuple->u.buf &&
                                config->tuple[i].used) {
                                mcapi_endpoint_cfg_t* ep = (mcapi_endpoint_cfg_t*)config->tuple[i].parent;
                                printf("(%d %d) endpoint %s (%ld) tuple (%d %d %d) already in use by %s\n",
                                    domain,node,s_from_name,from_ep->index,fed->domain,fed->node,fed->port,ep->name);
                                break;
                            }
                        }
                        if(i < nep) {
                            xnode = mxmlGetNextSibling(xnode);
                            continue;
                        }

                        mcapi_trans_endpoint_get(&from_ep->endpoint,fed->domain,fed->node,fed->port);
                        from_ep->tuple->used = 1;
                        from_ep->valid = 1;
                        break;
                    }
#ifdef NOTUSED
                    printf("(%d %d) endpoint (remote) %s (%d 0x%x) - domain: %d, node: %d, port: %d\n",
                        domain,node,s_from_name,from_ep->index,from_ep->endpoint,fed->domain,fed->node,fed->port);
#endif  // NOTUSED
                    break;
                }

                // Create the connection asynchronously
                switch(ct) {
                case MCAPI_NO_CHAN:
                    break;
                case MCAPI_PKT_CHAN:
                    mcapi_trans_pktchan_connect_i(from_ep->endpoint,to_ep->endpoint,&c->request,&status);
                    c->state = MCAPI_CONN_STATE;
                    break;
                case MCAPI_SCL_CHAN:
                    mcapi_trans_sclchan_connect_i(from_ep->endpoint,to_ep->endpoint,&c->request,&status);
                    c->state = MCAPI_CONN_STATE;
                    break;
                }

                c->valid = 1;
#ifdef NOTUSED
                printf("(%d %d) channel (%s) %s (%d)\n\tfrom: %s (%d)\n\tto: %s (%d)\n",
                    domain,node,s_t,s_name,c->index,
                    s_from_name,from_ep->index,
                    s_to_name,to_ep->index);
#endif  // NOTUSED
                break;
            }
        }
        xnode = mxmlGetNextSibling(xnode);
    }

    // Bind channels into associated links
    xnode = mxmlGetFirstChild(top);
    while(NULL != xnode) {
        if(MXML_ELEMENT != mxmlGetType(xnode)) {
            xnode = mxmlGetNextSibling(xnode);
            continue;
        }
        if(!strcmp("link",mxmlGetElement(xnode))) {
            const char* s_name = mxmlElementGetAttr(xnode,"name");
            const char* s_t = mxmlElementGetAttr(xnode,"type");
            if(NULL == s_name) {
                printf("(%d %d) missing name for link\n",domain,node);
                xnode = mxmlGetNextSibling(xnode);
                continue;
            }
            else if(MCAPI_MAX_ENDPOINTS <= nl) {
                printf("(%d %d) maximum number (%d) of links exceeded\n",domain,node,MCAPI_MAX_ENDPOINTS);
                xnode = mxmlGetNextSibling(xnode);
                continue;
            }
            else {
                if(MAX_CFG_NAME_LEN < strlen(s_name)) {
                    printf("(%d %d) link name %s truncated to %d characters\n",domain,node,s_name,MAX_CFG_NAME_LEN);
                }
                if(NULL == s_t) {
                    printf("(%d %d) missing type for link %s\n",domain,node,s_t);
                    xnode = mxmlGetNextSibling(xnode);
                    continue;
                }
            }
            mxmlSetUserData(xnode,NULL);
            if(!strcmp("fullduplex",s_t)) {
                int sci = 0;
                int aci = 0;
                const char* s_send_name = NULL;
                const char* s_ack_name = NULL;
                mxml_node_t* send = mxmlFindElement(xnode,top,"send",NULL,NULL,MXML_DESCEND);
                mxml_node_t* ack = mxmlFindElement(xnode,top,"ack",NULL,NULL,MXML_DESCEND);
                mxml_node_t* send_node = NULL;
                mxml_node_t* ack_node = NULL;
                mcapi_chan_cfg_t* send_chan = NULL;
                mcapi_chan_cfg_t* ack_chan = NULL;
                mcapi_link_cfg_t* l = NULL;
                assert(NULL != send);
                assert(NULL != ack);
                s_send_name = mxmlElementGetAttr(send,"channel");
                s_ack_name = mxmlElementGetAttr(ack,"channel");
                send_node = mxmlFindElement(top,top,"channel","name",s_send_name,MXML_DESCEND);
                ack_node = mxmlFindElement(top,top,"channel","name",s_ack_name,MXML_DESCEND);
                assert(NULL != send_node);
                assert(NULL != ack_node);
                sci = (uintptr_t)mxmlGetUserData(send_node);
                aci = (uintptr_t)mxmlGetUserData(ack_node);
                if(0 < sci && 0 < aci) {
                    send_chan = &config->chan[sci-1];
                    ack_chan = &config->chan[aci-1];
                    if(send_chan->valid && ack_chan->valid) {
                        assert(ack_chan->type == send_chan->type);
                        switch(send_chan->type) {
                        case MCAPI_NO_CHAN:
                        case MCAPI_PKT_CHAN:
                        case MCAPI_SCL_CHAN:
                            l = &config->link[nl++];
                            memset(l,0,sizeof(mcapi_link_cfg_t));
                            l->type = MCAPI_FULLDUPLEX_LINK;
                            l->index = nl;
                            l->links.fullduplex.send = send_chan;
                            l->links.fullduplex.ack = ack_chan;
                            l->links.fullduplex.send->parent = l;
                            l->links.fullduplex.ack->parent = l;
#if !(__unix__)
                            strncpy_s(l->name,MAX_CFG_NAME_LEN,s_name,MAX_CFG_NAME_LEN);
#else
                            strncpy(l->name,s_name,MAX_CFG_NAME_LEN);
#endif  // (__unix__)
                            mxmlSetUserData(xnode,(void*)l->index);
#ifdef NOTUSED
                            printf("(%d %d) link (fullduplex) %s (%d)\n\tsend: %s (%d)\n\tack: %s (%d)\n",
                                domain,node,s_name,l->index,
                                s_send_name,sci,
                                s_ack_name,ai);
#endif  // NOTUSED
                            break;
                        }
                    }
                }
            }
            else {
                printf("(%d %d) invalid link type %s\n",domain,node,s_t);
            }
        }
        xnode = mxmlGetNextSibling(xnode);
    }

    // Complete connection requests and open communication
    for(int i = 0; i < config->nchans; i++) {
        mcapi_chan_cfg_t* c = &config->chan[i];
        if(c->valid) {
            switch(c->type) {
            case MCAPI_NO_CHAN:
                break;
            case MCAPI_PKT_CHAN:
            case MCAPI_SCL_CHAN:
                if(0 != c->request) {
                    status = MCAPI_SUCCESS;
                    if(!mcapi_trans_wait(&c->request,&size,&status,MCA_INFINITE)) {
                        printf("channel connect request failed\n");
                    }
                    switch(c->direction) {
                    case MCAPI_SEND_DIRECTION:
                        switch(c->type) {
                        case MCAPI_NO_CHAN:
                            break;
                        case MCAPI_PKT_CHAN:
                            mcapi_trans_pktchan_send_open_i(&c->send.pkt_hndl,c->from_ep->endpoint,&c->request,&status);
                            c->state = MCAPI_CONN_STATE;
                            break;
                        case MCAPI_SCL_CHAN:
                            mcapi_trans_sclchan_send_open_i(&c->send.scl_hndl,c->from_ep->endpoint,&c->request,&status);
                            c->state = MCAPI_CONN_STATE;
                            break;
                        }
                        break;
                    case MCAPI_RECV_DIRECTION:
                        switch(c->type) {
                        case MCAPI_NO_CHAN:
                            break;
                        case MCAPI_PKT_CHAN:
                            mcapi_trans_pktchan_recv_open_i(&c->recv.pkt_hndl,c->to_ep->endpoint,&c->request,&status);
                            c->state = MCAPI_CONN_STATE;
                            break;
                        case MCAPI_SCL_CHAN:
                            mcapi_trans_sclchan_recv_open_i(&c->recv.scl_hndl,c->to_ep->endpoint,&c->request,&status);
                            c->state = MCAPI_CONN_STATE;
                            break;
                        }
                    }
                }
                break;
            }
        }
    }

    // Complete open requests
    for(int i = 0; i < config->nchans; i++) {
        mcapi_chan_cfg_t* c = &config->chan[i];
        if(c->valid) {
            if(0 == c->request) {
                c->state = MCAPI_ACTIVE_STATE;
            }
            else {
                status = MCAPI_SUCCESS;
                if(mcapi_trans_wait(&c->request,&size,&status,MCA_INFINITE)) {
                    c->state = MCAPI_ACTIVE_STATE;
                }
                else {
                    printf("(%d %d) channel %s failed to open\n",domain,node,c->name);
                }
            }
        }
    }
}

void connect_rundown(mcapi_domain_t domain,mcapi_node_t node,mcapi_config_t* config)
{
    mcapi_status_t status = MCAPI_SUCCESS;
    size_t size = 0;

    // Request to close the channel ends controlled by this node
    for(int i = 0; i < config->nchans; i++) {
        mcapi_chan_cfg_t* c = &config->chan[i];
        if(c->valid) {
            switch(c->direction) {
            case MCAPI_SEND_DIRECTION:
                switch(c->type) {
                case MCAPI_NO_CHAN:
                    break;
                case MCAPI_PKT_CHAN:
                    mcapi_trans_pktchan_send_close_i(c->send.pkt_hndl,&c->request,&status);
                    c->state = MCAPI_DISCONN_STATE;
                    break;
                case MCAPI_SCL_CHAN:
                    mcapi_trans_sclchan_send_close_i(c->send.scl_hndl,&c->request,&status);
                    c->state = MCAPI_DISCONN_STATE;
                    break;
                }
                break;
            case MCAPI_RECV_DIRECTION:
                switch(c->type) {
                case MCAPI_NO_CHAN:
                    break;
                case MCAPI_PKT_CHAN:
                    mcapi_trans_pktchan_recv_close_i(c->recv.pkt_hndl,&c->request,&status);
                    c->state = MCAPI_DISCONN_STATE;
                    break;
                case MCAPI_SCL_CHAN:
                    mcapi_trans_sclchan_recv_close_i(c->recv.scl_hndl,&c->request,&status);
                    c->state = MCAPI_DISCONN_STATE;
                    break;
                }
                break;
            }
        }
    }

    // Complete close requests
    for(int i = 0; i < config->nchans; i++) {
        mcapi_chan_cfg_t* c = &config->chan[i];
        if(c->valid) {
            if(0 != c->request) {
                status = MCAPI_SUCCESS;
                if(mcapi_trans_wait(&c->request,&size,&status,MCA_INFINITE)) {
                    c->state = MCAPI_INVALID_STATE;
                }
                else {
                    printf("(%d %d) channel %s failed to close\n",domain,node,c->name);
                }
            }
            c->valid = 0;
        }
    }

    // Delete the endpoints controlled by this node
    for(int i = 0; i < config->nchans; i++) {
        mcapi_chan_cfg_t* c = &config->chan[i];
        switch(c->direction) {
        case MCAPI_SEND_DIRECTION:
            if(c->from_ep->valid) {
                mcapi_trans_endpoint_delete(c->from_ep->endpoint);
                c->from_ep->valid = 0;
                c->to_ep->valid = 0;
            }
            break;
        case MCAPI_RECV_DIRECTION:
            if(c->to_ep->valid) {
                mcapi_trans_endpoint_delete(c->to_ep->endpoint);
                c->from_ep->valid = 0;
                c->to_ep->valid = 0;
            }
            break;
        }
    }
}

const int MAX_TRANSACTIONS = 1000;

void client_session(mcapi_test_args_t* mta)
{
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
    mca_timestamp_t start;
    double elapsed = 0.0;
    double split_run = 0.0;

    mcapi_status_t status = MCAPI_FALSE;
	mcapi_node_attributes_t node_attrs = { { { 0 } } };

#if (__unix__)
    cpu_set_t mask = { { 0 } };
    CPU_ZERO(&mask);
    CPU_SET(mta->affinity,&mask);
    assert(0 == sched_setaffinity(0,sizeof(cpu_set_t),&mask));
#endif  // (__unix__)

    //printf("start client %d, thread %d, run (%d %d %d)\n",mta->node,tid,mta->multicore,mta->iteration,mta->sample);

    mca_set_debug_level(0);

    assert(mcapi_trans_node_init_attributes(&node_attrs,&status));
    assert(mcapi_trans_initialize(mta->domain,mta->node,&node_attrs));
	assert(mcapi_trans_initialized());
    assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));

    //DebugBreak();

    mca_set_debug_level(0);

    connect_init(domain,node,mta->root,&config);

    mca_set_debug_level(0);

    // Filter active channels
    for(i = 0; i < config.nchans; i++) {
        mcapi_chan_cfg_t* c = &config.chan[i];
        // Only process the channels and modes that have been enabled
        if(c->valid &&
            (mta->mode & (1<<(int)c->type)) &&
            MCAPI_ACTIVE_STATE == c->state) {
            mcapi_endpoint_index_t* fpath = &c->from_ep->tuple->path;
            mcapi_endpoint_index_t* tpath = &c->to_ep->tuple->path;
            active[j++] = i;
            mcapi_trans_decode_handle(c->from_ep->endpoint,&fpath->domain,&fpath->node,&fpath->ep);
            mcapi_trans_decode_handle(c->to_ep->endpoint,&tpath->domain,&tpath->node,&tpath->ep);
        }
    }
    cactive = j;

    if(0 >= cactive) {
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
                            assert(c->request);
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
                            assert(c->request);
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
            for(j = 0; j < cactive; j++) {
                int k = 0;
                i = active[j];
                for(k = 0; k < MAX_TRANSACTIONS; k++) {
                    assert(k+1 == t[i][k]);
                }
            }
            break;
        }
    }

    // Collect elapsed time
    elapsed = mca_end_ts(&start);
    split_run = elapsed/MAX_TRANSACTIONS;
    printf("client %3d, thread 0x%-16lx,\trun (%d %2d %d),\t%12.2f\t%7.2f\n",
           mta->node,tid,mta->multicore,mta->iteration,mta->sample,1.0E6/split_run,split_run);

    // Wait for server to finish
    sys_os_usleep(100000);

    mca_set_debug_level(0);

    connect_rundown(domain,node,&config);
    //printf("rundown client %d, thread %d, run (%d %d %d)\n",mta->node,tid,mta->multicore,mta->iteration,mta->sample);

    mca_set_debug_level(0);

	assert(mcapi_trans_finalize());
    //printf("end client %d, thread %d, run (%d %d %d)\n", mta->node,tid,mta->multicore,mta->iteration,mta->sample);
}

void server_session(mcapi_test_args_t* mta)
{
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
    mca_timestamp_t start;
    double elapsed = 0.0;
    double split_run = 0.0;

    mcapi_status_t status = MCAPI_FALSE;
	mcapi_node_attributes_t node_attrs = { { { 0 } } };

#if (__unix__)
    cpu_set_t mask = { { 0 } };
    CPU_ZERO(&mask);
    CPU_SET(mta->affinity,&mask);
    assert(0 == sched_setaffinity(0,sizeof(cpu_set_t),&mask));
#endif  // (__unix__)

    //printf("start server, thread %d, run (%d %d %d), mode 0x%x\n",tid,mta->multicore,mta->iteration,mta->sample,mta->mode);

    assert(mcapi_trans_node_init_attributes(&node_attrs,&status));
    assert(mcapi_trans_initialize(mta->domain,mta->node,&node_attrs));
	assert(mcapi_trans_initialized());
    assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));

    //DebugBreak();

    mca_set_debug_level(0);

    connect_init(domain,node,mta->root,&config);

    mca_set_debug_level(0);

    // Filter active channels
    for(i = 0; i < config.nchans; i++) {
        mcapi_chan_cfg_t* c = &config.chan[i];
        // Only process the channels and modes that have been enabled
        if(c->valid &&
            (mta->mode & (1<<(int)c->type)) &&
            MCAPI_ACTIVE_STATE == c->state) {
            mcapi_endpoint_index_t* fpath = &c->from_ep->tuple->path;
            mcapi_endpoint_index_t* tpath = &c->to_ep->tuple->path;
            active[j++] = i;
            mcapi_trans_decode_handle(c->from_ep->endpoint,&fpath->domain,&fpath->node,&fpath->ep);
            mcapi_trans_decode_handle(c->to_ep->endpoint,&tpath->domain,&tpath->node,&tpath->ep);
        }
    }
    cactive = j;

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
                            assert(c->request);
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
                            assert(c->request);
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
                            if(0 < tcount[i]) {
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
            for(j = 0; j < cactive; j++) {
                int k = 0;
                i = active[j];
                for(k = 0; k < MAX_TRANSACTIONS; k++) {
                    assert(k+1 == t[i][k]);
                }
            }
            break;
        }
    }

    // Collect elapsed time
    elapsed = mca_end_ts(&start);
    split_run = elapsed/MAX_TRANSACTIONS;
    printf("server %3d, thread 0x%-16lx,\trun (%d %2d %d),\t%12.2f\t%7.2f\n",
           mta->node,tid,mta->multicore,mta->iteration,mta->sample,1.0E6/split_run,split_run);

    // Wait for clients to finish
    sys_os_usleep(100000);

    mca_set_debug_level(0);

    connect_rundown(domain,node,&config);
    //printf("rundown server, thread %d, run (%d %d %d)\n",tid,mta->multicore,mta->iteration,mta->sample);

    mca_set_debug_level(0);

	assert(mcapi_trans_finalize());
    //printf("end server, thread %d, run (%d %d %d)\n",tid,mta->multicore,mta->iteration,mta->sample);
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
