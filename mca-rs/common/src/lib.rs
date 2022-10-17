#![allow(dead_code)]
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

use std::sync::atomic::{AtomicUsize};

// Global debug setting
pub static MCA_DEBUG: AtomicUsize = AtomicUsize::new(0);
pub static MCA_DEBUG_INITIALIZED: AtomicUsize = AtomicUsize::new(0);

// MCA type definitions
pub type McaInt = isize;
pub type McaInt8 = i8;
pub type McaInt16 = i16;
pub type McaInt32 = i32;
pub type McaInt64 = i64;

pub type McaUint = usize;
pub type McaUint8 = u8;
pub type McaUint16 = u16;
pub type McaUint32 = u32;
pub type McaUint64 = u64;

pub type McaBoolean = usize;
pub type McaNode = usize;
pub type McaStatus = usize;
pub type McaTimeout = usize;
pub type McaDomain = usize;

// Constants
pub const MCA_TRUE: McaBoolean = 1;
pub const MCA_FALSE: McaBoolean = 0;
pub const MCA_NULL: McaUint  = 0; // MCA Zero value
pub const MCA_INFINITE: McaUint = !0; // Wait forever, no timeout
pub const MCA_RETURN_VALUE_INVALID: McaUint = !0;
pub const MCA_NODE_INVALID: McaUint = !0; 
pub const MCA_DOMAIN_INVALID: McaUint = !0;

pub mod logging;
pub mod crc;

fn block_signals() {
    os_impl::block_signals();
}

fn unblock_signals() {
    os_impl::unblock_signals();
}

// time measurement
#[derive(Debug)]
pub struct McaTimestamp {
    split_samples: u64, 
    split_sum: f64,
    perf: os_impl::McaPerf,
}

impl Default for McaTimestamp {
    fn default() -> Self {
	Self {
	    split_samples: 0,
	    split_sum: 0.0,
	    perf: os_impl::McaPerf::default(),
	}
    }
}

impl McaTimestamp {
    pub fn initialized(&self) -> bool {
	self.perf.initialized()
    }
}

// Get clock ID and starting timestamp for this process
fn begin_ts(ts: &mut McaTimestamp) {
    os_impl::begin_ts(ts);
}

// Get starting split timestamp for this process
fn begin_split_ts(ts: &mut McaTimestamp) {
    os_impl::begin_split_ts(ts);
}

// Get elapsed split time in microseconds
fn end_split_ts(ts: &mut McaTimestamp) -> f64 {
    os_impl::end_split_ts(ts)
}

// Get elapsed total time in microseconds
fn end_ts(ts: &mut McaTimestamp) -> f64 {
    os_impl::end_ts(ts)
}

#[allow(unused_imports)]
use more_asserts as ma;
#[allow(unused_imports)]
use std::{thread, time};

#[cfg(test)]
mod tests { 
    use super::*;

    #[test]
    fn signals() {
	block_signals();
	unblock_signals();
    }

    #[test]
    fn timestamp() {
	let mut ts: McaTimestamp = McaTimestamp::default();
	assert!(!ts.initialized());
	assert_eq!(0, ts.split_samples);
	assert_eq!(0.0, ts.split_sum);
	
	begin_ts(&mut ts);
	
	assert!(ts.initialized());
	assert_eq!(0, ts.split_samples);
	assert_eq!(0.0, ts.split_sum);

	let elapsed: f64 = end_ts(&mut ts);
	ma::assert_le!(0.0, elapsed);
    }

    #[test]
    fn split_timestamp() {
	
	let mut ts: McaTimestamp = McaTimestamp::default();
	assert!(!ts.initialized());
	assert_eq!(0, ts.split_samples);
	assert_eq!(0.0, ts.split_sum);
	
	begin_split_ts(&mut ts);
	
	assert!(ts.initialized());
	assert_eq!(0, ts.split_samples);
	assert_eq!(0.0, ts.split_sum);

	let elapsed: f64 = end_split_ts(&mut ts);
	
	ma::assert_le!(0.0, elapsed);
	assert_eq!(1, ts.split_samples);
	assert_eq!(elapsed, ts.split_sum);
    }

    #[test]
    fn latency() {
	let mut ts: McaTimestamp = McaTimestamp::default();
	begin_ts(&mut ts);
	begin_split_ts(&mut ts);

	let ten_millis = time::Duration::from_millis(10);
	thread::sleep(ten_millis);
	let interval = end_split_ts(&mut ts);

	thread::sleep(ten_millis);
	let total = end_ts(&mut ts);

	ma::assert_ge!(total, interval);
    }

    // https://www.geeksforgeeks.org/sieve-of-eratosthenes/
    #[allow(dead_code)]
    fn sieve_of_eratosthenes(n: usize) {
	// Create a boolean array "prime[0..n]" and initialize 
	// all entries in it as true. A value in prime[i] will 
	// finally be false if i is Not a prime, else true.
	let mut prime = vec![true; n+1];
	let mut p: usize = 2;
	while p * p <= n {
	    // If prime[p] is not changed, then it is a prime
	    if prime[p] == true {
		// Update all multiples of p greater than or equal to the square of it 
		// numbers which are multiple of p and are less than p^2 are already been marked.
		let mut i = p * p;
		while i <= n {
		    prime[i] = false;
		    i += p;
		}
	    }
	    p += 1;
	}

	// Print all prime numbers
	let mut buffer: String = "".to_owned();
	p = 2;
	while p <= n {
	    if prime[p] {
		if p > 2 {
		    let space: &str = " ";
		    buffer.push_str(space);
		}
		let prime_number = format!("{}", p);
		buffer.push_str(&prime_number);
	    }
	    p += 1;
	}
	println!("sieve: {}", buffer);
    }

    #[test]
    fn sieve() {
	sieve_of_eratosthenes(20);
    }
}











