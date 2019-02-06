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

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_dprintf (1,"mcapi_trans_endpoint_create (0x%x,%u);",port_num,anonymous);

    mcapi_assert(mcapi_trans_whoami(&node_num2,&node_index,&domain_id2,&domain_index));


    /* make sure there's room - mcapi should have already checked this */
    mcapi_assert (mcapi_db->domains[domain_index].nodes[node_index].node_d.num_endpoints < MCAPI_MAX_ENDPOINTS);

    /* create the endpoint */
    /* find the first available endpoint index */
    endpoint_index = MCAPI_MAX_ENDPOINTS;
    for (i = 0; i < MCAPI_MAX_ENDPOINTS; i++) {
      if (! mcapi_db->domains[domain_index].nodes[node_index].node_d.endpoints[i].valid) {
        endpoint_index = i;
        break;
      }
    }

    if (endpoint_index < MCAPI_MAX_ENDPOINTS) {

      /* initialize the endpoint entry*/
      mcapi_db->domains[domain_index].nodes[node_index].node_d.endpoints[endpoint_index].valid = MCAPI_TRUE;
      mcapi_db->domains[domain_index].nodes[node_index].node_d.endpoints[endpoint_index].port_num = port_num;
      mcapi_db->domains[domain_index].nodes[node_index].node_d.endpoints[endpoint_index].open = MCAPI_FALSE;
      mcapi_db->domains[domain_index].nodes[node_index].node_d.endpoints[endpoint_index].anonymous = anonymous;
      mcapi_db->domains[domain_index].nodes[node_index].node_d.endpoints[endpoint_index].num_attributes = 0;

      mcapi_db->domains[domain_index].nodes[node_index].node_d.num_endpoints++;

      /* set the handle */
      *ep = mcapi_trans_encode_handle (domain_index,node_index,endpoint_index);

      rc = MCAPI_TRUE;
    }

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));

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
    int* attr = attribute;

    *mcapi_status = MCAPI_ERR_ATTR_NUM;

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));

    if (attribute_num == MCAPI_ENDP_ATTR_NUM_RECV_BUFFERS) {
      *attr = MCAPI_MAX_QUEUE_ENTRIES -
        mcapi_db->domains[d].nodes[n].node_d.endpoints[e].recv_queue.num_elements;
      *mcapi_status = MCAPI_SUCCESS;
    }

    /* unlock the database */
    mcapi_assert (mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));

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

    fprintf(stderr,"WARNING: setting endpoint attributes not implemented\n");
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

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    /* make sure we have an available request entry*/
    if ( mcapi_trans_reserve_request_have_lock(&r)) {
      if (valid) {
        /* try to get the endpoint */
        if (mcapi_trans_endpoint_get_have_lock (endpoint,domain_id,node_num,port_num)) {
          completed = MCAPI_TRUE;
        }
      }

      mcapi_assert(setup_request_have_lock(endpoint,request,mcapi_status,completed,0,NULL,GET_ENDPT,node_num,port_num,domain_id,r));
    }

    /* unlock the database */
    mcapi_assert (mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }

  /***************************************************************************
  NAME:mcapi_trans_endpoint_get_have_lock
  DESCRIPTION:get endpoint for the given <node_num,port_num>
  PARAMETERS:
     endpoint - the endpoint handle to be filled in
     node_num - the node id
     port_num - the port id
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE indicating success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_endpoint_get_have_lock (mcapi_endpoint_t *ep,
                                                     mcapi_domain_t domain_id,
                                                     mcapi_uint_t node_num,
                                                     mcapi_uint_t port_num)
  {
    uint32_t d,n,e;

    /* the database should already be locked */
    assert(locked == 1);
    mcapi_dprintf(3,"mcapi_trans_endpoint_get_have_lock(&ep,%u,%u,%u);",domain_id,node_num,port_num);

    // look for the endpoint <domain,node,port>
    for (d = 0; d < MCA_MAX_DOMAINS; d++) {
      if (mcapi_db->domains[d].valid &&
          (mcapi_db->domains[d].domain_id == domain_id)) {
        for (n = 0; n < MCA_MAX_NODES; n++) {
          if (mcapi_db->domains[d].nodes[n].valid &&
              (mcapi_db->domains[d].nodes[n].node_num == node_num)) {
            // iterate over all the endpoints on this node
            for (e = 0; e < mcapi_db->domains[d].nodes[n].node_d.num_endpoints; e++) {
              if (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].valid &&
                  (mcapi_db->domains[d].nodes[n].node_d.endpoints[e].port_num == port_num)) {
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
  NAME:mcapi_trans_endpoint_get
  DESCRIPTION:blocking get endpoint for the given <node_num,port_num>
  PARAMETERS:
     endpoint - the endpoint handle to be filled in
     node_num - the node id
     port_num - the port id
  RETURN VALUE: MCAPI_TRUE/MCAPI_FALSE indicating success or failure
  ***************************************************************************/
  void mcapi_trans_endpoint_get(mcapi_endpoint_t *e,
                                mcapi_domain_t domain_id,
                                mcapi_uint_t node_num,
                                mcapi_uint_t port_num)
  {
    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));
    mcapi_dprintf(1,"mcapi_trans_endpoint_get d=%u,n=%u,p=%u",domain_id,node_num,port_num);

    while(!mcapi_trans_endpoint_get_have_lock (e,domain_id,node_num,port_num)) {
      /* yield */
      mcapi_dprintf(5,"mcapi_trans_endpoint_get - attempting to yield");
      mcapi_trans_yield_have_lock();
    }

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
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

    mcapi_dprintf(1,"mcapi_trans_endpoint_delete(0x%x);",endpoint);
    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_assert(mcapi_trans_decode_handle(endpoint,&d,&n,&e));

    mcapi_dprintf(2,"mcapi_trans_endpoint_delete_have_lock node_num=%u, port_num=%u",
                  mcapi_db->domains[d].nodes[n].node_num,mcapi_db->domains[d].nodes[n].node_d.endpoints[e].port_num);

    /* remove the endpoint */
    mcapi_db->domains[d].nodes[n].node_d.num_endpoints--;
    /* zero out the old endpoint entry in the shared memory database */
    memset (&mcapi_db->domains[d].nodes[n].node_d.endpoints[e],0,sizeof(endpoint_entry));

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));
  }
