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

/// Reference specific member of semaphore set
#[allow(dead_code)]
#[derive(Copy, Clone)]
pub struct SemRef {
    set: i32,
    member: u16,
    spinlock_guard: bool,
}

/// Create system semaphore
pub fn sem_create(key: u32, num_locks: usize) -> Option<u32> {
    os_impl::sem_create(key, num_locks)
}

/// Open existing system semaphore
pub fn sem_get(key: u32, num_locks: usize) -> Option<u32> {
    os_impl::sem_get(key, num_locks)
}

/// Release attachment to existing system semaphore
pub fn sem_release(semid: u32) {
    os_impl::sem_release(semid)
}

/// Delete existing system semaphore
pub fn sem_delete(semid: u32) {
    os_impl::sem_delete(semid)
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
	let key = os_file_key("", 'c' as u32).unwrap();
        ma::assert_lt!(0, key);
	// Empty set
	match sem_create(key, 0) {
	    Some(_) => { assert!(false) },
	    None => { assert!(true) },
	}
	// Single semaphore
	let mut created = false;
	let id1 = match sem_get(key, 1) {
	    Some(v) => v,
	    None => { // race condition with another process?
		created = true;
		match sem_create(key, 1) {
		    Some(v) => v,
		    None => {
			assert!(false);
			0
		    },
		}
	    },
	};
	ma::assert_le!(0, id1);
	// Duplicate create fails
	match sem_create(key, 1) {
	    Some(_) => { assert!(false) },
	    None => { assert!(true); },
	};
	// Clean up semaphore set with single member
	if created {
	    sem_delete(id1);
	}
	else {
	    sem_release(id1);
	}
    }
}
