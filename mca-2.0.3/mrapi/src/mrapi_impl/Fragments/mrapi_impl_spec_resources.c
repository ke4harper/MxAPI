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
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

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
  Function: mrapi_impl_resources_get

  Description: An implementation of getting a resource tree

  Parameters:

  Returns:

  ***************************************************************************/
  mrapi_resource_t* mrapi_impl_resources_get(
                                             mrapi_rsrc_filter_t subsystem_filter,
                                             mrapi_status_t* status)
  {
    uint16_t number_of_nodes = 0;
    mrapi_resource_type rsrc_type;
    mrapi_resource_t *filtered_tree = NULL;
#if !(__MINGW32__||__unix__)
    uint16_t number_of_filtered_children;
#endif  /* !(__MINGW32__) */

    if (subsystem_filter == MRAPI_RSRC_MEM) {
      rsrc_type = MEM;
    } else if (subsystem_filter == MRAPI_RSRC_CPU) {
      rsrc_type = CPU;
    } else if (subsystem_filter == MRAPI_RSRC_CACHE) {
      rsrc_type = CACHE;
    } else if (subsystem_filter == MRAPI_RSRC_CROSSBAR) {
      rsrc_type = CROSSBAR;
    } else if (subsystem_filter == MRAPI_RSRC_DMA) {
      *status = MRAPI_SUCCESS;
      return MRAPI_NULL;
    } else {
      *status = MRAPI_ERR_RSRC_INVALID_SUBSYSTEM;
      return MRAPI_NULL;
    }

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));

    mrapi_dprintf(1,"mrapi_impl_resources_get");
    number_of_nodes = mrapi_impl_get_number_of_nodes(rsrc_type, resource_root);
    if (number_of_nodes != 0) {
      char *tree_name = "filtered tree";
      int name_length = strlen(tree_name);
      filtered_tree = (mrapi_resource_t *) malloc(sizeof(mrapi_resource_t));
      filtered_tree->name = (char *) malloc((name_length+1) * sizeof(char));
#if (__unix__||__MINGW32__)
      strcpy(filtered_tree->name, tree_name);
#else
      strcpy_s(filtered_tree->name,name_length+1, tree_name);
#endif  /* !(__unix__||__MINGW32__) */
      filtered_tree->resource_type = SYSTEM;
      filtered_tree->number_of_children = number_of_nodes;
      filtered_tree->children = (mrapi_resource_t **) malloc(number_of_nodes * sizeof(mrapi_resource_t*));
      filtered_tree->number_of_attributes = 0;
      filtered_tree->attribute_types = NULL;
      filtered_tree->attribute_values = NULL;

      /* Populate the filtered tree */
#if (__MINGW32__||__unix__)
      (void)mrapi_impl_create_rsrc_tree(rsrc_type,
								resource_root,
								0,
								filtered_tree);
#else
      number_of_filtered_children = mrapi_impl_create_rsrc_tree(rsrc_type,
								resource_root,
								0,
								filtered_tree);
#endif  /* (__MINGW32__) */
    }

    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(semid,0));

    *status = MRAPI_SUCCESS;
    return filtered_tree;
  }

  /*-------------------------------------------------------------------
    Search the resource tree to find the number of nodes of a certain
    resource type.
    -------------------------------------------------------------------*/
  uint16_t mrapi_impl_get_number_of_nodes(mrapi_resource_type rsrc_type,
                                          mrapi_resource_t *tree) {
    uint16_t number_of_nodes = 0;
    uint16_t number_of_children;
    uint16_t i;

    mrapi_dprintf(1,"mrapi_impl_get_number_of_nodes");
    number_of_children = tree->number_of_children;

    if (number_of_children == 0) {
      if (rsrc_type == tree->resource_type) {
        number_of_nodes++;
      }

    } else {
      for (i = 0; i < number_of_children; i++) {
        number_of_nodes += mrapi_impl_get_number_of_nodes(rsrc_type, tree->children[i]);
      }
    }

    return number_of_nodes;
  }

  /*-------------------------------------------------------------------
    Search the resource tree to find the number of nodes of a certain
    resource type.
    -------------------------------------------------------------------*/
  uint16_t mrapi_impl_create_rsrc_tree(mrapi_resource_type rsrc_type,
                                       mrapi_resource_t *src_tree,
                                       uint16_t start_index_child,
                                       mrapi_resource_t *filtered_tree) {
    uint16_t index_child;
    uint16_t i;
    uint32_t number_of_children;
    uint32_t number_of_attrs;
    mrapi_resource_t *the_child;
    mrapi_resource_t *new_child;
    char *new_name;
    int name_length;
    void *src = NULL;
    void *dest = NULL;
    rsrc_type_t the_type;
    rsrc_type_t *new_type;
    mrapi_attribute_static the_static;
    mrapi_attribute_static *new_static;
    mrapi_boolean_t the_start;
    mrapi_boolean_t *new_start;

    mrapi_dprintf(1,"mrapi_impl_create_rsrc_tree");
    /* Check if this node is one we're interested in */
    index_child = start_index_child;
    if (rsrc_type == src_tree->resource_type) {
      new_child = (mrapi_resource_t *) malloc(sizeof(mrapi_resource_t));
      /* Make a copy of the node, leaving out the pointers to the children */
      name_length = strlen(src_tree->name);
      new_name = (char *) malloc((name_length+1) * sizeof(char));
#if (__unix__||__MINGW32__)
      strcpy(new_name, src_tree->name);
#else
      strcpy_s(new_name,name_length+1, src_tree->name);
#endif  /* !(__unix__||__MINGW32__) */
      new_child->name = new_name;
      new_child->resource_type = src_tree->resource_type;
      new_child->number_of_children = 0;
      new_child->children = NULL;
      number_of_attrs = src_tree->number_of_attributes;
      new_child->number_of_attributes = number_of_attrs;
      new_child->attribute_values = (void **) malloc(number_of_attrs * sizeof(void*));
      new_child->attribute_types  = (void **) malloc(number_of_attrs * sizeof(void*));
      new_child->attribute_static =
        (mrapi_attribute_static **) malloc(number_of_attrs * sizeof(mrapi_attribute_static *));
      new_child->attribute_started =
        (mrapi_boolean_t **) malloc(number_of_attrs * sizeof(mrapi_boolean_t *));
      for (i = 0; i < number_of_attrs; i++) {
        the_type = *((rsrc_type_t *)src_tree->attribute_types[i]);
        src = src_tree->attribute_values[i];
        if (the_type == RSRC_UINT16_T) {
          dest = (void *)malloc(sizeof(uint16_t));
          *((uint16_t*)dest) = *((uint16_t*)src);

        } else if (the_type == RSRC_UINT32_T) {
          dest = (void *)malloc(sizeof(uint32_t));
          *((uint32_t*)dest) = *((uint32_t*)src);

        } else {
          /* Should not reach here */
          mrapi_dprintf(1, "Bad resource type while copying a resource node\n");
        }
        new_child->attribute_values[i] = dest;

        /* Copy the types */
        new_type = (rsrc_type_t *)malloc(sizeof(rsrc_type_t));
        *new_type = the_type;
        new_child->attribute_types[i] = new_type;

        /* Copy the statics */
        the_static = *(src_tree->attribute_static[i]);
        new_static = (mrapi_attribute_static *)malloc(sizeof(mrapi_attribute_static));
        *new_static = the_static;
        new_child->attribute_static[i] = new_static;

        /* Copy the starts */
        the_start = *(src_tree->attribute_started[i]);
        new_start = (mrapi_boolean_t *)malloc(sizeof(mrapi_boolean_t));
        *new_start = the_start;
        new_child->attribute_started[i] = new_start;
      }

      /* Add this to the filter tree */
      filtered_tree->children[index_child] = new_child;

      /* Increment the index so that the next addition to the tree is to */
      /* the correct child */
      index_child++;
    }

    /* Check the children */
    number_of_children = src_tree->number_of_children;
    if (number_of_children != 0) {
      /* Recursively check the children to see if they are type we're interested in */
      for (i = 0; i < number_of_children; i++) {
        the_child = src_tree->children[i];
        index_child = mrapi_impl_create_rsrc_tree(rsrc_type,
						  the_child,
						  index_child,
						  filtered_tree);
      }
    }

    return index_child;
  }

  /***************************************************************************
  Function: mrapi_impl_resource_tree_free

  Description:  Frees the memory of a filtered resource tree.

  Parameters:  A pointer to the tree

  Returns:  Nothing

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_resource_tree_free(
                                                mrapi_resource_t* const * root_ptr,
                                                mrapi_status_t* status)
  {
    uint16_t number_of_children;
    uint16_t number_of_attributes;
    uint16_t i;
    mrapi_resource_t *the_child;
    mrapi_resource_t *root = *root_ptr;  /* FIXME: COMPILE WARNING RE CONST */
    mrapi_boolean_t rc = MRAPI_TRUE;

    /* Check for invalid resource trees */
    if (root == NULL) {
      *status = MRAPI_ERR_RSRC_INVALID_TREE;
      rc = MRAPI_FALSE;
      return rc;
    }

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));

    mrapi_dprintf(1,"mrapi_impl_resource_tree_free");
    number_of_children = root->number_of_children;
    for (i = 0; i < number_of_children; i++) {
      the_child = root->children[i];
      free(the_child->name);
      free(the_child);
    }
    free(root->children);
    free(root->name);
    number_of_attributes = root->number_of_attributes;
    for (i = 0; i < number_of_attributes; i++) {
      free(root->attribute_values[i]);
    }
    free(root->attribute_values);
    free(root);

    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(semid,0));

    *status = MRAPI_SUCCESS;
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_resource_get_attribute

  Description:  Frees the memory of a filtered resource tree.

  Parameters:  A pointer to the tree

  Returns:  Nothing

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_resource_get_attribute(
                                                    const mrapi_resource_t* resource,
                                                    mrapi_uint_t attribute_number,
                                                    void* attribute_value,
                                                    size_t attr_size,
                                                    mrapi_status_t* status)
  {
    rsrc_type_t the_type;
    mrapi_boolean_t rc = MRAPI_TRUE;

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));

    mrapi_dprintf(1,"mrapi_impl_resource_get_attribute");
    /* Not all possible combinations are implemented in this example */
    the_type = *((rsrc_type_t *)resource->attribute_types[attribute_number]);
    if (the_type == RSRC_UINT16_T) {
      uint16_t val = *((uint16_t *)resource->attribute_values[attribute_number]);
      *((uint16_t*)attribute_value) = val;
    } else if (the_type == RSRC_UINT32_T) {
      uint32_t val = *((uint32_t *)resource->attribute_values[attribute_number]);
      *((uint32_t*)attribute_value) = val;
    }

    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(semid,0));

    *status = MRAPI_SUCCESS;
    return rc;
  }

  /*-------------------------------------------------------------------
    A backdoor to artificially cause a dynamic attribute to change
    Current only works for attribute number 1.
    -------------------------------------------------------------------*/
  void mrapi_impl_increment_cache_hits(mrapi_resource_t *resource, int increment)
  {
    int attribute_number = 1;  /* Good only for L3cache number of cache hits */
    *((uint32_t*)resource->attribute_values[attribute_number]) += increment;
  }

  /***************************************************************************
  Function: mrapi_impl_dynamic_attribute_reset

  Description:  Resets a dynamic attribute

  Parameters:  A pointer to the resource

  Returns:  Nothing

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_dynamic_attribute_reset(
                                                     const mrapi_resource_t *resource,
                                                     mrapi_uint_t attribute_number,
                                                     mrapi_status_t* status)
  {
    rsrc_type_t the_type;
    mrapi_boolean_t rc = MRAPI_TRUE;

    /* lock the database */
    mrapi_assert (mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));

    mrapi_dprintf(1,"mrapi_impl_dynamic_attribute_reset");
    the_type = *((rsrc_type_t *)resource->attribute_types[attribute_number]);
    if (the_type == RSRC_UINT16_T) {
      *((uint16_t*)resource->attribute_values[attribute_number]) = 0;
    } else if (the_type == RSRC_UINT32_T) {
      *((uint32_t*)resource->attribute_values[attribute_number]) = 0;
    }

    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(semid,0));

    *status = MRAPI_SUCCESS;
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_dynamic_attribute_start

  Description:  Resets a dynamic attribute

  Parameters:  A pointer to the resource

  Returns:  Nothing

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_dynamic_attribute_start(
                                                     const mrapi_resource_t* resource,
                                                     mrapi_uint_t attribute_number,
                                                     void (*rollover_callback) (void),
                                                     mrapi_status_t* status)
  {
    mrapi_boolean_t rc = MRAPI_TRUE;

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));

    mrapi_dprintf(1,"mrapi_impl_dynamic_attribute_start");
    if (strcmp(resource->name, "L3 Cache") == 0) {
      /* Since there are no hooks to underlying hardware in this example, we will fake
         L3 cache hits.  Here, we only change the attribute to started.  On
         real hardware, one might program performance monitors to gather
         L3 cache hits. */
      if (attribute_number == 1) {
        *(resource->attribute_started[attribute_number]) = MRAPI_TRUE;
        mrapi_db->rollover_callbacks_ptr[mrapi_db->rollover_index] = rollover_callback;
        mrapi_db->rollover_index++;
      }
    }

    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(semid,0));

    *status = MRAPI_SUCCESS;
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_dynamic_attribute_stop

  Description:  Resets a dynamic attribute

  Parameters:  A pointer to the resource

  Returns:  Nothing

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_dynamic_attribute_stop(
                                                    const mrapi_resource_t* resource,
                                                    mrapi_uint_t attribute_number,
                                                    mrapi_status_t* status)
  {
    mrapi_boolean_t rc = MRAPI_TRUE;

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));

    mrapi_dprintf(1,"mrapi_impl_dynamic_attribute_stop");
    if (strcmp(resource->name, "L3 Cache") == 0) {
      /* Since there are no hooks to underlying hardware in this example, we will fake
         L3 cache hits.  Here, we only change the attribute to started.  On
         real hardware, one might program performance monitors to gather
         L3 cache hits. */
      if (attribute_number == 1) {
        *(resource->attribute_started[attribute_number]) = MRAPI_FALSE;
      }
    }

    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(semid,0));

    *status = MRAPI_SUCCESS;
    return rc;
  }

  /***************************************************************************
  Function: mrapi_impl_resource_register_callback

  Description:

  Parameters:

  Returns:  Nothing

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_resource_register_callback(
                                                        mrapi_event_t event,
                                                        unsigned int frequency,
                                                        void (*callback_function) (mrapi_event_t event),
                                                        mrapi_status_t* status)
  {
    mrapi_boolean_t rc = MRAPI_TRUE;
    uint32_t d,n;
    mrapi_domain_t domain_id;
    mrapi_node_t node_id;
    mrapi_uint16_t index;

    if (event == (mrapi_event_t)NULL) {
      *status = MRAPI_ERR_RSRC_INVALID_EVENT;
      return MRAPI_FALSE;
    }

    if(!mrapi_impl_whoami(&node_id,&n,&domain_id,&d)) {
      *status = MRAPI_ERR_NODE_NOTINIT;
      return MRAPI_FALSE;
    }

    /* lock the database */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));

    index = mrapi_db->callback_index;
    mrapi_dprintf(1,"mrapi_impl_resource_register_callback, index %d, node id %d",
		  index, node_id);
    (mrapi_db->callbacks_array[index]).callback_func      = callback_function;
    (mrapi_db->callbacks_array[index]).callback_event     = event;
    (mrapi_db->callbacks_array[index]).callback_frequency = frequency;
    (mrapi_db->callbacks_array[index]).callback_count     = 0;
    (mrapi_db->callbacks_array[index]).node_id            = node_id;
    mrapi_db->callback_index = index + 1;
    mrapi_dprintf(1,"mrapi_impl_resource_register_callback, callback index %d",
		  mrapi_db->callback_index);

    /* Set up an alarm that will artificially cause the event to happen */
#if (__unix__)
    sigemptyset(&alarm_struct.sa_mask);
    alarm_struct.sa_flags = 0;
    alarm_struct.sa_handler = mrapi_impl_alarm_catcher;
    sigaction(SIGALRM, &alarm_struct, NULL);
    /* Set the alarm for 1 second) */
    alarm(1);
#else
    {
      int signum = 0;
      /* Set the alarm for 1 second) */
      if(!CreateTimerQueueTimer(
          &mrapi_db->domains[d].nodes[n].hAlarm,
          NULL,(WAITORTIMERCALLBACK)mrapi_impl_alarm_catcher,&signum,1000,0,WT_EXECUTEONLYONCE)) {
        rc = MRAPI_FALSE;
      }
    }
#endif  /* !(__unix__) */

    /* unlock the database */
    mrapi_assert(mrapi_impl_access_database_post(semid,0));

    *status = MRAPI_SUCCESS;
    return rc;
  }

  /***************************************************************************
  Have the mrapi_impl_alarm_catcher artificially call the function that causes the
  event to occur.
  ***************************************************************************/
  void mrapi_impl_alarm_catcher(int signal) {
    mrapi_impl_cause_event();
  }

  /***************************************************************************
   * Because the alarms are not tied to a specific node in this example,
   * it is possible that any one of the nodes will actually get the callback
   * invoked.
   ***************************************************************************/
  void mrapi_impl_cause_event() {
    mrapi_event_t the_event;
    uint32_t d,n;
    mrapi_domain_t domain_id;
    mrapi_node_t node_id;
    mrapi_node_t current_node_id;
    int i;
    int index = -1;
    int max_index = 0;

    if(!mrapi_impl_whoami(&node_id,&n,&domain_id,&d)) {
      return;
    }

    mrapi_dprintf(4, "mrapi_impl_cause_event current node %d", node_id);

    /* Lock the database, increment the callback count, and unlock the database */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));
    max_index = mrapi_db->callback_index;
    /* Find a callback with the same node id */
    for (i = 0; i < max_index && index == -1; i++) {
      current_node_id = (mrapi_db->callbacks_array[i]).node_id;
      /*       mrapi_dprintf(4, "checking index %d with node id %d", i, current_node_id); */
      if (current_node_id == node_id) {
        index = i;
      }
    }
    mrapi_assert(mrapi_impl_access_database_post(semid,0));

    if (index > -1 && index < max_index) {
      mrapi_dprintf(4, "found callback at index %d", index);

    } else {
#if (__unix__)
      sigaction(SIGALRM, &alarm_struct, NULL);
      /* Set the alarm for 1 second) */
      alarm(1);
#else
      {
        int signum = 0;
        /* Set the alarm for 1 second) */
        if(!CreateTimerQueueTimer(
            &mrapi_db->domains[d].nodes[n].hAlarm,
            NULL,(WAITORTIMERCALLBACK)mrapi_impl_alarm_catcher,&signum,1000,0,WT_EXECUTEONLYONCE)) {
          mrapi_dprintf(4,"mrapi_impl_cause_event reseting alarm failed");
        }
      }
#endif  /* !(__unix__) */
      mrapi_dprintf(4,"mrapi_impl_cause_event reseting alarm, no callback found");
      return;
    }

    /* Lock the database, increment the callback count, and unlock the database */
    mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));
    (mrapi_db->callbacks_array[index]).callback_count++;
    mrapi_assert(mrapi_impl_access_database_post(semid,0));

    /* Since this function artifically cause an event, a callback needs to be */
    /* selected.  Arbitrarily pick the first callback. */
    /* Check if it is time to invoke the callback */
    if ((mrapi_db->callbacks_array[index]).callback_count >=
        (mrapi_db->callbacks_array[index]).callback_frequency) {
      /* The frequency count has been met, so invoke the callback */
      mrapi_assert(mrapi_impl_access_database_pre(semid,0,MRAPI_TRUE));
      (mrapi_db->callbacks_array[index]).callback_count = 0;
      the_event = (mrapi_db->callbacks_array[index]).callback_event;
      mrapi_assert(mrapi_impl_access_database_post(semid,0));
      mrapi_dprintf(4, "mrapi_impl_cause_event calling callback, index %d", index);
      (*(mrapi_db->callbacks_array[index]).callback_func)(the_event);
    }

    /* The frequency count has not been met, so reschedule the event */
#if (__unix__)
    sigaction(SIGALRM, &alarm_struct, NULL);
    /* Set the alarm for 1 second) */
    alarm(1);
#else
    {
      int signum = 0;
      /* Set the alarm for 1 second) */
      if(!CreateTimerQueueTimer(
          &mrapi_db->domains[d].nodes[n].hAlarm,
          NULL,(WAITORTIMERCALLBACK)mrapi_impl_alarm_catcher,&signum,1000,0,WT_EXECUTEONLYONCE)) {
        mrapi_dprintf(4,"mrapi_impl_cause_event reseting alarm failed");
      }
    }
#endif  /* !(__unix__) */
    mrapi_dprintf(4,"mrapi_impl_cause_event reseting alarm, index %d", index);
  }

  /***************************************************************************
   ***************************************************************************/
  void mrapi_impl_trigger_rollover(uint16_t index) {
    (*(mrapi_db->rollover_callbacks_ptr[index]))();
  }

  /***************************************************************************
  Function: mrapi_impl_valid_attribute_number

  Description:  Checks if this is a valid attribute number

  Parameters:

  Returns:  mrapi_boolean_t

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_valid_attribute_number(const mrapi_resource_t* resource,
						    const mrapi_uint_t attribute_number)
  {
    uint32_t number_of_attributes = resource->number_of_attributes;

    if (attribute_number >= number_of_attributes) {
      return MRAPI_FALSE;
    } else if (attribute_number < 0) {
      return MRAPI_FALSE;
    } else {
      return MRAPI_TRUE;
    }
  }

  /***************************************************************************
  Function: mrapi_impl_is_static

  Description:  Checks to see if the attribute is static

  Parameters:

  Returns:  mrapi_boolean_t

  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_is_static(const mrapi_resource_t* resource,
                                       const mrapi_uint_t attribute_number) {
    mrapi_attribute_static attr_static = *(resource->attribute_static[attribute_number]);
    if (attr_static == MRAPI_ATTR_STATIC) {
      return MRAPI_TRUE;
    } else {
      return MRAPI_FALSE;
    }
  }
