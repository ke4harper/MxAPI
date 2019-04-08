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

	uint8_t cTest = 0;
    uint16_t wTest = 0;
    uint32_t lTest = 0;
    uint64_t llTest = 0;
    uint8_t cComp = 0;
    uint16_t wComp = 0;
    uint32_t lComp = 0;
    uint64_t llComp = 0;
    uint8_t cOut = 0;
    uint16_t wOut = 0;
    uint32_t lOut = 0;
    uint64_t llOut = 0;

	int key = 0;
	mrapi_shmem_hndl_t shmem_id = 0;
	mrapi_shmem_attributes_t attributes = { 0 };
    atomic_data* addr = NULL;

    assert(sys_file_key(NULL,'z',&key));
	mrapi_impl_shmem_init_attributes(&attributes);
	mrapi_impl_shmem_create(&shmem_id,key,sizeof(atomic_data),&attributes,&status);
	addr = (atomic_data*)mrapi_impl_shmem_attach(shmem_id);

#if (__unix__||__atomic_barrier_test__)
	// Local memory write/read barrier
	{
		mrapi_atomic_barrier_t axb;
        mca_timeout_t timeout = MCA_INFINITE;
		mrapi_msg_t msg = { 0 };
		unsigned index = 0;
        unsigned txn;
		pid_t dest = 2;
		assert(mrapi_impl_atomic_barrier_init(&axb,dest,&msg,1,sizeof(msg),&index,timeout));
        assert(!mrapi_impl_atomic_hold(&axb,MRAPI_TRUE));
        assert(mrapi_impl_atomic_inc(&axb,&msg.txn,NULL,sizeof(msg.txn),&status));
        assert(mrapi_impl_atomic_hold(&axb,MRAPI_FALSE));
        assert(mrapi_impl_atomic_inc(&axb,&msg.txn,NULL,sizeof(msg.txn),&status));
        assert(mrapi_impl_atomic_read(&axb,&msg.txn,&txn,sizeof(msg.txn),&status));
        assert(2 == txn);
	}

	// Local memory exchange barrier
	{
		mrapi_atomic_barrier_t axb;
		mrapi_atomic_barrier_t raxb;
        mca_timeout_t timeout = MCA_INFINITE;
		mrapi_msg_t msg = { 0 };
		unsigned index = 0;
        unsigned txn;
		pid_t src = 1;
		pid_t dest = 2;
		assert(mrapi_impl_atomic_exchange_init(&axb,dest,&msg,1,sizeof(msg),&index,timeout));
        axb.src = src;  // rather than the real PID
		assert(mrapi_impl_atomic_exchange_init(&raxb,src,&msg,1,sizeof(msg),&index,timeout));
        raxb.src = dest;  // rather than the real PID
        assert(!mrapi_impl_atomic_hold(&axb,MRAPI_TRUE));
        assert(mrapi_impl_atomic_inc(&axb,&msg.txn,NULL,sizeof(msg.txn),&status));
        assert(mrapi_impl_atomic_hold(&axb,MRAPI_FALSE));
        assert(mrapi_impl_atomic_inc(&axb,&msg.txn,NULL,sizeof(msg.txn),&status));
        assert(mrapi_impl_atomic_read(&raxb,&msg.txn,&txn,sizeof(msg.txn),&status));
        assert(2 == txn);
	}

	// Shared memory write/read barrier
    {
		mrapi_atomic_barrier_t axb;
        mca_timeout_t timeout = MCA_INFINITE;
        int i = 0;
        unsigned txn;
		pid_t dest = 2;
        addr->index = 0;
        for(i = 0; i < 4; i++) {
          memset(&addr->msg[i],0,sizeof(addr->msg[i]));
        }
		assert(mrapi_impl_atomic_barrier_init(&axb,dest,addr->msg,4,sizeof(addr->msg[addr->index]),&addr->index,timeout));
        assert(!mrapi_impl_atomic_hold(&axb,MRAPI_TRUE));
        assert(mrapi_impl_atomic_inc(&axb,&addr->msg[addr->index].txn,NULL,sizeof(addr->msg[addr->index].txn),&status));
        assert(mrapi_impl_atomic_hold(&axb,MRAPI_FALSE));
        assert(mrapi_impl_atomic_inc(&axb,&addr->msg[addr->index].txn,NULL,sizeof(addr->msg[addr->index].txn),&status));
        assert(mrapi_impl_atomic_read(&axb,&addr->msg[addr->index].txn,&txn,sizeof(addr->msg[addr->index].txn),&status));
        assert(2 == txn);
	}

	// Shared memory exchange barrier
	{
		mrapi_atomic_barrier_t axb;
		mrapi_atomic_barrier_t raxb;
        mca_timeout_t timeout = MCA_INFINITE;
        int i = 0;
        unsigned txn;
		pid_t src = 1;
		pid_t dest = 2;
        addr->index = 0;
        for(i = 0; i < 4; i++) {
          memset(&addr->msg[i],0,sizeof(addr->msg[i]));
        }
		assert(mrapi_impl_atomic_exchange_init(&axb,dest,addr->msg,4,sizeof(addr->msg[addr->index]),&addr->index,timeout));
        axb.src = src;  // rather than the real PID
		assert(mrapi_impl_atomic_exchange_init(&raxb,src,addr->msg,4,sizeof(addr->msg[addr->index]),&addr->index,timeout));
        raxb.src = dest;  // rather than the real PID
        assert(!mrapi_impl_atomic_hold(&axb,MRAPI_TRUE));
        assert(mrapi_impl_atomic_inc(&axb,&addr->msg[addr->index].txn,NULL,sizeof(addr->msg[addr->index].txn),&status));
        assert(mrapi_impl_atomic_hold(&axb,MRAPI_FALSE));
        assert(mrapi_impl_atomic_inc(&axb,&addr->msg[addr->index].txn,NULL,sizeof(addr->msg[addr->index].txn),&status));
        assert(mrapi_impl_atomic_read(&raxb,&addr->msg[addr->index].txn,&txn,sizeof(addr->msg[addr->index].txn),&status));
        assert(2== txn);
	}
#endif  /* (__unix__||__atomic_barrier_test__) */

    // Atomic operations
	{
#if (__unix__)
        addr->cValue = 0;
        cTest = 2;
        assert(mrapi_impl_atomic_add(NULL,&addr->cValue,&cTest,NULL,sizeof(addr->cValue),&status));
        assert(2 == addr->cValue);
        assert(mrapi_impl_atomic_read(NULL,&addr->cValue,&cOut,sizeof(addr->cValue),&status));
        assert(2 == cOut);
        assert(mrapi_impl_atomic_add(NULL,&addr->cValue,&cTest,&cOut,sizeof(addr->cValue),&status));
        assert(2 == cOut);
        assert(4 == addr->cValue);

        addr->wValue = 0;
        wTest = 2;
        assert(mrapi_impl_atomic_add(NULL,&addr->wValue,&wTest,NULL,sizeof(addr->wValue),&status));
        assert(2 == addr->wValue);
        assert(mrapi_impl_atomic_read(NULL,&addr->wValue,&wOut,sizeof(addr->wValue),&status));
        assert(2 == wOut);
        assert(mrapi_impl_atomic_add(NULL,&addr->wValue,&wTest,&wOut,sizeof(addr->wValue),&status));
        assert(2 == wOut);
        assert(4 == addr->wValue);
#endif  // (__unix__)

        addr->lValue = 0;
        lTest = 2;
        assert(mrapi_impl_atomic_add(NULL,&addr->lValue,&lTest,NULL,sizeof(addr->lValue),&status));
        assert(2 == addr->lValue);
        assert(mrapi_impl_atomic_read(NULL,&addr->lValue,&lOut,sizeof(addr->lValue),&status));
        assert(2 == lOut);
        assert(mrapi_impl_atomic_add(NULL,&addr->lValue,&lTest,&lOut,sizeof(addr->lValue),&status));
        assert(2 == lOut);
        assert(4 == addr->lValue);

        addr->llValue = 0;
        llTest = 2;
        assert(mrapi_impl_atomic_add(NULL,&addr->llValue,&llTest,NULL,sizeof(addr->llValue),&status));
        assert(2 == addr->llValue);
        assert(mrapi_impl_atomic_read(NULL,&addr->llValue,&llOut,sizeof(addr->llValue),&status));
        assert(2 == llOut);
        assert(mrapi_impl_atomic_add(NULL,&addr->llValue,&llTest,&llOut,sizeof(addr->llValue),&status));
        assert(2 == llOut);
        assert(4 == addr->llValue);
	}

	// Atomic increment / decrement
	{
#if (__unix__)
        addr->cValue = 0;
        assert(mrapi_impl_atomic_inc(NULL,&addr->cValue,NULL,sizeof(addr->cValue),&status));
        assert(1 == addr->cValue);
        assert(mrapi_impl_atomic_inc(NULL,&addr->cValue,&cOut,sizeof(addr->cValue),&status));
        assert(2 == cOut);
        assert(2 == addr->cValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->cValue,NULL,sizeof(addr->cValue),&status));
        assert(1 == addr->cValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->cValue,&cOut,sizeof(addr->cValue),&status));
        assert(0 == cOut);
        assert(0 == addr->cValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->cValue,NULL,sizeof(addr->cValue),&status));
        assert((uint8_t)-1 == addr->cValue);
        assert(mrapi_impl_atomic_inc(NULL,&addr->cValue,NULL,sizeof(addr->cValue),&status));
        assert(0 == addr->cValue);
#endif  // (__unix__)

        addr->wValue = 0;
        assert(mrapi_impl_atomic_inc(NULL,&addr->wValue,NULL,sizeof(addr->wValue),&status));
        assert(1 == addr->wValue);
        assert(mrapi_impl_atomic_inc(NULL,&addr->wValue,&wOut,sizeof(addr->wValue),&status));
        assert(2 == wOut);
        assert(2 == addr->wValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->wValue,NULL,sizeof(addr->wValue),&status));
        assert(1 == addr->wValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->wValue,&wOut,sizeof(addr->wValue),&status));
        assert(0 == wOut);
        assert(0 == addr->wValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->wValue,NULL,sizeof(addr->wValue),&status));
        assert((uint16_t)-1 == addr->wValue);
        assert(mrapi_impl_atomic_inc(NULL,&addr->wValue,NULL,sizeof(addr->wValue),&status));
        assert(0 == addr->wValue);

        addr->lValue = 0;
        assert(mrapi_impl_atomic_inc(NULL,&addr->lValue,NULL,sizeof(addr->lValue),&status));
        assert(1 == addr->lValue);
        assert(mrapi_impl_atomic_inc(NULL,&addr->lValue,&lOut,sizeof(addr->lValue),&status));
        assert(2 == lOut);
        assert(2 == addr->lValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->lValue,NULL,sizeof(addr->lValue),&status));
        assert(1 == addr->lValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->lValue,&lOut,sizeof(addr->lValue),&status));
        assert(0 == lOut);
        assert(0 == addr->lValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->lValue,NULL,sizeof(addr->lValue),&status));
        assert((uint32_t) -1 == addr->lValue);
        assert(mrapi_impl_atomic_inc(NULL,&addr->lValue,NULL,sizeof(addr->lValue),&status));
        assert(0 == addr->lValue);

        addr->llValue = 0;
        assert(mrapi_impl_atomic_inc(NULL,&addr->llValue,NULL,sizeof(addr->llValue),&status));
        assert(1 == addr->llValue);
        assert(mrapi_impl_atomic_inc(NULL,&addr->llValue,&llOut,sizeof(addr->llValue),&status));
        assert(2 == llOut);
        assert(2 == addr->llValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->llValue,NULL,sizeof(addr->llValue),&status));
        assert(1 == addr->llValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->llValue,&llOut,sizeof(addr->llValue),&status));
        assert(0 == llOut);
        assert(0 == addr->llValue);
        assert(mrapi_impl_atomic_dec(NULL,&addr->llValue,NULL,sizeof(addr->llValue),&status));
        assert((uint64_t)-1 == addr->llValue);
        assert(mrapi_impl_atomic_inc(NULL,&addr->llValue,NULL,sizeof(addr->llValue),&status));
        assert(0 == addr->llValue);
	}

	// Atomic or
	{
        addr->cValue = 0;
        cTest = 0x55;
        assert(mrapi_impl_atomic_or(NULL,&addr->cValue,&cTest,NULL,sizeof(addr->cValue),&status));
        assert(0x55 == addr->cValue);
        assert(mrapi_impl_atomic_read(NULL,&addr->cValue,&cOut,sizeof(addr->cValue),&status));
        assert(0x55 == cOut);
        cTest = 0xAA;
        assert(mrapi_impl_atomic_or(NULL,&addr->cValue,&cTest,&cOut,sizeof(addr->cValue),&status));
        assert(0x55 == cOut);
        assert(0xFF == addr->cValue);

        addr->wValue = 0;
        wTest = 0x5555;
        assert(mrapi_impl_atomic_or(NULL,&addr->wValue,&wTest,NULL,sizeof(addr->wValue),&status));
        assert(0x5555 == addr->wValue);
        assert(mrapi_impl_atomic_read(NULL,&addr->wValue,&wOut,sizeof(addr->wValue),&status));
        assert(0x5555 == wOut);
        wTest = 0xAAAA;
        assert(mrapi_impl_atomic_or(NULL,&addr->wValue,&wTest,&wOut,sizeof(addr->wValue),&status));
        assert(0x5555 == wOut);
        assert(0xFFFF == addr->wValue);

        addr->lValue = 0;
        lTest = 0x55555555;
        assert(mrapi_impl_atomic_or(NULL,&addr->lValue,&lTest,NULL,sizeof(addr->lValue),&status));
        assert(0x55555555 == addr->lValue);
        assert(mrapi_impl_atomic_read(NULL,&addr->lValue,&lOut,sizeof(addr->lValue),&status));
        assert(0x55555555 == lOut);
        lTest = 0xAAAAAAAA;
        assert(mrapi_impl_atomic_or(NULL,&addr->lValue,&lTest,&lOut,sizeof(addr->lValue),&status));
        assert(0x55555555 == lOut);
        assert(0xFFFFFFFF == addr->lValue);

        addr->llValue = 0;
        llTest = 0x5555555555555555;
        assert(mrapi_impl_atomic_or(NULL,&addr->llValue,&llTest,NULL,sizeof(addr->llValue),&status));
        assert(0x5555555555555555 == addr->llValue);
        assert(mrapi_impl_atomic_read(NULL,&addr->llValue,&llOut,sizeof(addr->llValue),&status));
        assert(0x5555555555555555 == llOut);
        llTest = 0xAAAAAAAAAAAAAAAA;
        assert(mrapi_impl_atomic_or(NULL,&addr->llValue,&llTest,&llOut,sizeof(addr->llValue),&status));
        assert(0x5555555555555555 == llOut);
        assert(0xFFFFFFFFFFFFFFFF == addr->llValue);
	}

	// Atomic and
	{
        addr->cValue = 0xFF;
        cTest = 0x55;
        assert(mrapi_impl_atomic_and(NULL,&addr->cValue,&cTest,NULL,sizeof(addr->cValue),&status));
        assert(0x55 == addr->cValue);
        cTest = 0xAA;
        assert(mrapi_impl_atomic_and(NULL,&addr->cValue,&cTest,&cOut,sizeof(addr->cValue),&status));
        assert(0x55 == cOut);
        assert(0 == addr->cValue);

        addr->wValue = 0xFFFF;
        wTest = 0x5555;
        assert(mrapi_impl_atomic_and(NULL,&addr->wValue,&wTest,NULL,sizeof(addr->wValue),&status));
        assert(0x5555 == addr->wValue);
        wTest = 0xAAAA;
        assert(mrapi_impl_atomic_and(NULL,&addr->wValue,&wTest,&wOut,sizeof(addr->wValue),&status));
        assert(0x5555 == wOut);
        assert(0 == addr->wValue);

        addr->lValue = 0xFFFFFFFF;
        lTest = 0x55555555;
        assert(mrapi_impl_atomic_and(NULL,&addr->lValue,&lTest,NULL,sizeof(addr->lValue),&status));
        assert(0x55555555 == addr->lValue);
        lTest = 0xAAAAAAAA;
        assert(mrapi_impl_atomic_and(NULL,&addr->lValue,&lTest,&lOut,sizeof(addr->lValue),&status));
        assert(0x55555555 == lOut);
        assert(0 == addr->lValue);

        addr->llValue = 0xFFFFFFFFFFFFFFFF;
        llTest = 0x5555555555555555;
        assert(mrapi_impl_atomic_and(NULL,&addr->llValue,&llTest,NULL,sizeof(addr->llValue),&status));
        assert(0x5555555555555555 == addr->llValue);
        llTest = 0xAAAAAAAAAAAAAAAA;
        assert(mrapi_impl_atomic_and(NULL,&addr->llValue,&llTest,&llOut,sizeof(addr->llValue),&status));
        assert(0x5555555555555555 == llOut);
        assert(0 == addr->llValue);
	}

	// Atomic xor
	{
        addr->cValue = 0;
        cTest = 0x55;
        assert(mrapi_impl_atomic_xor(NULL,&addr->cValue,&cTest,NULL,sizeof(addr->cValue),&status));
        assert(0x55 == addr->cValue);
        assert(mrapi_impl_atomic_xor(NULL,&addr->cValue,&cTest,&cOut,sizeof(addr->cValue),&status));
        assert(0x55 == cOut);
        assert(0 == addr->cValue);

        addr->wValue = 0;
        wTest = 0x5555;
        assert(mrapi_impl_atomic_xor(NULL,&addr->wValue,&wTest,NULL,sizeof(addr->wValue),&status));
        assert(0x5555 == addr->wValue);
        assert(mrapi_impl_atomic_xor(NULL,&addr->wValue,&wTest,&wOut,sizeof(addr->wValue),&status));
        assert(0x5555 == wOut);
        assert(0 == addr->wValue);

        addr->lValue = 0;
        lTest = 0x55555555;
        assert(mrapi_impl_atomic_xor(NULL,&addr->lValue,&lTest,NULL,sizeof(addr->lValue),&status));
        assert(0x55555555 == addr->lValue);
        assert(mrapi_impl_atomic_xor(NULL,&addr->lValue,&lTest,&lOut,sizeof(addr->lValue),&status));
        assert(0x55555555 == lOut);
        assert(0 == addr->lValue);

        addr->llValue = 0;
        llTest = 0x5555555555555555;
        assert(mrapi_impl_atomic_xor(NULL,&addr->llValue,&llTest,NULL,sizeof(addr->llValue),&status));
        assert(0x5555555555555555 == addr->llValue);
        assert(mrapi_impl_atomic_xor(NULL,&addr->llValue,&llTest,&llOut,sizeof(addr->llValue),&status));
        assert(0x5555555555555555 == llOut);
        assert(0 == addr->llValue);
	}

	// Atomic compare and swap integer
	{
        addr->cValue = 0;
        cTest = 1;
        cComp = 1;
        cOut = 0;
        assert(cComp != addr->cValue);
        assert(!mrapi_impl_atomic_cas(NULL,&addr->cValue,&cTest,&cComp,&cOut,sizeof(addr->cValue),&status));
        assert(0 == cOut);
        assert(0 == addr->cValue);
        cComp = 0;
        assert(cComp == addr->cValue);
        assert(mrapi_impl_atomic_cas(NULL,&addr->cValue,&cTest,&cComp,&cOut,sizeof(addr->cValue),&status));
        assert(0 == cOut);
        assert(cTest == addr->cValue);

        addr->wValue = 0;
        wTest = 1;
        wComp = 1;
        wOut = 0;
        assert(wComp != addr->wValue);
        assert(!mrapi_impl_atomic_cas(NULL,&addr->wValue,&wTest,&wComp,&wOut,sizeof(addr->wValue),&status));
        assert(0 == wOut);
        assert(0 == addr->wValue);
        wComp = 0;
        assert(wComp == addr->wValue);
        assert(mrapi_impl_atomic_cas(NULL,&addr->wValue,&wTest,&wComp,&wOut,sizeof(addr->wValue),&status));
        assert(0 == wOut);
        assert(wTest == addr->wValue);

        addr->lValue = 0;
        lTest = 1;
        lComp = 1;
        lOut = 0;
        assert(lComp != addr->lValue);
        assert(!mrapi_impl_atomic_cas(NULL,&addr->lValue,&lTest,&lComp,&lOut,sizeof(addr->lValue),&status));
        assert(0 == lOut);
        assert(0 == addr->lValue);
        lComp = 0;
        assert(lComp == addr->lValue);
        assert(mrapi_impl_atomic_cas(NULL,&addr->lValue,&lTest,&lComp,&lOut,sizeof(addr->lValue),&status));
        assert(0 == lOut);
        assert(lTest == addr->lValue);

        addr->llValue = 0;
        llTest = 1;
        llComp = 1;
        llOut = 0;
        assert(llComp != addr->llValue);
        assert(!mrapi_impl_atomic_cas( NULL,&addr->llValue,&llTest,&llComp,&llOut,sizeof(addr->llValue),&status));
        assert(0 == llOut);
        assert(0 == addr->llValue);
        llComp = 0;
        assert(llComp == addr->llValue);
        assert(mrapi_impl_atomic_cas(NULL,&addr->llValue,&llTest,&llComp,&llOut,sizeof(addr->llValue),&status));
        assert(0 == llOut);
        assert(llTest == addr->llValue);
    }

	// Atomic exchange integer
	{
        addr->cValue = 0;
        cTest = 1;
        assert(mrapi_impl_atomic_xchg(NULL,&addr->cValue,&cTest,NULL,sizeof(addr->cValue),&status));
        assert(1 == addr->cValue);
        cTest = 0;
        assert(mrapi_impl_atomic_xchg(NULL,&addr->cValue,&cTest,&cOut,sizeof(addr->cValue),&status));
        assert(1 == cOut);
        assert(0 == addr->cValue);

        addr->wValue = 0;
        wTest = 1;
        assert(mrapi_impl_atomic_xchg(NULL,&addr->wValue,&wTest,NULL,sizeof(addr->wValue),&status));
        assert(1 == addr->wValue);
        wTest = 0;
        assert(mrapi_impl_atomic_xchg(NULL,&addr->wValue,&wTest,&wOut,sizeof(addr->wValue),&status));
        assert(1 == wOut);
        assert(0 == addr->wValue);

        addr->lValue = 0;
        lTest = 1;
        assert(mrapi_impl_atomic_xchg(NULL,&addr->lValue,&lTest,NULL,sizeof(addr->lValue),&status));
        assert(1 == addr->lValue);
        lTest = 0;
        assert(mrapi_impl_atomic_xchg(NULL,&addr->lValue,&lTest,&lOut,sizeof(addr->lValue),&status));
        assert(1 == lOut);
        assert(0 == addr->lValue);

        addr->llValue = 0;
        llTest = 1;
        assert(mrapi_impl_atomic_xchg(NULL,&addr->llValue,&llTest,NULL,sizeof(addr->llValue),&status));
        assert(1 == addr->llValue);
        llTest = 0;
        assert(mrapi_impl_atomic_xchg(NULL,&addr->llValue,&llTest,&llOut,sizeof(addr->llValue),&status));
        assert(1 == llOut);
        assert(0 == addr->llValue);
	}

	// Atomic compare and swap pointer
	{
        uintptr_t pValue = 0;
        uintptr_t pTest = 0;
        uintptr_t pComp = 0;
        uintptr_t pOut = 0;

        pValue = (uintptr_t)addr->pValue1;
        pTest = (uintptr_t)addr->pValue2;
        pComp = (uintptr_t)addr->pValue2;
        pOut = 0;
        assert(pComp != pValue);
        assert(!mrapi_impl_atomic_cas_ptr(NULL,&pValue,pTest,pComp,&pOut,&status));
        assert((uintptr_t)addr->pValue1 == pOut);
        assert((uintptr_t)addr->pValue1 == pValue);
        pComp = (uintptr_t)addr->pValue1;
        assert(pComp == pValue);
        assert(mrapi_impl_atomic_cas_ptr(NULL,&pValue,pTest,pComp,&pOut,&status));
        assert((uintptr_t)addr->pValue1 == pOut);
        assert(pTest == pValue);
        assert(mrapi_impl_atomic_read_ptr(NULL,&pValue,&pOut,&status));
        assert(pTest == pOut);
    }

	// Atomic exchange pointer
	{
        uintptr_t pValue = 0;
        uintptr_t pTest = 0;
        uintptr_t pOut = 0;

        pValue = (uintptr_t)addr->pValue1;
        pTest = (uintptr_t)addr->pValue2;
        assert(mrapi_impl_atomic_xchg_ptr(NULL,&pValue,pTest,NULL,&status));
        assert(pTest == pValue);
        pTest = (uintptr_t)addr->pValue1;
        assert(mrapi_impl_atomic_xchg_ptr(NULL,&pValue,pTest,&pOut,&status));
        assert((uintptr_t)addr->pValue2 == pOut);
        assert((uintptr_t)addr->pValue1 == pValue);
	}

    // Memory barrier
	{
        mrapi_impl_atomic_sync(NULL);
	}

    // Atomic lock and release
	{
#if (__unix__)
        addr->cValue = 0;
        assert(mrapi_impl_atomic_lock(NULL,&addr->cValue,&cOut,sizeof(uint8_t),&status));
        assert(0 == cOut);
        assert(1 == addr->cValue);
        assert(mrapi_impl_atomic_release(NULL,&addr->cValue,sizeof(uint8_t),&status));
        assert(0 == addr->cValue);

        addr->wValue = 0;
        assert(mrapi_impl_atomic_lock(NULL,&addr->wValue,&wOut,sizeof(uint16_t),&status));
        assert(0 == wOut);
        assert(1 == addr->wValue);
        assert(mrapi_impl_atomic_release(NULL,&addr->wValue,sizeof(uint16_t),&status));
        assert(0 == addr->wValue);
#endif  // (__unix__)

        addr->lValue = 0;
        assert(mrapi_impl_atomic_lock(NULL,&addr->lValue,&lOut,sizeof(uint32_t),&status));
        assert(0 == lOut);
        assert(1 == addr->lValue);
        assert(mrapi_impl_atomic_release(NULL,&addr->lValue,sizeof(uint32_t),&status));
        assert(0 == addr->lValue);

#if (__unix__)
        addr->llValue = 0;
        assert(mrapi_impl_atomic_lock(NULL,&addr->llValue,&llOut,sizeof(uint64_t),&status));
        assert(0 == llOut);
        assert(1 == addr->llValue);
        assert(mrapi_impl_atomic_release(NULL,&addr->llValue,sizeof(uint64_t),&status));
        assert(0 == addr->llValue);
#endif  // (__unix__)
	}

    // Atomic bit operations
	{
        int orig = 0;

        addr->lValue = 0;
        assert(mrapi_impl_atomic_set(NULL,&addr->lValue,0,NULL,sizeof(uint32_t),&status));
        assert(1 == addr->lValue);
        assert(!mrapi_impl_atomic_set(NULL,&addr->lValue,0,NULL,sizeof(uint32_t),&status));
        assert(MRAPI_ERR_ATOM_OP_FAILED == status);
        assert(mrapi_impl_atomic_set(NULL,&addr->lValue,1,&orig,sizeof(uint32_t),&status));
        assert(0 == orig);
        assert(3 == addr->lValue);
        assert(mrapi_impl_atomic_clear(NULL,&addr->lValue,0,NULL,sizeof(uint32_t),&status));
        assert(2 == addr->lValue);
        assert(!mrapi_impl_atomic_clear(NULL,&addr->lValue,0,NULL,sizeof(uint32_t),&status));
        assert(MRAPI_ERR_ATOM_OP_FAILED == status);
        assert(mrapi_impl_atomic_clear(NULL,&addr->lValue,1,&orig,sizeof(uint32_t),&status));
        assert(1 == orig);
        assert(0 == addr->lValue);
        assert(mrapi_impl_atomic_change(NULL,&addr->lValue,0,NULL,sizeof(uint32_t),&status));
        assert(1 == addr->lValue);
        assert(mrapi_impl_atomic_change(NULL,&addr->lValue,1,&orig,sizeof(uint32_t),&status));
        assert(0 == orig);
        assert(3 == addr->lValue);
	}

	assert(mrapi_impl_shmem_detach(shmem_id));
	assert(mrapi_impl_shmem_delete(shmem_id));
