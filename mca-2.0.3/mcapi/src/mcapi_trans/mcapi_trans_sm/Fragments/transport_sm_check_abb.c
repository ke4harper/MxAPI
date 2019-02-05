  //////////////////////////////////////////////////////////////////////////////
  //                                                                          //
  //                   mcapi_trans API: error checking routines               //
  //                                                                          //
  //////////////////////////////////////////////////////////////////////////////
  /***************************************************************************
  NAME: mcapi_trans_channel_type
  DESCRIPTION: Given an endpoint, returns the type of channel (if any)
   associated with it.
  PARAMETERS: endpoint: the endpoint to be checked
  RETURN VALUE: the type of the channel (pkt,scalar or none)
  ***************************************************************************/
  channel_type mcapi_trans_channel_type (mcapi_endpoint_t endpoint)
  {
    uint16_t d,n,e;
    int rc;

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));
    rc = mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type;

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_send_endpoint
  DESCRIPTION:checks if the given endpoint is a send endpoint
  PARAMETERS: endpoint: the endpoint to be checked
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_send_endpoint (mcapi_endpoint_t endpoint)
  {
    uint16_t d,n,e;
    int rc = MCAPI_TRUE;

    mcapi_dprintf(2,"mcapi_trans_send_endpoint(0x%x);",endpoint);

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));
    if ((mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.connected) &&
        (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.state.data.recv_endpt == endpoint)) {
      /* this endpoint has already been marked as a receive endpoint */
      mcapi_dprintf(2,"mcapi_trans_send_endpoint ERROR: this endpoint (0x%x) has already been connected as a receive endpoint",
                    endpoint);
      rc = MCAPI_FALSE;
    }

    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_recv_endpoint
  DESCRIPTION:checks if the given endpoint can be or is already a receive endpoint
  PARAMETERS: endpoint: the endpoint to be checked
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_recv_endpoint (mcapi_endpoint_t endpoint)
  {
    uint16_t d,n,e;
    int rc = MCAPI_TRUE;

    mcapi_dprintf(2,"mcapi_trans_recv_endpoint(0x%x);",endpoint);

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));
    if ((mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.connected) &&
        (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.state.data.send_endpt == endpoint)) {
      /* this endpoint has already been marked as a send endpoint */
      mcapi_dprintf(2,"mcapi_trans_recv_endpoint ERROR: this endpoint (0x%x) has already been connected as a send endpoint",
                    endpoint);
      rc = MCAPI_FALSE;
    }

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_valid_port
  DESCRIPTION:checks if the given port_num is a valid port_num for this system
  PARAMETERS: port_num: the port num to be checked
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_port(mcapi_uint_t port_num)
  {
    return MCAPI_TRUE;
  }

  /***************************************************************************
  NAME:mcapi_trans_valid_node
  DESCRIPTION: checks if the given node_num is a valid node_num for this system
  PARAMETERS: node_num: the node num to be checked
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_node(mcapi_uint_t node_num)
  {
    return MCAPI_TRUE;
  }

  /***************************************************************************
  NAME: mcapi_trans_valid_endpoint
  DESCRIPTION: checks if the given endpoint handle refers to a valid endpoint
  PARAMETERS: endpoint
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_endpoint (mcapi_endpoint_t endpoint)
  {
    uint16_t d,n,e;
    int rc = MCAPI_FALSE;

    mcapi_dprintf(2,"mcapi_trans_valid_endpoint(0x%x);",endpoint);

    if (mcapi_trans_decode_handle(endpoint,&d,&n,&e)) {
      rc = ( mcapi_db->domains[d].state.data.valid &&
          mcapi_db->domains[d].nodes[n].state.data.valid &&
          mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.valid);
    }

    mcapi_dprintf(2,"mcapi_trans_valid_endpoint endpoint=0x%llx (database indices: n=%u,e=%u) rc=%u",
                  (unsigned long long)endpoint,n,e,rc);

    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_endpoint_exists
  DESCRIPTION: checks if an endpoint has been created for this port id
  PARAMETERS: port id
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_endpoint_exists (mcapi_domain_t domain_id,
                                               uint32_t port_num)
  {
    uint32_t d = 0;
    uint32_t n = 0;
    uint32_t i;
    mcapi_node_t node_id;
    int rc = MCAPI_FALSE;

    if (port_num == MCAPI_PORT_ANY) {
      return rc;
    }

    mcapi_assert(mcapi_trans_whoami(&node_id,&n,&domain_id,&d));

    /* Note: we can't just iterate for i < num_endpoints because endpoints can
       be deleted which would fragment the endpoints array. */
    for (i = 0; i < MCAPI_MAX_ENDPOINTS; i++) {
      endpoint_state state = mcapi_db->domains[d].nodes[n].node_d.endpoints[i].state;
      if (state.data.valid &&
          port_num == state.data.port_num) {
        rc = MCAPI_TRUE;
        break;
      }
    }

    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_valid_endpoints
  DESCRIPTION: checks if the given endpoint handles refer to valid endpoints
  PARAMETERS: endpoint1, endpoint2
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_endpoints (mcapi_endpoint_t endpoint1,
                                               mcapi_endpoint_t endpoint2)
  {
    uint16_t d1,n1,e1;
    uint16_t d2,n2,e2;
    mcapi_boolean_t rc = MCAPI_FALSE;

    if (mcapi_trans_decode_handle(endpoint1,&d1,&n1,&e1) &&
        mcapi_db->domains[d1].nodes[n1].node_d.endpoints[e1].state.data.valid &&
        mcapi_trans_decode_handle(endpoint2,&d2,&n2,&e2) &&
        mcapi_db->domains[d2].nodes[n2].node_d.endpoints[e2].state.data.valid) {
      rc = MCAPI_TRUE;
    }

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_pktchan_recv_isopen
  DESCRIPTION:checks if the channel is open for a given handle
  PARAMETERS: receive_handle
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_pktchan_recv_isopen (mcapi_pktchan_recv_hndl_t receive_handle)
  {
    uint16_t d,n,e;
    int rc = MCAPI_FALSE;

    mcapi_assert(mcapi_trans_decode_handle(receive_handle,&d,&n,&e));
    rc = (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.open);

    return rc;
  }


  /***************************************************************************
  NAME:mcapi_trans_pktchan_send_isopen
  DESCRIPTION:checks if the channel is open for a given handle
  PARAMETERS: send_handle
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_pktchan_send_isopen (mcapi_pktchan_send_hndl_t send_handle)
  {
    uint16_t d,n,e;
    int rc = MCAPI_FALSE;

    mcapi_assert(mcapi_trans_decode_handle(send_handle,&d,&n,&e));
    rc =  (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.open);

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_sclchan_recv_isopen
  DESCRIPTION:checks if the channel is open for a given handle
  PARAMETERS: receive_handle
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_sclchan_recv_isopen (mcapi_sclchan_recv_hndl_t receive_handle)
  {
    uint16_t d,n,e;
    int rc = MCAPI_FALSE;

    mcapi_assert(mcapi_trans_decode_handle(receive_handle,&d,&n,&e));
    rc = (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.open);

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_sclchan_send_isopen
  DESCRIPTION:checks if the channel is open for a given handle
  PARAMETERS: send_handle
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_sclchan_send_isopen (mcapi_sclchan_send_hndl_t send_handle)
  {
    uint16_t d,n,e;
    int rc = MCAPI_FALSE;

    mcapi_assert(mcapi_trans_decode_handle(send_handle,&d,&n,&e));
    rc = (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.open);

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_endpoint_channel_isopen
  DESCRIPTION:checks if a channel is open for a given endpoint
  PARAMETERS: endpoint
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_endpoint_channel_isopen (mcapi_endpoint_t endpoint)
  {
    uint16_t d,n,e;
    int rc = MCAPI_FALSE;

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));
    rc =  (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.open);

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_endpoint_isowner
  DESCRIPTION:checks if the given endpoint is owned by the calling node
  PARAMETERS: endpoint
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_endpoint_isowner (mcapi_endpoint_t endpoint)
  {
    uint16_t d,n,e;
    mcapi_node_t node_num=0;
    int rc = MCAPI_FALSE;

    mcapi_assert(mcapi_trans_get_node_num(&node_num));

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));
    rc = ((mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.valid) &&
          (mcapi_db->domains[d].nodes[n].state.data.node_num == node_num));

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_channel_connected
  DESCRIPTION:checks if the given endpoint channel is connected
  PARAMETERS: endpoint
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_channel_connected (mcapi_endpoint_t endpoint)
  {
    uint16_t d,n,e;
    endpoint_state state;
    int rc = MCAPI_FALSE;

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));
    state = mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state;
    rc = (state.data.valid &&
        state.data.connected);

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_compatible_endpoint_attributes
  DESCRIPTION:checks if the given endpoints have compatible attributes
  PARAMETERS: send_endpoint,recv_endpoint
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_compatible_endpoint_attributes (mcapi_endpoint_t send_endpoint,
                                                              mcapi_endpoint_t recv_endpoint)
  {
    /* FIXME: (errata A3) currently un-implemented */
    return MCAPI_TRUE;
  }

  /***************************************************************************
  NAME:mcapi_trans_valid_pktchan_send_handle
  DESCRIPTION:checks if the given pkt channel send handle is valid
  PARAMETERS: handle
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_pktchan_send_handle( mcapi_pktchan_send_hndl_t handle)
  {
    uint16_t d,n,e;
    channel_type type;

    int rc = MCAPI_FALSE;

    mcapi_dprintf (2,"mcapi_trans_valid_pktchan_send_handle (0x%x);",handle);

    type =MCAPI_PKT_CHAN;
    if (mcapi_trans_decode_handle(handle,&d,&n,&e)) {
      if (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type == type) {
        rc = MCAPI_TRUE;
      } else {
        mcapi_dprintf(2,"mcapi_trans_valid_pktchan_send_handle node=%u,port=%u returning false channel_type != MCAPI_PKT_CHAN",
                      mcapi_db->domains[d].nodes[n].state.data.node_num,
                      mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.port_num);
      }
    }

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_valid_pktchan_recv_handle
  DESCRIPTION:checks if the given pkt channel recv handle is valid
  PARAMETERS: handle
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_pktchan_recv_handle( mcapi_pktchan_recv_hndl_t handle)
  {
    uint16_t d,n,e;
    channel_type type;
    int rc = MCAPI_FALSE;

    mcapi_dprintf (2,"mcapi_trans_valid_pktchan_recv_handle (0x%x);",handle);

    type = MCAPI_PKT_CHAN;
    if (mcapi_trans_decode_handle(handle,&d,&n,&e)) {
      if (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type == type) {
        rc = MCAPI_TRUE;
      } else {
        mcapi_dprintf(2,"mcapi_trans_valid_pktchan_recv_handle node=%u,port=%u returning false channel_type != MCAPI_PKT_CHAN",
                      mcapi_db->domains[d].nodes[n].state.data.node_num,
                      mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.port_num);
      }
    }

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_valid_sclchan_send_handle
  DESCRIPTION: checks if the given scalar channel send handle is valid
  PARAMETERS: handle
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_sclchan_send_handle( mcapi_sclchan_send_hndl_t handle)
  {
    uint16_t d,n,e;
    channel_type type;
    int rc = MCAPI_FALSE;

    mcapi_dprintf (2,"mcapi_trans_valid_sclchan_send_handle (0x%x);",handle);

    type = MCAPI_SCL_CHAN;
    if (mcapi_trans_decode_handle(handle,&d,&n,&e)) {
      if (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type == type) {
        rc = MCAPI_TRUE;
      } else {
        mcapi_dprintf(2,"mcapi_trans_valid_sclchan_send_handle node=%u,port=%u returning false channel_type != MCAPI_SCL_CHAN",
                      mcapi_db->domains[d].nodes[n].state.data.node_num,
                      mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.port_num);
      }
    }

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_valid_sclchan_recv_handle
  DESCRIPTION:checks if the given scalar channel recv handle is valid
  PARAMETERS:
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_sclchan_recv_handle( mcapi_sclchan_recv_hndl_t handle)
  {
    uint16_t d,n,e;
    channel_type type;

    int rc = MCAPI_FALSE;

    mcapi_dprintf (2,"mcapi_trans_valid_sclchan_recv_handle (0x%x);",handle);

    type= MCAPI_SCL_CHAN;
    if (mcapi_trans_decode_handle(handle,&d,&n,&e)) {
      if (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type == type) {
        rc = MCAPI_TRUE;
      } else {
        mcapi_dprintf(2,"mcapi_trans_valid_sclchan_recv_handle node=%u,port=%u returning false channel_type != MCAPI_SCL_CHAN",
                      mcapi_db->domains[d].nodes[n].state.data.node_num,
                      mcapi_db->domains[d].nodes[n].node_d.endpoints[e].state.data.port_num);
      }
    }

    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_initialized
  DESCRIPTION: checks if the calling node has called initialize
  PARAMETERS:
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_initialized ()
  {
    uint32_t d,n;
    mcapi_node_t node;
    mcapi_domain_t domain;
    mcapi_boolean_t rc = MCAPI_FALSE;

    mcapi_dprintf (1,"mcapi_trans_initialized();");

    rc = mcapi_trans_whoami(&node,&n,&domain,&d);

    return rc;
  }

  /***************************************************************************
  NAME: mcapi_trans_num_endpoints
  DESCRIPTION: returns the current number of endpoints for the calling node
  PARAMETERS:  none
  RETURN VALUE: num_endpoints
  ***************************************************************************/
  mcapi_uint32_t mcapi_trans_num_endpoints()
  {
    uint32_t d = 0;
    uint32_t n = 0;
    uint32_t rc = 0;
    mcapi_node_t node_id=0;
    mcapi_domain_t domain_id=0;

    mcapi_assert(mcapi_trans_whoami(&node_id,&n,&domain_id,&d));

    mcapi_dprintf (2,"mcapi_trans_num_endpoints (0x%x);",domain_id);

    rc = mcapi_db->domains[d].nodes[n].node_d.num_endpoints;

    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_valid_priority
  DESCRIPTION:checks if the given priority level is valid
  PARAMETERS: priority
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_priority(mcapi_priority_t priority)
  {
    return ((priority >=0) && (priority <= MCAPI_MAX_PRIORITY));
  }

  /***************************************************************************
  NAME:mcapi_trans_connected
  DESCRIPTION: checks if the given endpoint is connected
  PARAMETERS: endpoint
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_connected(mcapi_endpoint_t endpoint)
  {
    mcapi_boolean_t rc = MCAPI_FALSE;
    uint16_t d,n,e;

    mcapi_dprintf (2,"mcapi_trans_connected (0x%x);",endpoint);

    rc = (mcapi_trans_decode_handle(endpoint,&d,&n,&e) &&
          (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.channel_type != MCAPI_NO_CHAN));
    return rc;
  }

  /***************************************************************************
  NAME:valid_status_param
  DESCRIPTION: checks if the given status is a valid status parameter
  PARAMETERS: status
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_status_param (mcapi_status_t* mcapi_status)
  {
    return (mcapi_status != NULL);
  }

  /***************************************************************************
  NAME:mcapi_trans_valid_version_param
  DESCRIPTION: checks if the given version is a valid version parameter
  PARAMETERS: version
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_version_param (mcapi_info_t* mcapi_version)
  {
    return (mcapi_version != NULL);
  }


  /***************************************************************************
  NAME:mcapi_trans_valid_buffer_param
  DESCRIPTION:checks if the given buffer is a valid buffer parameter
  PARAMETERS: buffer
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_buffer_param (void* buffer)
  {
    return (buffer != NULL);
  }


  /***************************************************************************
  NAME: mcapi_trans_valid_request_handle
  DESCRIPTION:checks if the given request handle is valid
        This is the parameter test/wait/cancel pass in to be processed.
  PARAMETERS: request
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_request_handle (mcapi_request_t* request)
  {
    uint16_t r;
    return (mcapi_trans_decode_request_handle(request,&r));
  }


  /***************************************************************************
  NAME:mcapi_trans_valid_size_param
  DESCRIPTION: checks if the given size is a valid size parameter
  PARAMETERS: size
  RETURN VALUE:MCAPI_TRUE/MCAPI_FALSE
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_valid_size_param (size_t* size)
  {
    return (size != NULL);
  }
