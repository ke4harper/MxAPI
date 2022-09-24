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

use crate::*;
use crate::sysvr4::key::{sys_file_key};

use heliograph::{Key};

#[allow(unused_variables)]
pub fn sys_sem_create(key: i32, num_locks: i32, semid: &mut i32) -> MrapiBoolean {
    const MAX_SEMAPHORES_PER_ARRAY: i32 = 65536;
    if num_locks > MAX_SEMAPHORES_PER_ARRAY {
	mrapi_dprintf!(0, "sys_sem_create failed: num_locks requested is greater then the OS supports (SEMMSL)");
	return MRAPI_FALSE;
    }

    // 1. create the semaphore
    
    mrapi_dprintf!(1, "sys_sem_create (create)");
    
    unsafe {
	*semid = semget(key, num_locks, IPC_CREAT | IPC_EXCL | 0666);
	if *semid == -1 {
	    let str_slice: &str = CStr::from_ptr(strerror(errno().0)).to_str().unwrap();
	    mrapi_dprintf!(1, "sys_sem_create failed: errno={}", str_slice);
	    return MRAPI_FALSE;
	}
    }
    
    // 2. initialize all members (Note: create and initialize are NOT atomic!
    //   This is why semget must poll to make sure the sem is done with
    //   initialization
    
    mrapi_dprintf!(1, "sys_sem_create (initialize)");

    let sb: sembuf = sembuf::new();
    sb.sem_op = 1;
    sb.sem_flg = 0;

    for i in 0..num_locks {
	sb.sem_num = i;
	// do a semop() to "free" the semaphores.
	// this sets the sem_otime field, as needed below.
	unsafe {
	    let rc = semop(*semid, &sb, 1);
	    if rc == -1 {
		let e = errno;
		// clean up
		semctl(*semid, 0, IPC_RMID);
		errno = e;
		// error, check errno
		return MRAPI_FALSE;
	    }
	}

	sb.sem_num += 1;
    }

    MRAPI_TRUE
}

#[allow(unused_variables)]
pub fn sys_sem_get(key: i32, num_locks: i32, semid: &mut i32) -> MrapiBoolean {
    let mut rc = MRAPI_FALSE;

    rc
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    fn os_sem() {
	let mut Key = new Key::private();
	assert_eq!(MRAPI_TRUE, sys_file_key("", 'd' as i32, &mut key));
	// Empty set
	let mut id = 0;
	assert_eq!(MRAPI_FALSE, sys_sem_create(key, 0, &mut id));
	// Single semaphore
	let mut created = 0;
	if MRAPI_FALSE == sys_sem_get(key, 1, &mut id) {
	    created = 1;
	    assert_eq!(MRAPI_TRUE, sys_sem_create(key, 1, &mut id));
	}
	assert_ne!(0, id);
    }
}
