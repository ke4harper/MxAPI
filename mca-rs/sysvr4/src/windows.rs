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

use std::{
    ptr,
    fmt,
    mem::{
	//size_of,
	MaybeUninit,
    },
    /*
    fs::{
	File,
	OpenOptions,
    },
    io::{
	ErrorKind,
    },
    ffi::{
	c_void,
    },
    */
};

use errno::{
    Errno,
    errno,
    set_errno,
};
use libc::{
    EALREADY,
    EAGAIN,
    EINVAL,
    EINTR,
};
use widestring::U16CString;

use windows_sys::Win32::{
    Foundation::{
	MAX_PATH,
	INVALID_HANDLE_VALUE,
	WAIT_TIMEOUT, WAIT_FAILED, WAIT_ABANDONED,
	HANDLE,
	CloseHandle,
    },
    System::{
	Environment::ExpandEnvironmentStringsW,
	Threading::{
	    SEMAPHORE_MODIFY_STATE,
	    OpenSemaphoreW,
	    CreateSemaphoreW,
	    ReleaseSemaphore,
	    WaitForSingleObject,
	},
    },
    Storage::FileSystem::{
	FILE_READ_ATTRIBUTES,
	OPEN_EXISTING,
	SYNCHRONIZE,
	FILE_ATTRIBUTE_NORMAL,
	BY_HANDLE_FILE_INFORMATION,
	CreateFileW,
	GetFileInformationByHandle,
    },
};

/// Generate unique integer key
#[allow(dead_code)]
pub fn os_file_key(mut pathname: &str, proj_id: u32) -> Option<u32> {
    // Use file ID from specified path directory (embedded environment
    // variables allowed), XORed with repeated 8 least significant bits
    // of proj_id.
    let mut newkey = u32::MAX;
    const DEF: &str = "%WINDIR%\\system.ini";
    if pathname.len() <= 0 {
        pathname = DEF;
    }

    let pathwstr: U16CString = U16CString::from_str(pathname).unwrap();
    // Expand environment variables
    let mut localwstr = vec![0; MAX_PATH as usize + 1];
    let nlen = unsafe { ExpandEnvironmentStringsW(pathwstr.as_ptr(), localwstr.as_mut_ptr(), MAX_PATH) };

    if nlen != 0 {
	// Get file handle, must be read accessible
	let hfile: HANDLE; 
	unsafe {
	    hfile = CreateFileW(localwstr.as_ptr(),
				    FILE_READ_ATTRIBUTES, 0, ptr::null(),
				    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	}
	if hfile != INVALID_HANDLE_VALUE {
	    unsafe {
		let mut fileinfo: BY_HANDLE_FILE_INFORMATION =
		    MaybeUninit::<BY_HANDLE_FILE_INFORMATION>::zeroed().assume_init();
		if GetFileInformationByHandle(hfile, &mut fileinfo) != 0 {
		    // File is accessible
		    newkey = fileinfo.nFileIndexHigh + fileinfo.nFileIndexLow;
		    CloseHandle(hfile);
		    // Mask least significant 8 bits and create XOR value
		    let mut xor = 0;
		    let mut code = proj_id & 0x000000FF;
		    for _i in 0..4 {
			xor |= code;
			code <<= 8;
		    }
		    newkey ^= xor;
		}
	    }
	}
    }
    if newkey == u32::MAX {
	return None;
    }
    
    mca_dprintf!(1, "sysvr4::file_key: pathname: {}, proj_id: {}, key: {:#X}", pathname, proj_id, newkey);

    Some(newkey)
}

/// Internal semaphore set representation
#[derive(Eq)]
pub struct SemSet {
    pub key: u32,
    pub num_locks: usize,
    id: Vec<u32>,
}

impl Default for SemSet {
    fn default() -> SemSet {
	SemSet {
	    key: 0,
	    num_locks: 0,
	    id: Vec::<u32>::default(),
	}
    }
}

impl PartialEq for SemSet {
    fn eq(&self, other: &Self) -> bool {
	self.key == other.key
	    && self.num_locks == other.num_locks
    }
}

impl fmt::Debug for SemSet {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
	let mut semlist: String = String::from(format!("["));
	if self.id.len() > 0 {
	    semlist.push_str(&format!("{}", self.id[1]));
	    for i in 2..self.id.len() {
		semlist.push_str(&format!(", {}", self.id[i]));
	    }
	}
	semlist.push_str(&format!("]"));
	write!(f, "SemSet {{ key: {}, num_locks: {}, sem: {} }}", self.key, self.num_locks, semlist)
    }
}

impl Drop for SemSet {
    fn drop(&mut self) {
	if self.id.len() <= 0 {
	    return;
	}
	
	let id = self.id[0];
	
	let result = unsafe { WaitForSingleObject(id as HANDLE, 0) };
	match result {
	    WAIT_TIMEOUT => {
		mca_dprintf!(2, "sysvr4::SemSet::drop: id: {}; WAIT_TIMEOUT", id);
		// Reference count is zero
		for i in 0..self.id.len() {
		    unsafe {
			CloseHandle(self.id[i] as HANDLE);
		    }
		}
	    },
	    WAIT_FAILED | WAIT_ABANDONED => {
		mca_dprintf!(2, "sysvr4::SemSet::drop: id: {}; WAIT_FAILED / _ABANDONED", id);
	    },
	    _ => {
		mca_dprintf!(2, "sysvr4::SemSet::drop: id: {}; OK", id);
	    },
	}

	mca_dprintf!(2, "sysvr4::SemSet::drop: id: {}", id);
    }
}

impl SemSet {
    /// Allocate empty semaphore set
    fn new(key: u32, num_locks: usize) -> Option<SemSet> {
	let mut ss: SemSet = SemSet::default();
	ss.key = key;
	ss.num_locks  = num_locks;
	// First semaphore is used as reference counter
	ss.id = Vec::<u32>::with_capacity(num_locks + 1);

	Some(ss)
    }

    /// Add semaphore to set
    fn add(&mut self, id: u32) {
	self.id.push(id);
    }
}

/// Create system semaphore
#[allow(dead_code)]
pub fn sem_create(key: u32, num_locks: usize) -> Option<SemSet> {
    if num_locks <= 0 {
	mca_dprintf!(1, "sysvr4::sem_create: key: {}, num_locks: {}; Number of locks is not supported", key, num_locks);
	return None;
    }
	
    // Check if semaphore is already in use
    // Named semaphores can be shared between local processes
    let semstr: String = String::from(format!("Local\\mca_{}_{}", key, 0));
    let semwstr: U16CString = U16CString::from_str(semstr).unwrap();
    let mut hsem = unsafe { OpenSemaphoreW(
	SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,
	0, semwstr.as_ptr()
    ) };
    if hsem != 0 {
	set_errno(Errno(EALREADY));
	let err = errno();
	mca_dprintf!(1, "sysvr4::sem_create: key: {}, num_locks: {}; Create failed, {}", key, num_locks, err);
	return None;
    }
	
    // Allocate the semaphore set
    let mut ss = match SemSet::new(key, num_locks) {
	Some(val) => val,
	None => { return None; },
    };

    // Initialize the semaphores in the set
    for i in 0..ss.num_locks + 1 {
	let lockstr = String::from(format!("Local\\mca_{}_{}", key, i));
	let lockwstr = U16CString::from_str(lockstr).unwrap();
	// Create first semaphore in locked state (ref count zero) for guard, the rest free
	let locked = if i == 0 { 0 } else { 1 };
	hsem = unsafe { CreateSemaphoreW(ptr::null(), locked, 1, lockwstr.as_ptr()) };
	if hsem == 0 {
	    let err = errno();
	    mca_dprintf!(1, "sysvr4::sem_create: key: {}, num_locks: {}; Create failed, {}", key, num_locks, err);
	    return None;
	}
	ss.add(hsem as u32);
    }

    // Increment reference count so competing sem_get can complete
    let mut prev: i32 = 0;
    let _rc = unsafe { ReleaseSemaphore(ss.id[0] as HANDLE, 1, &mut prev) };

    mca_dprintf!(1, "sysvr4::sem_create: {:?}", ss);

    Some(ss)
}

/// Open existing system semaphore
#[allow(dead_code)]
#[allow(unused_variables)]
pub fn sem_get(key: u32, num_locks: usize) -> Option<SemSet> {
    // Allocate the semaphore set
    let mut ss = match SemSet::new(key, num_locks) {
	Some(val) => val,
	None => { return None; },
    };

    // Collect the semaphores in the set
    for i in 0..ss.num_locks + 1 {
	let semstr: String = String::from(format!("Local\\mca_{}_{}", key, i));
	let semwstr: U16CString = U16CString::from_str(semstr).unwrap();
	let hsem = unsafe { OpenSemaphoreW(
	    SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,
	    0, semwstr.as_ptr()
	) };
	if hsem == 0 {
	    let err = errno();
	    mca_dprintf!(1, "sysvr4::sem_get: key: {}, num_locks: {}, member: {}; failed, {}", key, num_locks, i, err);
	    return None;
	}
	ss.add(hsem as u32);
    }

    // At this point, process 2 will have to wait until the semaphore is initialized
    // by process 1. This is accomplished by blocking on the first member in the
    // semaphore set which is initialized by sem_create to zero and incremented when complete.
    let mut rc = 0;
    loop {
	let result = unsafe { WaitForSingleObject(ss.id[0] as HANDLE, 0) };
	match result {
	    WAIT_TIMEOUT => {
		rc = -1;
		set_errno(Errno(EAGAIN));
	    },
	    WAIT_FAILED | WAIT_ABANDONED => {
		rc = -1;
		set_errno(Errno(EINVAL));
	    },
	    _ => {},
	}
	if rc >= 0 {
	    break;
	}
	else {
	    let err = errno();
	    if err.0 != EINTR {
		mca_dprintf!(3, "sysvr4::sem_get: key: {}, num_locks: {}; {}", key, num_locks, err);
		return None;
	    }
	    mca_dprintf!(6, "sysvr4::sem_get: key: {}, num_locks: {}; {}", key, num_locks, err);
	}
    }
    
    // Increment reference count by two to replace what was consumed by wait and account for get
    let mut prev: i32 = 0;
    let _rc = unsafe { ReleaseSemaphore(ss.id[0] as HANDLE, 2, &mut prev) };

    mca_dprintf!(1, "sysvr4::sem_get: {:?}", ss);

    Some(ss)
}

/// Spin waiting to unlock semaphore set member
/// Return without retry if semaphore is deleted
#[allow(dead_code)]
pub fn sem_trylock(semref: &SemRef) -> Result<bool, Errno> {
    let mut err: Errno;
    let ss = &semref.sem.set;
    let member = semref.member;
    let spin = semref.spinlock_guard;
    mca_dprintf!(4, "sysvr4::sem_trylock: set: {:?}, member: {}, spin: {}", ss, member, spin);
    let mut rc = 0;
    loop {
	let result = unsafe { WaitForSingleObject(ss.id[member as usize +1] as HANDLE, 0) };
	match result {
	    WAIT_TIMEOUT => {
		rc = -1;
		set_errno(Errno(EAGAIN));
	    },
	    WAIT_FAILED | WAIT_ABANDONED => {
		rc = -1;
		set_errno(Errno(EINVAL));
	    },
	    _ => {},
	}
	if rc >= 0 {
	    mca_dprintf!(1, "sysvr4::sem_trylock: set: {:?}, member: {}, spin: {}", ss, member, spin);
	    return Ok(true);
	}
	else {
	    err = errno();
	    if err.0 != EINTR {
		mca_dprintf!(3, "sysvr4::sem_trylock: set: {:?}, member: {}, spin: {}; {}", ss, member, spin, err);
		break;
	    }
	    mca_dprintf!(6, "sysvr4::sem_get: trylock: set: {:?}, member: {}, spin: {}; {}", ss, member, spin, err);
	}
    }
    
    Err(err)
}

/// Block until semaphore set member can be locked
#[allow(dead_code)]
pub fn sem_lock(semref: &SemRef) -> Option<bool> {
    loop {
	match sem_trylock(semref) {
	    Ok(_) => {
		return Some(true);
	    },
	    Err(e) => {
		println!("err: {} {}", e.0, EAGAIN);
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
pub fn sem_unlock(semref: &SemRef) -> Option<bool> {
    let ss = &semref.sem.set;
    let member = semref.member;
    let spin = semref.spinlock_guard;
    mca_dprintf!(4, "sysvr4::sem_unlock ss: {:?}, member: {}, spin: {}", ss, member, spin);
    let mut prev: i32 = 0;
    let rc = unsafe { ReleaseSemaphore(ss.id[member as usize +1] as HANDLE, 1, &mut prev) };
    if rc == 0 {
	let err = errno();
	mca_dprintf!(1, "sysvr4::sem_unlock: set: {:?}, member: {}, spin: {}; {}", ss, member, spin, err);
	return None;
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
	match os_file_key("%WINDIR%\\system.foo", 'c' as u32) {
	    Some(_) => { assert!(false) },
	    None => { assert!(true) },
	}
        // Valid file
	let key1 = os_file_key("%WINDIR%\\system.ini", 'c' as u32).unwrap();
        ma::assert_lt!(0, key1);
        // File variance
	let key2 = os_file_key("%WINDIR%\\win.ini", 'd' as u32).unwrap();
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
	    for i in 0..NUM_LOCKS + 1 {
		assert_ne!(ss1.id[i], ss2.id[i]);
	    }
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
	    let sr = SemRef::new(Semaphore::new(set1), 0, false);
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
