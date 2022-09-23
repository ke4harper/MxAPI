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

// Operating system functions

use std::{thread};
use std::time::{Duration};

// Allow other threads to run
#[allow(dead_code)]
fn sys_os_yield() {
    thread::yield_now();
}

// Pause thread until time has elapsed
#[allow(dead_code)]
fn sys_os_usleep(microsecs: u64) {
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

const _RAND_MAX: i64 = 32767;
static mut _RANDOM: LateStatic::<SysRandom> = LateStatic::new();
static _RANDOM_MUTEX: Mutex<u64> = Mutex::new(0);
static mut _RANDOM_INITIALIZED: bool = false;

#[allow(unused_variables)]
impl SysRandom {
    // Execute assuming already under lock
    fn _srand(seed: u32) {
	unsafe {
	    let mut random = SysRandom::default();
	    random._rng = ChaCha8Rng::seed_from_u64(seed as u64);
	    if !_RANDOM_INITIALIZED {
		LateStatic::assign(&_RANDOM, random);
		_RANDOM_INITIALIZED = true;
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
	let mut _result: i64 = 0;
	let mutex_changer = _RANDOM_MUTEX.lock().unwrap();
	unsafe {
	    if !_RANDOM_INITIALIZED {
		SysRandom::_srand(1);
	    }

	    _result = _RANDOM._rng.gen_range(0.._RAND_MAX);
	}
	
	_result
    }
}

#[allow(dead_code)]
fn sys_os_srand(seed: u32) {
    SysRandom::srand(seed);
}

#[allow(dead_code)]
fn sys_os_rand() -> i64 {
    SysRandom::rand()
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    fn yield_thread() {
	sys_os_yield();
    }

    #[test]
    fn sleep_thread() {
	sys_os_usleep(10);
    }

    #[test]
    fn random() {
        let r1: i64 = sys_os_rand();
        sys_os_srand(1);
        assert_eq!(r1, sys_os_rand());
        sys_os_srand(20);
        let r2 = sys_os_rand();
        assert_ne!(r1, r2);
        sys_os_srand(20);
        assert_eq!(r2, sys_os_rand());
    }
}
