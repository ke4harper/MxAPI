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

// Semaphores

const MAX_SEMAPHORES_PER_ARRAY: usize = 65536;

use crate::*;
#[allow(unused_imports)]
use crate::sysvr4::key::{sys_file_key};

use std::{io};
use heliograph::{Key, Semaphore, Exclusive, Mode};

#[allow(unused_variables)]
pub fn sys_sem_create(key: Key, num_locks: usize) -> io::Result<Semaphore> {
    // 1. create the semaphore
    match Semaphore::create(key, num_locks, Exclusive::Yes, Mode::from_bits(0o666).unwrap()) {
	Ok(val) => {
	    mrapi_dprintf!(1, "sys_sem_create: key: {:?} num_locks: {} {:?}", key, num_locks, val);
	    // 2. initialize all members (Note: create and initialize are NOT atomic!
	    //    This is why sys_sem_get must poll to make sure the sem is done with
	    //    initialization
	    for i in 0..num_locks {
		val.op(&[val.at(i as u16).add(1)]).unwrap();
	    };
	    return sysvr4::sem::ma::__core::result::Result::Ok(val);
	},
	Err(e) => {
	    mrapi_dprintf!(0, "sys_sem_create: key: {:?} num_locks: {} {}", key, num_locks, e);
	    return sysvr4::sem::ma::__core::result::Result::Err(e);
	},
    }
}

#[allow(unused_variables)]
pub fn sys_sem_get(key: Key, num_locks: usize) -> io::Result<Semaphore> {
    const _MAX_RETRIES: u32 = 0xffff;
    let sem = match Semaphore::open(key, num_locks) {
	Ok(val) => {
	    mrapi_dprintf!(1, "sys_sem_get: key: {:?} num_locks: {} {:?}", key, num_locks, val);
	    // At that point, will have to wait until the semaphore is initialized by creating process
	    let mut ready = false;
	    for i in 0.._MAX_RETRIES {
		if ready { break; }
		match val.get_all() {
		    Ok(set) => {
			for lock in set {
			    if lock <= 0 { break; }
			}
			ready = true;
		    },
		    Err(e) => {},
		};
	    }
	    sysvr4::sem::ma::__core::result::Result::Ok(val)
	},
	Err(e) => {
	    mrapi_dprintf!(0, "sys_sem_get: key: {:?} num_locks: {} {}", key, num_locks, e);
	    sysvr4::sem::ma::__core::result::Result::Err(e)
	},
    };

    sem
}

#[allow(unused_variables)]
pub fn sys_sem_duplicate(sem: &Semaphore) -> io::Result<Semaphore> {
    let sem = match sem.try_clone() {
	Ok(val) => {
	    mrapi_dprintf!(1, "sys_sem_duplicate: sem: {:?} {:?}", sem, val);
	    sysvr4::sem::ma::__core::result::Result::Ok(val)
	},
	Err(e) => {
	    mrapi_dprintf!(0, "sys_sem_duplicate: sem: {:?} {}", sem, e);
	    sysvr4::sem::ma::__core::result::Result::Err(e)
	},
    };

    sem
}

#[allow(unused_variables)]
pub fn sys_sem_trylock(sem: &Semaphore) -> io::Result<()> {
    // Only operates on first in set
    let result = match sem.op(&[sem.at(0).remove(1).wait(false)]) {
	Ok(val) => {
	    mrapi_dprintf!(1, "sys_sem_trylock: sem: {:?}", sem);
	    return sysvr4::sem::ma::__core::result::Result::Ok(val);
	},
	Err(e) => {
	    mrapi_dprintf!(0, "sys_sem_trylock: sem: {:?} {}", sem, e);
	    return sysvr4::sem::ma::__core::result::Result::Err(e);
	},
    };
}

#[allow(unused_variables)]
pub fn sys_sem_lock(sem: &Semaphore) -> io::Result<()> {
    loop {
	match sys_sem_trylock(sem) {
	    Ok(val) => {
		mrapi_dprintf!(1, "sys_sem_lock: sem: {:?} {:?}", sem, val);
		return sysvr4::sem::ma::__core::result::Result::Ok(val);
	    },
	    Err(e) => {
		match e.kind() {
		    other_error => {
			println!("Error kind: {}", other_error);
		    }
		}
		return sysvr4::sem::ma::__core::result::Result::Err(e);
	    },
	}
    }
}

#[allow(unused_variables)]
pub fn sys_sem_unlock(sem: &Semaphore) -> io::Result<()> {
    // Only operates on first in set
    let result = match sem.op(&[sem.at(0).add(1)]) {
	Ok(val) => {
	    mrapi_dprintf!(1, "sys_sem_unlock: sem: {:?} {:?}", sem, val);
	    sysvr4::sem::ma::__core::result::Result::Ok(val)
	},
	Err(e) => {
	    mrapi_dprintf!(0, "sys_sem_unlock: sem: {:?} {}", sem, e);
	    sysvr4::sem::ma::__core::result::Result::Err(e)
	},
    };

    result
}

#[allow(unused_variables)]
pub fn sys_sem_delete(sem: Semaphore) {
    drop(sem);
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    #[allow(unused_assignments)]
    fn os_sem() {
	let key1 = sys_file_key("", 'd' as i32).unwrap();
	// Invalid set
	match sys_sem_create(key1, usize::MAX) {
	    Ok(_) => assert!(false),
	    Err(_) => assert!(true),
	}
	let mut created = false;
	// Single semaphore
	let sem1 = match sys_sem_get(key1, 1) { // race with other process?
	    Ok(val) => {
		created = false;
		val
	    },
	    Err(_) => {
		match sys_sem_create(key1, 1) {
		    Ok(val) => {
			created = true;
			val
		    },
		    Err(_) => { panic!() },
		}
	    },
	};
	// Duplicate create fails
	_ = match sys_sem_create(key1, 1) {
	    Ok(_) => { assert!(false) } | Err(_) => {},
	};
	if created {
	    sys_sem_delete(sem1);
	}
	// Semaphore set
	let key2 = sys_file_key("", 'e' as i32).unwrap();
	let sem2 = match sys_sem_get(key2, 2) { // race with other process?
	    Ok(val) => {
		created = false;
		val
	    },
	    Err(_) => {
		match sys_sem_create(key2, 2) {
		    Ok(val) => {
			created = true;
			val
		    },
		    Err(_) => { panic!() },
		}
	    },
	};
	// Get semaphore
	_ = match sys_sem_get(key2, 2) {
	    Ok(_) => {},
	    Err(_) => { assert!(false) },
	};
	if created {
	    sys_sem_delete(sem2);
	}
	// Duplicate semaphore
	let key3 = sys_file_key("", 'f' as i32).unwrap();
	let sem3 = match sys_sem_get(key3, 2) { // race with other process?
	    Ok(val) => {
		created = false;
		val
	    },
	    Err(_) => {
		match sys_sem_create(key3, 2) {
		    Ok(val) => {
			created = true;
			val
		    },
		    Err(_) => { panic!() },
		}
	    },
	};
	_ = match sys_sem_duplicate(&sem3) {
	    Ok(_) => {} | Err(_) => { assert!(false) },
	};
	if created {
	    sys_sem_delete(sem3);
	}
	// Lock and unlock semaphore
	let key4 = sys_file_key("", 'g' as i32).unwrap();
	let sem4 = match sys_sem_get(key4, 1) { // race with other process?
	    Ok(val) => {
		created = false;
		val
	    },
	    Err(_) => {
		match sys_sem_create(key4, 1) {
		    Ok(val) => {
			created = true;
			val
		    },
		    Err(_) => { panic!() },
		}
	    },
	};
	_ = match sys_sem_trylock(&sem4) {
	    Ok(_) => {} | Err(_) => { assert!(false) },
	};
	// Locks exhausted
	_ = match sys_sem_trylock(&sem4) {
	    Ok(_) => { assert!(false) } | Err(_) => {},
	};
	_ = match sys_sem_unlock(&sem4) {
	    Ok(_) => {} | Err(_) => { assert!(false) },
	};
	_ = match sys_sem_lock(&sem4) {
	    Ok(_) => {} | Err(_) => { assert!(false) },
	};
	_ = match sys_sem_unlock(&sem4) {
	    Ok(_) => {} | Err(_) => { assert!(false) },
	};
	if created {
	    sys_sem_delete(sem4);
	}
	// Lock and unlock multiple semaphores
	let mut key: Vec<Key> = Vec::with_capacity(2);
    }
}
