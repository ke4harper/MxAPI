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

// Shared memory

use crate::*;
use shared_memory::{ShmemConf, Shmem, ShmemError};

#[allow(unused_variables)]
pub fn sys_shmem_create(mapping: &str, size: usize) -> Result<Shmem, ShmemError> {
    match if mapping.len() <= 0 { ShmemConf::new().size(size).create() }
    else { ShmemConf::new().size(size).flink(mapping).force_create_flink().create() } {
	Ok(val) => {
	    mrapi_dprintf!(1, "sys_shmem_create: mapping: '{}' size: {}", mapping, size);
	    return sysvr4::shmem::ma::__core::result::Result::Ok(val);
	},
	Err(e) => {
	    if matches!(e, ShmemError::LinkExists) {
		mrapi_dprintf!(0, "sys_shmem_create (exists): mapping: '{}' size: {}", mapping, size);
		let val = ShmemConf::new().size(size).open().unwrap();
		return sysvr4::shmem::ma::__core::result::Result::Ok(val);
	    }
	    else {
		mrapi_dprintf!(0, "sys_shmem_create: mapping: '{}' size: {} {}", mapping, size, e);
		return sysvr4::shmem::ma::__core::result::Result::Err(e);
	    };
	},
    }
}

#[allow(unused_variables)]
pub fn sys_shmem_get(mapping: &str, size: usize) -> Result<Shmem, ShmemError> {
    if mapping.len() <= 0 {
	let e = ShmemError::NoLinkOrOsId;
	mrapi_dprintf!(0, "sys_shmem_get: mapping: '{}' size: {} {}", mapping, size, e);
	return sysvr4::shmem::ma::__core::result::Result::Err(e);
    };
    
    match ShmemConf::new().size(size).flink(mapping).open() {
	Ok(val) => {
	    mrapi_dprintf!(1, "sys_shmem_get: mapping: '{}' size: {}", mapping, size);
	    return sysvr4::shmem::ma::__core::result::Result::Ok(val);
	},
	Err(e) => {
	    mrapi_dprintf!(0, "sys_shmem_get: mapping: '{}' size: {} {}", mapping, size, e);
	    sysvr4::shmem::ma::__core::result::Result::Err(e)
	},
    }
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    #[allow(unused_variables)]
    fn os_shmem() {
	// Create shared memory
	let shm = match sys_shmem_get("test", 10) { // race with other process?
	    Ok(val) => val,
	    Err(_) => {
		match sys_shmem_create("test", 10) {
		    Ok(val) => val,
		    Err(_) => { panic!() },
		}
	    },
	};
	// Get existing shared memory
	_ = match sys_shmem_get("test", 10) {
	    Ok(_) => {} | Err(_) => { assert!(false) },
	};
    }
}
