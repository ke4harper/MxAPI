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

	// Use atomic counter to prevent concurrent rundown or initialization by another thread
	// Counter is tested in mcapi_trans_initialize
    while (
#if !(__unix__||__MINGW32__)
           0 != InterlockedCompareExchange(&mcapi_finalize_ctr,1,0)) {
      SleepEx(10,0);
#else
	      0 != __sync_lock_test_and_set(&mcapi_finalize_ctr,1,0)) {
      usleep(10000);
#endif  /* (__unix__||__MINGW32__) */
    }

	// Do not free resources while runtime is being initialized
    while(0 < mcapi_initialize_ctr) {
#if (__unix__||__MINGW32__)
      usleep(10000);
#else
      SleepEx(10,0);
#endif  /* !(__unix__||__MINGW32__) */
	}

    if (mcapi_db == NULL) { return MCAPI_FALSE;}

    /* lock the database */
    mcapi_assert(mcapi_trans_access_database_pre(global_rwl,MCAPI_TRUE));

    mcapi_assert(mcapi_trans_whoami(&node_id,&n,&domain_id,&d));

    mcapi_dprintf(1,"mcapi_trans_finalize(domain=%u node=%u);",domain_id,node_id);

    /* mark myself as invalid and see if there are any other valid nodes in the system */
    mcapi_db->domains[d].nodes[n].valid = MCAPI_FALSE;

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

	  // Rundown complete, decrement atomic counter
#if !(__unix__||__MINGW32__)
	  InterlockedDecrement(&mcapi_finalize_ctr);
#else
      __sync_lock_release(&mcapi_finalize_ctr);
#endif  /* (__unix__||__MINGW32__) */

    return rc;
  }
