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
