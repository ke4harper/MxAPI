//
// Copyright(c) 2022, Karl Eric Harper
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// * Neither the name of Karl Eric Harper nor the names of its contributors may be used
//   to endorse or promote products derived from this software without specific
//   prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

/*
/// Runtime initialization

use std::{
    thread_local,
    env,
    process,
    mem::{
	MaybeUninit,
    },
    sync::{
	atomic::Ordering,
    },
    path::{
	Path,
    },
    ffi::{
	OsStr,
    },
};

use thread_id;
use atomig::{Atom};

use once_cell::{
    sync::{
	OnceCell,
    },
};

use crate::*;
use crate::MrapiStatusFlag::*;
use crate::internal::db::{
    MrapiDatabase,
    MrapiDomainData,
    MrapiDomainState,
    MrapiNodeState,
    MrapiProcessData,
    MrapiProcessState,
    MRAPI_MGR,
    MRAPI_SEM,
    access_database_pre,
    access_database_post,
};

//pub static RESOURCE_ROOT: LateStatic<MrapiResource<'static>> = LateStatic::new(); // root of the resource tree
//threadlocal!(ALARM_STRUCT: struct sigaction); // used for testing resource tree

thread_local! {
    pub static MRAPI_PID: OnceCell<MrapiUint32> = OnceCell::new();
    pub static MRAPI_PROC: OnceCell<MrapiUint32> = OnceCell::new();
    pub static MRAPI_TID: OnceCell<MrapiUint32> = OnceCell::new();
    pub static MRAPI_NINDEX: OnceCell<MrapiUint> = OnceCell::new();
    pub static MRAPI_DINDEX: OnceCell<MrapiUint> = OnceCell::new();
    pub static MRAPI_PINDEX: OnceCell<MrapiUint> = OnceCell::new();
    pub static MRAPI_NODE_ID: OnceCell<MrapiNode> = OnceCell::new();
    pub static MRAPI_DOMAIN_ID: OnceCell<MrapiDomain> = OnceCell::new();
}

// finer grained locks for these sections of the database.
thread_local! {
    pub static SEMS_SEM: OnceCell<Semaphore> = OnceCell::new(); // sems array
    pub static SHMEMS_SEM: OnceCell<Semaphore> = OnceCell::new(); // shmems array
    pub static REQUESTS_SEM: OnceCell<Semaphore> = OnceCell::new(); // requests array
    pub static RMEMS_SEM: OnceCell<Semaphore> = OnceCell::new(); // rmems array
}

// Keep copies of global objects for comparison
thread_local! {
    pub static REQUESTS_GLOBAL: OnceCell<Semaphore> = OnceCell::new();
    pub static SEMS_GLOBAL: OnceCell<Semaphore> = OnceCell::new();
    pub static SHMEMS_GLOBAL: OnceCell<Semaphore> = OnceCell::new();
    pub static RMEMS_GLOBAL: OnceCell<Semaphore> = OnceCell::new();
}

// Tell the system whether or not to use the finer-grained locking
pub const use_global_only: bool = false;

/// Returns the initialized characteristics of this thread
#[allow(dead_code)]
fn whoami() -> Result<(MrapiNode, MrapiUint, MrapiDomain, MrapiUint), MrapiStatusFlag> {
    let mut initialized: bool;
    let mut result: Result<(MrapiNode, MrapiUint, MrapiDomain, MrapiUint), MrapiStatusFlag>;
    match MRAPI_MGR.get() {
	None => {
	    initialized = false;
	    result = Err(MrapiErrDbNotInitialized);
	},
	Some(_) => {
	    initialized = true;
	},
    };

    if !initialized {
	return result;
    }

    MRAPI_PID.with(|pid| {
	match pid.get() {
	    None => {
		initialized = false;
		result = Err(MrapiErrProcessNotRegistered);
	    },
	    Some(_) => {
		initialized = true;
	    },
	}
    });

    if !initialized {
	return result;
    }

    Ok((
	MRAPI_NODE_ID.with(|id| {
	    match id.get() {
		None => 0,
		Some(v) => *v,
	    }
	}),
	MRAPI_NINDEX.with(|index| {
	    match index.get() {
		None => 0,
		Some(v) => *v,
	    }
	}),
	MRAPI_DOMAIN_ID.with(|id| {
	    match id.get() {
		None => 0,
		Some(v) => *v,
	    }
	}),
	MRAPI_DINDEX.with(|index| {
	    match index.get() {
		None => 0,
		Some(v) => *v,
	    }
	}),
    ))
}

/// Checks if the given domain_id/node_id is already assoc w/ this pid/tid
#[allow(dead_code)]
fn initialized() -> bool {
    match whoami() {
	Ok(_) => true,
	Err(_) => false,
    }
}
fn finalize_node_locked(d: MrapiUint, n: MrapiUint) {
    let mrapi_db = MrapiDatabase::global_db();
    let dpacked = &mrapi_db.domains[d].state.load(Ordering::Relaxed);
    let dstate: MrapiDomainState = Atom::unpack(*dpacked);
    let npacked = &mrapi_db.domains[d].nodes[n].state.load(Ordering::Relaxed);
    let mut nstate: MrapiNodeState = Atom::unpack(*npacked);
    let domain_id = dstate.domain_id();
    let node_num = nstate.node_num();
    
    mrapi_dprintf!(2, "mrapi::internal::lifecycle::finalize_node_locked dindex={} nindex={} domain={} node={}", d, n, domain_id, node_num);
    
    // Mark the node as finalized
    nstate.set_valid(MRAPI_FALSE);
    nstate.set_allocated(MRAPI_FALSE);
    &mrapi_db.domains[d].nodes[n].state.store(Atom::pack(nstate), Ordering::Relaxed);
    // Rundown the node's process association
    let p = mrapi_db.domains[d].nodes[n].proc_num;
    let ppacked = &mrapi_db.processes[p].state.load(Ordering::Relaxed);
    let pstate: MrapiProcessState = Atom::unpack(*ppacked);
    if pstate.valid() {
	let num_nodes = mrapi_db.processes[p].num_nodes.fetch_sub(1, Ordering::Relaxed);
	if 0 >= num_nodes {
	    // Last node in this process, remove process references in shared memory
	    for s in 0..MRAPI_MAX_SHMEMS {
		if mrapi_db.shmems[s].valid {
		    &mrapi_db.shmems[s].set_process(p, 0);
		}
	    }
	    unsafe { mrapi_db.processes[p] = MaybeUninit::zeroed().assume_init(); };
	}
    }
    unsafe { mrapi_db.domains[d].nodes[n] = MaybeUninit::zeroed().assume_init(); };
    &mrapi_db.domains[d].num_nodes.fetch_sub(1, Ordering::Relaxed);
    
    // Decrement the shmem reference count if necessary
    for shmem in 0..MRAPI_MAX_SHMEMS {
	if mrapi_db.shmems[shmem].valid == MRAPI_TRUE {
	    if mrapi_db.domains[d].nodes[n].shmems[shmem] == 1 {
		// If this node was a user of this shm, decrement the ref count
		mrapi_db.shmems[shmem].refs -= 1;
	    }
	    // If the reference count is 0, free the shared memory resource
	    if mrapi_db.shmems[shmem].refs == 0 {
		drop(&mrapi_db.shmems[shmem].mem[p]);
	    }
	}
    }

    // Decrement the sem reference count if necessary
    for sem in 0..MRAPI_MAX_SEMS {
	if mrapi_db.sems[sem].valid == MRAPI_TRUE {
	    if mrapi_db.domains[d].nodes[n].sems[sem] == 1 {
		mrapi_db.domains[d].nodes[n].sems[sem] = 0;
		// If this node was a user of this sem, decrement the ref count
		if mrapi_db.sems[sem].refs.fetch_sub(1, Ordering::Relaxed) <= 0 {
		    // If the reference count is 0 free the resource
		    mrapi_db.sems[sem].valid = MRAPI_FALSE;
		}
	    }
	}
    }
}

fn free_resources(panic: MrapiBoolean) {
    let mut last_node_standing = MRAPI_TRUE;
    let mut last_node_standing_for_this_process = MRAPI_TRUE;
    let pid = process::id();
    let semref = MrapiSemRef::new(MrapiDatabase::global_sem(), 0, MRAPI_FALSE);

    // Try to lock the database
    let locked = access_database_pre(&semref, MRAPI_FALSE);
    mrapi_dprintf!(1, "mrapi::internal::lifecycle::free_resources panic: {} freeing any existing resources", panic);

    match MRAPI_MGR.get() {
	None => { },
	Some(_) => {
	    // Finalize this node
	    match whoami() {
		Ok((node, n, domain_num, d)) => {
		    finalize_node_locked(d, n);
		},
		Err(_) => { },
	    }

	    // If we are in panic mode, then forcefully finalize all other nodes that belong to this process
	    if panic {
		let mrapi_db = MrapiDatabase::global_db();
		for d in 0..MRAPI_MAX_DOMAINS {
		    for n in 0..MRAPI_MAX_NODES {
			let npacked = &mrapi_db.domains[d].nodes[n].state.load(Ordering::Relaxed);
			let nstate: MrapiNodeState = Atom::unpack(*npacked);
			if nstate.valid() == MRAPI_TRUE {
			    let p = *&mrapi_db.domains[d].nodes[n].proc_num as usize;
			    let ppacked = &mrapi_db.processes[p].state.load(Ordering::Relaxed);
			    let pstate: MrapiProcessState = Atom::unpack(*ppacked);
			    if pstate.pid() == pid {
				finalize_node_locked(d, n);
			    }
			}
		    }
		}
		for p in 0..MRAPI_MAX_PROCESSES {
		    let ppacked = &mrapi_db.processes[p].state.load(Ordering::Relaxed);
		    let pstate: MrapiProcessState = Atom::unpack(*ppacked);
		    if pstate.valid() == MRAPI_TRUE &&
			pstate.pid() == pid {
			    let mut process = &mut mrapi_db.processes[p];
			    process.clear();
			    break;
			}
		}
	    }
	}
    }
}

/*
{
	mrapi_boolean_t rc = MRAPI_TRUE;
	uint32_t d, n, p;
	mrapi_domain_t domain_num;
	mrapi_database* mrapi_db_local = NULL;
	mrapi_node_t node;
#if (__unix__)
	pid_t pid = getpid();
#else
	pid_t pid = (pid_t)GetCurrentProcessId();
#endif  // !(__unix__)
	mrapi_boolean_t last_man_standing = MRAPI_TRUE;
	mrapi_boolean_t last_man_standing_for_this_process = MRAPI_TRUE;
	mrapi_boolean_t locked;

	// try to lock the database
	mrapi_impl_sem_ref_t ref = { semid, 0, MRAPI_FALSE };
	locked = mrapi_impl_access_database_pre(ref, MRAPI_FALSE);
	mrapi_dprintf(1, "mrapi_impl_free_resources (panic=%d): freeing any existing resources in the database mrapi_db=%p semid=%x shmemid=%x\n",
		panic, mrapi_db, semid, shmemid);

	if (mrapi_db) {

		// finalize this node
		if (mrapi_impl_whoami(&node, &n, &domain_num, &d)) {
			mrapi_impl_finalize_node_locked(d, n);
		}

		// if we are in panic mode, then forcefully finalize all other nodes that belong to this process
		if (panic) {
			for (d = 0; d < MRAPI_MAX_DOMAINS; d++) {
				for (n = 0; n < MRAPI_MAX_NODES; n++) {
					mrapi_node_state nstate;
					mrapi_assert(sys_atomic_read(NULL, &mrapi_db->domains[d].nodes[n].state, &nstate, sizeof(mrapi_db->domains[d].nodes[n].state)));
					if (nstate.data.valid == MRAPI_TRUE) {
						mrapi_uint_t p = mrapi_db->domains[d].nodes[n].proc_num;
						mrapi_process_state pstate;
						mrapi_assert(sys_atomic_read(NULL, &mrapi_db->processes[p].state, &pstate, sizeof(mrapi_db->processes[p].state)));
						if (pstate.data.pid == pid) {
							mrapi_impl_finalize_node_locked(d, n);
						}
					}
				}
			}
			for (p = 0; p < MRAPI_MAX_PROCESSES; p++) {
				mrapi_process_state pstate;
				mrapi_assert(sys_atomic_read(NULL, &mrapi_db->processes[p].state, &pstate, sizeof(mrapi_db->processes[p].state)));
				if ((pstate.data.valid == MRAPI_TRUE) &&
					(pstate.data.pid == pid)) {
#if !(__unix)
					if (NULL != mrapi_db->processes[p].hAtomicEvt) {
						CloseHandle(mrapi_db->processes[p].hAtomicEvt);
					}
#endif  // !(__unix)
					memset(&mrapi_db->processes[p], 0, sizeof(mrapi_process_data));
					break;
				}
			}
		}

		// see if there are any valid nodes left in the system and for this process
		for (d = 0; d < MRAPI_MAX_DOMAINS; d++) {
			for (n = 0; n < MRAPI_MAX_NODES; n++) {
				mrapi_node_state nstate;
				mrapi_assert(sys_atomic_read(NULL, &mrapi_db->domains[d].nodes[n].state, &nstate, sizeof(mrapi_db->domains[d].nodes[n].state)));
				if (nstate.data.valid == MRAPI_TRUE) {
					mrapi_process_state pstate;
					p = mrapi_db->domains[d].nodes[n].proc_num;
					mrapi_assert(sys_atomic_read(NULL, &mrapi_db->processes[p].state, &pstate, sizeof(mrapi_db->processes[p].state)));
					last_man_standing = MRAPI_FALSE;
					if (pstate.data.pid == pid) {
						last_man_standing_for_this_process = MRAPI_FALSE;
					}
				}
			}
		}

		if (panic) {
			mrapi_assert(last_man_standing_for_this_process);
		}

		// if there are no other valid nodes in the whole system, then free the sems
		if (last_man_standing) {
			mrapi_dprintf(1, "mrapi_impl_free_resources: freeing mrapi internal semaphore and shared memory\n");

			// free the mrapi internal semaphores
			if (sems_semid != sems_global) {
				rc = sys_sem_delete(sems_semid);
				sems_semid = -1;
				if (!rc) {
					fprintf(stderr, "mrapi_impl_free_resources: ERROR: sys_sem_delete (mrapi_db) failed\n");
				}
			}

			if (shmems_semid != shmems_global) {
				rc = sys_sem_delete(shmems_semid);
				shmems_semid = -1;
				if (!rc) {
					fprintf(stderr, "mrapi_impl_free_resources: ERROR: sys_sem_delete (mrapi_db) failed\n");
				}
			}
			if (rmems_semid != rmems_global) {
				rc = sys_sem_delete(rmems_semid);
				rmems_semid = -1;
				if (!rc) {
					fprintf(stderr, "mrapi_impl_free_resources: ERROR: sys_sem_delete (mrapi_db) failed\n");
				}
			}
			if (requests_semid != requests_global) {
				rc = sys_sem_delete(requests_semid);
				requests_semid = -1;
				if (!rc) {
					fprintf(stderr, "mrapi_impl_free_resources: ERROR: sys_sem_delete (mrapi_db) failed\n");
				}
			}
		}

		// if there are no other valid nodes for this process, then detach from shared memory
		if (last_man_standing_for_this_process) {
			mrapi_status_t status = 0;
			mrapi_atomic_op op = { MRAPI_ATOM_CLOSEPROC, 0 };
			// Signal remote processes to unlink this process
			mrapi_impl_atomic_forward(0, &op, &status);

			memset(&mrapi_db->processes[mrapi_pindex], 0, sizeof(mrapi_process_data));

			// detach from the mrapi internal shared memory
			mrapi_dprintf(1, "mrapi_impl_free_resources: detaching from mrapi internal shared memory\n");

			mrapi_db_local = mrapi_db;
			sys_atomic_xchg_ptr(NULL, (uintptr_t*)&mrapi_db, (uintptr_t)NULL, (uintptr_t*)NULL);
			rc = sys_shmem_detach(mrapi_db_local);
			if (!rc) {
				fprintf(stderr, "mrapi_impl_free_resources: ERROR: sys_shmem detach (mrapi_db) failed\n");
			}
		}

		// if there are no other valid nodes in the whole system, then free the shared memory
		if (last_man_standing) {
			// free the mrapi internal shared memory
			rc = sys_shmem_delete(shmemid);
			if (!rc) {
				fprintf(stderr, "mrapi_impl_free_resources: ERROR: sys_shmem_delete (mrapi_db) failed\n");
			}
			mrapi_db = NULL;
			shmemid = -1;
		}

		// if we locked the database and didn't delete it, then we need to unlock it
		if (locked) {
			if (!last_man_standing)
			{
				// unlock the database
				mrapi_impl_sem_ref_t ref = { semid, 0 };
				mrapi_assert(mrapi_impl_access_database_post(ref));
			}
		}
		if (last_man_standing) {
			// free the global semaphore last
			rc = sys_sem_delete(semid);
			semid = -1;
			if (!rc) {
				fprintf(stderr, "mrapi_impl_free_resources: ERROR: sys_sem_delete (mrapi_db) failed\n");
			}
		}
	}

	return last_man_standing;
}
 */

/// Create or get the semaphore corresponding to the key
fn create_sys_semaphore(num_locks: usize, key: u32, lock: MrapiBoolean) -> Option<Semaphore>
{
    let max_tries: u32 = 0xffffffff;
    let trycount: u32 = 0;

    while trycount < max_tries {
	trycount += 1;
	let sem = match sem_create(key, num_locks) {
	    Some(v) => v,
	    None => {
		match sem_get(key, num_locks) {
		    Some(v) => v,
		    None => Semaphore::default(),
		}
	    },
	};

	if sem != Semaphore::default() {
	    if lock {
		let sr = MrapiSemRef::new(&sem, 0, false);
		while trycount < max_tries {
		    match sr.trylock() {
			Ok(v) => {
			    if v { return Some(sem); }
			},
			Err(_) => { },
		    }
		    sysvr4::os_yield();
		}
	    }
	}
    }
    
    None
}

/// Initializes the MRAPI internal layer (sets up the database and semaphore)
///
/// # Arguments
///
/// domain_id - collection of nodes that share resources
/// node_id - task that synchronizes with other nodes in a domain
///
/// # Errors
///
/// MrapiDbNotInitialized
/// MrapiNodeInitfailed
/// MrapiAtomOpNoforward
#[allow(unused_variables)]
pub fn initialize(domain_id: MrapiDomain, node_id: MrapiNode) -> Result<MrapiStatus, MrapiStatusFlag> {
    static use_uid: MrapiBoolean = MRAPI_TRUE;

    // associate this node w/ a pid,tid pair so that we can recognize the caller on later calls

    mrapi_dprintf!(1, "mrapi::internal::lifecycle::initialize ({},{});", domain_id, node_id);

    if initialized() {
	return Err(MrapiErrNodeInitfailed);
    };

    // Get process name
    let proc_name = env::args().next()
	.as_ref()
	.map(Path::new)
	.and_then(Path::file_name)
	.and_then(OsStr::to_str);
    let buff = proc_name.unwrap().to_owned() + "_mrapi";

    let mut key: u32;
    let mut db_key: u32;
    let mut sems_key: u32;
    let mut shmems_key: u32;
    let mut rmems_key: u32;
    let mut requests_key: u32;

    if use_uid {
	key = common::crc::crc32_compute_buf(0, &buff);
	db_key = common::crc::crc32_compute_buf(key, &(buff + "_db"));
	sems_key = common::crc::crc32_compute_buf(key, &(buff + "_sems"));
	shmems_key = common::crc::crc32_compute_buf(key, &(buff + "_shmems"));
	rmems_key = common::crc::crc32_compute_buf(key, &(buff + "_rmems"));
	requests_key = common::crc::crc32_compute_buf(key, &(buff + "_requests"));
    }
    else {
	key = match os_file_key("", 'z' as u32) {
	    Some(v) => v,
	    None => {
		mrapi_dprintf!(1, "MRAPI ERROR: Invalid file key");
		0
	    },
	};
	
	db_key = key + 10;
	sems_key = key + 20;
	shmems_key = key + 30;
	rmems_key = key + 40;
	requests_key = key + 50;
    }

    // 1) setup the global database
    // get/create the shared memory database
    MrapiDatabase::initialize_db(domain_id, node_id, db_key);
    
    // 2) create or get the semaphore and lock it
    // we loop here and inside of create_sys_semaphore because of the following race condition:
    // initialize                  finalize
    // 1: create/get sem           1: lock sem
    // 2: lock sem                 2: check db: any valid nodes?
    // 3: setup db & add self      3a:  no -> delete db & delete sem
    // 4: unlock sem               3b:  yes-> unlock sem
    //
    // finalize-1 can occur between initialize-1 and initialize-2 which will cause initialize-2
    // to fail because the semaphore no longer exists.

    let sem_local = match create_sys_semaphore(1, key, MRAPI_TRUE) {
	None => {
	    mrapi_dprintf!(1, "MRAPI ERROR: Unable to get the semaphore key: {}", key);
	    return Err(MrapiErrNodeInitfailed);
	},
	Some(v) => v,
    };

    mrapi_dprintf!(1, "mrapi_impl_initialize lock acquired, now adding node to database");

    // At this point we've managed to acquire and lock the semaphore ...
    // NOTE: with use_global_only it's important to write to the globals only while
    // we have the semaphore otherwise we introduce race conditions.  This
    // is why we are using the local variable id until everything is set up.

    // set the global semaphore reference
    MrapiDatabase::initialize_sem(sem_local);
	
    // get or create our finer grained locks
    // in addition to a lock on the sems array, every lock (rwl,sem,mutex) has it's own
    // database semaphore, this allows us to access different locks in parallel

    let sems_sem = create_sys_semaphore(MRAPI_MAX_SEMS + 1, sems_key, MRAPI_FALSE);
    if use_global_only || sems_sem.is_none() {
	SEMS_GLOBAL.with(|sem| { sem.set(MrapiDatabase::global_sem().clone()); });
	SEMS_SEM.with(|sem| { sem.set(MrapiDatabase::global_sem().clone()); });
    }
    else {
	SEMS_SEM.with(|sem| { sem.set(sems_sem.unwrap()); });
    }
    
    let shmems_sem = create_sys_semaphore(1, shmems_key, MRAPI_FALSE);
    if shmems_sem.is_none() {
	SHMEMS_GLOBAL.with(|sem| { sem.set(MrapiDatabase::global_sem().clone()); });
	SHMEMS_SEM.with(|sem| { sem.set(MrapiDatabase::global_sem().clone()); });
    }
    else {
	SHMEMS_SEM.with(|sem| { sem.set(shmems_sem.unwrap()); });
    }
    
    let rmems_sem = create_sys_semaphore(1, rmems_key, MRAPI_FALSE);
    if rmems_sem.is_none() {
	RMEMS_GLOBAL.with(|sem| { sem.set(MrapiDatabase::global_sem().clone()); });
	RMEMS_SEM.with(|sem| { sem.set(MrapiDatabase::global_sem().clone()); });
    }
    else {
	RMEMS_SEM.with(|sem| { sem.set(rmems_sem.unwrap()); });
    }
    
    let requests_sem = create_sys_semaphore(1, requests_key, MRAPI_FALSE);
    if requests_sem.is_none() {
	REQUESTS_GLOBAL.with(|sem| { sem.set(MrapiDatabase::global_sem().clone()); });
	REQUESTS_SEM.with(|sem| { sem.set(MrapiDatabase::global_sem().clone()); });
    }
    else {
	REQUESTS_SEM.with(|sem| { sem.set(rmems_sem.unwrap()); });
    }

    // Get our identity
    let pid = process::id();
    let tid = thread_id::get() as MrapiUint32;
    MRAPI_PID.with(|id| { id.set(pid); });
    MRAPI_PROC.with(|id| { id.set(pid); });
    MRAPI_TID.with(|id| { id.set(tid); });

    // Seed random number generator
    os_srand(tid);

    let mrapi_db = MrapiDatabase::global_db();

    // 3) Add the process/node/domain to the database
    
    let mut d: usize = 0;
    let mut n: usize = 0;
    let mut p: usize = 0;

    // First see if this domain already exists
    for d in 0..MRAPI_MAX_DOMAINS {
	let packed = &mrapi_db.domains[d].state.load(Ordering::Relaxed);
	let dstate: MrapiDomainState = Atom::unpack(*packed);
	if dstate.domain_id() == domain_id as MrapiUint32 {
	    break;
	}
    }

    if d == MRAPI_MAX_DOMAINS {
	// Find first available entry
	for d in 0..MRAPI_MAX_DOMAINS {
	    let packed = &mrapi_db.domains[d].state.load(Ordering::Relaxed);
	    let mut oldstate: MrapiDomainState = Atom::unpack(*packed);
	    let mut newstate = oldstate;
	    oldstate.set_allocated(MRAPI_FALSE);
	    newstate.set_domain_id(domain_id as MrapiUint32);
	    newstate.set_allocated(MRAPI_TRUE);
	    match &mrapi_db.domains[d].state.compare_exchange(
		Atom::pack(oldstate), Atom::pack(newstate), Ordering::Acquire, Ordering::Relaxed) {
		Ok(_) => {
		    break;
		},
		Err(_) => continue,
	    }
	}
    }

    if d != MRAPI_MAX_DOMAINS {
	// now find an available node index...
	for n in 0..MRAPI_MAX_NODES {
 	    let packed = &mrapi_db.domains[d].nodes[n].state.load(Ordering::Relaxed);
	    let state: MrapiNodeState = Atom::unpack(*packed);
	    // Even though initialized() is checked by mrapi, we have to check again here because
	    // initialized() and initalize() are  not atomic at the top layer
	    if state.allocated() && state.node_num() == node_id as MrapiUint32 {
		// this node already exists for this domain
		mrapi_dprintf!(1, "This node ({}) already exists for this domain ({})", node_id, domain_id);
		break;
	    }
	}

	if n == MRAPI_MAX_NODES {
	    // it didn't exist so find the first available entry
	    for n in 0..MRAPI_MAX_NODES {
 		let packed = &mrapi_db.domains[d].nodes[n].state.load(Ordering::Relaxed);
		let mut oldstate: MrapiNodeState = Atom::unpack(*packed);
		let mut newstate = oldstate;
		oldstate.set_allocated(MRAPI_FALSE);
		newstate.set_node_num(node_id as MrapiUint32);
		newstate.set_allocated(MRAPI_TRUE);
		match &mrapi_db.domains[d].nodes[n].state.compare_exchange(
		    Atom::pack(oldstate), Atom::pack(newstate), Ordering::Acquire, Ordering::Relaxed) {
		    Ok(_) => {
			break;
		    },
		    Err(_) => continue,
		}
	    }
	    if n != MRAPI_MAX_NODES {
		// See if this process exists
		for p in 0..MRAPI_MAX_PROCESSES {
 		    let packed = &mrapi_db.processes[p].state.load(Ordering::Relaxed);
		    let pstate: MrapiProcessState = Atom::unpack(*packed);
		    if pstate.pid() == pid {
			break;
		    }
		}
		if p == MRAPI_MAX_PROCESSES {
		    // It didn't exist so find the first available entry
		    for p in 0..MRAPI_MAX_PROCESSES {
 			let packed = &mrapi_db.processes[p].state.load(Ordering::Relaxed);
			let mut oldstate: MrapiProcessState = Atom::unpack(*packed);
			let mut newstate = oldstate;
			oldstate.set_allocated(MRAPI_FALSE);
			newstate.set_pid(pid);
			newstate.set_allocated(MRAPI_TRUE);
			match &mrapi_db.processes[p].state.compare_exchange(
			    Atom::pack(oldstate), Atom::pack(newstate), Ordering::Acquire, Ordering::Relaxed) {
			    Ok(_) => {
				break;
			    },
			    Err(_) => continue,
			}
		    }
		}
	    }
	}
    }
    else {
	// We did not find an available domain index
	mrapi_dprintf!(1, "You have hit MRAPI_MAX_DOMAINS, either use less domains or reconfigure with more domains");
	return Err(MrapiErrNodeInitfailed);
    }
    if n == MRAPI_MAX_NODES {
	// We did not find an available node index
	mrapi_dprintf!(1, "You have hit MRAPI_MAX_NODES, either use less nodes or reconfigure with more nodes");
	return Err(MrapiErrNodeInitfailed);
    }
    if p == MRAPI_MAX_PROCESSES {
	// We did not find an available process index
	mrapi_dprintf!(1, "You have hit MRAPI_MAX_PROCESSES, either use less processes or reconfigure with more processes");
	return Err(MrapiErrNodeInitfailed);
    }
    else {
	MRAPI_PINDEX.with(|index| { index.set(p); });
    }

    Err(MrapiErrNodeInitfailed)
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    fn startup() {
	let domain_id: MrapiDomain = 1;
	let node_id: MrapiNode = 1;
	_ = match initialize(domain_id, node_id) {
	    Ok(_) => {} | Err(_) => {},
	};
    }
}
*/
