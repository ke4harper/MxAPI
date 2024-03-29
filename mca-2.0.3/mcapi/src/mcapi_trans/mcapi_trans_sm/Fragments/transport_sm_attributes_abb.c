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
  NAME:mcapi_trans_node_init_attributes
  DESCRIPTION:
  PARAMETERS:
  RETURN VALUE:
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_node_init_attributes(
                                        mcapi_node_attributes_t* mcapi_node_attributes,
                                        mcapi_status_t* mcapi_status
                                        ) {
    mcapi_boolean_t rc = MCAPI_TRUE;
    /* default values are all 0 */
    memset(mcapi_node_attributes,0,sizeof(mcapi_node_attributes));
    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_node_get_attribute
  DESCRIPTION:
  PARAMETERS:
  RETURN VALUE:
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_node_get_attribute(
                                                 mcapi_domain_t domain_id,
                                                 mcapi_node_t node_id,
                                                 mcapi_uint_t attribute_num,
                                                 void* attribute,
                                                 size_t attribute_size,
                                                 mcapi_status_t* mcapi_status) {
    mcapi_boolean_t rc = MCAPI_FALSE;
    mcapi_boolean_t found_node = MCAPI_FALSE;
    mcapi_boolean_t found_domain = MCAPI_FALSE;
    uint32_t d,n;
    mcapi_domain_t my_domain_id;
    mcapi_node_t my_node_id;
    size_t size;

    if (!mcapi_trans_whoami(&my_node_id,&n,&my_domain_id,&d)) {
      *mcapi_status = MCAPI_ERR_NODE_NOTINIT;
    } else if (attribute_num != MCAPI_NODE_ATTR_TYPE_REGULAR) {
      /* only the node_attr_type attribute is currently supported */
      *mcapi_status = MCAPI_ERR_ATTR_NOTSUPPORTED;
    } else {
      // look for the <domain,node>
      for (d = 0; ((d < MCA_MAX_DOMAINS) && (found_domain == MCAPI_FALSE)); d++) {
        mcapi_domain_state dstate = mcapi_db->domains[d].state;
        if (dstate.data.valid &&
            domain_id == dstate.data.domain_id) {
          found_domain = MCAPI_TRUE;
          for (n = 0; ((n < MCA_MAX_NODES) &&  (found_node == MCAPI_FALSE)); n++) {
            mcapi_node_state nstate = mcapi_db->domains[d].nodes[n].state;
            if (nstate.data.valid &&
                node_id == nstate.data.node_num) {
              found_node = MCAPI_TRUE;
              size = mcapi_db->domains[d].nodes[n].attributes.entries[attribute_num].bytes;
              if (size != attribute_size) {
                *mcapi_status = MCAPI_ERR_ATTR_SIZE;
              } else {
                memcpy(attribute,
                    &mcapi_db->domains[d].nodes[n].attributes.entries[attribute_num].attribute_d,
                    size);
                rc = MCAPI_TRUE;
              }
            }
          }
        }
      }
      if (!found_domain) {
        *mcapi_status = MCAPI_ERR_DOMAIN_INVALID;
      } else if (!found_node) {
        *mcapi_status = MCAPI_ERR_NODE_INVALID;
      }
    }
    return rc;
  }

  /***************************************************************************
  NAME:mcapi_trans_node_set_attribute
  DESCRIPTION:
  PARAMETERS:
  RETURN VALUE:
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_node_set_attribute(
                                      mcapi_node_attributes_t* mcapi_node_attributes,
                                      mcapi_uint_t attribute_num,
                                      const void* attribute,
                                      size_t attribute_size,
                                      mcapi_status_t* mcapi_status
                                      ){
    mcapi_boolean_t rc = MCAPI_FALSE;

    if (attribute_num != MCAPI_NODE_ATTR_TYPE_REGULAR) {
      /* only the node_attr_type attribute is currently supported */
      *mcapi_status = MCAPI_ERR_ATTR_NOTSUPPORTED;
    } else if (attribute_size != sizeof(mcapi_node_attr_type_t) ) {
      *mcapi_status = MCAPI_ERR_ATTR_SIZE;
    } else {
      rc = MCAPI_TRUE;
      /* copy the attribute into the attributes data structure */
      memcpy(&mcapi_node_attributes->entries[attribute_num].attribute_d,
             attribute,
             attribute_size);
    }
    return rc;
  }
