///
/// Copyright(c) 2023, Karl Eric Harper
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

use std::{
    ffi::{
	CString,
    },
    mem::{
	MaybeUninit,
    },
    fmt,
};

use errno::{
    errno,
};
use nc::{
    types::{
	sembuf_t,
    },
    SEMVMX,
    SETVAL,
    GETVAL,
    semctl,
    semop,
};
use nix::{
    libc::{
	ftok, semget,
	IPC_CREAT, IPC_EXCL,
	IPC_RMID,
	IPC_NOWAIT,
	EINTR, EAGAIN,
    },
};

/// Generate unique integer key
#[allow(dead_code)]
pub fn os_file_key(mut pathname: &str, proj_id: u32) -> Option<u32>  {
    const DEF: &str = "/dev/null";
    if pathname.len() <= 0 {
        pathname = DEF;
    }

    let pathcstr: CString = CString::new(pathname).unwrap();
    let newkey: i32 = unsafe { ftok(pathcstr.as_ptr(), proj_id as i32) };
    if newkey == -1 {
	return None;
    }
    mca_dprintf!(1,"sysvr4::file_key: pathname: {}, proj_id: {}, key: {:#X}", pathname, proj_id, newkey);

    Some(newkey as u32)
}

const SEMSET_MAX_LOCKS: usize = 32;

/// Internal semaphore set representation
pub struct SemSet {
    pub key: u32,
    pub num_locks: usize,
    id: i32,
    //spin: SharedMem<[Arc<AtomicU32>; SEMSET_MAX_LOCKS]>,
}

impl Default for SemSet {
    fn default() -> SemSet {
	SemSet {
	    key: 0,
	    num_locks: 0,
	    id: 0,
	    //spin: SharedMem::default(),
	}
    }
}

impl PartialEq for SemSet {
    fn eq(&self, other: &Self) -> bool {
	self.key == other.key
	    && self.num_locks == other.num_locks
    }
}

impl Eq for SemSet {}

impl fmt::Debug for SemSet {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
	write!(f, "SemSet {{ key: {:#X}, num_locks: {}, id: {} }}", self.key, self.num_locks, self.id)
    }
}

impl Drop for SemSet {
    fn drop(&mut self) {

	let id = self.id;

	// Decrement the reference count
	let mut decrement: [sembuf_t; 1] = unsafe { MaybeUninit::zeroed().assume_init() };
	decrement[0].sem_num = 0;
	decrement[0].sem_op = -1;
	decrement[0].sem_flg = IPC_NOWAIT as i16;
	unsafe {
	    match semop(id, &mut decrement) {
		Ok(v) => v,
		Err(err) => {
		    mca_dprintf!(1, "sysvr4::SemSet::drop: {:?}; decrement failed: {}", self, err);
		},
	    }
	};

	// Get the current reference count, if zero then delete the resource
	let val = unsafe { semctl(id, 0, GETVAL, 0).unwrap() };
	mca_dprintf!(2, "sysvr4::SemSet::drop: id: {}, ref: {}", id, val);
	if val <= 0 {
	    match unsafe { semctl(id as i32, 0, IPC_RMID, 0) } {
		Ok(_) => { },
		Err(err) => {
		    mca_dprintf!(0, "sysvr4::SemSet::drop: {:?}; delete failed: {}", self, err);
		},
	    }
	};
    }
}

impl SemSet {
    /// Allocate empty semaphore set
    fn new(key: u32, num_locks: usize, id: u32) -> Option<SemSet> {
	let mut ss: SemSet = SemSet::default();
	ss.key = key;
	ss.num_locks  = num_locks;
	ss.id = id as i32;

	if num_locks > SEMSET_MAX_LOCKS {
	    mca_dprintf!(1, "sysvr4::SemSet::new: {:?}: Maximum number of locks ({}) exceeded", ss, SEMSET_MAX_LOCKS);
	    
	    None
	}
	else {
/*
	    // Create shared memory for spin locks
	    let mgr = match shmem_get::<[Arc<AtomicU32>; 32]>(key) {
		Some(v) => v,
		None => { // race condition with another process?
		    let obj = match shmem_create::<[Arc<AtomicU32>; SEMSET_MAX_LOCKS]>(key) {
			Some(v) => v,
			None => {
			    mca_dprintf!(1, "sysvr4::SemSet::new: {:?}: Cannot create spinlock shared memory", ss);
			    SharedMem::default()
			},
		    };
		    obj
		},
	    };
	    ss.spin = mgr;
*/

	    mca_dprintf!(6, "sysvr4::SemSet::new: {:?}", ss);

	    Some(ss)
	}
    }
}

/// Create system semaphore
#[allow(dead_code)]
pub fn sem_create(key: u32, num_locks: usize) -> Option<SemSet> {
    const MAX_SEMAPHORES_PER_ARRAY: usize = SEMVMX as usize;
    if num_locks <= 0 || num_locks > MAX_SEMAPHORES_PER_ARRAY {
	mca_dprintf!(1, "sysvr4::sem_create: key: {:#X}, num_locks: {}; Number of locks is not supported", key, num_locks);
	return None;
    }
	
    // 1. create the semaphore
    let semid: i32 = unsafe { semget(key as i32, num_locks as i32 + 1, IPC_CREAT | IPC_EXCL | 0o777) };
    if semid == -1 {
	let err = errno();
	mca_dprintf!(1, "sysvr4::sem_create: key: {:#X}, num_locks: {}; Create failed, {}", key, num_locks, err);
	return None;
    }

    // Create the semaphore set
    let ss = match SemSet::new(key, num_locks, semid as u32) {
	Some(val) => val,
	None => { return None; },
    };

    // 2. initialize all members (Note: create and initialize are NOT atomic!)
    // This is why sem_get must check to make sure the sem is done with initialization
    // Use the first member as a reference counter and only increment when the semaphore
    // set ready to be used. This handles the sem_create / sem_get race condition.

    for i in 0..num_locks + 1 {
	// the first member is used as a reference counter
	let lock = if i == 0 { 0 } else { 1 };
	match unsafe { semctl(semid, i as i32, SETVAL, lock) } {
	    Ok(_) => { },
	    Err(err) => {
		unsafe { semctl(semid, 0, IPC_RMID, 0).unwrap() }; // clean up
		mca_dprintf!(1, "sysvr4::sem_create (initialize: {}) failed: {}", i, err);
		return None;
	    },
	}
    }

    // Increment the first member so a competing sem_get can complete
    match unsafe { semctl(semid, 0, SETVAL, 1) } {
	Ok(_) => { },
	Err(err) => {
	    unsafe { semctl(semid, 0, IPC_RMID, 0).unwrap() }; // clean up
	    mca_dprintf!(1, "sysvr4::sem_create (complete) failed: {}", err);
	    return None;
	},
    }

    mca_dprintf!(1, "sysvr4::sem_create: {:?}", ss);
    
    Some(ss)
}

/// Open existing system semaphore
#[allow(dead_code)]
pub fn sem_get(key: u32, num_locks: usize) -> Option<SemSet> {
    let semid: i32 = unsafe { semget(key as i32, num_locks as i32 + 1, 0) }; // get the id
    if semid == -1 {
	let err = errno();
	mca_dprintf!(1, "sysvr4::sem_get failed key: {:#X}, num_locks: {} {}", key, num_locks, err);
	return None;
    }

    // At this point, process 2 will have to wait until the semaphore is initialized
    // by process 1. This is accomplished by spinning on on the first member in the
    // semaphore set which is set to zero by sem_create and incremented to one when
    // initialization is complete.

    loop {
	let val = unsafe { semctl(semid, 0, GETVAL, 0).unwrap() };
	if val > 0 {
	    break;
	}
	else {
	    os_yield();
	}
    }

    // Increment the reference count
    let mut increment: [sembuf_t; 1] = unsafe { MaybeUninit::zeroed().assume_init() };
    increment[0].sem_num = 0;
    increment[0].sem_op = 1;
    increment[0].sem_flg = 0;
    unsafe { semop(semid, &mut increment).unwrap() };

    // Create the semaphore set
    let ss = match SemSet::new(key, num_locks, semid as u32) {
	Some(val) => val,
	None => { return None; },
    };

    mca_dprintf!(1, "sysvr4::sem_get: {:?}", ss);

    Some(ss)
}

/// Spin waiting to unlock semaphore set member
/// Return without retry if semaphore is deleted
#[allow(dead_code)]
pub fn sem_trylock(semref: &SemRef) -> Result<bool, Errno> {
    let ss = &semref.sem.set;
    let member = semref.member;
    let spin = semref.spinlock_guard;
    
    let mut lock: [sembuf_t; 1] = unsafe { MaybeUninit::zeroed().assume_init() };
    lock[0].sem_num = member + 1;
    lock[0].sem_op = -1;
    lock[0].sem_flg = IPC_NOWAIT as i16;

    loop {
	match unsafe { semop(ss.id, &mut lock) } {
	    Ok(_) => { break; },
	    Err(e) => {
		if e != EINTR {
		    mca_dprintf!(3, "sysvr4::sem_trylock: set: {:?}, member: {}, spin: {}; {}", ss, member, spin, e);
		    return Err(Errno(e));
		}
		
		mca_dprintf!(6, "sysvr4::sem_trylock: set: {:?}, member: {}, spin: {}; Attempt failed", ss, member, spin);
	    },
	}
    }
    
    mca_dprintf!(1, "sysvr4::sem_trylock: set: {:?}, member: {}, spin: {}", ss, member, spin);

    Ok(true)
}

/// Block until semaphore set member can be locked
#[allow(dead_code)]
pub fn sem_lock(semref: &SemRef) -> Option<bool> {
    loop {
	match sem_trylock(semref) {
	    Ok(v) => {
		return Some(v);
	    },
	    Err(e) => {
		if e != Errno(EAGAIN) {
		    mca_dprintf!(2, "sysvr4::sem_lock attempt failed: {}", e);
		    break;
		}
		else {
		    os_yield();
		}
	    },
	}
    }

    None
}

/// Unlock semaphore set member
#[allow(dead_code)]
pub fn sem_unlock(semref: &SemRef) -> Option<bool> {
    let ss = &semref.sem.set;
    let member = semref.member;
    let spin = semref.spinlock_guard;

    let mut unlock: [sembuf_t; 1] = unsafe { MaybeUninit::zeroed().assume_init() };
    unlock[0].sem_num = member + 1;
    unlock[0].sem_op = 1;
    unlock[0].sem_flg = 0;
    
    mca_dprintf!(4, "sysvr4::sem_unlock ss: {:?}, member: {}, spin: {}", ss, member, spin);
    match unsafe { semop(ss.id, &mut unlock) } {
	Ok(_) => { },
	Err(err) => {
	    mca_dprintf!(1, "sysvr4::sem_unlock: set: {:?}, member: {}, spin: {}; {}", ss, member, spin, err);
	    return None;
	},
    }

    mca_dprintf!(1, "sysvr4::sem_unlock: set: {:?}, member: {}, spin: {}", ss, member, spin);

    Some(true)
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn key() {
        // Invalid file
	match os_file_key("/dev/null0", 'c' as u32) {
	    Some(_) => { assert!(false) },
	    None => { assert!(true) },
	}
        // Valid file
	let key1 = os_file_key("/dev/null", 'c' as u32).unwrap();
        ma::assert_lt!(0, key1);
        // File variance
	let key2 = os_file_key("/etc/passwd", 'd' as u32).unwrap();
        assert_ne!(key2, key1);
    }
	
    #[test]
    fn semaphore() {
	const NUM_LOCKS: usize = 2;
	// Get semaphore
	{
	    let key = os_file_key("", 'e' as u32).unwrap();
            ma::assert_lt!(0, key);
	    let ss1 = match sem_get(key, NUM_LOCKS) {
		Some(v) => v,
		None => { // race condition with another process?
		    match sem_create(key, NUM_LOCKS) {
			Some(v) => v,
			None => {
			    assert!(false);
			    SemSet::default()
			},
		    }
		},
	    };
	    let ss2 = match sem_get(key, NUM_LOCKS) {
		Some(v) => v,
		None => {
		    assert!(false);
		    SemSet::default()
		},
	    };
	    assert!(ss1 == ss2);
	    assert!(ss1.id == ss2.id);
	}
	
	// Lock and unlock semaphore member
	{
	    let key = os_file_key("", 'g' as u32).unwrap();
            ma::assert_lt!(0, key);
	    let set1 = match sem_get(key, 1) {
		Some(v) => v,
		None => { // race condition with another process?
		    match sem_create(key, 1) {
			Some(v) => v,
			None => {
			    assert!(false);
			    SemSet::default()
			},
		    }
		},
	    };
	    let sr = SemRef::new(&Semaphore::new(set1), 0, false);
	    match sem_trylock(&sr) {
		Ok(_) => { assert!(true) }, // lock succeeds
		Err(_) => { assert!(false) },
	    }
	    match sem_trylock(&sr) {
		Ok(_) => { assert!(false) }, // lock fails
		Err(_) => { assert!(true) },
	    }
	    match sem_unlock(&sr) {
		Some(_) => { assert!(true) }, // unlock succeeds
		None => { assert!(false) },
	    }
	}
    }
}
