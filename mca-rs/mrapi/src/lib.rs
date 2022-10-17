#![allow(dead_code)]
#![feature(thread_local)]
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
use crate::sysvr4::sem::SemRef;

use common::*;

use std::thread;
use std::collections::{LinkedList};

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

pub type MrapiSemRef<'a> = SemRef<'a>;

// Constants
pub const MRAPI_TRUE: MrapiBoolean = MCA_TRUE;
pub const MRAPI_FALSE: MrapiBoolean = MCA_FALSE;
pub const MRAPI_NULL: MrapiUint = MCA_NULL;

// error codes
pub enum MrapiStatusFlag {
    MrapiSuccess,
    MrapiTimeout,
    MrapiIncomplete,
    MrapiErrAttrNum,
    MrapiErrAttrReadonly,
    MrapiErrAttrSize,
    MrapiErrDbNotInitialized,
    MrapiErrProcessNotRegistered,
    MrapiErrSysSemaphoreFail,
    MrapiErrDomainInvalid,
    MrapiErrDomainNotshared,
    MrapiErrMemLimit,
    MrapiErrMutexDeleted,
    MrapiErrMutexExists,
    MrapiErrMutexIdInvalid,
    MrapiErrMutexInvalid,
    MrapiErrMutexKey,
    MrapiErrMutexLimit,
    MrapiErrMutexLocked,
    MrapiErrMutexLockorder,
    MrapiErrMutexNotlocked,
    MrapiErrMutexNotvalid,
    MrapiErrNodeInitfailed,
    MrapiErrNodeFinalfailed,
    MrapiErrNodeInitialized,
    MrapiErrNodeInvalid,
    MrapiErrNodeNotinit,
    MrapiErrNotSupported,
    MrapiErrParameter,
    MrapiErrRequestCanceled,
    MrapiErrRequestInvalid,
    MrapiErrRequestLimit,
    MrapiErrRmemIdInvalid,
    MrapiErrRmemAttach,
    MrapiErrRmemAttached,
    MrapiErrRmemAtype,
    MrapiErrRmemAtypeNotvalid,
    MrapiErrRmemBlocked,
    MrapiErrRmemBuffOverrun,
    MrapiErrRmemConflict,
    MrapiErrRmemExists,
    MrapiErrRmemInvalid,
    MrapiErrRmemNotattached,
    MrapiErrRmemNotowner,
    MrapiErrRmemStride,
    MrapiErrRmemTypenotvalid,
    MrapiErrRsrcCounterInuse,
    MrapiErrRsrcInvalid,
    MrapiErrRsrcInvalidCallback,
    MrapiErrRsrcInvalidEvent,
    MrapiErrRsrcInvalidSubsystem,
    MrapiErrRsrcInvalidTree,
    MrapiErrRsrcNotdynamic,
    MrapiErrRsrcNotowner,
    MrapiErrRsrcNotstarted,
    MrapiErrRsrcStarted,
    MrapiErrRwlDeleted,
    MrapiErrRwlExists,
    MrapiErrRwlIdInvalid,
    MrapiErrRwlInvalid,
    MrapiErrRwlLimit,
    MrapiErrRwlLocked,
    MrapiErrRwlNotlocked,
    MrapiErrSemDeleted,
    MrapiErrSemExists,
    MrapiErrSemIdInvalid,
    MrapiErrSemInvalid,
    MrapiErrSemLimit,
    MrapiErrSemLocked,
    MrapiErrSemLocklimit,
    MrapiErrSemNotlocked,
    MrapiErrShmAttached,
    MrapiErrShmAttch,
    MrapiErrShmExists,
    MrapiErrShmIdInvalid,
    MrapiErrShmInvalid,
    MrapiErrShmNodesIncompat,
    MrapiErrShmNodeNotshared,
    MrapiErrShmNotattached,
    MrapiErrAtomInvalidArg,
    MrapiErrAtomOpFailed,
    MrapiErrAtomOpNoforward,
}

enum Attributes {
    MrapiMutexRecursive(MrapiBoolean),
    MrapiErrorExt(MrapiBoolean),
    MrapiDomainShared(MrapiBoolean),
    MrapiSpinlockGuard(MrapiBoolean),
    MrapiShmemResource,
    MrapiShmemAddress,
    MrapiShmemSize,
}

enum MrapiRsrcMemAttrs {
    MrapiRsrcMemBaseaddr,
    MrapiRsrcMemNumwords,
    MrapiRsrcMemWordsize,
}

enum MrapiRsrcCacheAttrs {
    MrapiRsrcCacheSize,
    MrapiRsrcCacheLineSize,
    MrapiRsrcCacheAssociativity,
    MrapiRsrcCacheLevel,
}

enum MrapiRsrcCpuAttrs {
    MrapiRsrcCpuFrequency,
    MrapiRsrcCpuType,
    MrapiRsrcCpuId,
}

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
struct MrapiAtomicSync {
  pindex: MrapiUint, // buffer index reference
  last_counter: MrapiUint, // non-blocking buffer previous state
  hold: MrapiBoolean, // MRAPI_TRUE if valid msg flag should be retained
                      // across atomic calls; last call must release
  onetime: MrapiBoolean, // one-time override to allow specified mode
  active: MrapiBoolean, // MRAPI_TRUE if caller holds sync
}

// sync descriptor for non-Windows cross-process atomic operations
struct MrapiAtomicBarrier {
  mode: MrapiAtomicMode,
  xchg: MrapiBoolean, // read only when valid, write only when invalid;
                      // flip flag state on completion
  timeout: MrapiTimeout,
  src: MrapiUint32, // local pid
  dest: MrapiUint32, // remote pid; local proc == remote proc, not sync required
  elems: MrapiUint, // number of buffer elements
  size: usize, // element size
  sync: MrapiAtomicSync,
  buffer: LinkedList<MrapiMsg>, // messages
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

struct MrapiResource {
    name: String,
    resource_type: MrapiResourceType,
    number_of_children: MrapiUint32,
    children: LinkedList<MrapiResource>,
    number_of_attributes: MrapiUint32,
    attribute_types: LinkedList<RsrcType>,                        
    attribute_values: LinkedList<RsrcValue>,                        
    attribute_static: LinkedList<MrapiAttributeStatic>,
    attribute_started: MrapiBoolean,               
}

union AttribValue {
    byte: MrapiUint8,
    word: MrapiUint16,
    long: MrapiUint32,
}

struct MrapiImplAttributes {
    ext_error_checking: MrapiBoolean,
    shared_across_domains: MrapiBoolean,
    recursive: MrapiBoolean,  // only applies to mutexes
    spinlock_guard: MrapiBoolean,
    value: Attributes,
    resource: MrapiResource,
}

type MrapiMutexAttributes = MrapiImplAttributes;
type MrapiSemAttributes = MrapiImplAttributes;
type MrapiRwlAttributes = MrapiImplAttributes;
type MrapiShmemAttributes = MrapiImplAttributes;
type MrapiRmemAttributes = MrapiImplAttributes;

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

use mca_dprintf as mrapi_dprintf;

/// mrapi_assert! - exit on failure
macro_rules! mrapi_assert {
    ($condition: expr) => {{
	if MRAPI_FALSE == $condition {
	    eprintln!("INTERNAL ERROR: MRAPI failed assertion ({}:{}) shutting down\n", file!, line!);
	    //mrapi_impl_free_resources(1/*panic*/);
	    exit(1);
	}
    }};
}

pub mod sysvr4 {
    pub mod os;
    pub mod key;
    pub mod sem;
    pub mod shmem;
}

pub mod internal {
    pub mod db;
    pub mod lifecycle;
}
