///
/// Copyright(c) 2022, Karl Eric Harper
/// All rights reserved.
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions are met :
///
/// * Redistributions of source code must retain the above copyright
///   notice, this list of conditions and the following disclaimer.
/// * Redistributions in binary form must reproduce the above copyright
///   notice, this list of conditions and the following disclaimer in the
///   documentation and/or other materials provided with the distribution.
/// * Neither the name of Karl Eric Harper nor the names of its contributors may be used
///   to endorse or promote products derived from this software without specific
///   prior written permission.
///
/// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
/// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
/// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
/// DISCLAIMED.IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
/// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
/// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
/// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
/// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
/// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
/// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///

use crate::*;

#[allow(unused_imports)]
use std::{
    collections::{LinkedList},
    sync::{
	atomic::{
	    AtomicU8,
	    AtomicU16,
	    AtomicU32,
	    AtomicU64,
	},
	Arc,
	Mutex,
    },
    process::{exit},
    sync::{
	atomic::Ordering,
    },
};

use late_static::{LateStatic};
use heliograph::{
    Semaphore,
};
use shared_memory::{Shmem};
use fragile::{Fragile};
use bitfield::{bitfield};
use atomig::{Atom};

use crate::sysvr4::{
    os::{
	sys_os_usleep,
	sys_os_rand,
    },
    sem::{
	sys_sem_lock,
	sys_sem_unlock,
    },
};
use crate::internal::lifecycle::{
    MRAPI_TID,
};

//-------------------------------------------------------------------
//  the mrapi_impl database structure
//-------------------------------------------------------------------
// resource structure
struct MrapiCallback {
    callback_func: fn(MrapiEvent),
    callback_event: MrapiEvent,
    callback_frequency: MrapiUint,
    callback_count: MrapiUint,
    node_id: MrapiNode,
}

// lock state is atomic
bitfield! {
    #[derive(Atom)]
    struct MrapiLock(u64);
    lock_key, set_lock_key: 31, 0;
    lock_holder_nindex, set_lock_holder_nindex: 39, 32;
    lock_holder_dindex, set_lock_holder_dindex: 47, 40;
    id, set_id: 55, 48;
    valid, set_valid: 56;
    locked, set_locked: 57;
}

// mutexes, semaphores and reader-writer locks share this data structure
pub struct MrapiSem {
    handle: MrapiUint32, // used for reverse look-up when ext error checking is enabled
    num_locks: MrapiInt32,
    locks: [MrapiLock; MRAPI_MAX_SHARED_LOCKS],
    key: MrapiInt32, // the shared key passed in on get/create
    spin: AtomicU32,
    shared_lock_limit: MrapiInt32,
    ltype: LockType,
    attributes: MrapiSemAttributes,
    pub valid: MrapiBoolean,
    // only used when ext error checking is enabled.  Basically protects the
    // entry from ever being overwritten
    deleted: MrapiBoolean,
    pub refs: AtomicU8, // the number of nodes using the sem (for reference counting)
}

// shared memory
pub struct MrapiShmem {
    pub valid: MrapiBoolean,
    id: [MrapiUint32; MRAPI_MAX_PROCESSES], // the handle returned by the os or whoever creates it
    pub mem: [Arc<Mutex<*mut u8>>; MRAPI_MAX_PROCESSES],
    pub size: MrapiUint32,
    attributes: MrapiShmemAttributes,
    pub refs: MrapiUint8, // the number of nodes currently attached (for reference counting)
    num_procs: MrapiUint8,
    pub processes: [MrapiUint8; MRAPI_MAX_PROCESSES], // the list of processes currently attached
}

impl MrapiShmem {
    pub fn set_process(&mut self, p: usize, proc: MrapiUint8) {
	self.processes[p] = proc;
    }
}

unsafe impl Send for MrapiShmem {}
unsafe impl Sync for MrapiShmem {}

// remote memory
struct MrapiRmem {
    valid: MrapiBoolean,
    access_type: MrapiRmemAtype,
    key: MrapiUint32, // the shared key passed in on get/create
    /*
    addr: *mut u8,
    size: usize,
     */
    attributes: MrapiRmemAttributes,
    refs: MrapiUint8, // the number of nodes currently attached (for reference counting)
    nodes: [MrapiUint8; MRAPI_MAX_NODES], // the list of nodes currently attached
}

bitfield!{
    #[derive(Atom, Copy, Clone)]
    pub struct MrapiNodeState(u64);
    u32;
    pub node_num, set_node_num: 31, 0;
    bool;
    pub allocated, set_allocated: 32;
    pub valid, set_valid: 33;
}

pub struct MrapiNodeData {
    //struct sigaction signals[MCA_MAX_SIGNALS];
    pub state: AtomicU64, // MrapiNodeState,
    tid: thread::ThreadId,
    pub proc_num: MrapiUint,
    pub sems: [MrapiUint8; MRAPI_MAX_SEMS], // list of sems this node is referencing
    pub shmems: [MrapiUint8; MRAPI_MAX_SHMEMS], // list of shmems this node is referencing
}

bitfield!{
    #[derive(Atom, Copy, Clone)]
    pub struct MrapiDomainState(u64);
    u32;
    pub domain_id, set_domain_id: 31, 0;
    bool;
    pub allocated, set_allocated: 32;
    pub valid, set_valid: 33;
}

pub struct MrapiDomainData {
    pub num_nodes: AtomicU16, // not decremented
    pub state: AtomicU64, // MrapiDomainState
    pub nodes: [MrapiNodeData; MRAPI_MAX_NODES],
}

struct MrapiRequestData {
    valid: MrapiBoolean,
    completed: MrapiBoolean,
    cancelled: MrapiBoolean,
    node_num: MrapiUint32,
    domain_id: MrapiDomain,
    status: MrapiStatus,
}

struct MrapiAtomicOpData {
    index: MrapiInt, // shared memory index
    source: MrapiInt, // handle source
}

enum MrapiAtomicOperation {
    Open(MrapiAtomicOpData),
    Close(MrapiAtomicOpData),
    Dup(MrapiAtomicOpData),
    Sync(MrapiAtomicOpData),
}

// atomic operation
struct MrapiAtomicOp {
    atype: MrapiAtomic,
    spindex: MrapiInt, // source process index
    valid: MrapiBoolean,
    shmem: MrapiShmemHndl,
    dest: MrapiUint32, // offset from base addr
    operation: MrapiAtomicOperation,
}

impl MrapiAtomicOp {
    fn clear(&mut self) {
	self.atype = MrapiAtomic::MrapiAtomNoop;
    }
}

bitfield! {
    #[derive(Atom, Copy, Clone)]
    pub struct MrapiProcessState(u64);
    u32;
    pub pid, set_pid: 31, 0;
    bool;
    pub allocated, set_allocated: 32;
    pub valid, set_valid: 33;
}

// process address space
pub struct MrapiProcessData {
    pub state: AtomicU64, // MrapiProcessState,
    pub num_nodes: AtomicU16,
    proc: MrapiInt, // process ID for duplicating shared memory
    link: [MrapiInt; MRAPI_MAX_PROCESSES], // 1 if can be signaled
    // signal SIGUSR1 indicates atomic operation signal */
    // struct sigaction atomic;
    op: MrapiAtomicOp,
}

impl MrapiProcessData {
    pub fn clear(&mut self) {
	self.state = AtomicU64::default();
	self.num_nodes = AtomicU16::default();
	self.proc = 0;
	for p in 0..MRAPI_MAX_PROCESSES {
	    self.link[p] = 0;
	}
	self.op.clear();
    }
}

struct Lock {
  lock: MrapiUint32,
}

struct MrapiSysSem {
  num_locks: MrapiInt32,
  locks: [Lock; MRAPI_MAX_SHARED_LOCKS],
  key: MrapiInt32, // the shared key passed in on get/create
  valid: MrapiBoolean,
}

pub struct MrapiDatabase {
  global_lock: Lock, // not used
  num_shmems: MrapiUint8,
  num_sems: MrapiUint8, // not decremented
  num_rmems: MrapiUint8, // not decremented
  num_domains: MrapiUint8, // not used
  num_processes: MrapiUint8, // not used
  pub shmems: [MrapiShmem; MRAPI_MAX_SHMEMS],
  pub sems: [MrapiSem; MRAPI_MAX_SEMS],
  sys_sems: [MrapiSysSem; MRAPI_MAX_SEMS],
  rmems: [MrapiRmem; MRAPI_MAX_RMEMS],
  pub domains: [MrapiDomainData; MRAPI_MAX_DOMAINS],
  requests: [MrapiRequestData; MRAPI_MAX_REQUESTS],
  pub processes: [MrapiProcessData; MRAPI_MAX_PROCESSES],
  // Rollover variables
  rollover_callbacks_ptr: [fn(); MRAPI_MAX_CALLBACKS],
  rollover_index: MrapiUint16,
  // Callback variables
  callbacks_array: [MrapiCallback; MRAPI_MAX_CALLBACKS],
  callback_index: MrapiUint16,
}

impl MrapiDatabase {
    pub fn shmem(&mut self, index: usize) -> &mut MrapiShmem {
	&mut self.shmems[index]
    }
}

// do not put the following globals in thread-local-storage for two reasons:
//	 1) they are reset when a system call occurs which prevents us from being able to clean up properly
//	 2) gdb can't see thread local storage database is needed for whitebox testing

pub static MRAPI_DB: LateStatic<Box<&MrapiDatabase>> = LateStatic::new(); // shared memory addr for internal database
pub static SHMEMID: LateStatic<Fragile<Shmem>> = LateStatic::new(); // shared memory for internal database
pub static SEMID: LateStatic<Fragile<Semaphore>> = LateStatic::new(); // global semaphore id for locking/unlocking database

/// Locks the database (blocking)
pub fn access_database_pre(semref: &MrapiSemRef, fail_on_error: MrapiBoolean) -> MrapiBoolean {
    if !semref.spinlock_guard {
	match sys_sem_lock(&semref.set) {
	    Ok(_) => { return MRAPI_TRUE },
	    Err(e) => {
		let sem = &semref.set;
		mrapi_dprintf!(4, "mrapi::internal::access_database_pre {:?} err: {}", sem, e);
		if fail_on_error {
		    eprintln!("FATAL ERROR: unable to lock mrapi database {:?} err: {}", sem, e);
		    exit(1);
		}

		return MRAPI_FALSE;
	    },
	}
    }
    else {
	let mrapi_db: &MrapiDatabase = &*MRAPI_DB;
	
	// Spinlock
	let lock = *MRAPI_TID;
	let unlock = 0;
	loop {
	    match &mrapi_db.sems[semref.member].spin.compare_exchange(unlock, lock, Ordering::Acquire, Ordering::Relaxed) {
		Ok(_) => break,
		Err(_) => sys_os_usleep(sys_os_rand() as MrapiUint64),
	    }
	}
    };
    
    mrapi_dprintf!(4, "mrapi::internal::db::access_database_pre (got the internal mrapi db lock)");

    MRAPI_TRUE
}

/// Unlocks the database
pub fn access_database_post(semref: &MrapiSemRef) {
    mrapi_dprintf!(4, "mrapi::internal::db::access_database_post (released the internal mrapi db lock)");

    if !semref.spinlock_guard {
	match sys_sem_unlock(&semref.set) {
	    Ok(_) => {},
	    Err(e) => {
		let sem = &semref.set;
		mrapi_dprintf!(4, "mrapi::internal::access_database_post {:?} err: {}", sem, e);
	    },
	}
    }
    else {
	// Spinlock
	let mrapi_db: &MrapiDatabase = &*MRAPI_DB;
	
	let lock = *MRAPI_TID;
	let unlock = 0;
	match &mrapi_db.sems[semref.member].spin.compare_exchange(lock, unlock, Ordering::Acquire, Ordering::Relaxed) {
	    Ok(_) => {},
	    Err(tid) => {
		mrapi_dprintf!(4, "mrapi::internal::access_database_post lock mismatch tid: {}", tid);
	    }
	}
    }
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    fn cycle() {
	let mut semref = MrapiSemRef {
	    set: SEMID.get(),
	    member: 0,
	    spinlock_guard: MRAPI_FALSE
	};

	// Semaphore, no fail
	assert!(access_database_pre(&semref, MRAPI_FALSE));
	access_database_post(&semref);
	// Semaphore, fail
	access_database_pre(&semref, MRAPI_TRUE);
	access_database_post(&semref);

	// Spinlock
	semref.spinlock_guard = MRAPI_TRUE;
	access_database_pre(&semref, MRAPI_FALSE);
	access_database_post(&semref);
    }
}
