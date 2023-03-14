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
use crate::MrapiStatusFlag::*;

#[allow(unused_imports)]
use std::{
    env,
    process,
    path::{
	Path,
    },
    ffi::{
	OsStr,
    },
    collections::{
	LinkedList,
    },
    sync::{
	atomic::{
	    AtomicBool,
	    AtomicU8,
	    AtomicU16,
	    AtomicU32,
	    AtomicU64,
	    fence,
	},
	Arc,
	Mutex,
    },
    mem::{
	MaybeUninit,
    },
    process::{
	exit,
    },
    ops::{
	Deref,
	DerefMut,
    },
};

use nix:: {
    unistd:: {
	Pid,
    },
    libc:: {
	sigaction,
    },
    sys:: {
	signal:: {
	    Signal,
	    SIGUSR1,
	    SIGSEGV,
	    SIGABRT,
	    kill,
	},
    },
};

use bitfield::{
    bitfield,
};
use atomig::{
    Atom,
    Ordering,
};

use once_cell::{
    sync::{
	OnceCell,
    },
};

use cfg_if::cfg_if;

//Load up the proper OS implementation
cfg_if! {
    if #[cfg(target_os="windows")] {
        mod windows;
        use windows as os_impl;
    } else if #[cfg(any(target_os="freebsd", target_os="linux", target_os="macos"))] {
        mod unix;
        use unix as os_impl;
    } else {
        compile_error!("shared_memory isnt implemented for this platform...");
    }
}

const USE_GLOBAL_ONLY: bool = false;
const USE_UID: bool = true;

#[derive(Clone, Default)]
/// Configure MRAPI database for mapping to shared memory
pub struct MrapiConf {
    unique_id: String,
    key: MrapiUint32,
    db_key: MrapiUint32,
    sems_key: MrapiUint32,
    shmems_key: MrapiUint32,
    rmems_key: MrapiUint32,
    requests_key: MrapiUint32,
}

impl Drop for MrapiConf {
    fn drop(&mut self) {
    }
}

impl MrapiConf {
    /// Create a new default MRAPI configuration manager
    pub fn new() -> Self {
	
        MrapiConf::default()
    }

    pub fn unique_id(mut self, id: String) -> Self {
	self.unique_id = id;
	self
    }

    /// Attach to shared memory according to specifications
    pub fn attach(mut self) -> Result<SharedMem<MrapiConf, MrapiDatabase>, MrapiStatusFlag> {
	if USE_UID {
	    if self.unique_id.len() == 0 {
		// Get process name
		let binding = env::args().next();
		let proc_name = binding
		    .as_ref()
		    .map(Path::new)
		    .and_then(Path::file_name)
		    .and_then(OsStr::to_str);
		self.unique_id = proc_name.unwrap().to_owned() + "_mrapi";
	    }
	    self.key = common::crc::crc32_compute_buf(0, &self.unique_id);
	    self.db_key = common::crc::crc32_compute_buf(self.key, &(self.unique_id.clone() + "_db"));
	    self.sems_key = common::crc::crc32_compute_buf(self.key, &(self.unique_id.clone() + "_sems"));
	    self.shmems_key = common::crc::crc32_compute_buf(self.key, &(self.unique_id.clone() + "_shmems"));
	    self.rmems_key = common::crc::crc32_compute_buf(self.key, &(self.unique_id.clone() + "_rmems"));
	    self.requests_key = common::crc::crc32_compute_buf(self.key, &(self.unique_id.clone() + "_requests"));
	}
	else {
	    if self.unique_id.len() > 0 {
		self.key = common::crc::crc32_compute_buf(0, &self.unique_id);
	    }
	    else {
		self.key = match os_file_key("", 'z' as u32) {
		    Some(v) => v,
		    None => {
			mrapi_dprintf!(1, "mrapi::internal::db::MrapiConf::attach: Invalid file key");
			return Err(MrapiErrNodeInitfailed);
		    },
		};
	    }
	    self.db_key = self.key + 10;
	    self.sems_key = self.key + 20;
	    self.shmems_key = self.key + 30;
	    self.rmems_key = self.key + 40;
	    self.requests_key = self.key + 50;
	}

	let mgr = match sysvr4::shmem_get::<MrapiConf, MrapiDatabase>(self.db_key, self.clone()) {
	    Some(v) => v,
	    None => { // race condition with another process?
		let mgr = match shmem_create::<MrapiConf, MrapiDatabase>(self.db_key, self.clone()) {
		    Some(v) => v,
		    None => {
			mrapi_dprintf!(1, "mrapi::internal::db::MrapiConf::attach: failed to attach shared memory db");
			SharedMem::default()
		    },
		};
		
		mgr
	    },
	};
	
	Ok(mgr)
    }
}

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
    locks: [AtomicU64; 4], // MrapiLock
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
    key: [MrapiUint32; MRAPI_MAX_PROCESSES], // the key used to map shared memory
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

// Node state
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
    signals: [sigaction; MCA_MAX_SIGNALS],
    pub state: AtomicU64, // MrapiNodeState,
    tid: MrapiUint32,
    pub proc_num: MrapiUint,
    pub sems: [MrapiUint8; MRAPI_MAX_SEMS], // list of sems this node is referencing
    pub shmems: [MrapiUint8; MRAPI_MAX_SHMEMS], // list of shmems this node is referencing
}

impl MrapiNodeData {
    pub fn clear(&mut self) {
	self.state = AtomicU64::default();
	self.proc_num = 0;
	self.sems = [0u8; MRAPI_MAX_SEMS];
	self.shmems = [0u8; MRAPI_MAX_SHMEMS];
    }
}

// Domain state
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

#[derive(Copy, Clone)]
struct MrapiAtomicOpData {
    index: MrapiInt, // shared memory index
    source: usize, // handle source
}

#[derive(Copy, Clone)]
enum MrapiAtomicOperation {
    Open,
    Close,
    Dup(MrapiAtomicOpData),
    Sync,
}

// atomic operation
pub struct MrapiAtomicOp {
    atype: MrapiAtomic,
    spindex: usize, // source process index
    valid: AtomicBool,
    shmem: usize,
    dest: MrapiUint32, // offset from base addr
    operation: MrapiAtomicOperation,
}

impl MrapiAtomicOp {
    fn clear(&mut self) {
	self.atype = MrapiAtomic::MrapiAtomNoop;
    }

    fn update(&mut self, op: &MrapiAtomicOp) {
	// Leave valid unchanged
	self.atype = op.atype;
	self.spindex = op.spindex;
	self.shmem = op.shmem;
	self.dest = op.dest;
	self.operation = op.operation;
    }
}

// Process state
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
  num_shmems: MrapiUint8,
  num_sems: MrapiUint8, // not decremented
  num_rmems: MrapiUint8, // not decremented
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

// do not put the following globals in thread-local-storage for two reasons:
//	 1) they are reset when a system call occurs which prevents us from being able to clean up properly
//	 2) gdb can't see thread local storage database is needed for whitebox testing

pub static MRAPI_SEM: OnceCell<Semaphore> = OnceCell::new(); // global lock for internal database

thread_local! {
    pub static MRAPI_PID: OnceCell<MrapiUint32> = OnceCell::new();
    pub static MRAPI_PROC: OnceCell<MrapiUint32> = OnceCell::new();
    pub static MRAPI_TID: OnceCell<MrapiUint32> = OnceCell::new();
    pub static MRAPI_NINDEX: OnceCell<usize> = OnceCell::new();
    pub static MRAPI_DINDEX: OnceCell<usize> = OnceCell::new();
    pub static MRAPI_PINDEX: OnceCell<usize> = OnceCell::new();
    pub static MRAPI_NODE_ID: OnceCell<MrapiNode> = OnceCell::new();
    pub static MRAPI_DOMAIN_ID: OnceCell<MrapiDomain> = OnceCell::new();
}

// finer grained locks for these sections of the database.
thread_local! {
    pub static SEMS_SEM: OnceCell<Semaphore> = OnceCell::new(); // sems array
    pub static SHMEMS_SEM: OnceCell<Semaphore> = OnceCell::new(); // shmems array
    pub static RMEMS_SEM: OnceCell<Semaphore> = OnceCell::new(); // rmems array
    pub static REQUESTS_SEM: OnceCell<Semaphore> = OnceCell::new(); // requests array
}

// Keep copies of global objects for comparison
thread_local! {
    pub static SEMS_GLOBAL: OnceCell<Semaphore> = OnceCell::new();
    pub static SHMEMS_GLOBAL: OnceCell<Semaphore> = OnceCell::new();
    pub static RMEMS_GLOBAL: OnceCell<Semaphore> = OnceCell::new();
    pub static REQUESTS_GLOBAL: OnceCell<Semaphore> = OnceCell::new();
}

/// Create or get the semaphore corresponding to the key
fn create_sys_semaphore(num_locks: usize, key: u32, lock: MrapiBoolean) -> Option<Semaphore>
{
    let max_tries: u32 = 0xffffffff;
    let mut trycount: u32 = 0;

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

	if sem.key() > 0 {
	    if lock {
		let mut sr = MrapiSemRef::new(&sem, 0, false);
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
	    else {
		return Some(sem);
	    }
	}
    }
    
    None
}

unsafe impl Sync for MrapiDatabase {}
unsafe impl Send for MrapiDatabase {}

impl MrapiDatabase {
    fn attach(db_key: u32, conf: MrapiConf) -> Option<SharedMem<MrapiConf, MrapiDatabase>> {
	let mgr = match sysvr4::shmem_get::<MrapiConf, MrapiDatabase>(db_key, conf.clone()) {
	    Some(v) => v,
	    None => { // race condition with another process?
		let mgr = match shmem_create::<MrapiConf, MrapiDatabase>(db_key, conf.clone()) {
		    Some(v) => v,
		    None => {
			mrapi_dprintf!(1, "mrapi::internal::db::MrapiDatabase: failed to attach shared memory db");
			SharedMem::default()
		    },
		};
		mgr
	    },
	};
	
	Some(mgr)
    }

    fn initialize_sem(sem: Semaphore) {
	MRAPI_SEM.set(sem).unwrap();
    }

    fn global_sem() -> &'static Semaphore {
	MRAPI_SEM.get().expect("MRAPI semaphore not initialized")
    }

    /// Returns the initialized characteristics of this thread
    pub fn whoami() -> Result<(MrapiNode, MrapiUint, MrapiDomain, MrapiUint), MrapiStatusFlag> {
	let mut result = Err(MrapiErrDbNotInitialized);
	let mut initialized = false;

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

	let node_id = MRAPI_NODE_ID.with(|id| {
	    match id.get() {
		None => 0,
		Some(v) => *v,
	    }
	});
	let nindex = MRAPI_NINDEX.with(|index| {
	    match index.get() {
		None => 0,
		Some(v) => *v,
	    }
	});
	let domain_id = MRAPI_DOMAIN_ID.with(|id| {
	    match id.get() {
		None => 0,
		Some(v) => *v,
	    }
	});
	let dindex = MRAPI_DINDEX.with(|index| {
	    match index.get() {
		None => 0,
		Some(v) => *v,
	    }
	});

	Ok((node_id, nindex, domain_id, dindex))
    }

    /// Checks if the database has been initialized
    #[allow(dead_code)]
    pub fn initialized() -> bool {
	match Self::whoami() {
	    Ok(_) => true,
	    Err(_) => false,
	}
    }

    /// Notify other processes of initialization
    fn atomic_forward(&mut self, s: usize, mut op: MrapiAtomicOp) -> Result<MrapiStatus, MrapiStatusFlag> {
	let status: MrapiStatusFlag = MrapiErrAtomOpNoforward;

	let atype = op.atype;
	MRAPI_PINDEX.with(|index| {
	    match index.get() {
		Some(v) => {
		    op.spindex = *v;
		},
		None => {},
	    }
	});
	op.shmem = s;
	op.valid = MRAPI_TRUE.into();

	// Fire events
	match atype {
	    MrapiAtomic::MrapiAtomOpenproc | MrapiAtomic::MrapiAtomCloseproc => {
		for p in 0..MRAPI_MAX_PROCESSES {
 		    let packed = &self.processes[p].state.load(Ordering::Relaxed);
		    let pstate: MrapiProcessState = Atom::unpack(*packed);
		    if op.spindex != p && pstate.valid() {
			let pid = pstate.pid();
			mrapi_dprintf!(2, "MrapiDatabase::atomic_forward ({} /*s*/, {:?} /*op->type*/); {} /*target*/",
				       s, atype, pid);
			
			// Spin in this process while operation is in use
			loop {
			    match &self.processes[p].op.valid.compare_exchange(
				false, true, Ordering::Acquire, Ordering::Relaxed) {
				Ok(_) => {
				    break;
				},
				Err(_) => {
				    os_yield();
				    continue;
				},
			    }
			}

			self.processes[p].op.update(&op);

			os_impl::signal_remote_process(p, self);

			// Force memory synchronization
			fence(Ordering::AcqRel);

			match atype {
			    MrapiAtomic::MrapiAtomOpenproc | MrapiAtomic::MrapiAtomCloseproc => {
				// Spin waiting for link update
				while (atype as u8 - MrapiAtomic::MrapiAtomOpenproc as u8) == self.processes[p].link[op.spindex] as u8 {
				    os_yield();
				}
			    },
			    _other => { },
			}
			
		    }
		}

		return Ok(MRAPI_TRUE);
	    },

	    MrapiAtomic::MrapiAtomShmdup => {
		let p =
		    if let MrapiAtomicOperation::Dup(data) = op.operation {
			data.source
		    }
		else { 0 };
		
 		let packed = &self.processes[p].state.load(Ordering::Relaxed);
		let pstate: MrapiProcessState = Atom::unpack(*packed);
		if op.spindex != p {
		    let pid = pstate.pid();
		    mrapi_dprintf!(2, "MrapiDatabase::atomic_forward ({} /*s*/, {:?} /*op->type*/); {} /*target*/",
				  s, atype, pid);
		    // Spin until source process is valid
		    loop {
 			let packed = &self.processes[p].state.load(Ordering::Relaxed);
			let pstate: MrapiProcessState = Atom::unpack(*packed);
			if !pstate.valid() {
			    os_yield();
			}

			// Spin in this process while operation is in use
			loop {
			    match &self.processes[p].op.valid.compare_exchange(
				false, true, Ordering::Acquire, Ordering::Relaxed) {
				Ok(_) => {
				    break;
				},
				Err(_) => {
				    os_yield();
				    continue;
				},
			    }
			}

			self.processes[p].op.update(&op);

			os_impl::signal_remote_process(p, self);

			// Force memory synchronization
			fence(Ordering::AcqRel);

		    }
		}

		return Ok(MRAPI_TRUE);
	    },
	    _other => {
		mrapi_dprintf!(2, "MrapiDatabase::atomic_forward ({} /*s*/, {:?} /*op->type*/);",
			       s, atype);
	    },
	}
	
	Err(status)
    }

    /// Initializes the MRAPI internal layer (sets up the database and semaphore)
    pub fn initialize(&mut self, conf: MrapiConf, domain_id: MrapiDomain, node_id: MrapiNode) -> Result<MrapiStatus, MrapiStatusFlag> {

	// associate this node w/ a pid,tid pair so that we can recognize the caller on later calls

	mrapi_dprintf!(1, "mrapi::internal::db::MrapiDatabase::initialize ({},{});", domain_id, node_id);

	if Self::initialized() {
	    return Err(MrapiErrNodeInitfailed);
	};

	// 1) create or get the semaphore and lock it
	// we loop here and inside of create_sys_semaphore because of the following race condition:
	// initialize                  finalize
	// 1: create/get sem           1: lock sem
	// 2: lock sem                 2: check db: any valid nodes?
	// 3: setup db & add self      3a:  no -> delete db & delete sem
	// 4: unlock sem               3b:  yes-> unlock sem
	//
	// finalize-1 can occur between initialize-1 and initialize-2 which will cause initialize-2
	// to fail because the semaphore no longer exists.

	let key = conf.key;
	let sem_local = match create_sys_semaphore(1, key, MRAPI_TRUE) {
	    None => {
		
		mrapi_dprintf!(1, "MRAPI ERROR: Unable to get the semaphore key: {}", key);
		return Err(MrapiErrNodeInitfailed);
	    },
	    Some(v) => v,
	};

	mrapi_dprintf!(1, "mrapi::internal::db::MrapiDatabase::initialize lock acquired, now adding node to database");

	// At this point we've managed to acquire and lock the semaphore ...
	// NOTE: with USE_GLOBAL_ONLY it's important to write to the globals only while
	// we have the semaphore otherwise we introduce race conditions.  This
	// is why we are using the local variable id until everything is set up.

	// set the global semaphore reference
	MrapiDatabase::initialize_sem(sem_local);
	
	// get or create our finer grained locks
	// in addition to a lock on the sems array, every lock (rwl,sem,mutex) has it's own
	// database semaphore, this allows us to access different locks in parallel

	let sems_key = conf.sems_key;
	let sems_sem = create_sys_semaphore(MRAPI_MAX_SEMS + 1, sems_key, MRAPI_FALSE);
	if USE_GLOBAL_ONLY || sems_sem.is_none() {
	    SEMS_GLOBAL.with(|sem| {
		match sem.set(MrapiDatabase::global_sem().clone()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	    SEMS_SEM.with(|sem| {
		match sem.set(MrapiDatabase::global_sem().clone()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	}
	else {
	    SEMS_SEM.with(|sem| {
		match sem.set(sems_sem.unwrap()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	}

	let shmems_key = conf.shmems_key;
	let shmems_sem = create_sys_semaphore(1, shmems_key, MRAPI_FALSE);
	if shmems_sem.is_none() {
	    SHMEMS_GLOBAL.with(|sem| {
		match sem.set(MrapiDatabase::global_sem().clone()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	    SHMEMS_SEM.with(|sem| {
		match sem.set(MrapiDatabase::global_sem().clone()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	}
	else {
	    SHMEMS_SEM.with(|sem| {
		match sem.set(shmems_sem.unwrap()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	}

	let rmems_key = conf.rmems_key;
	let rmems_sem = create_sys_semaphore(1, rmems_key, MRAPI_FALSE);
	if rmems_sem.is_none() {
	    RMEMS_GLOBAL.with(|sem| {
		match sem.set(MrapiDatabase::global_sem().clone()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	    RMEMS_SEM.with(|sem| {
		match sem.set(MrapiDatabase::global_sem().clone()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	}
	else {
	    RMEMS_SEM.with(|sem| {
		match sem.set(rmems_sem.unwrap()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	}

	let requests_key = conf.requests_key;
	let requests_sem = create_sys_semaphore(1, requests_key, MRAPI_FALSE);
	if requests_sem.is_none() {
	    REQUESTS_GLOBAL.with(|sem| {
		match sem.set(MrapiDatabase::global_sem().clone()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	    REQUESTS_SEM.with(|sem| {
		match sem.set(MrapiDatabase::global_sem().clone()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	}
	else {
	    REQUESTS_SEM.with(|sem| {
		match sem.set(requests_sem.unwrap()) {
		    Ok(_) => {},
		    Err(_) => {},
		}
	    });
	}

	// Get our identity
	let pid = process::id();
	let tid = thread_id::get() as MrapiUint32;
	MRAPI_PID.with(|id| {
	    match id.set(pid) {
		Ok(_) => {},
		Err(_) => {},
	    }
	});
	MRAPI_PROC.with(|id| {
	    match id.set(pid) {
		Ok(_) => {},
		Err(_) => {},
	    }
	});
	MRAPI_TID.with(|id| {
	    match id.set(tid) {
		Ok(_) => {},
		Err(_) => {},
	    }
	});

	// Seed random number generator
	os_srand(tid);

	// 3) Add the process/node/domain to the database
	
	let d: usize = 0;
	let n: usize = 0;
	let p: usize = 0;

	// First see if this domain already exists
	for d in 0..MRAPI_MAX_DOMAINS {
	    let packed = &self.domains[d].state.load(Ordering::Relaxed);
	    let dstate: MrapiDomainState = Atom::unpack(*packed);
	    if dstate.domain_id() == domain_id as MrapiUint32 {
		break;
	    }
	}

	if d == MRAPI_MAX_DOMAINS {
	    // Find first available entry
	    for d in 0..MRAPI_MAX_DOMAINS {
		let packed = &self.domains[d].state.load(Ordering::Relaxed);
		let mut oldstate: MrapiDomainState = Atom::unpack(*packed);
		let mut newstate = oldstate;
		oldstate.set_allocated(MRAPI_FALSE);
		newstate.set_domain_id(domain_id as MrapiUint32);
		newstate.set_allocated(MRAPI_TRUE);
		match &self.domains[d].state.compare_exchange(
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
 		let packed = &self.domains[d].nodes[n].state.load(Ordering::Relaxed);
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
 		    let packed = &self.domains[d].nodes[n].state.load(Ordering::Relaxed);
		    let mut oldstate: MrapiNodeState = Atom::unpack(*packed);
		    let mut newstate = oldstate;
		    oldstate.set_allocated(MRAPI_FALSE);
		    newstate.set_node_num(node_id as MrapiUint32);
		    newstate.set_allocated(MRAPI_TRUE);
		    match &self.domains[d].nodes[n].state.compare_exchange(
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
 			let packed = &self.processes[p].state.load(Ordering::Relaxed);
			let pstate: MrapiProcessState = Atom::unpack(*packed);
			if pstate.pid() == pid {
			    break;
			}
		    }
		    if p == MRAPI_MAX_PROCESSES {
			// It didn't exist so find the first available entry
			for p in 0..MRAPI_MAX_PROCESSES {
 			    let packed = &self.processes[p].state.load(Ordering::Relaxed);
			    let mut oldstate: MrapiProcessState = Atom::unpack(*packed);
			    let mut newstate = oldstate;
			    oldstate.set_allocated(MRAPI_FALSE);
			    newstate.set_pid(pid);
			    newstate.set_allocated(MRAPI_TRUE);
			    match &self.processes[p].state.compare_exchange(
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
	    MRAPI_PINDEX.with(|index| {
		index.set(p).unwrap();
	    });
	}

	mrapi_dprintf!(1, "MrapiDatabase::initialize: adding domain_id:{} node_id:{} to d:{} n:{}",
		      domain_id, node_id, d, n);
 	let npacked = self.domains[d].nodes[n].state.load(Ordering::Relaxed);
	let mut nstate: MrapiNodeState = Atom::unpack(npacked);
	assert!(nstate.valid() == MRAPI_FALSE);
 	let dpacked = &self.domains[d].state.load(Ordering::Relaxed);
	let mut dstate: MrapiDomainState = Atom::unpack(*dpacked);
	let olddstate = dstate;
	dstate.set_domain_id(domain_id as u32);
	dstate.set_valid(MRAPI_TRUE);
	self.domains[d].state.compare_exchange(
	    Atom::pack(olddstate), Atom::pack(dstate), Ordering::Acquire, Ordering::Relaxed).unwrap();
	self.domains[d].num_nodes.fetch_add(1, Ordering::Relaxed);
	self.domains[d].nodes[n].tid = tid;
	self.domains[d].nodes[n].proc_num = p;
	let oldnstate = nstate;
	nstate.set_node_num(node_id as u32);
	nstate.set_valid(MRAPI_TRUE);
	self.domains[d].nodes[n].state.compare_exchange(
	    Atom::pack(oldnstate), Atom::pack(nstate), Ordering::Acquire, Ordering::Relaxed).unwrap();

	// Set our cached (thread-local-storage) identity
	MRAPI_NINDEX.with(|index| {
	    index.set(n).unwrap();
	});
	MRAPI_DINDEX.with(|index| {
	    index.set(d).unwrap();
	});
	MRAPI_DOMAIN_ID.with(|id| {
	    id.set(domain_id).unwrap();
	});
	MRAPI_NODE_ID.with(|id| {
	    id.set(domain_id).unwrap();
	});

 	let packed = &self.processes[p].state.load(Ordering::Relaxed);
	let pstate: MrapiProcessState = Atom::unpack(*packed);
	if !pstate.valid() {
	}

	Err(MrapiErrNodeInitfailed)
    }

    /// Release node resources, assumed under guard
    fn finalize_node_locked(&mut self, d: MrapiDomain, n: MrapiNode) {
	let dpacked = &self.domains[d].state.load(Ordering::Relaxed);
	let dstate: MrapiDomainState = Atom::unpack(*dpacked);
 	let npacked = &self.domains[d].nodes[n].state.load(Ordering::Relaxed);
	let oldstate: MrapiNodeState = Atom::unpack(*npacked);
	let mut newstate = oldstate;

	let domain_id = dstate.domain_id();
	let node_num = oldstate.node_num();
	mrapi_dprintf!(2, "MrapiDatabase::finalize_node_locked: dindex = {}, nindex = {}, domain = {}, node = {}",
		       d, n, domain_id, node_num);

	// Mark the node as finalized
	newstate.set_valid(MRAPI_FALSE);
	newstate.set_allocated(MRAPI_FALSE);

	self.domains[d].nodes[n].state.compare_exchange(
	    Atom::pack(oldstate), Atom::pack(newstate), Ordering::Acquire, Ordering::Relaxed).unwrap();

	// Rundown the node's process association
	let p = self.domains[d].nodes[n].proc_num;
	let ppacked = &self.processes[p].state.load(Ordering::Relaxed);
	let pstate: MrapiProcessState = Atom::unpack(*ppacked);
	if pstate.valid() {
	    let prev = self.processes[p].num_nodes.fetch_sub(1, Ordering::Relaxed);
	    if prev <= 1 {
		// Last node in this process, remove process references in shared memory
		for s in 0..MRAPI_MAX_SHMEMS {
		    if self.shmems[s].valid {
			self.shmems[s].processes[p] = 0;
		    }
		}
		self.processes[p].clear();
	    }
	}

	self.domains[d].nodes[n].clear();
	self.domains[d].num_nodes.fetch_sub(1, Ordering::Relaxed);

	// Decrement the shmem reference count if necessary
	for shmem in 0..MRAPI_MAX_SHMEMS {
	    if self.shmems[shmem].valid == MRAPI_TRUE {
		if self.domains[d].nodes[n].shmems[shmem] == 1 {
		    // If this node was a user of this shm, decrement the ref count
		    self.shmems[shmem].refs -= 1;
		}
		// Shared memory is released automatically when all references are removed
	    }
	}
	
	// Decrement the sem reference count if necessary
	for sem in 0..MRAPI_MAX_SEMS {
	    if self.sems[sem].valid == MRAPI_TRUE {
		if self.domains[d].nodes[n].sems[sem] == 1 {
		    self.domains[d].nodes[n].sems[sem] = 0;
		    // If this node was a user of this sem, decrement the ref count
		    let prev = self.sems[sem].refs.fetch_sub(1, Ordering::Relaxed);
		    // If the reference count is 0 free the resource
		    if prev <= 1 {
			self.sems[sem].valid = MRAPI_FALSE;
		    }
		}
		// Semaphore is released automatically when all references are removed
	    }
	}

	/*
	#if !(__unix__)
	/* Release alarm timer */
	if (NULL != mrapi_db->domains[d].nodes[n].hAlarm) {
	    DeleteTimerQueueTimer(NULL, mrapi_db->domains[d].nodes[n].hAlarm, NULL);
	    CloseHandle(mrapi_db->domains[d].nodes[n].hAlarm);
	    mrapi_db->domains[d].nodes[n].hAlarm = NULL;
	}
	#endif  /* !(__unix__) */
	 */
    }

    /// Release database resources
    fn free_resources(&mut self, panic: MrapiBoolean) -> MrapiBoolean {
	let pid = process::id();
	let mut last_task_standing: MrapiBoolean = MRAPI_TRUE;
	let mut last_task_standing_for_this_process: MrapiBoolean = MRAPI_TRUE;

	let mut sr = MrapiSemRef::new(&MrapiDatabase::global_sem(), 0, false);
	let _locked = access_database_pre(&mut sr, MRAPI_FALSE);
	
	mrapi_dprintf!(1, "MrapiDatabase::free_resources (panic={}): freeing any existing resources in the database", panic);

	// finalize this node
	match Self::whoami() {
	    Ok(v) => {
		self.finalize_node_locked(v.0, v.2);
	    },
	    Err(_) => {},
	}

	// If we are in panic mode, then forcefully finalize all other nodes that belong to this process
	if panic {
	    for d in 0..MRAPI_MAX_DOMAINS {
		for n in 0..MRAPI_MAX_NODES {
 		    let npacked = &self.domains[d].nodes[n].state.load(Ordering::Relaxed);
		    let nstate: MrapiNodeState = Atom::unpack(*npacked);
		    if nstate.valid() == MRAPI_TRUE {
			let p = self.domains[d].nodes[n].proc_num;
			let ppacked = &self.processes[p].state.load(Ordering::Relaxed);
			let pstate: MrapiProcessState = Atom::unpack(*ppacked);
			if pstate.pid() == pid {
			    self.finalize_node_locked(d, n);
			}
		    }
		}
	    }
	    for p in 0..MRAPI_MAX_PROCESSES {
		let ppacked = &self.processes[p].state.load(Ordering::Relaxed);
		let pstate: MrapiProcessState = Atom::unpack(*ppacked);
		if pstate.valid() == MRAPI_TRUE && pstate.pid() == pid {
		    /*
		    #if !(__unix)
		    if (NULL != mrapi_db->processes[p].hAtomicEvt) {
			CloseHandle(mrapi_db->processes[p].hAtomicEvt);
		    }
		    #endif  /* !(__unix) */
		     */
		    self.processes[p].clear();
		    break;
		}
	    }
	}

	// See if there are any valid nodes left in the system and for this process
	for d in 0..MRAPI_MAX_DOMAINS {
	    for n in 0..MRAPI_MAX_NODES {
 		let npacked = &self.domains[d].nodes[n].state.load(Ordering::Relaxed);
		let nstate: MrapiNodeState = Atom::unpack(*npacked);
		if nstate.valid() == MRAPI_TRUE {
		    let p = self.domains[d].nodes[n].proc_num;
		    let ppacked = &self.processes[p].state.load(Ordering::Relaxed);
		    let pstate: MrapiProcessState = Atom::unpack(*ppacked);
		    last_task_standing = MRAPI_FALSE;
		    if pstate.pid() == pid {
			last_task_standing_for_this_process = MRAPI_FALSE;
		    }
		}
	    }
	}
	
	if panic {
	    assert!(last_task_standing_for_this_process);
	}
	
	// If there are no other valid nodes in the whole system, then free the sems
	if last_task_standing {
	    mrapi_dprintf!(1, "MrapiDatabase::free_resources: freeing mrapi internal semaphore and shared memory");

	    // Free the mrapi internal semaphores
	    SEMS_SEM.with(|cell| {
		match cell.get() {
		    Some(v) => {
			if v != MrapiDatabase::global_sem() {
			    drop(v);
			}
		    },
		    None => {},
		};
	    });
	    SHMEMS_SEM.with(|cell| {
		match cell.get() {
		    Some(v) => {
			if v != MrapiDatabase::global_sem() {
			    drop(v);
			}
		    },
		    None => {},
		};
	    });
	    RMEMS_SEM.with(|cell| {
		match cell.get() {
		    Some(v) => {
			if v != MrapiDatabase::global_sem() {
			    drop(v);
			}
		    },
		    None => {},
		};
	    });
	    REQUESTS_SEM.with(|cell| {
		match cell.get() {
		    Some(v) => {
			if v != MrapiDatabase::global_sem() {
			    drop(v);
			}
		    },
		    None => {},
		};
	    });
	}
	
	// if there are no other valid nodes for this process, then detach from shared memory
	
	MRAPI_FALSE
    }

    /// Release MRAPI database
    pub fn finalize(&mut self) -> MrapiBoolean {
	self.free_resources(MRAPI_FALSE);
	
	MRAPI_TRUE
    }

    /*
    /// Rundown node
    fn finalize_node_locked(d: MrapiUint, n: MrapiUint) {
	let mrapi_db: &MrapiDatabase = &MRAPI_MGR.get().unwrap().deref_mut().obj;
	let dpacked = &mrapi_db.domains[d].state.load(Ordering::Relaxed);
	let dstate: MrapiDomainState = Atom::unpack(*dpacked);
	let npacked = &mrapi_db.domains[d].nodes[n].state.load(Ordering::Relaxed);
	let mut nstate: MrapiNodeState = Atom::unpack(*npacked);
	let domain_id = dstate.domain_id();
	let node_num = nstate.node_num();
	
	mrapi_dprintf!(2, "mrapi::internal::db::finalize_node_locked dindex={} nindex={} domain={} node={}", d, n, domain_id, node_num);
	
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
			mrapi_db.shmems[s].set_process(p, 0);
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
*/
}

/// Locks the database (blocking)
pub fn access_database_pre(sr: &mut MrapiSemRef, fail_on_error: MrapiBoolean) -> MrapiBoolean {
    match sr.lock() {
	Some(_) => {},
	None => {
	    mrapi_dprintf!(4, "mrapi::internal::access_database_pre {:?}", sr);
	    if fail_on_error {
		eprintln!("FATAL ERROR: unable to lock mrapi database {:?}", sr);
		exit(1);
	    }

	    return MRAPI_FALSE
	},
    };
    
    mrapi_dprintf!(4, "mrapi::internal::db::access_database_pre (got the internal mrapi db lock)");

    MRAPI_TRUE
}

/// Unlocks the database
pub fn access_database_post(sr: &mut MrapiSemRef) {
    mrapi_dprintf!(4, "mrapi::internal::db::access_database_post (released the internal mrapi db lock)");

    match sr.unlock() {
	Some(_) => {},
	None => {
	    mrapi_dprintf!(4, "mrapi::internal::access_database_post {:?}", sr);
	},
    }
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    #[allow(unused_variables)]
    #[allow(unused_assignments)]
    #[allow(unused_mut)]
    fn cycle() {
	let mgr = match MrapiConf::new().attach() {
	    Ok(v) => v,
	    Err(_) => SharedMem::default(),
	};
	let mrapi_db = mgr.obj.initialize(mgr.conf.clone(), 0, 0);

	let key = os_file_key("", 'c' as u32).unwrap();
	let sem1 = match sem_get(key, 1) {
	    Some(v) => v,
	    None => { // race condition with another process?
		match sem_create(key, 1) {
		    Some(v) => v,
		    None => {
			assert!(false);
			Semaphore::default()
		    },
		}
	    },
	};

	let mut sr1 = MrapiSemRef::new(&sem1, 0, false);

	// Semaphore, no fail
	assert!(access_database_pre(&mut sr1, MRAPI_FALSE));
	access_database_post(&mut sr1);
	// Semaphore, fail
	access_database_pre(&mut sr1, MRAPI_TRUE);
	access_database_post(&mut sr1);

	// Spinlock
	let mut sr2 = MrapiSemRef::new(
	    MrapiDatabase::global_sem(), 0, MRAPI_TRUE);
	access_database_pre(&mut sr2, MRAPI_FALSE);
	access_database_post(&mut sr2);
    }
}
