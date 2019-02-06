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

// Invalid operations without initialization
	{
		int key = 1;
		int num_locks = 1;
		mrapi_sem_id_t sem_id = 0;

        assert(!mrapi_impl_initialized());
		n_num = mrapi_impl_node_id_get(&status);
        assert((mrapi_node_t)-1 == n_num);
		assert(MRAPI_SUCCESS != status);
		assert(!mrapi_impl_get_domain_num(&d_num));
		assert(!mrapi_impl_whoami(&n_num,&n_index,&d_num,&d_index));
        // System semaphores do not require initialization
		assert(mrapi_impl_create_sys_semaphore(&sem_id,num_locks,key,MRAPI_TRUE));
		assert(sys_sem_delete(sem_id));
    }

	// Runtime initialization
	{

		// One node
		d_num = d_offset + 1;
		n_num = n_offset + 1;
        assert(NULL == mrapi_db);
		assert(mrapi_impl_initialize(d_num,n_num,&status));
		assert(mrapi_impl_whoami(&n_num,&n_index,&d_num,&d_index));
        p_index = mrapi_db->domains[d_index].nodes[n_index].proc_num;
        assert(NULL != mrapi_db);
		assert(d_num == mrapi_db->domains[d_index].state.data.domain_id);
        assert(0 == mrapi_db->num_domains); // num_domains not used
		assert(MRAPI_TRUE == mrapi_db->domains[d_index].state.data.valid);
		assert(0 < mrapi_db->domains[d_index].num_nodes);
		assert(MRAPI_TRUE == mrapi_db->domains[d_index].nodes[n_index].state.data.valid);
		assert(n_num == mrapi_db->domains[d_index].nodes[n_index].state.data.node_num);
#if !(__unix__)
		assert(GetCurrentProcessId() == mrapi_db->processes[p_index].state.data.pid);
		assert((pthread_t)GetCurrentThreadId() == mrapi_db->domains[d_index].nodes[n_index].tid);
#else
		assert(getpid() == mrapi_db->processes[p_index].state.data.pid);
		assert(pthread_self() == mrapi_db->domains[d_index].nodes[n_index].tid);
#endif  // (__unix__)
        assert(MRAPI_TRUE == mrapi_db->processes[p_index].state.data.valid);
        assert(1 == mrapi_db->processes[p_index].num_nodes);
		assert(mrapi_impl_initialized());
		assert(!mrapi_impl_initialize(d_num,n_num,&status)); // Error to initialize duplicate node on same thread
		assert(mrapi_impl_finalize());
        assert(NULL == mrapi_db);
		assert(mrapi_impl_initialize(d_num,n_num,&status));
		assert(mrapi_impl_finalize());
	}
