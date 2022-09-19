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

use nix::unistd::{Pid};
use nix::time::{ClockId};
use libc::{clockid_t, timespec, c_long, time_t};

const MCA_MAGIC: u32 = 0x1234;

// time measurement
#[allow(dead_code)]
struct McaTimestamp {
    magic: u32,
    split_samples: u64, 
    split_sum: f64,
    clock_id: clockid_t,
    start: timespec,
    split_start: timespec,
}

#[allow(dead_code)]
impl McaTimestamp {
    fn initialize(&mut self) {
	self.magic = MCA_MAGIC;
    }
}

impl Default for McaTimestamp {
    fn default() -> Self {
	Self {
	    magic: 0,
	    split_samples: 0,
	    split_sum: 0.0,
	    clock_id: 0,
	    start: timespec {
		tv_sec: 0,
		tv_nsec: 0,
	    },
	    split_start: timespec {
		tv_sec: 0,
		tv_nsec: 0,
	    },
	}
    }
}

// Get clock ID and starting timestamp for this process
#[allow(dead_code)]
fn mca_begin_ts(ts: &mut McaTimestamp) {
    ts.initialize();
    
    let mut clock = ClockId::from_raw(ts.clock_id);
    match ClockId::pid_cpu_clock_id(Pid::this()) {
	Ok(val) => clock = val,
	Err(e) => println!("McaTimestamp pid_cpu_clock_id: {}", e),
    };
    ts.clock_id = clock.as_raw();

    match clock.now() {
	Ok(val) => {
	    ts.start.tv_sec = val.tv_sec();
	    ts.start.tv_nsec = val.tv_nsec();
	},
	Err(e) => println!("McaTimestamp start: {}", e),
    };
}

// Get starting split timestamp for this process
#[allow(dead_code)]
fn mca_begin_split_ts(ts: &mut McaTimestamp) {
    if MCA_MAGIC != ts.magic {
	mca_begin_ts(ts);
    };
    
    let clock = ClockId::from_raw(ts.clock_id);
    match clock.now() {
	Ok(val) => {
	    ts.split_start.tv_sec = val.tv_sec();
	    ts.split_start.tv_nsec = val.tv_nsec();
	},
	Err(e) => println!("McaTimestamp split_start: {}", e),
    };
}

// Get elapsed split time in microseconds
#[allow(dead_code)]
fn mca_end_split_ts(ts: &mut McaTimestamp) -> f64 {
    let mut elapsed: f64 = 0.0;
    if MCA_MAGIC == ts.magic {
	if 0 < ts.split_start.tv_nsec {
	    let clock = ClockId::from_raw(ts.clock_id);
	    match clock.now() {
		Ok(val) => {
		    let tmp_nsec: c_long = val.tv_nsec() - ts.split_start.tv_nsec;
		    if 0 < tmp_nsec {
			let tmp_sec: time_t = val.tv_sec() - ts.split_start.tv_sec - 1;
			elapsed = 1.0E9 * tmp_sec as f64;
			elapsed += 1.0E9 + tmp_nsec as f64;
		    }
		    else {
			let tmp_sec: time_t = val.tv_sec() - ts.split_start.tv_sec;
			elapsed = 1.0E9 * tmp_sec as f64;
			elapsed += tmp_nsec as f64;
		    };
		    elapsed /= 1.0E3; // microseconds
		    ts.split_start.tv_sec = val.tv_sec();
		    ts.split_start.tv_nsec = val.tv_nsec();

		    if 0.0 < elapsed {
			ts.split_samples += 1;
			ts.split_sum += elapsed;
		    }
		},
		Err(e) => println!("McaTimestamp split_end: {}", e),
	    }
	}
    }

    elapsed
}

// Get elapsed total time in microseconds
#[allow(dead_code)]
fn mca_end_ts(ts: &mut McaTimestamp) -> f64 {
    let mut elapsed: f64 = 0.0;
    if MCA_MAGIC == ts.magic {
	if 0 < ts.start.tv_nsec {
	    let clock = ClockId::from_raw(ts.clock_id);
	    match clock.now() {
		Ok(val) => {
		    let tmp_nsec: c_long = val.tv_nsec() - ts.start.tv_nsec;
		    if 0 < tmp_nsec {
			let tmp_sec: time_t = val.tv_sec() - ts.start.tv_sec - 1;
			elapsed = 1.0E9 * tmp_sec as f64;
			elapsed += 1.0E9 + tmp_nsec as f64;
		    }
		    else {
			let tmp_sec: time_t = val.tv_sec() - ts.start.tv_sec;
			elapsed = 1.0E9 * tmp_sec as f64;
			elapsed += tmp_nsec as f64;
		    };
		    elapsed /= 1.0E3; // microseconds
		},
		Err(e) => println!("McaTimestamp end: {}", e),
	    }
	}
    }

    elapsed
}

#[allow(unused_imports)]
use more_asserts as ma;
#[allow(unused_imports)]
use std::{thread, time};

#[cfg(test)]
mod tests { 
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

    use super::*;

    #[test]
    fn timestamp() {
	let timespec_zero = timespec {
	    tv_sec: 0,
	    tv_nsec: 0,
	};
	
	let mut ts: McaTimestamp = McaTimestamp::default();
	assert_eq!(0, ts.magic);
	assert_eq!(0, ts.split_samples);
	assert_eq!(0.0, ts.split_sum);
	assert_eq!(0, ts.clock_id);
	assert_eq!(timespec_zero, ts.start);
	assert_eq!(timespec_zero, ts.split_start);
	
	mca_begin_ts(&mut ts);
	
	assert_eq!(MCA_MAGIC, ts.magic);
	assert_eq!(0, ts.split_samples);
	assert_eq!(0.0, ts.split_sum);
	assert_ne!(0, ts.clock_id);
	assert_ne!(timespec_zero, ts.start);
	assert_eq!(timespec_zero, ts.split_start);

	let elapsed: f64 = mca_end_ts(&mut ts);
	ma::assert_lt!(0.0, elapsed);
    }

    #[test]
    fn split_timestamp() {
	let timespec_zero = timespec {
	    tv_sec: 0,
	    tv_nsec: 0,
	};
	
	let mut ts: McaTimestamp = McaTimestamp::default();
	assert_eq!(0, ts.magic);
	assert_eq!(0, ts.split_samples);
	assert_eq!(0.0, ts.split_sum);
	assert_eq!(0, ts.clock_id);
	assert_eq!(timespec_zero, ts.start);
	assert_eq!(timespec_zero, ts.split_start);
	
	mca_begin_split_ts(&mut ts);
	
	assert_eq!(MCA_MAGIC, ts.magic);
	assert_eq!(0, ts.split_samples);
	assert_eq!(0.0, ts.split_sum);
	assert_ne!(0, ts.clock_id);
	assert_ne!(timespec_zero, ts.start);
	assert_ne!(timespec_zero, ts.split_start);

	let elapsed: f64 = mca_end_split_ts(&mut ts);
	ma::assert_lt!(0.0, elapsed);
	assert_eq!(1, ts.split_samples);
	assert_eq!(elapsed, ts.split_sum);
    }

    #[test]
    fn latency() {
	let mut ts: McaTimestamp = McaTimestamp::default();
	mca_begin_ts(&mut ts);
	mca_begin_split_ts(&mut ts);

	let ten_millis = time::Duration::from_millis(10);
	thread::sleep(ten_millis);
	let interval = mca_end_split_ts(&mut ts);

	thread::sleep(ten_millis);
	let total = mca_end_ts(&mut ts);

	ma::assert_ge!(total, interval);
    }

    #[test]
    fn sieve() {
	sieve_of_eratosthenes(20);
    }
}
