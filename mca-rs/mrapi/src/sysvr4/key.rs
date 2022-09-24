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

// Keys for operating system files

use crate::*;
use std::ffi::{CString};
use libc::{ftok};

#[allow(unused_imports)]
use more_asserts as ma;

#[allow(unused_assignments)]
pub fn sys_file_key(pathname: &str, proj_id: i32, key: &mut i32) -> MrapiBoolean {
    let mut rc = MRAPI_FALSE;
    static DEF: &str = "/dev/null";
    let file = if pathname.len() <= 0 {
	DEF
    }
    else {
	pathname
    };

    let cchar_file = CString::new(file).expect("CString::new failed");
    unsafe {
	let ftok_key = ftok(cchar_file.as_ptr(), proj_id);
	*key = ftok_key;
    };

    if key < &mut 0 {
	mrapi_dprintf!(0, "sys_file_key: pathname: {} proj_id: {} fail", file, proj_id);
    }
    else {
	mrapi_dprintf!(1, "sys_file_key: pathname: '{}' proj_id: {} key: {}", file, proj_id, key);
	rc = MRAPI_TRUE;
    }

    rc
}

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    fn file_key() {
	let mut key1 = -1;
	// Empty file
	assert_eq!(MRAPI_TRUE, sys_file_key("", 'c' as i32, &mut key1));
	ma::assert_lt!(0, key1);
	// Negative proj_id
	assert_eq!(MRAPI_FALSE, sys_file_key("/dev/null", -1, &mut key1));
	// Invalid file
	assert_eq!(MRAPI_FALSE, sys_file_key("/dev/null0", 'c' as i32, &mut key1));
	// Valid file
	assert_eq!(MRAPI_TRUE, sys_file_key("/dev/null", 'c' as i32, &mut key1));
	ma::assert_lt!(0, key1);
	// Repeatable key
	let mut key2 = -1;
	assert_eq!(MRAPI_TRUE, sys_file_key("/dev/null", 'c' as i32, &mut key2));
	ma::assert_lt!(0, key2);
	assert_eq!(key1, key2);
	// File variance
	assert_eq!(MRAPI_TRUE, sys_file_key("/etc/passwd", 'c' as i32, &mut key2));
	ma::assert_lt!(0, key2);
	assert_ne!(key1, key2);
	// proj_id variance
	assert_eq!(MRAPI_TRUE, sys_file_key("/dev/null", 'd' as i32, &mut key2));
	ma::assert_lt!(0, key2);
	assert_ne!(key1, key2);
    }
}
