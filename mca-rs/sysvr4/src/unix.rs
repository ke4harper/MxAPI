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

use std::ffi::{CString};
use std::mem::{MaybeUninit};
use errno::{
    Errno,
    errno,
    set_errno,
};
use nc::SEMVMX;
use nix::{
    libc::{
	sembuf,
	ftok, semget, semop, semctl,
	IPC_CREAT, IPC_EXCL,
	IPC_RMID,
	IPC_NOWAIT,
	EINTR, EAGAIN,
    },
};

use common::*;

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
    mca_dprintf!(1,"sysvr4::file_key: pathname: {} proj_id: {} key: {}", pathname, proj_id, newkey);

    Some(newkey as u32)
}

/// Create system semaphore
#[allow(dead_code)]
pub fn sem_create(key: u32, num_locks: usize) -> Option<u32> {
    const MAX_SEMAPHORES_PER_ARRAY: usize = SEMVMX as usize;
    if num_locks == 0 {
	mca_dprintf!(0, "sysvr4::sem_create failed key: {} num_locks ({}) must be greater than zero", key, num_locks);
	return None;
    }
    else if num_locks > MAX_SEMAPHORES_PER_ARRAY {
	mca_dprintf!(0, "sysvr4::sem_create failed key: {} num_locks ({}) requested is greater then OS supports", key, num_locks);
	return None;
    }

    // 1. create the semaphore
    let semid: i32 = unsafe { semget(key as i32, num_locks as i32 + 1, IPC_CREAT | IPC_EXCL | 0o777) };
    if semid == -1 {
	let err = errno();
	mca_dprintf!(1, "sysvr4::sem_create (create) failed: {}", err);
	return None;
    }

    // 2. initialize all members (Note: create and initialize are NOT atomic!)
    // This is why sem_get must check to make sure the sem is done with initialization
    // Use an additional semaphore set member as a block to prevent sem_get from
    // proceeding until initialization is complete. Start initialization at the
    // highest member for a better chance to avoid race conditions

    let mut sb: sembuf = unsafe { MaybeUninit::zeroed().assume_init() };
    
    sb.sem_num = 0;
    sb.sem_op = 1; // increment by 1
    sb.sem_flg = 0;

    let mut rc;
    for i in (0..num_locks + 1).rev() {
	sb.sem_num = i as u16;
	// do a semop() to initialize each semaphore in the set
	rc = unsafe { semop(semid, &mut sb, 1) };
	if rc == -1 {
	    let err = errno();
	    unsafe { semctl(semid, 0, IPC_RMID, 0) }; // clean up
	    set_errno(err);
	    mca_dprintf!(1, "sysvr4::sem_create (initialize: {}) failed: {}", i, err);
	    return None;
	}
    }

    // Release the highest member so competing sem_get can complete
    sb.sem_num = num_locks as u16;
    sb.sem_op = -1; // decrement back to zero
    rc = unsafe { semop(semid, &mut sb, 1) };
    if rc == -1 {
	let err = errno();
	unsafe { semctl(semid, 0, IPC_RMID, 0) }; // clean up
	set_errno(err);
	mca_dprintf!(1, "sysvr4::sem_create (complete) failed: {}", err);
	return None;
    }
    
    mca_dprintf!(1, "sysvr4::sem_create: key: {} num_locks: {}: id: {}", key, num_locks, semid);
    
    Some(semid as u32)
}

/// Open existing system semaphore
#[allow(dead_code)]
pub fn sem_get(key: u32, num_locks: usize) -> Option<u32> {
    let semid: i32 = unsafe { semget(key as i32, num_locks as i32 + 1, 0) }; // get the id
    if semid == -1 {
	let err = errno();
	mca_dprintf!(1, "sysvr4::sem_get failed key: {} num_locks: {} {}", key, num_locks, err);
	return None;
    }
    
    // At this point, process 2 will have to wait until the semaphore is initialized
    // by process 1. This is accomplished by blocking on the highest member in the
    // semaphore set which is initialized by sem_create and cleared when complete.

    let mut sb: sembuf = unsafe { MaybeUninit::zeroed().assume_init() };
    
    sb.sem_num = num_locks as u16;
    sb.sem_op = 0; // wait until member is zero
    sb.sem_flg = 0;

    let rc = unsafe { semop(semid, &mut sb, 1) };
    if rc == -1 {
	let err = errno();
	unsafe { semctl(semid, 0, IPC_RMID, 0) }; // clean up
	set_errno(err);
	mca_dprintf!(1, "sysvr4::sem_get (wait) failed: {}", err);
	return None;
    }

    mca_dprintf!(1, "sysvr4::sem_get: key: {} num_locks: {}: id: {}", key, num_locks, semid);

    Some(semid as u32)
}

/// Spin waiting to unlock semaphore set member
/// Return without retry if semaphore is deleted
#[allow(dead_code)]
fn sem_trylock(semref: SemRef) -> Result<bool, Errno> {
    let err: Errno;
    let id = semref.set;
    let member = semref.member;
    let spin = semref.spinlock_guard;
    let mut sem_lock: sembuf = sembuf {
	sem_num: member,
	sem_op: -1,
	sem_flg: IPC_NOWAIT as i16,
    };
    mca_dprintf!(4, "sysvr4::sem_trylock id: {} member: {} spin: {}", id, member, spin);
    loop {
	let rc = unsafe { semop(id, &mut sem_lock, 1) };
	if rc >= 0 {
	    return Ok(true);
	}
	else {
	    err = errno();
	    if rc == -1 && err.0 != EINTR {
		mca_dprintf!(3, "sysvr4::sem_trylock failed: {}", err);
		return Err(err);
	    }
	    else {
		mca_dprintf!(6, "sysvr4::sem_trylock attempt failed: {}", err);
	    }
	    break;
	}
    }

    Err(err)
}

/// Block until semaphore set member can be locked
#[allow(dead_code)]
pub fn sem_lock(semref: SemRef) -> Option<bool> {
    loop {
	match sem_trylock(semref) {
	    Ok(_) => {
		Some(true);
		break;
	    },
	    Err(e) => {
		if e.0 != EAGAIN {
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
pub fn sem_unlock(semref: SemRef) -> Option<bool> {
    let id = semref.set;
    let member = semref.member;
    let spin = semref.spinlock_guard;
    let mut sem_unlock: sembuf = sembuf {
	sem_num: member,
	sem_op: 1,
	sem_flg: 0,
    };
    mca_dprintf!(4, "sysvr4::sem_unlock id: {} member: {} spin: {}", id, member, spin);
    let rc = unsafe { semop(id, &mut sem_unlock, 1) };
    if rc == -1 {
	let err = errno();
	mca_dprintf!(1, "sysvr4::sem_unlock failed: {}", err);
	return None;
    }

    Some(true)
}

/// Release resources associated with an opened system semaphore
#[allow(dead_code)]
pub fn sem_release(semid: u32) {
    mca_dprintf!(1, "sysvr4::sem_release: id: {}", semid);
    // Noop for unix
}

/// Delete existing system semaphore
#[allow(dead_code)]
pub fn sem_delete(semid: u32) {
    mca_dprintf!(1, "sysvr4::sem_delete: id: {}", semid);
    if unsafe { semctl(semid as i32, 0, IPC_RMID, 0) } < 0 {
	let err = errno();
	mca_dprintf!(0, "sysvr4::sem_delete failed: {}", err);
    }
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
}
