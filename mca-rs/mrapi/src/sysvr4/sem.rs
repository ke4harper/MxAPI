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

use libc::{seminfo};
use crate::*;
use crate::sysvr4::key::{sys_file_key};

#[allow(unused_variables)]
pub fn sys_sem_create(key: i32, num_locks: i32, semid: &mut i32) -> MrapiBoolean {
    const SEM_INFO: seminfo = seminfo::default();
    const MAX_SEMAPHORES_PER_ARRAY: i32 = SEM_INFO.semmsl;
    
    let mut rc = MRAPI_FALSE;

    rc
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
	let mut key = 0;
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
