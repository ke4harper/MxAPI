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
    int created = 0;
    uint32_t id = 0;
    shmem_data_t* data = NULL;

    for(int i = 0; i < 2; i++) {

        switch(i) {
        case 0:
            data = (shmem_data_t*)malloc(sizeof(shmem_data_t));
            break;
        case 1:
    		assert(sys_file_key(NULL,'j',&key));
    		created = 0;
            if(-1 == (id = sys_shmem_get(key,sizeof(shmem_data_t)))) {
                created = 1;
                id = sys_shmem_create(key,sizeof(shmem_data_t));
            }
            data = (shmem_data_t*)sys_shmem_attach(id);
            break;
        }

#if (__unix__||__atomic_barrier_test__)
	// Local memory write/read barrier
	{
		mrapi_atomic_barrier_t axb;
		mrapi_atomic_barrier_t axb2;
        mca_timeout_t timeout = MCA_INFINITE;
        mca_timeout_t timeout2 = 0;
		mrapi_msg_t msg = { 0 };
		unsigned index = 0;
		pid_t src = 1;
		pid_t dest = 2;
		assert(sys_atomic_barrier_init(&axb,src,dest,&msg,1,sizeof(msg),&index,timeout));
		assert(src == axb.src);
		assert(dest = axb.dest);
		assert(1 == axb.elems);
		assert(sizeof(msg) == axb.size);
		assert(&index == axb.sync.pindex);
        assert(0 == axb.sync.last_counter);
		assert(MRAPI_FALSE == axb.sync.hold);
		assert(&msg == axb.buffer);
		assert(sys_atomic_barrier_init(&axb2,src,dest,&msg,1,sizeof(msg),&index,timeout2));
        // Mode: MRAPI_ATOMIC_NONE
        atomic_barrier_mode(&axb,MRAPI_ATOMIC_NONE);
        sys_atomic_hold(&axb,MRAPI_TRUE);
        assert(MRAPI_TRUE == axb.sync.hold);
        assert(0 == msg.counter);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(0 == msg.counter);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        sys_atomic_hold(&axb,MRAPI_FALSE);
        assert(MRAPI_FALSE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        // Mode: MRAPI_ATOMIC_WRITE
        atomic_barrier_mode(&axb,MRAPI_ATOMIC_WRITE);
        atomic_barrier_mode(&axb2,MRAPI_ATOMIC_WRITE);
        sys_atomic_hold(&axb,MRAPI_TRUE);
        assert(MRAPI_TRUE == axb.sync.hold);
        assert(0 == msg.counter);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(1 == msg.counter);
        assert(!try_atomic_barrier_acquire(&axb2)); // concurrent write fails
        atomic_barrier_mode(&axb2,MRAPI_ATOMIC_READ);
        assert(!try_atomic_barrier_acquire(&axb2)); // concurrent read fails
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        sys_atomic_hold(&axb,MRAPI_FALSE);
        assert(MRAPI_FALSE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
        assert(2 == msg.counter);
		assert(MRAPI_FALSE == msg.valid);
        assert(!atomic_barrier_release(&axb)); // write release without acquire fails
        // Mode: MRAPI_ATOMIC_READ
        atomic_barrier_mode(&axb,MRAPI_ATOMIC_READ);
        atomic_barrier_mode(&axb2,MRAPI_ATOMIC_READ);
        sys_atomic_hold(&axb,MRAPI_TRUE);
        assert(MRAPI_TRUE == axb.sync.hold);
        assert(2 == msg.counter);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(2 == msg.counter);
		assert(try_atomic_barrier_acquire(&axb2)); // concurrent read succeeds
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        sys_atomic_hold(&axb,MRAPI_FALSE);
        assert(MRAPI_FALSE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(atomic_barrier_release(&axb)); // read release without acquire succeeds
		assert(try_atomic_barrier_acquire(&axb));
        atomic_barrier_mode(&axb2,MRAPI_ATOMIC_WRITE);
		assert(try_atomic_barrier_acquire(&axb2)); // concurrent write succeeds
        assert(!atomic_barrier_release(&axb)); // read fails after concurrent write
	}

	// Local memory exchange barrier
	{
		mrapi_atomic_barrier_t axb;
		mrapi_atomic_barrier_t raxb;
        mca_timeout_t timeout = MCA_INFINITE;
		mrapi_msg_t msg = { 0 };
		unsigned index = 0;
		pid_t src = 1;
		pid_t dest = 2;
		assert(sys_atomic_exchange_init(&axb,src,dest,&msg,1,sizeof(msg),&index,timeout));
		assert(src == axb.src);
		assert(dest = axb.dest);
		assert(1 == axb.elems);
		assert(sizeof(msg) == axb.size);
		assert(&index == axb.sync.pindex);
		assert(MRAPI_FALSE == axb.sync.hold);
		assert(&msg == axb.buffer);
		assert(sys_atomic_exchange_init(&raxb,dest,src,&msg,1,sizeof(msg),&index,timeout));
		assert(dest == raxb.src);
		assert(src = raxb.dest);
		assert(1 == raxb.elems);
		assert(sizeof(msg) == raxb.size);
		assert(&index == raxb.sync.pindex);
		assert(MRAPI_FALSE == raxb.sync.hold);
		assert(&msg == raxb.buffer);
        // Mode: MRAPI_ATOMIC_NONE
        atomic_barrier_mode(&axb,MRAPI_ATOMIC_NONE);
        sys_atomic_hold(&axb,MRAPI_TRUE);
        assert(MRAPI_TRUE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        sys_atomic_hold(&axb,MRAPI_FALSE);
        assert(MRAPI_FALSE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        // Mode: MRAPI_ATOMIC_WRITE
        atomic_barrier_mode(&axb,MRAPI_ATOMIC_WRITE);
        sys_atomic_hold(&axb,MRAPI_TRUE);
        assert(MRAPI_TRUE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        sys_atomic_hold(&axb,MRAPI_FALSE);
        assert(MRAPI_FALSE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_TRUE == msg.valid);
        // Mode: MRAPI_ATOMIC_READ
        atomic_barrier_mode(&raxb,MRAPI_ATOMIC_READ);
        sys_atomic_hold(&raxb,MRAPI_TRUE);
        assert(MRAPI_TRUE == raxb.sync.hold);
		assert(try_atomic_barrier_acquire(&raxb));
		assert(0 == *raxb.sync.pindex);
		assert(MRAPI_TRUE == msg.valid);
        assert(atomic_barrier_release(&raxb));
		assert(0 == *raxb.sync.pindex);
		assert(MRAPI_TRUE == msg.valid);
        sys_atomic_hold(&raxb,MRAPI_FALSE);
        assert(MRAPI_FALSE == raxb.sync.hold);
		assert(try_atomic_barrier_acquire(&raxb));
		assert(0 == *raxb.sync.pindex);
		assert(MRAPI_TRUE == msg.valid);
        assert(atomic_barrier_release(&raxb));
		assert(0 == *raxb.sync.pindex);
		assert(MRAPI_FALSE == msg.valid);
	}

	// Shared memory write/read barrier
    {
		mrapi_atomic_barrier_t axb;
        mca_timeout_t timeout = MCA_INFINITE;
        int i = 0;
		pid_t src = 1;
		pid_t dest = 2;
        data->index = 0;
        for(i = 0; i < 4; i++) {
          memset(&data->msg[i],0,sizeof(data->msg[i]));
        }
		assert(sys_atomic_barrier_init(&axb,src,dest,data->msg,4,sizeof(data->msg[0]),&data->index,timeout));
		assert(src == axb.src);
		assert(dest = axb.dest);
		assert(4 == axb.elems);
		assert(sizeof(data->msg[0]) == axb.size);
		assert(&data->index == axb.sync.pindex);
		assert(MRAPI_FALSE == axb.sync.hold);
		assert(data->msg == axb.buffer);
        // Mode: MRAPI_ATOMIC_NONE
        atomic_barrier_mode(&axb,MRAPI_ATOMIC_NONE);
        sys_atomic_hold(&axb,MRAPI_TRUE);
        assert(MRAPI_TRUE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        sys_atomic_hold(&axb,MRAPI_FALSE);
        assert(MRAPI_FALSE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        // Mode: MRAPI_ATOMIC_WRITE
        atomic_barrier_mode(&axb,MRAPI_ATOMIC_WRITE);
        sys_atomic_hold(&axb,MRAPI_TRUE);
        assert(MRAPI_TRUE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        sys_atomic_hold(&axb,MRAPI_FALSE);
        assert(MRAPI_FALSE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        // Mode: MRAPI_ATOMIC_READ
        atomic_barrier_mode(&axb,MRAPI_ATOMIC_READ);
        sys_atomic_hold(&axb,MRAPI_TRUE);
        assert(MRAPI_TRUE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        sys_atomic_hold(&axb,MRAPI_FALSE);
        assert(MRAPI_FALSE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
	}

	// Shared memory exchange barrier
	{
		mrapi_atomic_barrier_t axb;
		mrapi_atomic_barrier_t raxb;
        mca_timeout_t timeout = MCA_INFINITE;
        int i = 0;
		pid_t src = 1;
		pid_t dest = 2;
        data->index = 0;
        for(i = 0; i < 4; i++) {
          memset(&data->msg[i],0,sizeof(data->msg[i]));
        }
		assert(sys_atomic_exchange_init(&axb,src,dest,data->msg,4,sizeof(data->msg[0]),&data->index,timeout));
		assert(src == axb.src);
		assert(dest = axb.dest);
		assert(4 == axb.elems);
		assert(sizeof(data->msg[0]) == axb.size);
		assert(&data->index == axb.sync.pindex);
		assert(MRAPI_FALSE == axb.sync.hold);
		assert(data->msg == axb.buffer);
		assert(sys_atomic_exchange_init(&raxb,dest,src,data->msg,4,sizeof(data->msg[0]),&data->index,timeout));
		assert(dest == raxb.src);
		assert(src = raxb.dest);
		assert(4 == raxb.elems);
		assert(sizeof(data->msg[0]) == raxb.size);
		assert(&data->index == raxb.sync.pindex);
		assert(MRAPI_FALSE == raxb.sync.hold);
		assert(data->msg == raxb.buffer);
        // Mode: MRAPI_ATOMIC_NONE
        atomic_barrier_mode(&axb,MRAPI_ATOMIC_NONE);
        sys_atomic_hold(&axb,MRAPI_TRUE);
        assert(MRAPI_TRUE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        sys_atomic_hold(&axb,MRAPI_FALSE);
        assert(MRAPI_FALSE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        // Mode: MRAPI_ATOMIC_WRITE
        atomic_barrier_mode(&axb,MRAPI_ATOMIC_WRITE);
        sys_atomic_hold(&axb,MRAPI_TRUE);
        assert(MRAPI_TRUE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        sys_atomic_hold(&axb,MRAPI_FALSE);
        assert(MRAPI_FALSE == axb.sync.hold);
		assert(try_atomic_barrier_acquire(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&axb));
		assert(0 == *axb.sync.pindex);
		assert(MRAPI_TRUE == data->msg[data->index].valid);
        // Mode: MRAPI_ATOMIC_READ
        atomic_barrier_mode(&raxb,MRAPI_ATOMIC_READ);
        sys_atomic_hold(&raxb,MRAPI_TRUE);
        assert(MRAPI_TRUE == raxb.sync.hold);
		assert(try_atomic_barrier_acquire(&raxb));
		assert(0 == *raxb.sync.pindex);
		assert(MRAPI_TRUE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&raxb));
		assert(0 == *raxb.sync.pindex);
		assert(MRAPI_TRUE == data->msg[data->index].valid);
        sys_atomic_hold(&raxb,MRAPI_FALSE);
        assert(MRAPI_FALSE == raxb.sync.hold);
		assert(try_atomic_barrier_acquire(&raxb));
		assert(0 == *raxb.sync.pindex);
		assert(MRAPI_TRUE == data->msg[data->index].valid);
        assert(atomic_barrier_release(&raxb));
		assert(0 == *raxb.sync.pindex);
		assert(MRAPI_FALSE == data->msg[data->index].valid);
	}
#endif  /* (__unix__||__atomic_barrier_test__) */

	// Atomic add
	{
#if (__unix__)
        data->cValue = 0;
        cTest = 2;
        assert(sys_atomic_add(NULL,&data->cValue,&cTest,NULL,sizeof(data->cValue)));
        assert(2 == data->cValue);
        assert(sys_atomic_read(NULL,&data->cValue,&cOut,sizeof(data->cValue)));
        assert(2 == cOut);
        assert(sys_atomic_add(NULL,&data->cValue,&cTest,&cOut,sizeof(data->cValue)));
        assert(2 == cOut);
        assert(4 == data->cValue);

        data->wValue = 0;
        wTest = 2;
        assert(sys_atomic_add(NULL,&data->wValue,&wTest,NULL,sizeof(data->wValue)));
        assert(2 == data->wValue);
        assert(sys_atomic_read(NULL,&data->wValue,&wOut,sizeof(data->wValue)));
        assert(2 == wOut);
        assert(sys_atomic_add(NULL,&data->wValue,&wTest,&wOut,sizeof(data->wValue)));
        assert(2 == wOut);
        assert(4 == data->wValue);
#endif  // (__unix__)

        data->lValue = 0;
        lTest = 2;
        assert(sys_atomic_add(NULL,&data->lValue,&lTest,NULL,sizeof(data->lValue)));
        assert(2 == data->lValue);
        assert(sys_atomic_read(NULL,&data->lValue,&lOut,sizeof(data->lValue)));
        assert(2 == lOut);
        assert(sys_atomic_add(NULL,&data->lValue,&lTest,&lOut,sizeof(data->lValue)));
        assert(2 == lOut);
        assert(4 == data->lValue);

        data->llValue = 0;
        llTest = 2;
        assert(sys_atomic_add(NULL,&data->llValue,&llTest,NULL,sizeof(data->llValue)));
        assert(2 == data->llValue);
        assert(sys_atomic_read(NULL,&data->llValue,&llOut,sizeof(data->llValue)));
        assert(2 == llOut);
        assert(sys_atomic_add(NULL,&data->llValue,&llTest,&llOut,sizeof(data->llValue)));
        assert(2 == llOut);
        assert(4 == data->llValue);
	}

	// Atomic increment / decrement
	{
#if (__unix__)
        data->cValue = 0;
        assert(sys_atomic_inc(NULL,&data->cValue,NULL,sizeof(data->cValue)));
#endif  // (__unix__)

        data->wValue = 0;
        assert(sys_atomic_inc(NULL,&data->wValue,NULL,sizeof(data->wValue)));
        assert(1 == data->wValue);
        assert(sys_atomic_inc(NULL,&data->wValue,&wOut,sizeof(data->wValue)));
        assert(2 == wOut);
        assert(2 == data->wValue);
        assert(sys_atomic_dec(NULL,&data->wValue,NULL,sizeof(data->wValue)));
        assert(1 == data->wValue);
        assert(sys_atomic_dec(NULL,&data->wValue,&wOut,sizeof(data->wValue)));
        assert(0 == wOut);
        assert(0 == data->wValue);
        assert(sys_atomic_dec(NULL,&data->wValue,NULL,sizeof(data->wValue)));
        assert((uint16_t)-1 == data->wValue);
        assert(sys_atomic_inc(NULL,&data->wValue,NULL,sizeof(data->wValue)));
        assert(0 == data->wValue);

        data->lValue = 0;
        assert(sys_atomic_inc(NULL,&data->lValue,NULL,sizeof(data->lValue)));
        assert(1 == data->lValue);
        assert(sys_atomic_inc(NULL,&data->lValue,&lOut,sizeof(data->lValue)));
        assert(2 == lOut);
        assert(2 == data->lValue);
        assert(sys_atomic_dec(NULL,&data->lValue,NULL,sizeof(data->lValue)));
        assert(1 == data->lValue);
        assert(sys_atomic_dec(NULL,&data->lValue,&lOut,sizeof(data->lValue)));
        assert(0 == lOut);
        assert(0 == data->lValue);
        assert(sys_atomic_dec(NULL,&data->lValue,NULL,sizeof(data->lValue)));
        assert((uint32_t) -1 == data->lValue);
        assert(sys_atomic_inc(NULL,&data->lValue,NULL,sizeof(data->lValue)));
        assert(0 == data->lValue);

        data->llValue = 0;
        assert(sys_atomic_inc(NULL,&data->llValue,NULL,sizeof(data->llValue)));
        assert(1 == data->llValue);
        assert(sys_atomic_inc(NULL,&data->llValue,&llOut,sizeof(data->llValue)));
        assert(2 == llOut);
        assert(2 == data->llValue);
        assert(sys_atomic_dec(NULL,&data->llValue,NULL,sizeof(data->llValue)));
        assert(1 == data->llValue);
        assert(sys_atomic_dec(NULL,&data->llValue,&llOut,sizeof(data->llValue)));
        assert(0 == llOut);
        assert(0 == data->llValue);
        assert(sys_atomic_dec(NULL,&data->llValue,NULL,sizeof(data->llValue)));
        assert((uint64_t)-1 == data->llValue);
        assert(sys_atomic_inc(NULL,&data->llValue,NULL,sizeof(data->llValue)));
        assert(0 == data->llValue);
	}

	// Atomic or
	{
        data->cValue = 0;
        cTest = 0x55;
        assert(sys_atomic_or(NULL,&data->cValue,&cTest,NULL,sizeof(data->cValue)));
        assert(0x55 == data->cValue);
        assert(sys_atomic_read(NULL,&data->cValue,&cOut,sizeof(data->cValue)));
        assert(0x55 == cOut);
        cTest = 0xAA;
        assert(sys_atomic_or(NULL,&data->cValue,&cTest,&cOut,sizeof(data->cValue)));
        assert(0x55 == cOut);
        assert(0xFF == data->cValue);

        data->wValue = 0;
        wTest = 0x5555;
        assert(sys_atomic_or(NULL,&data->wValue,&wTest,NULL,sizeof(data->wValue)));
        assert(0x5555 == data->wValue);
        assert(sys_atomic_read(NULL,&data->wValue,&wOut,sizeof(data->wValue)));
        assert(0x5555 == wOut);
        wTest = 0xAAAA;
        assert(sys_atomic_or(NULL,&data->wValue,&wTest,&wOut,sizeof(data->wValue)));
        assert(0x5555 == wOut);
        assert(0xFFFF == data->wValue);

        data->lValue = 0;
        lTest = 0x55555555;
        assert(sys_atomic_or(NULL,&data->lValue,&lTest,NULL,sizeof(data->lValue)));
        assert(0x55555555 == data->lValue);
        assert(sys_atomic_read(NULL,&data->lValue,&lOut,sizeof(data->lValue)));
        assert(0x55555555 == lOut);
        lTest = 0xAAAAAAAA;
        assert(sys_atomic_or(NULL,&data->lValue,&lTest,&lOut,sizeof(data->lValue)));
        assert(0x55555555 == lOut);
        assert(0xFFFFFFFF == data->lValue);

        data->llValue = 0;
        llTest = 0x5555555555555555;
        assert(sys_atomic_or(NULL,&data->llValue,&llTest,NULL,sizeof(data->llValue)));
        assert(0x5555555555555555 == data->llValue);
        assert(sys_atomic_read(NULL,&data->llValue,&llOut,sizeof(data->llValue)));
        assert(0x5555555555555555 == llOut);
        llTest = 0xAAAAAAAAAAAAAAAA;
        assert(sys_atomic_or(NULL,&data->llValue,&llTest,&llOut,sizeof(data->llValue)));
        assert(0x5555555555555555 == llOut);
        assert(0xFFFFFFFFFFFFFFFF == data->llValue);
	}

	// Atomic and
	{
        data->cValue = 0xFF;
        cTest = 0x55;
        assert(sys_atomic_and(NULL,&data->cValue,&cTest,NULL,sizeof(data->cValue)));
        assert(0x55 == data->cValue);
        cTest = 0xAA;
        assert(sys_atomic_and(NULL,&data->cValue,&cTest,&cOut,sizeof(data->cValue)));
        assert(0x55 == cOut);
        assert(0 == data->cValue);

        data->wValue = 0xFFFF;
        wTest = 0x5555;
        assert(sys_atomic_and(NULL,&data->wValue,&wTest,NULL,sizeof(data->wValue)));
        assert(0x5555 == data->wValue);
        wTest = 0xAAAA;
        assert(sys_atomic_and(NULL,&data->wValue,&wTest,&wOut,sizeof(data->wValue)));
        assert(0x5555 == wOut);
        assert(0 == data->wValue);

        data->lValue = 0xFFFFFFFF;
        lTest = 0x55555555;
        assert(sys_atomic_and(NULL,&data->lValue,&lTest,NULL,sizeof(data->lValue)));
        assert(0x55555555 == data->lValue);
        lTest = 0xAAAAAAAA;
        assert(sys_atomic_and(NULL,&data->lValue,&lTest,&lOut,sizeof(data->lValue)));
        assert(0x55555555 == lOut);
        assert(0 == data->lValue);

        data->llValue = 0xFFFFFFFFFFFFFFFF;
        llTest = 0x5555555555555555;
        assert(sys_atomic_and(NULL,&data->llValue,&llTest,NULL,sizeof(data->llValue)));
        assert(0x5555555555555555 == data->llValue);
        llTest = 0xAAAAAAAAAAAAAAAA;
        assert(sys_atomic_and(NULL,&data->llValue,&llTest,&llOut,sizeof(data->llValue)));
        assert(0x5555555555555555 == llOut);
        assert(0 == data->llValue);
	}

	// Atomic xor
	{
        data->cValue = 0;
        cTest = 0x55;
        assert(sys_atomic_xor(NULL,&data->cValue,&cTest,NULL,sizeof(data->cValue)));
        assert(0x55 == data->cValue);
        assert(sys_atomic_xor( NULL,&data->cValue,&cTest,&cOut,sizeof(data->cValue)));
        assert(0x55 == cOut);
        assert(0 == data->cValue);

        data->wValue = 0;
        wTest = 0x5555;
        assert(sys_atomic_xor(NULL,&data->wValue,&wTest,NULL,sizeof(data->wValue)));
        assert(0x5555 == data->wValue);
        assert(sys_atomic_xor(NULL,&data->wValue,&wTest,&wOut,sizeof(data->wValue)));
        assert(0x5555 == wOut);
        assert(0 == data->wValue);

        data->lValue = 0;
        lTest = 0x55555555;
        assert(sys_atomic_xor(NULL,&data->lValue,&lTest,NULL,sizeof(data->lValue)));
        assert(0x55555555 == data->lValue);
        assert(sys_atomic_xor(NULL,&data->lValue,&lTest,&lOut,sizeof(data->lValue)));
        assert(0x55555555 == lOut);
        assert(0 == data->lValue);

        data->llValue = 0;
        llTest = 0x5555555555555555;
        assert(sys_atomic_xor(NULL,&data->llValue,&llTest,NULL,sizeof(data->llValue)));
        assert(0x5555555555555555 == data->llValue);
        assert(sys_atomic_xor(NULL,&data->llValue,&llTest,&llOut,sizeof(data->llValue)));
        assert(0x5555555555555555 == llOut);
        assert(0 == data->llValue);
	}

	// Atomic compare and swap integer
	{
        data->cValue = 0;
        cTest = 1;
        cComp = 1;
        cOut = 0;
        assert(cComp != data->cValue);
        assert(!sys_atomic_cas(NULL,&data->cValue,&cTest,&cComp,&cOut,sizeof(data->cValue)));
        assert(0 == cOut);
        assert(0 == data->cValue);
        cComp = 0;
        assert(cComp == data->cValue);
        assert(sys_atomic_cas(NULL,&data->cValue,&cTest,&cComp,&cOut,sizeof(data->cValue)));
        assert(0 == cOut);
        assert(cTest == data->cValue);

        data->wValue = 0;
        wTest = 1;
        wComp = 1;
        wOut = 0;
        assert(wComp != data->wValue);
        assert(!sys_atomic_cas(NULL,&data->wValue,&wTest,&wComp,&wOut,sizeof(data->wValue)));
        assert(0 == wOut);
        assert(0 == data->wValue);
        wComp = 0;
        assert(wComp == data->wValue);
        assert(sys_atomic_cas(NULL,&data->wValue,&wTest,&wComp,&wOut,sizeof(data->wValue)));
        assert(0 == wOut);
        assert(wTest == data->wValue);

        data->lValue = 0;
        lTest = 1;
        lComp = 1;
        lOut = 0;
        assert(lComp != data->lValue);
        assert(!sys_atomic_cas(NULL,&data->lValue,&lTest,&lComp,&lOut,sizeof(data->lValue)));
        assert(0 == lOut);
        assert(0 == data->lValue);
        lComp = 0;
        assert(lComp == data->lValue);
        assert(sys_atomic_cas(NULL,&data->lValue,&lTest,&lComp,&lOut,sizeof(data->lValue)));
        assert(0 == lOut);
        assert(lTest == data->lValue);

        data->llValue = 0;
        llTest = 1;
        llComp = 1;
        llOut = 0;
        assert(llComp != data->llValue);
        assert(!sys_atomic_cas(NULL,&data->llValue,&llTest,&llComp,&llOut,sizeof(data->llValue)));
        assert(0 == llOut);
        assert(0 == data->llValue);
        llComp = 0;
        assert(llComp == data->llValue);
        assert(sys_atomic_cas(NULL,&data->llValue,&llTest,&llComp,&llOut,sizeof(data->llValue)));
        assert(0 == llOut);
        assert(llTest == data->llValue);
    }

	// Atomic exchange integer
	{
        data->cValue = 0;
        cTest = 1;
        sys_atomic_xchg(NULL,&data->cValue,&cTest,NULL,sizeof(data->cValue));
        assert(1 == data->cValue);
        cTest = 0;
        sys_atomic_xchg(NULL,&data->cValue,&cTest,&cOut,sizeof(data->cValue));
        assert(1 == cOut);
        assert(0 == data->cValue);

        data->wValue = 0;
        wTest = 1;
        sys_atomic_xchg(NULL,&data->wValue,&wTest,NULL,sizeof(data->wValue));
        assert(1 == data->wValue);
        wTest = 0;
        sys_atomic_xchg(NULL,&data->wValue,&wTest,&wOut,sizeof(data->wValue));
        assert(1 == wOut);
        assert(0 == data->wValue);

        data->lValue = 0;
        lTest = 1;
        sys_atomic_xchg(NULL,&data->lValue,&lTest,NULL,sizeof(data->lValue));
        assert(1 == data->lValue);
        lTest = 0;
        sys_atomic_xchg(NULL,&data->lValue,&lTest,&lOut,sizeof(data->lValue));
        assert(1 == lOut);
        assert(0 == data->lValue);

        data->llValue = 0;
        llTest = 1;
        sys_atomic_xchg(NULL,&data->llValue,&llTest,NULL,sizeof(data->llValue));
        assert(1 == data->llValue);
        llTest = 0;
        sys_atomic_xchg(NULL,&data->llValue,&llTest,&llOut,sizeof(data->llValue));
        assert(1 == llOut);
        assert(0 == data->llValue);
	}

	// Atomic compare and swap pointer
	{
        char addr1[1] = "";
        char addr2[1] = "";
        uintptr_t pValue = 0;
        uintptr_t pTest = 0;
        uintptr_t pComp = 0;
        uintptr_t pOut = 0;

        pValue = (uintptr_t)addr1;
        pTest = (uintptr_t)addr2;
        pComp = (uintptr_t)addr2;
        pOut = (uintptr_t)addr2;
        assert(pComp != pValue);
        assert(!sys_atomic_cas_ptr(NULL,&pValue,pTest,pComp,&pOut));
        assert((uintptr_t)addr1 == pOut);
        assert((uintptr_t)addr1 == pValue);
        pComp = (uintptr_t)addr1;
        assert(pComp == pValue);
        assert(sys_atomic_cas_ptr(NULL,&pValue,pTest,pComp,&pOut));
        assert((uintptr_t)addr1 == pOut);
        assert(pTest == pValue);
        assert(sys_atomic_read_ptr(NULL,&pValue,&pOut));
        assert(pTest == pOut);
    }

	// Atomic exchange pointer
	{
        char addr1[1] = "";
        char addr2[1] = "";
        uintptr_t pValue = 0;
        uintptr_t pTest = 0;
        uintptr_t pOut = 0;

        pValue = (uintptr_t)addr1;
        pTest = (uintptr_t)addr2;
        sys_atomic_xchg_ptr(NULL,&pValue,pTest,NULL);
        assert(pTest == pValue);
        pTest = (uintptr_t)addr1;
        sys_atomic_xchg_ptr(NULL,&pValue,pTest,&pOut);
        assert((uintptr_t)addr2 == pOut);
        assert((uintptr_t)addr1 == pValue);
	}

    // Memory barrier
	{
        sys_atomic_sync(NULL);
	}

    // Atomic lock and release
	{
#if (__unix__)
        data->cValue = 0;
        assert(sys_atomic_lock(NULL,&data->cValue,&cOut,sizeof(uint8_t)));
        assert(0 == cOut);
        assert(1 == data->cValue);
        assert(sys_atomic_release(NULL,&data->cValue,sizeof(uint8_t)));
        assert(0 == data->cValue);

        data->wValue = 0;
        assert(sys_atomic_lock(NULL,&data->wValue,&wOut,sizeof(uint16_t)));
        assert(0 == wOut);
        assert(1 == data->wValue);
        assert(sys_atomic_release(NULL,&data->wValue,sizeof(uint16_t)));
        assert(0 == data->wValue);
#endif  // (__unix__)

        data->lValue = 0;
        assert(sys_atomic_lock(NULL,&data->lValue,&lOut,sizeof(uint32_t)));
        assert(0 == lOut);
        assert(1 == data->lValue);
        assert(sys_atomic_release(NULL,&data->lValue,sizeof(uint32_t)));
        assert(0 == data->lValue);

#if (__unix__)
        data->llValue = 0;
        assert(sys_atomic_lock(NULL,&data->llValue,&llOut,sizeof(uint64_t)));
        assert(0 == llOut);
        assert(1 == data->llValue);
        assert(sys_atomic_release(NULL,&data->llValue,sizeof(uint64_t)));
        assert(0 == data->llValue);
#endif  // (__unix__)
	}

    // Atomic bit operations
	{
        int orig = 0;

        data->lValue = 0;
        assert(sys_atomic_set(NULL,&data->lValue,0,NULL,sizeof(uint32_t)));
        assert(1 == data->lValue);
        assert(!sys_atomic_set(NULL,&data->lValue,0,NULL,sizeof(uint32_t)));
        assert(sys_atomic_set(NULL,&data->lValue,1,&orig,sizeof(uint32_t)));
        assert(0 == orig);
        assert(3 == data->lValue);
        assert(sys_atomic_clear(NULL,&data->lValue,0,NULL,sizeof(uint32_t)));
        assert(2 == data->lValue);
        assert(!sys_atomic_clear(NULL,&data->lValue,0,NULL,sizeof(uint32_t)));
        assert(sys_atomic_clear(NULL,&data->lValue,1,&orig,sizeof(uint32_t)));
        assert(1 == orig);
        assert(0 == data->lValue);
        assert(sys_atomic_change(NULL,&data->lValue,0,NULL,sizeof(uint32_t)));
        assert(1 == data->lValue);
        assert(sys_atomic_change(NULL,&data->lValue,1,&orig,sizeof(uint32_t)));
        assert(0 == orig);
        assert(3 == data->lValue);

        data->llValue = 0;
        assert(sys_atomic_set(NULL,&data->llValue,0,NULL,sizeof(uint32_t)));
        assert(1 == data->llValue);
        assert(sys_atomic_set(NULL,&data->llValue,1,&orig,sizeof(uint32_t)));
        assert(0 == orig);
        assert(3 == data->llValue);
        assert(sys_atomic_clear(NULL,&data->llValue,0,NULL,sizeof(uint32_t)));
        assert(2 == data->llValue);
        assert(sys_atomic_clear(NULL,&data->llValue,1,&orig,sizeof(uint32_t)));
        assert(1 == orig);
        assert(0 == data->llValue);
        assert(sys_atomic_change(NULL,&data->llValue,0,NULL,sizeof(uint32_t)));
        assert(1 == data->llValue);
        assert(sys_atomic_change(NULL,&data->llValue,1,&orig,sizeof(uint32_t)));
        assert(0 == orig);
        assert(3 == data->llValue);
	}

        switch(i) {
        case 0:
            free((void*)data);
            break;
        case 1:
            sys_shmem_detach(data);
            if(created) {
                sys_shmem_delete(id);
            }
            else {
                sys_shmem_release(id);
            }
            break;
        }

    }
