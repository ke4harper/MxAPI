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

  /***************************************************************************
  NAME: mcapi_trans_collect
  DESCRIPTION: collect endpoints, channels and links based on XML configuration
   and wait for remote endpoints to be initialized
  PARAMETERS:
    domain_id - domain index
    node_id - node index
    root - DOM loaded from topology XML configuration
    config - address to configuration database
    timeout - number of attempts before abandoning connection
    mcapi_status - MCAPI_SUCCESS if all endpoints available, MCAPI_TIMEOUT otherwise
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_collect (mcapi_domain_t domain_id,
                                       mcapi_node_t node_id,
                                       mxml_node_t* root,
                                       mcapi_config_t* config,
                                       mca_timeout_t timeout,
                                       mcapi_status_t* mcapi_status)
  {
    mcapi_boolean_t rc = MCAPI_TRUE;

    int i,j = 0;
    int nchan = 0; /* number of active channels */
    int nl = 0;  /* number of active links */
    mcapi_domain_t domain;
    mcapi_node_t node;
    mcapi_uint32_t d_index,n_index;
    mcapi_status_t status = MCAPI_SUCCESS;
    mxml_node_t* top = mxmlFindElement(root,root,"topology",NULL,NULL,MXML_DESCEND);
    mxml_node_t* xnode = NULL;
    mcapi_endpoint_tuple_t* t = NULL;
    mcapi_endpoint_data_t* ed = NULL;
    mcapi_endpoint_cfg_t* ep = NULL;
    mcapi_buffer_type bt; /* channel buffer type */
    mcapi_uint_t bl; /* channel buffer length */

    mrapi_atomic_barrier_t axb_domains;
    mrapi_atomic_barrier_t axb_nodes;

    mcapi_dprintf(1,"mcapi_trans_collect (%d %d)\n",domain_id,node_id);

    assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));

    /* clear previous configuration */
    memset(config,0,sizeof(mcapi_config_t));

    /* collect endpoints */
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
          rc = MCAPI_FALSE;
          mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) missing name for endpoint\n",domain_id,node_id);
          xnode = mxmlGetNextSibling(xnode);
          continue;
        }
        else if(MCAPI_MAX_ENDPOINTS <= config->nep) {
          rc = MCAPI_FALSE;
          mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) maximum number of endpoints (%d) exceeded\n",
              domain_id,node_id,MCAPI_MAX_ENDPOINTS);
          xnode = mxmlGetNextSibling(xnode);
          continue;
        }
        else {
          if(MAX_CFG_NAME_LEN < strlen(s_name)) {
            mcapi_dprintf(1,"WARNING: mcapi_trans_collect (%d %d) endpoint name %s truncated to %d characters\n",
                domain_id,node_id,s_name,MAX_CFG_NAME_LEN);
          }
          if(NULL == s_d) {
            rc = MCAPI_FALSE;
            mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) missing domain for endpoint %s\n",
                domain_id,node_id,s_name);
            xnode = mxmlGetNextSibling(xnode);
            continue;
          }
          if(NULL == s_n) {
            rc = MCAPI_FALSE;
            mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) missing node for endpoint %s\n",
                domain_id,node_id,s_name);
            xnode = mxmlGetNextSibling(xnode);
            continue;
          }
          if(NULL == s_p) {
            rc = MCAPI_FALSE;
            mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) missing port for endpoint %s\n",
                domain_id,node_id,s_name);
            xnode = mxmlGetNextSibling(xnode);
            continue;
          }
        }
        /* valid endpoint */
        ep = &config->endpoint[config->nep++];
        memset(ep,0,sizeof(mcapi_endpoint_cfg_t));
        ep->endpoint = -1;
        ep->index = config->nep;
        t = &config->tuple[config->nep-1];
        t->parent = ep;
        ed = &t->u.data;
        ep->tuple = t;
        ed->domain = atoi(s_d);
        ed->node = atoi(s_n);
        ed->port = atoi(s_p);
#if !(__unix__)
        strncpy_s(ep->name,MAX_CFG_NAME_LEN,s_name,MAX_CFG_NAME_LEN);
#else
        strncpy(ep->name,s_name,MAX_CFG_NAME_LEN);
#endif  /* (__unix__) */
        /* XML user data references endpoint index */
        mxmlSetUserData(xnode,(void*)ep->index);
      }
      xnode = mxmlGetNextSibling(xnode);
    }

    /* collect connections originating and terminating from this node */
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
        const char* s_b = mxmlElementGetAttr(xnode,"buffer");
        const char* s_l = mxmlElementGetAttr(xnode,"length");
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
          rc = MCAPI_FALSE;
          mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) missing name for channel\n",
              domain_id,node_id);
          xnode = mxmlGetNextSibling(xnode);
          continue;
        }
        else if(MCAPI_MAX_ENDPOINTS <= config->nchans) {
          rc = MCAPI_FALSE;
          mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) maximum number (%d) of channels exceeded\n",
              domain_id,node_id,MCAPI_MAX_ENDPOINTS);
          xnode = mxmlGetNextSibling(xnode);
          continue;
        }
        else {
          if(MAX_CFG_NAME_LEN < strlen(s_name)) {
            mcapi_dprintf(1,"WARNING: mcapi_trans_collect (%d %d) channel name %s truncated to %d characters\n",
                domain_id,node_id,s_name,MAX_CFG_NAME_LEN);
          }
          if(NULL == s_t) {
            rc = MCAPI_FALSE;
            mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) missing type for channel %s\n",
                domain_id,node_id,s_t);
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
#if !(__unix__)
          if(!_stricmp("msg",s_t)) {
            ct = MCAPI_NO_CHAN;
          }
          else if(!_stricmp("pkt",s_t)) {
            ct = MCAPI_PKT_CHAN;
          }
          else if(!_stricmp("scl",s_t)) {
            ct = MCAPI_SCL_CHAN;
          }
#else  /* (__unix__) */
          if(!strcasecmp("msg",s_t)) {
            ct = MCAPI_NO_CHAN;
          }
          else if(!strcasecmp("pkt",s_t)) {
            ct = MCAPI_PKT_CHAN;
          }
          else if(!strcasecmp("scl",s_t)) {
            ct = MCAPI_SCL_CHAN;
          }
#endif  /* (__unix__) */
          else {
            rc = MCAPI_FALSE;
            mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) invalid channel type %s\n",
                domain_id,node_id,s_t);
            xnode = mxmlGetNextSibling(xnode);
            continue;
          }
          bt = MCAPI_ENDP_ATTR_FIFO_BUFFER;
          bl = MCAPI_MAX_QUEUE_ENTRIES;
          if(NULL != s_b) {
#if !(__unix__)
            if(!_stricmp("fifo",s_b)) {
              bt = MCAPI_ENDP_ATTR_FIFO_BUFFER;
            }
            else if(!_stricmp("state",s_b)) {
              bt = MCAPI_ENDP_ATTR_STATE_BUFFER;
              bl = 4;
              if(NULL != s_l) {
                bl = atoi(s_l);
              }
            }
#else  /* (__unix__) */
            if(!strcasecmp("fifo",s_b)) {
              bt = MCAPI_ENDP_ATTR_FIFO_BUFFER;
            }
            else if(!strcasecmp("state",s_b)) {
              bt = MCAPI_ENDP_ATTR_STATE_BUFFER;
              bl = 4;
              if(NULL != s_l) {
                bl = atoi(s_l);
              }
            }
#endif  /* (__unix__) */
            else {
              rc = MCAPI_FALSE;
              mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) invalid buffer type %s\n",
                  domain_id,node_id,s_b);
              xnode = mxmlGetNextSibling(xnode);
              continue;
            }
          }
        }
        /* valid channel */
        c = &config->chan[config->nchans++];
        memset(c,0,sizeof(mcapi_chan_cfg_t));
        c->type = ct;
        c->buffer_type = bt;
        c->buffer_length = bl;
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
#endif  /* (__unix__) */
        /* XML user data references channel index */
        mxmlSetUserData(xnode,(void*)c->index);

        if(domain_id == fed->domain && node_id == fed->node) {
          /* channel connected to this node as source */
          c->direction = MCAPI_SEND_DIRECTION;
        }
        else if(domain_id == ted->domain && node_id == ted->node) {
          /* channel connected to this node as destination */
          c->direction = MCAPI_RECV_DIRECTION;
        }
        else {
          xnode = mxmlGetNextSibling(xnode);
          continue;
        }

        /* create and get endpoints associated with this channel */
        switch(c->direction) {
        case MCAPI_SEND_DIRECTION:
          /* create local endpoint */
          for(i = 0; i < config->nep; i++) {
            if(config->tuple[i].u.buf == from_ep->tuple->u.buf &&
                config->tuple[i].used) {
              mcapi_endpoint_cfg_t* ep = (mcapi_endpoint_cfg_t*)config->tuple[i].parent;
              rc = MCAPI_FALSE;
              mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) endpoint %s (%ld) tuple (%d %d %d) already in use by %s\n",
                  domain_id,node_id,s_from_name,from_ep->index,fed->domain,fed->node,fed->port,ep->name);
              break;
            }
          }
          if(i < config->nep) {
            xnode = mxmlGetNextSibling(xnode);
            continue;
          }

          c->from_ep->tuple->used = 1;
          c->from_ep->initialized = 1;

          mcapi_dprintf(6,"mcapi_trans_collect (%d %d) endpoint (local) %s (%d 0x%x) - domain: %d, node: %d, port: %d\n",
              domain_id,node_id,s_from_name,c->from_ep->index,c->from_ep->endpoint,fed->domain,fed->node,fed->port);

          /* Request destination endpoint from remote node */
          for(i = 0; i < config->nep; i++) {
            if(config->tuple[i].u.buf == to_ep->tuple->u.buf &&
                config->tuple[i].used) {
              mcapi_endpoint_cfg_t* ep = (mcapi_endpoint_cfg_t*)config->tuple[i].parent;
              rc = MCAPI_FALSE;
              mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) endpoint %s (%ld) tuple (%d %d %d) already in use by %s\n",
                 domain_id,node_id,s_to_name,c->to_ep->index,ted->domain,ted->node,ted->port,ep->name);
              break;
            }
          }
          if(i < config->nep) {
            xnode = mxmlGetNextSibling(xnode);
            continue;
          }

          c->to_ep->initialized = 1;
          nchan++;
          break;
        case MCAPI_RECV_DIRECTION:
          /* Create local endpoint */
          for(i = 0; i < config->nep; i++) {
            if(config->tuple[i].u.buf == to_ep->tuple->u.buf &&
                config->tuple[i].used) {
              mcapi_endpoint_cfg_t* ep = (mcapi_endpoint_cfg_t*)config->tuple[i].parent;
              rc = MCAPI_FALSE;
              mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) endpoint %s (%ld) tuple (%d %d %d) already in use by %s\n",
                  domain_id,node_id,s_to_name,c->to_ep->index,ted->domain,ted->node,ted->port,ep->name);
              break;
            }
          }
          if(i < config->nep) {
            xnode = mxmlGetNextSibling(xnode);
            continue;
          }

          c->to_ep->tuple->used = 1;
          c->to_ep->initialized = 1;

          mcapi_dprintf(6,"mcapi_trans_collect (%d %d) endpoint (local) %s (%d 0x%x) - domain: %d, node: %d, port: %d\n",
              domain_id,node_id,s_to_name,c->to_ep->index,c->to_ep->endpoint,ted->domain,ted->node,ted->port);

          /* Request source endpoint from remote node */
          for(i = 0; i < config->nep; i++) {
            if(config->tuple[i].u.buf == from_ep->tuple->u.buf &&
                config->tuple[i].used) {
              mcapi_endpoint_cfg_t* ep = (mcapi_endpoint_cfg_t*)config->tuple[i].parent;
              rc = MCAPI_FALSE;
              mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) endpoint %s (%ld) tuple (%d %d %d) already in use by %s\n",
                  domain_id,node_id,s_from_name,c->from_ep->index,fed->domain,fed->node,fed->port,ep->name);
              break;
            }
          }
          if(i < config->nep) {
            xnode = mxmlGetNextSibling(xnode);
            continue;
          }

          c->from_ep->initialized = 1;
          nchan++;
          break;
        }
      }
      xnode = mxmlGetNextSibling(xnode);
    }

    /* Bind channels into associated links */
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
          rc = MCAPI_FALSE;
          mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) missing name for link\n",domain_id,node_id);
          xnode = mxmlGetNextSibling(xnode);
          continue;
        }
        else if(MCAPI_MAX_ENDPOINTS <= nl) {
          rc = MCAPI_FALSE;
          mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) maximum number (%d) of links exceeded\n",
              domain_id,node_id,MCAPI_MAX_ENDPOINTS);
          xnode = mxmlGetNextSibling(xnode);
          continue;
        }
        else {
          if(MAX_CFG_NAME_LEN < strlen(s_name)) {
          mcapi_dprintf(1,"WARNING: mcapi_trans_collect (%d %d) link name %s truncated to %d characters\n",
              domain_id,node_id,s_name,MAX_CFG_NAME_LEN);
          }
          if(NULL == s_t) {
            rc = MCAPI_FALSE;
            mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) missing type for link %s\n",domain_id,node_id,s_t);
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
              /* valid link */
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
#endif  /* (__unix__) */

              mcapi_dprintf(6,"mcapi_trans_collect (%d %d) link (fullduplex) %s (%d)\n\tsend: %s (%d)\n\tack: %s (%d)\n",
                  domain_id,node_id,s_name,l->index,
                  s_send_name,sci,
                  s_ack_name,aci);

              /* XML user data references link index */
              mxmlSetUserData(xnode,(void*)l->index);
            }
          }
        }
        else {
          rc = MCAPI_FALSE;
          mcapi_dprintf(1,"ERROR: mcapi_trans_collect (%d %d) invalid link type %s\n",domain_id,node_id,s_t);
        }
      }
      xnode = mxmlGetNextSibling(xnode);
    }

    *mcapi_status = status;

    /* reset endpoint timeouts */
    for(i = 0; i< config->nep; i++) {
      mcapi_endpoint_tuple_t* t = &config->tuple[i];
      mca_begin_ts(&t->start);
    }

    mrapi_barrier_init(&axb_domains,0,(mrapi_msg_t*)mcapi_db->domains,
        MCA_MAX_DOMAINS,sizeof(mcapi_domain_entry),(unsigned*)&j,MCA_INFINITE,&status);
    assert(MRAPI_SUCCESS == status);

    /* mark node endpoints with indexes */
    for(i = 0; i < config->nep; i++) {
      mcapi_endpoint_tuple_t* t = &config->tuple[i];
      if(!config->endpoint[i].initialized) {
        continue;
      }
      /* find domain */
      for(j = 0; j < MCA_MAX_DOMAINS; j++) {
        mcapi_domain_state state;
        mrapi_atomic_read(&axb_domains,&mcapi_db->domains[j].state,&state,
            sizeof(mcapi_db->domains[j].state),&status);
        assert(MRAPI_SUCCESS == status);
        if(state.data.allocated &&
            state.data.domain_id == t->u.data.domain) {
          t->d_index = j;
          break;
        }
      }
      if(MCA_MAX_DOMAINS <= j) {
        /* domain not registered yet */
        if(MCA_INFINITE != timeout &&
            (double)timeout < mca_end_ts(&t->start)) {
          *mcapi_status = MCAPI_TIMEOUT;
          rc = MCAPI_FALSE;
          break;
        }
        sys_os_yield();
        i--;  /* repeat previous tuple */
        continue;
      }

      mrapi_barrier_init(&axb_nodes,0,(mrapi_msg_t*)mcapi_db->domains[t->d_index].nodes,
          MCA_MAX_NODES,sizeof(mcapi_node_entry),(unsigned*)&j,MCA_INFINITE,&status);
      assert(MRAPI_SUCCESS == status);

      /* find node */
      for(j = 0; j < MCA_MAX_NODES; j++) {
        mcapi_node_state state;
        mrapi_atomic_read(&axb_nodes,&mcapi_db->domains[t->d_index].nodes[j].state,&state,
            sizeof(mcapi_db->domains[t->d_index].nodes[j].state),&status);
        assert(MRAPI_SUCCESS == status);
        if(state.data.allocated &&
            state.data.node_num == t->u.data.node) {
          t->n_index = j;
          break;
        }
      }
      if(MCA_MAX_NODES <= j) {
        /* node not registered yet */
        if(MCA_INFINITE != timeout &&
            (double)timeout < mca_end_ts(&t->start)) {
          *mcapi_status = MCAPI_TIMEOUT;
          rc = MCAPI_FALSE;
          break;
        }
        sys_os_yield();
        i--;  /* repeat previous tuple */
        continue;
      }
    }

    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_connect
  DESCRIPTION: initialize endpoints and connect channels using parsed configuration
  PARAMETERS:
    domain_id - domain index
    node_id - node index
    config - address to configuration database
    timeout - number of attempts before abandoning connection
    mcapi_status - MCAPI_SUCCESS if all node connections complete, MCAPI_TIMEOUT otherwise
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_connect (mcapi_domain_t domain_id,
                                          mcapi_node_t node_id,
                                          mcapi_config_t* config,
                                          mca_timeout_t timeout,
                                          mcapi_status_t* mcapi_status)
  {
    mcapi_boolean_t rc = MCAPI_TRUE;

    int i = 0;
    int nchan = 0; /* number of active channels */
    size_t size = 0;
    mcapi_domain_t domain;
    mcapi_node_t node;
    mcapi_uint32_t d_index,n_index;
    mcapi_status_t status = MCAPI_SUCCESS;
    mcapi_buffer_type bt; /* channel buffer type */
    mcapi_uint_t bl; /* channel buffer length */

    mcapi_dprintf(1,"mcapi_trans_connect (%d %d)\n",domain_id,node_id);

    assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));

    /* establish connections originating and terminating from this node */
    for(i = 0; i < config->nchans; i++) {
      mcapi_chan_cfg_t* c = &config->chan[i];
      /* create and get endpoints associated with this channel */
      switch(c->direction) {
      case MCAPI_SEND_DIRECTION:
        /* create local endpoint */
        if(c->from_ep->initialized) {
          assert(mcapi_trans_endpoint_create(&c->from_ep->endpoint,c->from_ep->tuple->u.data.port,MCAPI_FALSE));
          mcapi_trans_endpoint_set_attribute(c->from_ep->endpoint,MCAPI_ENDP_ATTR_BUFFER_TYPE,&c->buffer_type,sizeof(c->buffer_type),&status);
          mcapi_trans_endpoint_set_attribute(c->from_ep->endpoint,MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS,&c->buffer_length,sizeof(c->buffer_length),&status);
          assert(MCAPI_SUCCESS == status);
          c->from_ep->valid = 1;

          mcapi_dprintf(6,"mcapi_trans_connect (%d %d) endpoint (local) %s (%d 0x%x) - domain: %d, node: %d, port: %d\n",
              domain_id,node_id,c->from_ep->name,c->from_ep->index,c->from_ep->endpoint,
              c->from_ep->tuple->u.data.domain,c->from_ep->tuple->u.data.node,c->from_ep->tuple->u.data.port);
        }

        /* Request destination endpoint from remote node */
        if(c->to_ep->initialized) {
          mcapi_trans_endpoint_get_i(&c->to_ep->endpoint,
              c->to_ep->tuple->u.data.domain,c->to_ep->tuple->u.data.node,c->to_ep->tuple->u.data.port,&c->request,&status);
          assert(MCAPI_SUCCESS == status);
          nchan++;
        }
        break;
      case MCAPI_RECV_DIRECTION:
        /* Create local endpoint */
        if(c->to_ep->initialized) {
          assert(mcapi_trans_endpoint_create(&c->to_ep->endpoint,c->to_ep->tuple->u.data.port,MCAPI_FALSE));
          mcapi_trans_endpoint_set_attribute(c->to_ep->endpoint,MCAPI_ENDP_ATTR_BUFFER_TYPE,&c->buffer_type,sizeof(c->buffer_type),&status);
          mcapi_trans_endpoint_set_attribute(c->to_ep->endpoint,MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS,&c->buffer_length,sizeof(c->buffer_length),&status);
          assert(MCAPI_SUCCESS == status);
          c->to_ep->valid = 1;

          mcapi_dprintf(6,"mcapi_trans_connect (%d %d) endpoint (local) %s (%d 0x%x) - domain: %d, node: %d, port: %d\n",
              domain_id,node_id,c->to_ep->name,c->to_ep->index,c->to_ep->endpoint,
              c->to_ep->tuple->u.data.domain,c->to_ep->tuple->u.data.node,c->to_ep->tuple->u.data.port);
        }

        /* Request source endpoint from remote node */
        if(c->from_ep->initialized) {
          mcapi_trans_endpoint_get_i(&c->from_ep->endpoint,
              c->to_ep->tuple->u.data.domain,c->to_ep->tuple->u.data.node,c->to_ep->tuple->u.data.port,&c->request,&status);
          assert(MCAPI_SUCCESS == status);
          nchan++;
        }
        break;
      }
    }

    /* Complete remote endpoint get requests and create connections asynchronously */
    while(0 < nchan) {
      for(i = 0; i < config->nchans; i++) {
        mcapi_chan_cfg_t* c = &config->chan[i];
        if(0 == c->request) {
          continue;
        }
        if(!mcapi_trans_wait(&c->request,&size,&status,timeout)) {
          rc = MCAPI_FALSE;
          mcapi_dprintf(1,"ERROR: mcapi_trans_endpoint_get request failed\n");
        }
        else {
          assert(MCAPI_SUCCESS == status);
          --nchan;
          switch(c->direction) {
          case MCAPI_SEND_DIRECTION:
            mcapi_dprintf(6,"mcapi_trans_connect (%d %d) endpoint (remote) %s (%d 0x%x) - domain: %d, node: %d\n",
                domain_id,node_id,c->to_ep->name,c->to_ep->index,c->to_ep->endpoint,
                c->to_ep->tuple->path.domain,
                c->to_ep->tuple->path.node);
            c->to_ep->tuple->used = 1;
            c->to_ep->valid = 1;
            break;
          case MCAPI_RECV_DIRECTION:
            mcapi_dprintf(6,"mcapi_trans_connect (%d %d) endpoint (remote) %s (%d 0x%x) - domain: %d, node: %d\n",
                domain_id,node_id,c->from_ep->name,c->from_ep->index,c->from_ep->endpoint,
                c->from_ep->tuple->path.domain,
                c->from_ep->tuple->path.node);
            c->from_ep->tuple->used = 1;
            c->from_ep->valid = 1;
            break;
          }
        }
      }
    }

    /* Create the connections asynchronously */
    for(i = 0; i < config->nchans; i++) {
      mcapi_chan_cfg_t* c = &config->chan[i];
      assert(!c->from_ep->initialized || c->from_ep->valid);
      assert(!c->to_ep->initialized || c->to_ep->valid);
      if(!c->from_ep->valid || !c->to_ep->valid) {
        continue;
      }
      switch(c->type) {
      case MCAPI_NO_CHAN:
        break;
      case MCAPI_PKT_CHAN:
        mcapi_trans_pktchan_connect_i(c->from_ep->endpoint,c->to_ep->endpoint,&c->request,&status);
        c->state = MCAPI_CONN_STATE;
        break;
      case MCAPI_SCL_CHAN:
        mcapi_trans_sclchan_connect_i(c->from_ep->endpoint,c->to_ep->endpoint,&c->request,&status);
        c->state = MCAPI_CONN_STATE;
        break;
      }

      c->valid = 1;

      mcapi_dprintf(6,"mcapi_trans_connect (%d %d) channel %s (%d)\n\tfrom: %s (%d)\n\tto: %s (%d)\n",
          domain_id,node_id,c->name,c->index,
          c->from_ep->name,c->from_ep->index,
          c->to_ep->name,c->to_ep->index);
    }

    /* complete connection requests and open communication */
    for(i = 0; i < config->nchans; i++) {
      mcapi_chan_cfg_t* c = &config->chan[i];
      if(c->valid) {

        /* Confirm endpoint properties match */
        switch(c->direction) {
        case MCAPI_SEND_DIRECTION:
          if(!c->to_ep->valid) {
            continue;
          }
          mcapi_trans_endpoint_get_attribute(c->to_ep->endpoint,MCAPI_ENDP_ATTR_BUFFER_TYPE,&bt,sizeof(bt),&status);
          if(MCAPI_SUCCESS != status ||
              c->buffer_type != bt) {
            c->to_ep->valid = MCAPI_FALSE;
            continue;
          }
          mcapi_trans_endpoint_get_attribute(c->to_ep->endpoint,MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS,&bl,sizeof(bl),&status);
          if(MCAPI_SUCCESS != status ||
              c->buffer_length != bl) {
            c->to_ep->valid = MCAPI_FALSE;
            continue;
          }
          break;
        case MCAPI_RECV_DIRECTION:
          if(!c->from_ep->valid) {
            continue;
          }
          mcapi_trans_endpoint_get_attribute(c->from_ep->endpoint,MCAPI_ENDP_ATTR_BUFFER_TYPE,&bt,sizeof(bt),&status);
          if(MCAPI_SUCCESS != status ||
              c->buffer_type != bt) {
            c->from_ep->valid = MCAPI_FALSE;
            continue;
          }
          mcapi_trans_endpoint_get_attribute(c->from_ep->endpoint,MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS,&bl,sizeof(bl),&status);
          if(MCAPI_SUCCESS != status ||
              c->buffer_length != bl) {
            c->from_ep->valid = MCAPI_FALSE;
            continue;
          }
          break;
        }

        switch(c->type) {
        case MCAPI_NO_CHAN:
          break;
        case MCAPI_PKT_CHAN:
        case MCAPI_SCL_CHAN:
          if(0 != c->request) {
            status = MCAPI_SUCCESS;
            if(!mcapi_trans_wait(&c->request,&size,&status,timeout)) {
              rc = MCAPI_FALSE;
              mcapi_dprintf(1,"ERROR: mcapi_trans_connect channel connect request failed\n");
            }
            else {
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
              break;
            }
          }
          break;
        }
      }
    }

    /* complete open requests */
    for(i = 0; i < config->nchans; i++) {
      mcapi_chan_cfg_t* c = &config->chan[i];
      if(c->valid) {
        if(0 == c->request) {
          c->state = MCAPI_ACTIVE_STATE;
        }
        else {
          status = MCAPI_SUCCESS;
          if(mcapi_trans_wait(&c->request,&size,&status,timeout)) {
            c->state = MCAPI_ACTIVE_STATE;
          }
          else {
            rc = MCAPI_FALSE;
            mcapi_dprintf(1,"ERROR: mcapi_trans_connect (%d %d) channel %s failed to open\n",
                domain_id,node_id,c->name);
          }
        }
      }
    }
    *mcapi_status = status;
    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_disconnect
  DESCRIPTION: disconnect channels and release endpoints based on XML configuration
  PARAMETERS:
    domain_id - domain index
    node_id - node index
    config - address to configuration database
    timeout - number of attempts before abandoning disconnect
    mcapi_status - MCAPI_SUCCESS if all node connections rundown, MCAPI_TIMEOUT otherwise
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_disconnect (mcapi_domain_t domain_id,
                                          mcapi_node_t node_id,
                                          mcapi_config_t* config,
                                          mca_timeout_t timeout,
                                          mcapi_status_t* mcapi_status)
  {
    mcapi_boolean_t rc = MCAPI_TRUE;

    int i = 0;
    mcapi_domain_t domain;
    mcapi_node_t node;
    mcapi_uint32_t d_index,n_index;
    mcapi_status_t status = MCAPI_SUCCESS;
    size_t size = 0;

    assert(mcapi_trans_whoami(&node,&n_index,&domain,&d_index));

    mcapi_trans_rundown();

    /* request to close the channel ends controlled by this node */
    for(i = 0; i < config->nchans; i++) {
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

    /* complete close requests */
    for(i = 0; i < config->nchans; i++) {
      mcapi_chan_cfg_t* c = &config->chan[i];
      if(c->valid) {
        if(0 != c->request) {
          status = MCAPI_SUCCESS;
          if(mcapi_trans_wait(&c->request,&size,&status,timeout)) {
            c->state = MCAPI_INVALID_STATE;
          }
          else {
            rc = MCAPI_FALSE;
            mcapi_dprintf(1,"ERROR: mcapi_trans_disconnect (%d %d) channel %s failed to close\n",
                domain_id,node_id,c->name);
          }
        }
        c->valid = 0;
      }
    }

    /* delete the endpoints controlled by this node */
    for(i = 0; i < config->nchans; i++) {
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
    *mcapi_status = status;
    return rc;
  }
