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

use std::mem::{zeroed};
use libc::{
    sigset_t, sigemptyset, sigaddset, sigprocmask
	,SIGALRM, SIGINT, SIGHUP, SIGILL
	,SIGSEGV, SIGTERM, SIGFPE, SIGABRT
	,SIG_BLOCK, SIG_UNBLOCK
};

use nix::unistd::{Pid};
use nix::time::{ClockId};
use libc::{clockid_t, timespec, c_long, time_t};

use super::*;

// https://www.jameselford.com/blog/working-with-signals-in-rust-pt1-whats-a-signal/
pub fn block_signals() {
    unsafe {
	let mut block_alarm: sigset_t = zeroed();
	sigemptyset(&mut block_alarm);
	sigaddset(&mut block_alarm, SIGALRM);
	sigaddset(&mut block_alarm, SIGINT);
	sigaddset(&mut block_alarm, SIGHUP);
	sigaddset(&mut block_alarm, SIGILL);
	sigaddset(&mut block_alarm, SIGSEGV);
	sigaddset(&mut block_alarm, SIGTERM);
	sigaddset(&mut block_alarm, SIGFPE);
	sigaddset(&mut block_alarm, SIGABRT);
	let mut prev_alarm: sigset_t = zeroed();
	sigprocmask(SIG_BLOCK, &mut block_alarm, &mut prev_alarm);
    }
}

pub fn unblock_signals() {
    unsafe {
	let mut block_alarm: sigset_t = zeroed();
	sigemptyset(&mut block_alarm);
	sigaddset(&mut block_alarm, SIGALRM);
	sigaddset(&mut block_alarm, SIGINT);
	sigaddset(&mut block_alarm, SIGHUP);
	sigaddset(&mut block_alarm, SIGILL);
	sigaddset(&mut block_alarm, SIGSEGV);
	sigaddset(&mut block_alarm, SIGTERM);
	sigaddset(&mut block_alarm, SIGFPE);
	sigaddset(&mut block_alarm, SIGABRT);
	let mut prev_alarm: sigset_t = zeroed();
	sigprocmask(SIG_UNBLOCK, &mut block_alarm, &mut prev_alarm);
    }
}

const MCA_MAGIC: u32 = 0x1234;


// OS specific
#[derive(Debug)]
pub struct McaPerf {
    magic: u32,
    clock_id: clockid_t,
    start: timespec,
    split_start: timespec,
}

impl Default for McaPerf {
    fn default() -> Self {
	Self {
	    magic: 0,
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

impl McaPerf {
    fn initialize(&mut self) {
	self.magic = MCA_MAGIC;
	// One time access to clock frequency
	let mut clock = ClockId::from_raw(0);
	match ClockId::pid_cpu_clock_id(Pid::this()) {
	    Ok(val) => clock = val,
	    Err(e) => mca_dprintf!(0, "McaTimestamp pid_cpu_clock_id: {}", e),
	};
	self.clock_id = clock.as_raw();
    }

    pub fn initialized(&self) -> bool {
	self.magic == MCA_MAGIC
    }
}

// Get clock ID and starting timestamp for this process
pub fn begin_ts(ts: &mut McaTimestamp) {
    ts.perf.initialize();
    
    let clock = ClockId::from_raw(ts.perf.clock_id);
    match clock.now() {
	Ok(val) => {
	    ts.perf.start.tv_sec = val.tv_sec();
	    ts.perf.start.tv_nsec = val.tv_nsec();
	},
	Err(e) => mca_dprintf!(0, "McaTimestamp start: {}", e),
    };
}

// Get starting split timestamp for this process
pub fn begin_split_ts(ts: &mut McaTimestamp) {
    if !ts.perf.initialized() {
	begin_ts(ts);
    };
    
    let clock = ClockId::from_raw(ts.perf.clock_id);
    match clock.now() {
	Ok(val) => {
	    ts.perf.split_start.tv_sec = val.tv_sec();
	    ts.perf.split_start.tv_nsec = val.tv_nsec();
	},
	Err(e) => mca_dprintf!(0, "McaTimestamp split_start: {}", e),
    };
}

// Get elapsed split time in microseconds
pub fn end_split_ts(ts: &mut McaTimestamp) -> f64 {
    let mut elapsed: f64 = 0.0;
    // Cannot process split times unless initialized
    if ts.perf.initialized() {
	if 0 < ts.perf.split_start.tv_nsec {
	    let clock = ClockId::from_raw(ts.perf.clock_id);
	    match clock.now() {
		Ok(val) => {
		    let tmp_nsec: c_long = val.tv_nsec() - ts.perf.split_start.tv_nsec;
		    if 0 < tmp_nsec {
			let tmp_sec: time_t = val.tv_sec() - ts.perf.split_start.tv_sec - 1;
			elapsed = 1.0E9 * tmp_sec as f64;
			elapsed += 1.0E9 + tmp_nsec as f64;
		    }
		    else {
			let tmp_sec: time_t = val.tv_sec() - ts.perf.split_start.tv_sec;
			elapsed = 1.0E9 * tmp_sec as f64;
			elapsed += tmp_nsec as f64;
		    };
		    elapsed /= 1.0E3; // microseconds
		    ts.perf.split_start.tv_sec = val.tv_sec();
		    ts.perf.split_start.tv_nsec = val.tv_nsec();

		    if 0.0 < elapsed {
			ts.split_samples += 1;
			ts.split_sum += elapsed;
		    }
		},
		Err(e) => mca_dprintf!(0, "McaTimestamp split_end: {}", e),
	    }
	}
    }

    elapsed
}

// Get elapsed total time in microseconds
pub fn end_ts(ts: &mut McaTimestamp) -> f64 {
    let mut elapsed: f64 = 0.0;
    // Cannot process round trip times unless initialized
    if ts.perf.initialized() {
	if 0 < ts.perf.start.tv_nsec {
	    let clock = ClockId::from_raw(ts.perf.clock_id);
	    match clock.now() {
		Ok(val) => {
		    let tmp_nsec: c_long = val.tv_nsec() - ts.perf.start.tv_nsec;
		    if 0 < tmp_nsec {
			let tmp_sec: time_t = val.tv_sec() - ts.perf.start.tv_sec - 1;
			elapsed = 1.0E9 * tmp_sec as f64;
			elapsed += 1.0E9 + tmp_nsec as f64;
		    }
		    else {
			let tmp_sec: time_t = val.tv_sec() - ts.perf.start.tv_sec;
			elapsed = 1.0E9 * tmp_sec as f64;
			elapsed += tmp_nsec as f64;
		    };
		    elapsed /= 1.0E3; // microseconds
		},
		Err(e) => mca_dprintf!(0, "McaTimestamp end: {}", e),
	    }
	}
    }

    elapsed
}
