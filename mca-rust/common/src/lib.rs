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

/// Common functions

use std::sync::atomic::{AtomicUsize, Ordering};

// Global debug setting
static _MCA_DEBUG: AtomicUsize = AtomicUsize::new(0);
static _MCA_DEBUG_INITIALIZED: AtomicUsize = AtomicUsize::new(0);

/// Get global debug level
#[macro_export]
macro_rules! mca_debug {
    // Accessor
    () => {{
	{
	    use std::env;

	    let initialized = _MCA_DEBUG_INITIALIZED.load(Ordering::Relaxed);
	    if initialized <= 0 {
		let key = "MCA_DEBUG";
		match env::var(key) {
		    Ok(val) => _MCA_DEBUG.store(val.parse::<usize>().expect("$MCA_DEBUG is not a number"), Ordering::Relaxed),
		    Err(_) => _MCA_DEBUG.store(0, Ordering::Relaxed),
		}
		_MCA_DEBUG_INITIALIZED.store(1, Ordering::Relaxed);
	    }

	    let level = _MCA_DEBUG.load(Ordering::Relaxed);
	    
	    level
	}
    }};
    // Mutator
    ($level: expr) => {{
	{
	    _MCA_DEBUG.store($level, Ordering::Relaxed);
	    _MCA_DEBUG_INITIALIZED.store(1, Ordering::Relaxed);
	}
    }};
}

/// mca_dformat! - diagnostic formatted message with variable args
/// Formatting string is template for args
#[macro_export]
macro_rules! mca_dformat {
    // No arguments
    ($fmt: expr) => {{
	{
	    println!($fmt);
	}
    }};
    // Variable number of arguments
    ($fmt: expr, $($args: tt), *) => {{
	{
	    let msg = format!($fmt, $($args), *);
	    println!("{}", msg);
	}
    }};
}

/// mca_dprintf! - diagnostic print filtered by level
/// Debug level threshold is set with environment variable MCA_DEBUG
/// Formatting string is template for args
#[macro_export]
macro_rules! mca_dprintf {
    // No format and variable arguments
    ($level: expr) => {{
	{
	    use std::thread;
	    use std::process;
	    
	    if $level as usize <= mca_debug!() {
		print!("/* MCA PID:{} {:?} */   //", process::id(), thread::current().id());
	    }
	}
    }};
    // Additional message
    ($level: expr, $msg: expr) => {{
	mca_dprintf! { $level }
	mca_dformat! { $msg }
    }};
    // Variable formatted arguments
    ($level: expr, $fmt: expr, $($args: tt), *) => {{
	mca_dprintf! { $level }
	mca_dformat! { $fmt, $($args), * }
    }};
}

/// mca_set_debug_level - programmically control diagnostic reporting verbosity
fn mca_set_debug_level(level: usize) {
    mca_debug!(level);
}

/// crc32_compute_buf - computes the CRC-32 value of a memory buffer
//
//       Computes or accumulates the CRC-32 value for a memory buffer.
//       The in_crc32 gives a previously accumulated CRC-32 value to allow
//       a CRC to be generated for multiple sequential buffer-fuls of data.
//       The in_crc32 for the first buffer must be zero.
//
//  Parameters:
//       in_crc32 - accumulated CRC-32 value, must be 0 on first call
//       buf     - buffer to compute CRC-32 value for
//       buf_len  - number of bytes in buffer
//
//  Returns: crc32 - computed CRC-32 value
//
fn crc32_compute_buf(_in_crc32: u32, _buf: &u8, _buf_len: usize) -> u32 {
    println!("/// ERROR: replace crc32_compute_buf with native crc [3.0.0]");

    0
}

// https://www.jameselford.com/blog/working-with-signals-in-rust-pt1-whats-a-signal/
fn mca_block_signals() {
    println!("/// ERROR: replace mca_block_signals functionaliity with native signal_hook");
}

fn mca_unblock_signals() {
    println!("/// ERROR: replace mca_unblock_signals functionality with native signal_hook");
}

use nix::unistd::{Pid};
use nix::time::{ClockId};
use libc::{clockid_t, timespec, c_long, time_t};

const _MCA_MAGIC: u32 = 0x1234;

// real time measurement
struct McaTimestamp {
  magic: u32,
  split_samples: u64, 
  split_sum: f64,
  clock_id: clockid_t,
  start: timespec,
  split_start: timespec,
}

impl McaTimestamp {
    fn initialize(&mut self) {
	self.magic = _MCA_MAGIC;
	self.split_samples = 0;
	self.split_sum = 0.0;
	self.clock_id = 0;
	self.start = timespec {
	    tv_sec: 0,
	    tv_nsec: 0,
	};
	self.split_start = timespec {
	    tv_sec: 0,
	    tv_nsec: 0,
	};
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
fn mca_begin_split_ts(ts: &mut McaTimestamp) {
    if _MCA_MAGIC != ts.magic {
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
fn mca_end_split_ts(ts: &mut McaTimestamp) -> f64 {
    let mut elapsed: f64 = 0.0;
    if _MCA_MAGIC == ts.magic {
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
fn mca_end_ts(ts: &mut McaTimestamp) -> f64 {
    let mut elapsed: f64 = 0.0;
    if _MCA_MAGIC == ts.magic {
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

use more_asserts as ma;

#[cfg(test)]
mod tests { 
    use super::*;

    #[test]
    fn print_banner() {
	mca_dprintf!(0);
    }

    #[test]
    fn print_msg() {
	mca_dprintf!(0, "This is a message");
    }

    #[test]
    fn print_args() {
	mca_dprintf!(0, "{} {}", "This is an arg", 2);
    }

    #[test]
    fn set_debug_level() {
	mca_set_debug_level(1);
	assert_eq!(1, mca_debug!());
	mca_set_debug_level(0);
    }

    #[test]
    fn mca_timestamp() {
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
	
	assert_eq!(_MCA_MAGIC, ts.magic);
	assert_eq!(0, ts.split_samples);
	assert_eq!(0.0, ts.split_sum);
	assert_ne!(0, ts.clock_id);
	assert_ne!(timespec_zero, ts.start);
	assert_eq!(timespec_zero, ts.split_start);

	let elapsed: f64 = mca_end_ts(&mut ts);
	ma::assert_lt!(0.0, elapsed);
    }

    #[test]
    fn mca_split_timestamp() {
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
	
	assert_eq!(_MCA_MAGIC, ts.magic);
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
}
