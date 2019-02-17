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
  Function:mrapi_impl_finalize node

  Description: 

  Parameters:

  Returns:MRAPI_TRUE/MRAPI_FALSE
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_finalize_node_locked (uint32_t d, uint32_t n) 
  {
    
    mrapi_boolean_t rc = MRAPI_TRUE;
    int shmem,sem;
    
    mrapi_dprintf(2,"mrapi_impl_finalize_node_locked dindex=%d nindex=%d domain=%d node=%d",
                  d,
                  n,
                  mrapi_db->domains[d].domain_id,
                  mrapi_db->domains[d].nodes[n].node_num);
    /* mark the node as finalized */
    mrapi_db->domains[d].nodes[n].valid = MRAPI_FALSE;
    
    /* decrement the shmem reference count if necessary */
    for (shmem = 0; shmem < MRAPI_MAX_SHMEMS; shmem++) {
      if (mrapi_db->shmems[shmem].valid == MRAPI_TRUE) { 
        if (mrapi_db->domains[d].nodes[n].shmems[shmem] == 1) {
          /* if this node was a user of this shm, decrement the ref count */
          mrapi_db->shmems[shmem].refs--;
        } 
        /* if the reference count is 0, free the shared memory resource */
        if (mrapi_db->shmems[shmem].refs == 0) {
          rc |= sys_shmem_detach(mrapi_db->shmems[shmem].addr);
          /* note: this will only work if no-one is still attached to it */
          rc |= sys_shmem_delete(mrapi_db->shmems[shmem].id);
          if (!rc) {
            fprintf(stderr,"mrapi_impl_finalize_node_locked: ERROR: sys_shmem_detach/delete failed\n");
          }
        }
      }
    }
    
    /* decrement the sem reference count if necessary */
    for (sem = 0; sem < MRAPI_MAX_SEMS; sem++) {
      if (mrapi_db->sems[sem].valid == MRAPI_TRUE) { 
        if (mrapi_db->domains[d].nodes[n].sems[sem] == 1) {
          mrapi_db->domains[d].nodes[n].sems[sem] = 0;
          /* if this node was a user of this sem, decrement the ref count */
          mrapi_db->sems[sem].refs--;
        } 
        /* if the reference count is 0 free the resource */
        if (mrapi_db->sems[sem].refs == 0) {
          mrapi_db->sems[sem].valid = MRAPI_FALSE;
        }
      }
    }

#if !(__unix__)
    /* Release alarm timer */
    if(NULL != mrapi_db->domains[d].nodes[n].hAlarm) {
      DeleteTimerQueueTimer(NULL,mrapi_db->domains[d].nodes[n].hAlarm,NULL);
      CloseHandle(mrapi_db->domains[d].nodes[n].hAlarm);
      mrapi_db->domains[d].nodes[n].hAlarm = NULL;
    }
#endif  /* !(__unix__) */
    
    return rc;
  }
  
  /***************************************************************************
  Function:mrapi_impl_free_resources

  Description: 

  Parameters:

  Returns:MRAPI_TRUE/MRAPI_FALSE
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_free_resources(mrapi_boolean_t panic) 
  {
    mrapi_boolean_t rc = MRAPI_TRUE;
    uint32_t d,n;
    mrapi_domain_t domain_num;
	mrapi_database* mrapi_db_local = NULL;
    mrapi_node_t node;
#if (__unix__)
    pid_t pid = getpid();
#else
    pid_t pid = (pid_t)GetCurrentProcessId();
#endif  /* !(__unix__) */
    mrapi_boolean_t last_man_standing = MRAPI_TRUE;
    mrapi_boolean_t last_man_standing_for_this_process = MRAPI_TRUE;
    mrapi_boolean_t locked;

	// Use atomic counter to prevent concurrent rundown or initialization by another thread
	// Counter is also tested in mrapi_impl_initialize
    while (
#if !(__unix__||__MINGW32__)
           0 != InterlockedCompareExchange(&mrapi_finalize_ctr,1,0)) {
#else
	      0 != __sync_lock_test_and_set(&mrapi_finalize_ctr,1,0)) {
#endif  /* (__unix__||__MINGW32__) */
      sys_os_usleep(10000);
    }

	// Do not free resources while runtime is being initialized
    while(0 < mrapi_initialize_ctr) {
      sys_os_usleep(10000);
	}

    /* try to lock the database */
    locked = mrapi_impl_access_database_pre(semid,0,MRAPI_FALSE);
    mrapi_dprintf(1,"mrapi_impl_free_resources (panic=%d): freeing any existing resources in the database mrapi_db=%p semid=%x shmemid=%x\n",
                  panic,mrapi_db,semid,shmemid);
    
    if (mrapi_db) {
      
      /* finalize this node */
      if (mrapi_impl_whoami(&node,&n,&domain_num,&d)) {
        mrapi_impl_finalize_node_locked(d,n);
      }    
      
      /* if we are in panic mode, then forcefully finalize all other nodes that belong to this process */
      if (panic) { 
        for (d = 0; d < MRAPI_MAX_DOMAINS; d++) {
          for (n = 0; n < MRAPI_MAX_NODES; n++) {
            if ((mrapi_db->domains[d].nodes[n].valid == MRAPI_TRUE) &&
                (mrapi_db->domains[d].nodes[n].pid == pid)) {
              mrapi_impl_finalize_node_locked(d,n);
            }
          }
        }
      }
      
      /* see if there are any valid nodes left in the system and for this process */
      for (d = 0; d < MRAPI_MAX_DOMAINS; d++) {
        for (n = 0; n < MRAPI_MAX_NODES; n++) {
          if (mrapi_db->domains[d].nodes[n].valid == MRAPI_TRUE) {
            last_man_standing = MRAPI_FALSE;
            if (mrapi_db->domains[d].nodes[n].pid == pid) {
              last_man_standing_for_this_process = MRAPI_FALSE;
            }
          }
        }
      }
      
      if (panic) {
        mrapi_assert(last_man_standing_for_this_process);
      }
      
       /* if there are no other valid nodes in the whole system, then free the sems */
      if (last_man_standing) {
        mrapi_dprintf(1,"mrapi_impl_free_resources: freeing mrapi internal semaphore and shared memory\n");
        
        /* free the mrapi internal semaphores */
        if (sems_semid != semid) {
          rc = sys_sem_delete(sems_semid);
          sems_semid = -1;
          if (!rc) {
            fprintf(stderr,"mrapi_impl_free_resources: ERROR: sys_sem_delete (mrapi_db) failed\n");
          }
        }

        if (shmems_semid != semid) {
          rc = sys_sem_delete(shmems_semid);
          shmems_semid = -1;
          if (!rc) {
            fprintf(stderr,"mrapi_impl_free_resources: ERROR: sys_sem_delete (mrapi_db) failed\n");
          }
        }
        if (rmems_semid != semid) {
          rc = sys_sem_delete(rmems_semid);
          rmems_semid = -1;
          if (!rc) {
            fprintf(stderr,"mrapi_impl_free_resources: ERROR: sys_sem_delete (mrapi_db) failed\n");
          }
        }
        if (requests_semid != semid) {
          rc = sys_sem_delete(requests_semid);
          requests_semid = -1;
          if (!rc) {
            fprintf(stderr,"mrapi_impl_free_resources: ERROR: sys_sem_delete (mrapi_db) failed\n");
          }
        }
      }
      
      /* if there are no other valid nodes for this process, then detach from shared memory */
      if (last_man_standing_for_this_process) {
        
        /* detach from the mrapi internal shared memory */
        mrapi_dprintf(1,"mrapi_impl_free_resources: detaching from mrapi internal shared memory\n");
        
        mrapi_db_local = mrapi_db;
#if !(__unix__||__MINGW32__)
        InterlockedExchangePointer(&mrapi_db,NULL);
#else
        (void)__sync_lock_test_and_set(&mrapi_db,NULL);
#endif  /* (__unix__||__MINGW32__) */
        rc = sys_shmem_detach (mrapi_db_local);
        if (!rc) {
          fprintf(stderr,"mrapi_impl_free_resources: ERROR: sys_shmem detach (mrapi_db) failed\n");
        }  
      }
      
      /* if there are no other valid nodes in the whole system, then free the shared memory */
      if (last_man_standing) {
        /* free the mrapi internal shared memory */
        rc = sys_shmem_delete(shmemid);
        if (!rc) {
          fprintf(stderr,"mrapi_impl_free_resources: ERROR: sys_shmem_delete (mrapi_db) failed\n");
        }
        //mrapi_db = NULL;
        shmemid = -1;
      }
      
      /* if we locked the database and didn't delete it, then we need to unlock it */
      if (locked) {
        /* unlock the database */
        mrapi_assert(mrapi_impl_access_database_post(semid,0));
      }
      if(last_man_standing) {
        /* free the global semaphore last */
        rc = sys_sem_delete(semid);
        semid = -1;
        if (!rc) {
          fprintf(stderr,"mrapi_impl_free_resources: ERROR: sys_sem_delete (mrapi_db) failed\n");
        }
      }
    }
 
    // Rundown complete, decrement atomic counter
#if !(__unix__||__MINGW32__)
    InterlockedDecrement(&mrapi_finalize_ctr);
#else
    __sync_lock_release(&mrapi_finalize_ctr);
#endif  /* (__unix__||__MINGW32__) */

    return last_man_standing;
  }
  
  /***************************************************************************
  Function:mrapi_impl_finalize

  Description: 

  Parameters:

  Returns:MRAPI_TRUE/MRAPI_FALSE
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_finalize()
  {
    mrapi_impl_free_resources(MRAPI_FALSE);
    return MRAPI_TRUE;
  }
