//////////////////////////////////////////////////////////////////////////////
  //                                                                          //
  //                   mcapi_trans API: endpoints                             //
  //                                                                          //
  //////////////////////////////////////////////////////////////////////////////


  /***************************************************************************
  NAME:mcapi_trans_endpoint_create_
  DESCRIPTION:create endpoint <node_num,port_num> and return it's handle
  PARAMETERS:
       ep - the endpoint to be filled in
       port_num - the port id (only valid if !anonymous)
       anonymous - whether or not this should be an anonymous endpoint
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_endpoint_create (mcapi_endpoint_t* ep,
                                               mcapi_uint_t port_num,
                                               mcapi_boolean_t anonymous)
  {
    uint32_t domain_index = 0;
    uint32_t node_index = 0;
    uint32_t i, endpoint_index;
    mcapi_boolean_t rc = MCAPI_FALSE;
    mcapi_domain_t domain_id2;
    mcapi_node_t node_num2;
    mrapi_atomic_barrier_t axb_endpoints;
    mrapi_atomic_barrier_t axb_nodes;

    mcapi_dprintf (1,"mcapi_trans_endpoint_create (0x%x,%u);",port_num,anonymous);

    mcapi_assert(mcapi_trans_whoami(&node_num2,&node_index,&domain_id2,&domain_index));

    /* make sure there's room - mcapi should have already checked this */
    mcapi_assert (mcapi_db->domains[domain_index].nodes[node_index].node_d.num_endpoints < MCAPI_MAX_ENDPOINTS);

    /* create the endpoint */
    /* find the first available endpoint index */
    endpoint_index = MCAPI_MAX_ENDPOINTS;
    for (i = 0; i < MCAPI_MAX_ENDPOINTS; i++) {
      if (! mcapi_db->domains[domain_index].nodes[node_index].node_d.endpoints[i].state.data.valid) {
        endpoint_index = i;
        break;
      }
    }

    if (endpoint_index < MCAPI_MAX_ENDPOINTS) {
	  endpoint_entry* entry = &mcapi_db->domains[domain_index].nodes[node_index].node_d.endpoints[endpoint_index];
      endpoint_state* state = &entry->state;
      endpoint_state newstate = *state;
      mrapi_status_t status;

      mrapi_barrier_init(&axb_nodes,0,(mrapi_msg_t*)mcapi_db->domains[domain_index].nodes,
        MCA_MAX_NODES,sizeof(mcapi_node_entry),&node_index,MCA_INFINITE,&status);
      assert(MRAPI_SUCCESS == status);
      mrapi_barrier_init(&axb_endpoints,0,(mrapi_msg_t*)mcapi_db->domains[domain_index].nodes[node_index].node_d.endpoints,
        MCAPI_MAX_ENDPOINTS,sizeof(endpoint_entry),&endpoint_index,MCA_INFINITE,&status);
      assert(MRAPI_SUCCESS == status);

      /* initialize the endpoint entry*/
      entry->anonymous = anonymous;
      entry->num_attributes = 0;
	  memset(&entry->recv_queue,0,sizeof(entry->recv_queue));
      newstate.data.port_num = port_num;
      newstate.data.open = MCAPI_FALSE;
      newstate.data.valid = MCAPI_TRUE;
      mrapi_atomic_xchg(&axb_endpoints,state,&newstate,NULL,sizeof(entry->state),&status);
      mrapi_atomic_inc(&axb_nodes,&mcapi_db->domains[domain_index].nodes[node_index].node_d.num_endpoints,NULL,
          sizeof(mcapi_db->domains[domain_index].nodes[node_index].node_d.num_endpoints),&status);

      /* set the handle */
      *ep = mcapi_trans_encode_handle (domain_index,node_index,endpoint_index);

      rc = MCAPI_TRUE;
    }

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_endpoint_get_attribute
  DESCRIPTION:
  PARAMETERS:
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_endpoint_get_attribute(
                                          mcapi_endpoint_t endpoint,
                                          mcapi_uint_t attribute_num,
                                          void* attribute,
                                          size_t attribute_size,
                                          mcapi_status_t* mcapi_status)
  {
    uint16_t d,n,e;
    int* attr = (int*)attribute;
    endpoint_entry* ep = NULL;
    attribute_entry_t* ae = NULL;

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));
    ep = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e];

    switch(attribute_num) {
    case MCAPI_ENDP_ATTR_TYPE:
      *attr = ep->recv_queue.channel_type;
      *mcapi_status = MCAPI_SUCCESS;
      break;
    case MCAPI_ENDP_ATTR_NUM_RECV_BUFFERS:
      *attr = MCAPI_MAX_QUEUE_ENTRIES -
        mcapi_trans_queue_elements(&mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue);
      *mcapi_status = MCAPI_SUCCESS;
      break;
    case MCAPI_ENDP_ATTR_BUFFER_TYPE:
      ae = &ep->attributes.entries[MCAPI_ENDP_ATTR_BUFFER_TYPE];
      *attr = ae->attribute_d.type;
      *mcapi_status = MCAPI_SUCCESS;
      break;
    case MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS:
      ae = &ep->attributes.entries[MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS];
      *attr = ae->attribute_d.value;
      *mcapi_status = MCAPI_SUCCESS;
      break;
    default:
      *mcapi_status = MCAPI_ERR_ATTR_NUM;
      break;
    }
  }

  /***************************************************************************
  NAME:mcapi_trans_endpoint_set_attribute
  DESCRIPTION:
  PARAMETERS:
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_endpoint_set_attribute(
                                          mcapi_endpoint_t endpoint,
                                          mcapi_uint_t attribute_num,
                                          const void* attribute,
                                          size_t attribute_size,
                                          mcapi_status_t* mcapi_status)
  {
    uint16_t d,n,e;
    int* attr = (int*)attribute;
    endpoint_entry* ep = NULL;
    attribute_entry_t* ae = NULL;

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));
    ep = &mcapi_db->domains[d].nodes[n].node_d.endpoints[e];

    switch(attribute_num) {
    case MCAPI_ENDP_ATTR_BUFFER_TYPE:
      ae = &ep->attributes.entries[MCAPI_ENDP_ATTR_BUFFER_TYPE];
      switch(*attr) {
      case MCAPI_ENDP_ATTR_FIFO_BUFFER:
      case MCAPI_ENDP_ATTR_STATE_BUFFER:
        ae->attribute_num = MCAPI_ENDP_ATTR_BUFFER_TYPE;
        ae->attribute_d.type = *attr;
        ae->valid = MCAPI_TRUE;
        *mcapi_status = MCAPI_SUCCESS;
        break;
      default:
        *mcapi_status = MCAPI_ERR_ATTR_VALUE;
        break;
      }
      break;
    case MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS:
      ae = &ep->attributes.entries[MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS];
      ae->attribute_num = MCAPI_ENDP_ATTR_NUM_SEND_BUFFERS;
      ae->attribute_d.value = *attr;
      ae->valid = MCAPI_TRUE;
      *mcapi_status = MCAPI_SUCCESS;
      break;
    default:
      *mcapi_status = MCAPI_ERR_ATTR_NUM;
      break;
    }
  }

  /***************************************************************************
  NAME:mcapi_trans_endpoint_get_i
  DESCRIPTION:non-blocking get endpoint for the given <node_num,port_num>
  PARAMETERS:
     endpoint - the endpoint handle to be filled in
     node_num - the node id
     port_num - the port id
     request - the request handle to be filled in when the task is complete
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_endpoint_get_i( mcapi_endpoint_t* endpoint,
                                   mcapi_domain_t domain_id,
                                   mcapi_uint_t node_num,
                                   mcapi_uint_t port_num,
                                   mcapi_request_t* request,
                                   mcapi_status_t* mcapi_status)
  {

    mcapi_boolean_t valid =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_TRUE : MCAPI_FALSE;
    mcapi_boolean_t completed = MCAPI_FALSE;
    int r;

    /* make sure we have an available request entry*/
    if ( mcapi_trans_reserve_request(&r)) {
      if (valid) {
        /* try to get the endpoint */
        if (mcapi_trans_endpoint_get_(endpoint,domain_id,node_num,port_num)) {
          completed = MCAPI_TRUE;
        }
      }

      mcapi_assert(setup_request(endpoint,request,mcapi_status,completed,0,NULL,GET_ENDPT,node_num,port_num,domain_id,r));
    }
  }

  /***************************************************************************
  NAME:mcapi_trans_endpoint_get_
  DESCRIPTION:get endpoint for the given <node_num,port_num>
  PARAMETERS:
     endpoint - the endpoint handle to be filled in
     node_num - the node id
     port_num - the port id
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_endpoint_get_(mcapi_endpoint_t *ep,
                                            mcapi_domain_t domain_id,
                                            mcapi_uint_t node_num,
                                            mcapi_uint_t port_num)
  {
    uint32_t d,n,e;

    mcapi_dprintf(3,"mcapi_trans_endpoint_get_(&ep,%u,%u,%u);",domain_id,node_num,port_num);

    // look for the endpoint <domain,node,port>
    for (d = 0; d < MCA_MAX_DOMAINS; d++) {
      mcapi_domain_state dstate = mcapi_db->domains[d].state;
      if (dstate.data.valid &&
          domain_id == dstate.data.domain_id) {
        for (n = 0; n < MCA_MAX_NODES; n++) {
          mcapi_node_state nstate = mcapi_db->domains[d].nodes[n].state;
          if (nstate.data.valid &&
              node_num == nstate.data.node_num) {
            // iterate over all the endpoints on this node
            for (e = 0; e < mcapi_db->domains[d].nodes[n].node_d.num_endpoints; e++) {
              endpoint_state estate = mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state;
              if (estate.data.valid &&
                  port_num == estate.data.port_num) {
                /* we found it, return the handle */
                *ep = mcapi_trans_encode_handle (d,n,e);
                return MCAPI_TRUE;
              }
            }
          }
        }
      }
    }

    return MCAPI_FALSE;
  }

  /***************************************************************************
  NAME:mcapi_trans_endpoint_get_blocking
  DESCRIPTION:blocking get endpoint for the given <node_num,port_num>
  PARAMETERS:
     endpoint - the endpoint handle to be filled in
     node_num - the node id
     port_num - the port id
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE indicating success or failure
  ***************************************************************************/
  void mcapi_trans_endpoint_get (mcapi_endpoint_t *e,
                                 mcapi_domain_t domain_id,
                                 mcapi_uint_t node_num,
                                 mcapi_uint_t port_num)
  {
    mcapi_dprintf(1,"mcapi_trans_endpoint_get d=%u,n=%u,p=%u",domain_id,node_num,port_num);

    while(!mcapi_trans_endpoint_get_ (e,domain_id,node_num,port_num)) {
      /* yield */
      mcapi_dprintf(5,"mcapi_trans_endpoint_get - attempting to yield");
      mcapi_trans_yield();
    }
  }

  /***************************************************************************
  NAME: mcapi_trans_endpoint_delete
  DESCRIPTION:delete the given endpoint
  PARAMETERS: endpoint
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_endpoint_delete( mcapi_endpoint_t endpoint)
  {
    uint16_t d,n,e;
    unsigned nindex;
    mrapi_status_t status;
    mrapi_atomic_barrier_t axb_nodes;

    mcapi_dprintf(1,"mcapi_trans_endpoint_delete(0x%x);",endpoint);

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));

    mcapi_dprintf(2,"mcapi_trans_endpoint_delete_have_lock node_num=%u, port_num=%u",
                  mcapi_db->domains[d].nodes[n].state.data.node_num,
                  mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.port_num);

    nindex = n;
    mrapi_barrier_init(&axb_nodes,0,(mrapi_msg_t*)mcapi_db->domains[d].nodes,
      MCA_MAX_NODES,sizeof(mcapi_node_entry),&nindex,MCA_INFINITE,&status);
    assert(MRAPI_SUCCESS == status);

    /* remove the endpoint */
    mrapi_atomic_dec(&axb_nodes,&mcapi_db->domains[d].nodes[n].node_d.num_endpoints,NULL,
        sizeof(mcapi_db->domains[d].nodes[n].node_d.num_endpoints),&status);
    /* zero out the old endpoint entry in the shared memory database */
    memset (&mcapi_db->domains[d].nodes[n].node_d.endpoints[e],0,sizeof(endpoint_entry));
  }
