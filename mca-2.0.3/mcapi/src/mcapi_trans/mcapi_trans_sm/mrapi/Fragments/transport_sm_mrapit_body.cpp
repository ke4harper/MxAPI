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

extern "C" {
  extern void* shm_addr;
}

#if !(__unix__)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif  // (__unix__)
{
    uint32_t global_rwl = 0;
	mrapi_domain_t domain = 0;
	mrapi_node_t node = 0;

    // Runtime initialization
	{
        void* addr = NULL;
		mrapi_sem_id_t key = 0;
        mcapi_boolean_t last_man_standing = MRAPI_TRUE;
        mcapi_boolean_t last_man_standing_for_this_process = MRAPI_TRUE;
        domain = 1;
        node = 1;
		key = mca_Crc32_ComputeBuf(key,"transport_sm_mrapit_shm",23);
        assert(!transport_sm_get_shared_mem(&addr,key,sizeof(mcapi_database)));
        assert(transport_sm_initialize(domain,node,&global_rwl));
        assert(transport_sm_create_shared_mem(&shm_addr,key,sizeof(mcapi_database)));
        assert(!transport_sm_get_shared_mem(&addr,key,sizeof(mcapi_database)));
        assert(transport_sm_lock_rwl(global_rwl,MRAPI_TRUE));
        assert(transport_sm_unlock_rwl(global_rwl,MRAPI_TRUE));
        assert(transport_sm_lock_rwl(global_rwl,MRAPI_FALSE));
        assert(transport_sm_unlock_rwl(global_rwl,MRAPI_FALSE));
        assert(transport_sm_finalize(last_man_standing,last_man_standing_for_this_process,MCAPI_TRUE,global_rwl));
	}

	return 0;
}
