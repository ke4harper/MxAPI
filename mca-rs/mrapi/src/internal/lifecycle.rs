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

/// Runtime initialization

use std::{
    thread_local,
    process,
    mem::{
	size_of,
	MaybeUninit,
    },
    sync::{
	Mutex,
	atomic::Ordering,
    },
    path::{
	Path,
    },
    num::{NonZeroU8},
    fs::{File},
};

use late_static::{LateStatic};
use heliograph::{
    Exclusive,
    Mode,
    Key,
    Semaphore,
};
use shared_memory::{
    ShmemConf,
    Shmem,
};
use fragile::{Fragile};
use thread_id;
use atomig::{Atom};

use crate::*;
use crate::MrapiStatusFlag::*;
use crate::sysvr4::{
    os::{
	sys_os_yield,
	sys_os_srand,
    },
    sem::{
	sys_sem_unlock,
	sys_sem_delete,
    },
};
use crate::internal::db::{
    MrapiDatabase,
    MrapiDomainData,
    MrapiDomainState,
    MrapiNodeState,
    MrapiProcessData,
    MrapiProcessState,
    MRAPI_DB,
    SHMEMID,
    SEMID,
    access_database_pre,
    access_database_post,
};

static _DB_MUTEX: Mutex<u64> = Mutex::new(0);

//pub static RESOURCE_ROOT: LateStatic<MrapiResource<'static>> = LateStatic::new(); // root of the resource tree
//threadlocal!(ALARM_STRUCT: struct sigaction); // used for testing resource tree
#[thread_local]
pub static MRAPI_PID: LateStatic<MrapiUint32> = LateStatic::new();
#[thread_local]
pub static MRAPI_PROC: LateStatic<MrapiUint32> = LateStatic::new();
#[thread_local]
pub static MRAPI_TID: LateStatic<MrapiUint32> = LateStatic::new();
#[thread_local]
pub static MRAPI_NINDEX: LateStatic<MrapiUint> = LateStatic::new();
#[thread_local]
pub static MRAPI_DINDEX: LateStatic<MrapiUint> = LateStatic::new();
#[thread_local]
pub static MRAPI_PINDEX: LateStatic<MrapiUint> = LateStatic::new();
#[thread_local]
pub static MRAPI_NODE_ID: LateStatic<MrapiNode> = LateStatic::new();
#[thread_local]
pub static MRAPI_DOMAIN_ID: LateStatic<MrapiDomain> = LateStatic::new();
/*
// finer grained locks for these sections of the database.
pub static SEMS_SEMID: LateStatic<Semaphore> = LateStatic::new(); // sems array
pub static SHMEMS_SEMID: LateStatic<Semaphore> = LateStatic::new(); // shmems array
pub static REQUESTS_SEMID: LateStatic<Semaphore> = LateStatic::new(); // requests array
pub static RMEMS_SEMID: LateStatic<Semaphore> = LateStatic::new(); // rmems array
// Keep copies of global handles for comparison
pub static REQUESTS_GLOBAL: LateStatic<MrapiUint> = LateStatic::new();
pub static SEMS_GLOBAL: LateStatic<Semaphore> = LateStatic::new();
pub static SHMEMS_GLOBAL: LateStatic<MrapiUint> = LateStatic::new();
pub static RMEMS_GLOBAL: LateStatic<MrapiUint> = LateStatic::new();
 */

/// Gets the pid,tid pair for the caller and then looks up the corresponding node and domain info in our database.
#[allow(dead_code)]
fn whoami() -> Result<(MrapiNode, MrapiUint, MrapiDomain, MrapiUint), MrapiStatusFlag> {
    if unsafe { !LateStatic::has_value(&MRAPI_DB) } {
	return Err(MrapiErrDbNotInitialized);
    }
    if *MRAPI_PID == !1 {
	return Err(MrapiErrProcessNotRegistered);
    }

    Ok((*MRAPI_NODE_ID, *MRAPI_NINDEX, *MRAPI_DOMAIN_ID, *MRAPI_DINDEX))
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
    let dpacked = &MRAPI_DB.domains[d].state.load(Ordering::Relaxed);
    let dstate: MrapiDomainState = Atom::unpack(*dpacked);
    let npacked = &MRAPI_DB.domains[d].nodes[n].state.load(Ordering::Relaxed);
    let mut nstate: MrapiNodeState = Atom::unpack(*npacked);
    let domain_id = dstate.domain_id();
    let node_num = nstate.node_num();
    mrapi_dprintf!(2, "mrapi::internal::lifecycle::finalize_node_locked dindex={} nindex={} domain={} node={}", d, n, domain_id, node_num);
    // Mark the node as finalized
    nstate.set_valid(MRAPI_FALSE);
    nstate.set_allocated(MRAPI_FALSE);
    &MRAPI_DB.domains[d].nodes[n].state.store(Atom::pack(nstate), Ordering::Relaxed);
    // Rundown the node's process association
    let p = MRAPI_DB.domains[d].nodes[n].proc_num;
    let ppacked = &MRAPI_DB.processes[p].state.load(Ordering::Relaxed);
    let pstate: MrapiProcessState = Atom::unpack(*ppacked);
    if pstate.valid() {
	let num_nodes = MRAPI_DB.processes[p].num_nodes.fetch_sub(1, Ordering::Relaxed);
	if 0 >= num_nodes {
	    // Last node in this process, remove process references in shared memory
	    for s in 0..MRAPI_MAX_SHMEMS {
		if MRAPI_DB.shmems[s].valid {
		    &MRAPI_DB.shmem(s).set_process(p, 0);
		}
	    }
	    unsafe { MRAPI_DB.processes[p] = MaybeUninit::zeroed().assume_init(); };
	}
    }
    unsafe { MRAPI_DB.domains[d].nodes[n] = MaybeUninit::zeroed().assume_init(); };
    &MRAPI_DB.domains[d].num_nodes.fetch_sub(1, Ordering::Relaxed);
    
    // Decrement the shmem reference count if necessary
    for shmem in 0..MRAPI_MAX_SHMEMS {
	if MRAPI_DB.shmems[shmem].valid == MRAPI_TRUE {
	    if MRAPI_DB.domains[d].nodes[n].shmems[shmem] == 1 {
		// If this node was a user of this shm, decrement the ref count
		MRAPI_DB.shmems[shmem].refs -= 1;
	    }
	    // If the reference count is 0, free the shared memory resource
	    if MRAPI_DB.shmems[shmem].refs == 0 {
		drop(&MRAPI_DB.shmems[shmem].mem[p]);
	    }
	}
    }

    // Decrement the sem reference count if necessary
    for sem in 0..MRAPI_MAX_SEMS {
	if MRAPI_DB.sems[sem].valid == MRAPI_TRUE {
	    if MRAPI_DB.domains[d].nodes[n].sems[sem] == 1 {
		MRAPI_DB.domains[d].nodes[n].sems[sem] = 0;
		// If this node was a user of this sem, decrement the ref count
		if MRAPI_DB.sems[sem].refs.fetch_sub(1, Ordering::Relaxed) <= 0 {
		    // If the reference count is 0 free the resource
		    MRAPI_DB.sems[sem].valid = MRAPI_FALSE;
		}
	    }
	}
    }
}

fn free_resources(panic: MrapiBoolean) {
    let mut last_node_standing = MRAPI_TRUE;
    let mut last_node_standing_for_this_process = MRAPI_TRUE;
    let pid = process::id();
    let semref = MrapiSemRef {
	set: SEMID.get(),
	member: 0,
	spinlock_guard: MRAPI_FALSE,
    };
    let locked = access_database_pre(&semref, MRAPI_FALSE);
    mrapi_dprintf!(1, "mrapi::internal::lifecycle::free_resources panic: {} freeing any existing resources", panic);
    if unsafe { LateStatic::has_value(&MRAPI_DB) } {
	// Finalize this node
	match whoami() {
	    Ok((node, n, domain_num, d)) => {
		finalize_node_locked(d, n);
	    },
	    Err(_) => {},
	}

	// If we are in panic mode, then forcefully finalize all other nodes that belong to this process
	if panic {
	    for d in 0..MRAPI_MAX_DOMAINS {
		for n in 0..MRAPI_MAX_NODES {
		    let npacked = &MRAPI_DB.domains[d].nodes[n].state.load(Ordering::Relaxed);
		    let nstate: MrapiNodeState = Atom::unpack(*npacked);
		    if nstate.valid() == MRAPI_TRUE {
			let p = *&MRAPI_DB.domains[d].nodes[n].proc_num as usize;
			let ppacked = &MRAPI_DB.processes[p].state.load(Ordering::Relaxed);
			let pstate: MrapiProcessState = Atom::unpack(*ppacked);
			if pstate.pid() == pid {
			    finalize_node_locked(d, n);
			}
		    }
		}
	    }
	    for p in 0..MRAPI_MAX_PROCESSES {
		let ppacked = &MRAPI_DB.processes[p].state.load(Ordering::Relaxed);
		let pstate: MrapiProcessState = Atom::unpack(*ppacked);
		if pstate.valid() == MRAPI_TRUE &&
		    pstate.pid() == pid {
			let mut process = &mut MRAPI_DB.processes[p];
			process.clear();
			break;
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
    // associate this node w/ a pid,tid pair so that we can recognize the caller on later calls

    mrapi_dprintf!(1, "mrapi::internal::lifecycle::initialize ({},{});", domain_id, node_id);

    if initialized() {
	return Err(MrapiErrNodeInitfailed);
    };

    // 1) setup the global database
    // get/create the shared memory database
    let flink = Path::new("MRAPI_SHMEM");
    let shmem_local = match ShmemConf::new().flink(flink).size(size_of::<MrapiDatabase>()).open() {
	Ok(val) => {
	    val
	},
	Err(e) => {
	    ShmemConf::new().flink(flink).size(size_of::<MrapiDatabase>()).create().unwrap()
	},
    };

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

    let mut slink = Path::new("MRAPI_SEM");
    if !Path::new(&slink).exists() {
	File::create(&slink).unwrap();
    };
    let key = Key::new(&slink, NonZeroU8::new(b'a').unwrap()).unwrap();

    let mut _sys_sem_acquired = false;
    let sem_local = loop {
	unsafe {
	    let _mutex_changer = _DB_MUTEX.lock().unwrap();
	    if !LateStatic::has_value(&MRAPI_DB) {
		let boxed = Box::<&MrapiDatabase>::new(&*shmem_local.as_ptr().cast::<MrapiDatabase>());
		LateStatic::assign(&MRAPI_DB, boxed);
	    }
	}

	let sem = match Semaphore::open(key, 1 /* num sems */) {
	    Ok(val) => {
		_sys_sem_acquired = true;
		val
	    },
	    Err(_) => {
		match Semaphore::create(key, 1 /* num sems */, Exclusive::No, Mode::from_bits(0o666).unwrap()) {
		    Ok(val) => {
			_sys_sem_acquired = true;
			val
		    },
		    Err(_) => { continue; },
		}
	    }
	};
	if _sys_sem_acquired { break sem }
    };

    mrapi_dprintf!(1, "mrapi_impl_initialize lock acquired, now attaching to shared memory and adding node to database");

    // At this point we've managed to acquire and lock the semaphore ...
    // NOTE: it's important to write to the globals only while
    // we have the semaphore otherwise we introduce race conditions. This
    // is why we are using the local variable id until everything is set up.
    
    unsafe {
	LateStatic::<Fragile<Shmem>>::assign(&SHMEMID, Fragile::<Shmem>::new(shmem_local)); // set the global shmem key
	LateStatic::<Fragile<Semaphore>>::assign(&SEMID, Fragile::<Semaphore>::new(sem_local)); // set the global sem key

	let tid = thread_id::get() as MrapiUint32;
	sys_os_srand(tid); // Seed random number generator
	// Get our identity
	LateStatic::<MrapiUint32>::assign(&MRAPI_PID, process::id());
	LateStatic::<MrapiUint32>::assign(&MRAPI_PROC, process::id());
	LateStatic::<MrapiUint32>::assign(&MRAPI_TID, tid);
    }

    // 3) Add the process/node/domain to the database

    let mrapi_db: &MrapiDatabase = &*MRAPI_DB;
    
    // First see if this domain already exists
    let mut d: usize = 0;
    let mut active_domain = false;
    for i in 0..MRAPI_MAX_DOMAINS {
	let packed = &mrapi_db.domains[i].state.load(Ordering::Relaxed);
	let dstate: MrapiDomainState = Atom::unpack(*packed);
	if dstate.domain_id() == domain_id as MrapiUint32 {
	    d = i as usize;
	    active_domain = true;
	    break;
	}
    }

    if !active_domain {
	// Find first available entry
	for i in 0..MRAPI_MAX_DOMAINS {
	    let packed = &mrapi_db.domains[i].state.load(Ordering::Relaxed);
	    let mut oldstate: MrapiDomainState = Atom::unpack(*packed);
	    let mut newstate = oldstate;
	    oldstate.set_allocated(MRAPI_FALSE);
	    newstate.set_domain_id(domain_id as MrapiUint32);
	    newstate.set_allocated(MRAPI_TRUE);
	    match &mrapi_db.domains[d].state.compare_exchange(
		Atom::pack(oldstate), Atom::pack(newstate), Ordering::Acquire, Ordering::Relaxed) {
		Ok(_) => {
		    d = i as usize;
		    active_domain = true;
		    break;
		},
		Err(_) => continue,
	    }
	}
    }

    if !active_domain {
	mrapi_dprintf!(1, "You have hit MRAPI_MAX_DOMAINS, either use less domains or reconfigure with more domains");
	let semid = &*SEMID;
	let shmemid = &*SHMEMID;
	mrapi_dprintf!(0, "Unlock system semaphore: {:?}", semid);
	drop(shmemid);
	sys_sem_unlock(SEMID.get()).unwrap();
	sys_sem_delete(SEMID.get());
	return Err(MrapiErrNodeInitfailed);
    }

    // now find an available node index...
    for i in 0..MRAPI_MAX_NODES {
 	let packed = &mrapi_db.domains[d].nodes[i].state.load(Ordering::Relaxed);
	let state: MrapiNodeState = Atom::unpack(*packed);
	// Even though initialized() is checked by mrapi, we have to check again here because
	// initialized() and initalize() are  not atomic at the top layer
	if state.allocated() && state.node_num() == node_id as MrapiUint32 {
	    // this node already exists for this domain
	    mrapi_dprintf!(1, "This node ({}) already exists for this domain ({})", node_id, domain_id);
	    let semid = &*SEMID;
	    let shmemid = &*SHMEMID;
	    mrapi_dprintf!(0, "Unlock system semaphore: {:?}", semid);
	    drop(shmemid);
	    sys_sem_unlock(SEMID.get()).unwrap();
	    sys_sem_delete(SEMID.get());
	    return Err(MrapiErrNodeInitfailed);
	}
    }

    // it didn't exist so find the first available entry
    let mut n: usize = 0;
    let mut active_node = false;
    for i in 0..MRAPI_MAX_NODES {
 	let packed = &mrapi_db.domains[d].nodes[i].state.load(Ordering::Relaxed);
	let mut oldstate: MrapiNodeState = Atom::unpack(*packed);
	let mut newstate = oldstate;
	oldstate.set_allocated(MRAPI_FALSE);
	newstate.set_node_num(node_id as MrapiUint32);
	newstate.set_allocated(MRAPI_TRUE);
	match &mrapi_db.domains[d].nodes[i].state.compare_exchange(
	    Atom::pack(oldstate), Atom::pack(newstate), Ordering::Acquire, Ordering::Relaxed) {
	    Ok(_) => {
		n = i as usize;
		active_node = true;
		break;
	    },
	    Err(_) => continue,
	}
    }
    
    if !active_node {
	mrapi_dprintf!(1, "You have hit MRAPI_MAX_NODES, either use less nodes or reconfigure with more nodes");
	let semid = &*SEMID;
	let shmemid = &*SHMEMID;
	mrapi_dprintf!(0, "Unlock system semaphore: {:?}", semid);
	drop(shmemid);
	sys_sem_unlock(SEMID.get()).unwrap();
	sys_sem_delete(SEMID.get());
	return Err(MrapiErrNodeInitfailed);
    }

    // See if this process exists
    let mut p: usize = 0;
    let mut active_process = false;
    for i in 0..MRAPI_MAX_PROCESSES {
 	let packed = &mrapi_db.processes[i].state.load(Ordering::Relaxed);
	let pstate: MrapiProcessState = Atom::unpack(*packed);
	if pstate.pid() == *MRAPI_PID {
	    p = i as usize;
	    active_process = true;
	    break;
	}
    }
    if !active_process {
	// It didn't exist so find the first available entry
	for i in 0..MRAPI_MAX_PROCESSES {
 	    let packed = &mrapi_db.processes[i].state.load(Ordering::Relaxed);
	    let mut oldstate: MrapiProcessState = Atom::unpack(*packed);
	    let mut newstate = oldstate;
	    oldstate.set_allocated(MRAPI_FALSE);
	    newstate.set_pid(*MRAPI_PID);
	    newstate.set_allocated(MRAPI_TRUE);
	    match &mrapi_db.processes[i].state.compare_exchange(
		Atom::pack(oldstate), Atom::pack(newstate), Ordering::Acquire, Ordering::Relaxed) {
		Ok(_) => {
		    p = i as usize;
		    active_process = true;
		    break;
		},
		Err(_) => continue,
	    }
	}
    }

    if !active_process {
	mrapi_dprintf!(1, "You have hit MRAPI_MAX_PROCESSES, either use less processes or reconfigure with more processes");
	let semid = &*SEMID;
	let shmemid = &*SHMEMID;
	mrapi_dprintf!(0, "Unlock system semaphore: {:?}", semid);
	drop(shmemid);
	sys_sem_unlock(SEMID.get()).unwrap();
	sys_sem_delete(SEMID.get());
	return Err(MrapiErrNodeInitfailed);
    }

    let semid = &*SEMID;
    let shmemid = &*SHMEMID;
    mrapi_dprintf!(0, "Detach shared memory");
    drop(shmemid);
    mrapi_dprintf!(0, "Unlock system semaphore: {:?}", semid);
    sys_sem_unlock(SEMID.get()).unwrap();
    sys_sem_delete(SEMID.get());

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
