  /***************************************************************************
  NAME: mcapi_trans_rundown_have_lock
  DESCRIPTION: mark the node as running down.
  PARAMETERS: none
  RETURN VALUE: none
  ***************************************************************************/
  void mcapi_trans_rundown_have_lock () {
    assert (locked == 1);
    /* set rundown flag for this node */
    mcapi_db->domains[mcapi_dindex].nodes[mcapi_nindex].rundown = MCAPI_TRUE;
  }

  /***************************************************************************
  NAME:mcapi_trans_finalize
  DESCRIPTION: cleans up the semaphore and shared memory resources.
  PARAMETERS: none
  RETURN VALUE: boolean: success or failure
  ***************************************************************************/
  mcapi_boolean_t mcapi_trans_finalize () {
    mcapi_boolean_t last_man_standing = MCAPI_TRUE;
    mcapi_boolean_t last_man_standing_for_this_process = MCAPI_TRUE;
    mcapi_boolean_t rc = MCAPI_TRUE;
    uint32_t d = 0;
    uint32_t n = 0;
    uint32_t i;
    mcapi_node_t node_id=0;
    mcapi_domain_t domain_id=0;
#if (__unix__||__MINGW32__)
    pid_t pid = getpid();
#else
    pid_t pid = (pid_t)GetCurrentProcessId();
#endif  /* !(__unix__||__MINGW32__) */

    if (mcapi_db == NULL) { return MCAPI_FALSE;}

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_assert(mcapi_trans_whoami(&node_id,&n,&domain_id,&d));

    mcapi_dprintf(1,"mcapi_trans_finalize(domain=%u node=%u);",domain_id,node_id);

    /* mark myself as invalid and see if there are any other valid nodes in the system */
    mcapi_db->domains[d].nodes[n].valid = MCAPI_FALSE;
    mcapi_db->domains[d].num_nodes--;

    for (d = 0; d < MCA_MAX_DOMAINS; d++) {
      for (i = 0; i < MCA_MAX_NODES; i++) {
        if ( mcapi_db->domains[d].nodes[i].valid) {
          last_man_standing = MCAPI_FALSE;
          if  ( mcapi_db->domains[d].nodes[i].pid == pid) {
            last_man_standing_for_this_process = MCAPI_FALSE;
          }
        }
      }
    }

    mcapi_trans_rundown_have_lock();

    /* unlock the database */
    mcapi_assert(mcapi_trans_access_database_post(global_rwl,MCAPI_TRUE));

    /* finalize this node at the mrapi layer */
    /* if there are no other nodes, mrapi_finalize will free up our resources
       (semaphore and shared memory db) */
    rc = transport_sm_finalize(last_man_standing,last_man_standing_for_this_process,MCAPI_TRUE,global_rwl);

    /* transport_sm_finalize will have detached from the shared memory so
       null out our db pointer */
    if (last_man_standing_for_this_process) {
      mcapi_dprintf(1,"mcapi_trans_finalize: deleting mcapi_db\n");
#if !(__unix__||__MINGW32__)
        InterlockedExchangePointer(&mcapi_db,NULL);
#else
        (void)__sync_lock_test_and_set(&mcapi_db,NULL);
#endif  /* (__unix__||__MINGW32__) */
    }

    return rc;
  }
