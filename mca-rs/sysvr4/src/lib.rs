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

use cfg_if::cfg_if;

//Load up the proper OS implementation
cfg_if! {
    if #[cfg(target_os="windows")] {
        mod windows;
        use windows as os_impl;
    } else if #[cfg(any(target_os="freebsd", target_os="linux", target_os="macos"))] {
        mod unix;
        use crate::unix as os_impl;
    } else {
        compile_error!("shared_memory isnt implemented for this platform...");
    }
}

// Operating system functions

use std::{thread};
use std::time::{Duration};

/// Allow other threads to run
#[allow(dead_code)]
pub fn os_yield() {
    thread::yield_now();
}

/// Pause thread until time has elapsed
#[allow(dead_code)]
pub fn os_usleep(microsecs: u64) {
    let period = Duration::from_micros(microsecs);
    thread::sleep(period);
}

use late_static::{LateStatic};
use std::sync::{Mutex};
use rand::prelude::*;
use rand_chacha::{ChaCha8Rng};

struct SysRandom {
    _rng: ChaCha8Rng,
}

impl Default for SysRandom {
    fn default() -> Self {
	Self {
	    _rng: ChaCha8Rng::seed_from_u64(1),
	}
    }
}

static mut _RANDOM: LateStatic::<SysRandom> = LateStatic::new();
static _RANDOM_MUTEX: Mutex<u64> = Mutex::new(0);

#[allow(unused_variables)]
impl SysRandom {
    // Execute assuming already under lock
    fn _srand(seed: u32) {
	unsafe {
	    let mut random = SysRandom::default();
	    random._rng = ChaCha8Rng::seed_from_u64(seed as u64);
	    if !LateStatic::has_value(&_RANDOM) {
		LateStatic::assign(&_RANDOM, random);
	    }
	    else {
		_RANDOM._rng = random._rng;
	    }
	}
    }
    fn srand(seed: u32) {
	// Execute with lock
	let mutex_changer = _RANDOM_MUTEX.lock().unwrap();
	SysRandom::_srand(seed);
    }
    fn rand() -> i64 {
	const _RAND_MAX: i64 = 32767;
	
	let mut _result: i64 = 0;
	let mutex_changer = _RANDOM_MUTEX.lock().unwrap();
	unsafe {
	    if !LateStatic::has_value(&_RANDOM) {
		SysRandom::_srand(1);
	    }

	    _result = _RANDOM._rng.gen_range(0.._RAND_MAX);
	}
	
	_result
    }
}

/// Seed random number generator
#[allow(dead_code)]
pub fn os_srand(seed: u32) {
    SysRandom::srand(seed);
}

/// Generate random number
#[allow(dead_code)]
pub fn os_rand() -> i64 {
    SysRandom::rand()
}

/// Generate unique integer key
pub fn os_file_key(pathname: &str, proj_id: u32) -> Option<u32>  {
    os_impl::os_file_key(pathname, proj_id)
}

/// OS opaque semaphore
#[derive(Debug)]
#[derive(Eq, PartialEq)]
pub struct Semaphore {
    set: os_impl::SemSet,
}

impl Default for Semaphore {
    fn default() -> Self {
	Semaphore {
	    set: os_impl::SemSet::default(),
	}
    }
}

#[allow(dead_code)]
impl Semaphore {
    /// Create new semaphore set instance
    fn new(set: os_impl::SemSet) -> Self {
	Semaphore {
	    set: set,
	}
    }
}

/// Reference specific member of semaphore set
#[allow(dead_code)]
pub struct SemRef {
    sem: Semaphore,
    member: u16,
    spinlock_guard: bool,
}

impl Default for SemRef {
    fn default() -> Self {
	SemRef {
	    sem: Semaphore::default(),
	    member: 0,
	    spinlock_guard: false,
	}
    }
}

#[allow(dead_code)]
impl SemRef {
    /// Create semaphore reference instance
    fn new(sem: Semaphore, member: u16, spin: bool) -> Self {
	SemRef {
	    sem: sem,
	    member: member,
	    spinlock_guard: spin,
	}
    }

    /// Block until semaphore set member can be locked
    fn lock(&self) -> Option<bool> {
	os_impl::sem_lock(self)
    }

    /// Unlock semaphore set member
    fn unlock(&self) -> Option<bool> {
	os_impl::sem_unlock(self)
    }
}

/// Create system semaphore
pub fn sem_create(key: u32, num_locks: usize) -> Option<Semaphore> {
    match os_impl::sem_create(key, num_locks) {
	Some(ss) => Some(Semaphore::new(ss)),
	None => None,
    }
}

/// Open existing system semaphore
pub fn sem_get(key: u32, num_locks: usize) -> Option<Semaphore> {
    match os_impl::sem_get(key, num_locks) {
	Some(ss) => Some(Semaphore::new(ss)),
	None => None,
    }
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    fn yield_thread() {
	os_yield();
    }

    #[test]
    fn sleep_thread() {
	os_usleep(10);
    }

    #[test]
    fn random() {
        let r1: i64 = os_rand();
	os_srand(1);
        assert_eq!(r1, os_rand());
	os_srand(20);
        let r2 = os_rand();
        assert_ne!(r1, r2);
        os_srand(20);
        assert_eq!(r2, os_rand());
    }

    #[test]
    fn key() {
        // Empty file
	let empty_path: &str = "";
	let key1 = os_file_key(empty_path, 'c' as u32).unwrap();
        ma::assert_lt!(0, key1);
        // Repeatable key
	let mut key2 = os_file_key(empty_path, 'c' as u32).unwrap();
        assert_eq!(key2, key1);
        // proj_id variance
	key2 = os_file_key(empty_path, 'd' as u32).unwrap();
        assert_ne!(key2, key1);
    }

    #[test]
    fn semaphore() {
	{
	    let key = os_file_key("", 'c' as u32).unwrap();
            ma::assert_lt!(0, key);
	    // Empty set
	    match sem_create(key, 0) {
		Some(_) => { assert!(false) },
		None => { assert!(true) },
	    }
	    // Single semaphore
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
	    assert_eq!(1, sem1.set.num_locks);
	    // Duplicate create fails
	    match sem_create(key, 1) {
		Some(_) => { assert!(false) },
		None => { assert!(true); },
	    };
	    // Get adds to reference count
	    let _sem2 = match sem_get(key, 1) {
		Some(v) => v,
		None => {
		    assert!(false);
		    Semaphore::default()
		},
	    };
	    // Semaphore set
	    let sem3 = match sem_get(key + 2, 2) {
		Some(v) => v,
		None => { // race condition with another process?
		    match sem_create(key + 2, 2) {
			Some(v) => v,
			None => {
			    assert!(false);
			    Semaphore::default()
			},
		    }
		},
	    };
	    assert_eq!(2, sem3.set.num_locks);
	}

	// Lock and unlock semaphore member
	{
	    let key = os_file_key("", 'f' as u32).unwrap();
            ma::assert_lt!(0, key);
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
	    let sr = SemRef::new(sem1, 0, false);
	    match sr.lock() {
		Some(_) => { assert!(true) }, // lock succeeds
		None => { assert!(false) },
	    }
	    match sr.unlock() {
		Some(_) => { assert!(true) }, // unlock succeeds
		None => { assert!(false) },
	    }
	}
    }
}
