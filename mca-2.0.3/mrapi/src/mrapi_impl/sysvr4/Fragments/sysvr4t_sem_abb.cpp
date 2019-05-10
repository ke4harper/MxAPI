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

// Create semaphore
	{
	int id = 0;
#if !(__unix__)
	sem_set_t* ss = NULL;
#endif  // !(__unix__)
	int key = 0;
	int created = 0;
	assert(sys_file_key(NULL, 'd', &key));
	// Empty set
	assert(!sys_sem_create(key, 0, &id));
	// Single semaphore
	created = 0;
	if (!sys_sem_get(key, 1, &id)) { // race with other process?
		created = 1;
		assert(sys_sem_create(key, 1, &id));
	}
	assert(0 != id);
#if !(__unix__)
	ss = (sem_set_t*)id;
	assert(1 == ss->num_locks);
	assert(NULL != ss->sem);
	assert(NULL != ss->sem[0]);
#endif  // !(__unix__)
	if (created) {
		assert(sys_sem_delete(id)); // clean up Windows resources,
	}                               // possible Unix error is ignored
	else {
		assert(sys_sem_release(id));
	}
	// Semaphore set
	created = 0;
	if (!sys_sem_get(key + 2, 2, &id)) { // race with other process?
		created = 1;
		assert(sys_sem_create(key + 1, 2, &id));
	}
	assert(0 != id);
#if !(__unix__)
	ss = (sem_set_t*)id;
	assert(2 == ss->num_locks);
	assert(NULL != ss->sem);
	assert(NULL != ss->sem[0]);
	assert(NULL != ss->sem[1]);
#endif  // !(__unix__)
	if (created) {
		assert(sys_sem_delete(id)); // clean up Windows resources,
	}                               // possible Unix error is ignored
	else {
		assert(sys_sem_release(id));
	}
	}

	// Get semaphore
	{
		const int num_locks = 2;
		int id1 = 0;
		int id2 = 0;
		int key = 0;
		int created = 0;
		assert(sys_file_key(NULL, 'e', &key));
#if !(__unix__)
		int i = 0;
		sem_set_t* ss1 = NULL;
		sem_set_t* ss2 = NULL;
#endif  // !(__unix__)
		created = 0;
		if (!sys_sem_get(key, num_locks, &id1)) { // race with other process?
			created = 1;
			assert(sys_sem_create(key, num_locks, &id1));
		}
#if !(__unix__)
		ss1 = (sem_set_t*)id1;
#endif  // !(__unix__)
		assert(sys_sem_get(key, num_locks, &id2));
#if !(__unix__)
		ss2 = (sem_set_t*)id2;
		assert(ss1 != ss2);
		for (i = 0; i < num_locks; i++)
		{
			assert(NULL != ss2->sem[i]);
		}
#else
		assert(id2 == id1);
#endif  // (__unix__)
		if (created) {
			assert(sys_sem_delete(id1)); // clean up Windows resources,
		}
		else {
			assert(sys_sem_release(id1));
		}
		assert(sys_sem_release(id2));
	}

	// Duplicate semaphore
	{
		const int num_locks = 2;
		int id1 = 0;
		int id2 = 0;
		int key = 0;
		int created = 0;
		int pproc = 0;
		assert(sys_file_key(NULL, 'f', &key));
#if !(__unix__)
		int i = 0;
		sem_set_t* ss1 = NULL;
		sem_set_t* ss2 = NULL;
		pproc = (int)GetCurrentProcess();
#endif  // !(__unix__)
		created = 0;
		if (!sys_sem_get(key, num_locks, &id1)) { // race with other process?
			created = 1;
			assert(sys_sem_create(key, num_locks, &id1));
		}
#if !(__unix__)
		ss1 = (sem_set_t*)id1;
#endif  // !(__unix__)
		assert(sys_sem_duplicate(pproc, id1, &id2));
#if !(__unix__)
		ss2 = (sem_set_t*)id2;
		assert(ss1 != ss2);
		for (i = 0; i < num_locks; i++)
		{
			assert(NULL != ss2->sem[i]);
		}
#else
		assert(id2 == id1);
#endif  // (__unix__)
		if (created) {
			assert(sys_sem_delete(id1)); // clean up Windows resources,
		}
		else {
			assert(sys_sem_release(id1));
		}
		assert(sys_sem_release(id2));
	}

	// Lock and unlock semaphore
	{
		const int num_locks = 1;
		int id = 0;
		int key = 0;
		int created = 0;
		sem_ref_t semref;
		assert(sys_file_key(NULL, 'g', &key));
		created = 0;
		if (!sys_sem_get(key, num_locks, &id)) { // race with other process?
			created = 1;
			assert(sys_sem_create(key, num_locks, &id));
		}
		semref.set = id;
		semref.member = 0;
		assert(sys_sem_trylock(semref));
		assert(!sys_sem_trylock(semref));
		assert(sys_sem_unlock(semref));
		assert(sys_sem_lock(semref));
		assert(sys_sem_unlock(semref));
		if (created) {
			assert(sys_sem_delete(id)); // clean up Windows resources,
		}                               // Unix error is ignored
		else {
			assert(sys_sem_release(id));
		}
	}

	// Lock and unlock multiple semaphores
	{
		const int num_locks = 1;
		int created1 = 0;
		int created2 = 0;
		int key[2];
		sem_ref_t semref[2];
		assert(sys_file_key(NULL, 'o', &key[0]));
		assert(sys_file_key(NULL, 'p', &key[1]));
		created1 = 0;
		if (!sys_sem_get(key[0], num_locks, &semref[0].set)) { // race with other process?
			created1 = 1;
			assert(sys_sem_create(key[0], num_locks, &semref[0].set));
			semref[0].member = 0;
		}
		created2 = 0;
		if (!sys_sem_get(key[1], num_locks, &semref[1].set)) { // race with other process?
			created2 = 1;
			assert(sys_sem_create(key[1], num_locks, &semref[1].set));
			semref[1].member = 0;
		}
		void* obj = NULL;
		assert(sys_sem_trylock_multiple(&obj, semref, 2, TRUE));
		assert(NULL != obj);
		sys_sem_trylock_multiple_free(&obj);
		assert(NULL == obj);
		assert(!sys_sem_trylock_multiple(&obj, semref, 2, TRUE));
		sys_sem_trylock_multiple_free(&obj);
		assert(sys_sem_unlock_multiple(semref, 2));
		assert(sys_sem_lock_multiple(semref, 2, TRUE));
		assert(sys_sem_unlock_multiple(semref, 2));
		if (created1) {
			assert(sys_sem_delete(semref[0].set)); // clean up Windows resources,
		}                               // Unix error is ignored
		else {
			assert(sys_sem_release(semref[0].set));
		}
		if (created2) {
			assert(sys_sem_delete(semref[1].set)); // clean up Windows resources,
		}                               // Unix error is ignored
		else {
			assert(sys_sem_release(semref[1].set));
		}
	}
