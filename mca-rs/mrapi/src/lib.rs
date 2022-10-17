#![allow(dead_code)]
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

extern crate common;

use common::*;

use std::thread;
use std::cell::{RefCell};

// MRAPI data types
pub type MrapiDomain = McaDomain;
pub type MrapiNode = McaNode;
pub type MrapiStatus = McaStatus; 
pub type MrapiTimeout = McaTimeout; 

pub type MrapiBoolean = McaBoolean;

pub type MrapiUint = McaUint;
pub type MrapiUint8 = McaUint8;
pub type MrapiUint16 = McaUint16; 
pub type MrapiUint32 = McaUint32; 
pub type MrapiUint64 = McaUint64; 
pub type MrapiUint128 = McaUint128;

pub type MrapiInt = McaInt;
pub type MrapiInt8 = McaInt8;
pub type MrapiInt16 = McaInt16; 
pub type MrapiInt32 = McaInt32; 
pub type MrapiInt64 = McaInt64; 
pub type MrapiInt128 = McaInt128;

// Constants
pub const MRAPI_TRUE: MrapiBoolean = MCA_TRUE;
pub const MRAPI_FALSE: MrapiBoolean = MCA_FALSE;
pub const MRAPI_NULL: MrapiUint = MCA_NULL;

// lock type for semaphores
enum MrapiLockType {
    MrapiLockUnknown,
    MrapiLockRwl,
    MrapiLockSem,
    MrapiLockMutex,
}

// lock type for reader/writer locks
enum MrapiRwlMode {
    MrapiRwlReader,
    MrapiRwlWriter,
}

// access type for remote memory
enum MrapiRmemAtype {
    MrapiRmemDma,
    MrapiRmemSwcache,
    MrapiRmemDummy,
}

// atomic operation mode
enum MrapiAtomicMode {
  MrapiAtomicWrite = 0,
  MrapiAtomicRead,
  MrapiAtomicNone,
}

// base message object;
// messages include this as first member of their struct
struct MrapiMsg {
  valid: MrapiBoolean, // MRAPI_FALSE when MRAPI_ATOMIC_WRITE, MRAPI_TRUE otherwise
  txn: MrapiUint, // transaction ID
  counter: MrapiUint16, // non-blocking buffer synchronization
  tindex: MrapiUint16, // reference to mca_timestamp_t (FUTURE)
}

// atomic sync object
struct MrapiAtomicSync<'a> {
  pindex: &'a MrapiUint, // buffer index reference
  last_counter: MrapiUint, // non-blocking buffer previous state
  hold: MrapiBoolean, // MRAPI_TRUE if valid msg flag should be retained
                      // across atomic calls; last call must release
  onetime: MrapiBoolean, // one-time override to allow specified mode
  active: MrapiBoolean, // MRAPI_TRUE if caller holds sync
}

// sync descriptor for non-Windows cross-process atomic operations
struct MrapiAtomicBarrier<'a> {
  mode: MrapiAtomicMode,
  xchg: MrapiBoolean, // read only when valid, write only when invalid;
                      // flip flag state on completion
  timeout: MrapiTimeout,
  src: MrapiUint32, // local pid
  dest: MrapiUint32, // remote pid; local proc == remote proc, not sync required
  elems: MrapiUint, // number of buffer elements
  size: usize, // element size
  sync: MrapiAtomicSync<'a>,
  buffer: Vec<MrapiMsg>, // messages
}

type MrapiParameters = MrapiUint;

enum LockType {
    MrapiRwl,
    MrapiSem,
    MrapiMutex,
}

// Metadata resource related structs
enum MrapiResourceType {
    MrapiCpu,
    MrapiCache,
    MrapiMem,
    MrapiCoreComplex,
    MrapiCrossbar,
    MrapiSystem,
}

enum MrapiAttributeStatic {
    MrapiAttrStatic,
    MrapiAttrDynamic,
}

enum MrapiEvent {
    MrapiEventPowerManagement,
    MrapiEventCrossbarBufferUnder20percent,
    MrapiEventCrossbarBufferOver80percent,
}

enum MrapiAtomic {
    MrapiAtomNoop,
    MrapiAtomOpenproc,
    MrapiAtomCloseproc,
    MrapiAtomShmdup,
}

enum RsrcType {
    RsrcUint16,
    RsrcUint32,
}

#[repr(C)]
union RsrcValue {
    word: MrapiUint16,
    long: MrapiUint32,
}

struct MrapiResource<'a> {
    name: &'a str,
    resource_type: MrapiResourceType,
    number_of_children: MrapiUint32,
    children: Vec<MrapiResource<'a>>,
    number_of_attributes: MrapiUint32,
    attribute_types: Vec<RsrcType>,                        
    attribute_values: Vec<RsrcValue>,                        
    attribute_static: Vec<MrapiAttributeStatic>,
    attribute_started: MrapiBoolean,               
}

union AttribValue {
    byte: MrapiUint8,
    word: MrapiUint16,
    long: MrapiUint32,
}

struct MrapiImplAttributes<'a> {
  ext_error_checking: MrapiBoolean,
  shared_across_domains: MrapiBoolean,
  recursive: MrapiBoolean,  // only applies to mutexes
  spinlock_guard: MrapiBoolean,
  mem_addr: RefCell<AttribValue>,
  mem_size: usize,
  resource: MrapiResource<'a>,
}

type MrapiMutexAttributes<'a> = MrapiImplAttributes<'a>;
type MrapiSemAttributes<'a> = MrapiImplAttributes<'a>;
type MrapiRwlAttributes<'a> = MrapiImplAttributes<'a>;
type MrapiShmemAttributes<'a> = MrapiImplAttributes<'a>;
type MrapiRmemAttributes<'a> = MrapiImplAttributes<'a>;

type MrapiMutexHndl = MrapiUint32;
type MrapiSemHndl = MrapiUint32;
type MrapiRwlHndl = MrapiUint32;
type MrapiShmemHndl = MrapiUint32;
type MrapiRmemHndl = MrapiUint32;

// system created key used for locking/unlocking for recursive mutexes
type MrapiKey = MrapiInt;

const MRAPI_MAX_SEMS: MrapiUint = 4096;  // we don't currently support different values for max mutex/sem/rwl
const MRAPI_MAX_SHMEMS: MrapiUint = 10;
const MRAPI_MAX_RMEMS: MrapiUint = 10;
const MRAPI_MAX_REQUESTS: MrapiUint = MCA_MAX_REQUESTS;
const MRAPI_MAX_SHARED_LOCKS: MrapiUint = 256;

const MRAPI_RMEM_DEFAULT: MrapiRmemAtype = MrapiRmemAtype::MrapiRmemDummy;

const MRAPI_MAX_NODES: MrapiUint = MCA_MAX_NODES;
const MRAPI_MAX_PROCESSES: MrapiUint = MCA_MAX_PROCESSES;
const MRAPI_MAX_DOMAINS: MrapiUint = MCA_MAX_DOMAINS;
const MRAPI_MAX_CALLBACKS: MrapiUint = 10;
const MRAPI_ATOMIC_NULL: MrapiInt = -1;


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
struct MrapiLock {
    lock_key: MrapiUint32, 
    lock_holder_nindex: MrapiUint8,
    lock_holder_dindex: MrapiUint8,
    id: MrapiUint8,
    valid: MrapiBoolean,
    locked: MrapiBoolean,
} 

// mutexes, semaphores and reader-writer locks share this data structure
struct MrapiSem<'a> {
    handle: MrapiUint32, // used for reverse look-up when ext error checking is enabled
    num_locks: MrapiInt32,
    locks: [MrapiLock; MRAPI_MAX_SHARED_LOCKS],
    key: MrapiInt32, // the shared key passed in on get/create
    spin: MrapiInt32,
    shared_lock_limit: MrapiInt32,
    ltype: LockType,
    attributes: MrapiSemAttributes<'a>,
    valid: MrapiBoolean,
    // only used when ext error checking is enabled.  Basically protects the
    // entry from ever being overwritten
    deleted: MrapiBoolean,
    refs: MrapiUint8, // the number of nodes using the sem (for reference counting)
}

// shared memory
struct MrapiShmem<'a> {
    valid: MrapiBoolean,
    key: MrapiInt32, // the shared key passed in on get/create
    id: [MrapiUint32; MRAPI_MAX_PROCESSES], // the handle returned by the os or whoever creates it
    addr: [*mut u8; MRAPI_MAX_PROCESSES],
    size: MrapiUint32,
    attributes: MrapiShmemAttributes<'a>,
    refs: MrapiUint8, // the number of nodes currently attached (for reference counting)
    num_procs: MrapiUint8,
    processes: [MrapiUint8; MRAPI_MAX_PROCESSES], // the list of processes currently attached
}

// remote memory
struct MrapiRmem<'a> {
    valid: MrapiBoolean,
    access_type: MrapiRmemAtype,
    key: MrapiUint32, // the shared key passed in on get/create
    addr: *mut u8,
    size: usize,
    attributes: MrapiRmemAttributes<'a>,
    refs: MrapiUint8, // the number of nodes currently attached (for reference counting)
    nodes: [MrapiUint8; MRAPI_MAX_NODES], // the list of nodes currently attached
}

struct MrapiNodeState {
    node_num: MrapiUint,
    allocated: MrapiBoolean,
    valid: MrapiBoolean,
}

struct MrapiNodeData {
    //struct sigaction signals[MCA_MAX_SIGNALS];
    state: MrapiNodeState,
    tid: thread::ThreadId,
    proc_num: MrapiUint,
    sems: [MrapiUint8; MRAPI_MAX_SEMS], // list of sems this node is referencing
    shmems: [MrapiUint8; MRAPI_MAX_SHMEMS], // list of shmems this node is referencing
}

struct MrapiDomainState {
    domain_id: MrapiDomain,
    allocated: MrapiBoolean,
    valid: MrapiBoolean,
}

struct MrapiDomainData {
    num_nodes: MrapiUint16, // not decremented
    state: MrapiDomainState,
    nodes: [MrapiNodeData; MRAPI_MAX_NODES],
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
    source: MrapiInt, // handle source
}

union MrapiAtomicOperation {
    open: MrapiAtomicOpData,
    close: MrapiAtomicOpData,
    dup: MrapiAtomicOpData,
    sync: MrapiAtomicOpData,
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

struct MrapiProcessState {
    pid: MrapiUint32,
    allocated: MrapiBoolean,
    valid: MrapiBoolean,
}

// process address space
struct MrapiProcessData {
    state: MrapiProcessState,
    num_nodes: MrapiUint16,
    proc: MrapiInt, // process ID for duplicating shared memory
    link: [MrapiInt; MRAPI_MAX_PROCESSES], /* 1 if can be signaled */
    // signal SIGUSR1 indicates atomic operation signal */
    // struct sigaction atomic;
    op: MrapiAtomicOp,
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

struct MrapiDatabase<'a> {
  global_lock: Lock, // not used
  num_shmems: MrapiUint8,
  num_sems: MrapiUint8, // not decremented
  num_rmems: MrapiUint8, // not decremented
  num_domains: MrapiUint8, // not used
  num_processes: MrapiUint8, // not used
  shmems: [MrapiShmem<'a>; MRAPI_MAX_SHMEMS],
  sems: [MrapiSem<'a>; MRAPI_MAX_SEMS],
  sys_sems: [MrapiSysSem; MRAPI_MAX_SEMS],
  rmems: [MrapiRmem<'a>; MRAPI_MAX_RMEMS],
  domains: [MrapiDomainData; MRAPI_MAX_DOMAINS],
  requests: [MrapiRequestData; MRAPI_MAX_REQUESTS],
  processes: [MrapiProcessData; MRAPI_MAX_PROCESSES],
  // Rollover variables
  rollover_callbacks_ptr: [fn(); MRAPI_MAX_CALLBACKS],
  rollover_index: MrapiUint16,
  // Callback variables
  callbacks_array: [MrapiCallback; MRAPI_MAX_CALLBACKS],
  callback_index: MrapiUint16,
}

use mca_dprintf as mrapi_dprintf;

pub mod sysvr4 {
    pub mod os;
    pub mod key;
    pub mod sem;
    pub mod shmem;
}
