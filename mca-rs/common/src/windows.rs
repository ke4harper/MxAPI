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

use super::*;

use std::{
    sync::atomic::{
	AtomicI32,
	Ordering,
    },
};

use libc::{
    c_int,
    SIGINT,
    SIGILL,
    SIGSEGV,
    SIGTERM,
    SIGFPE,
    SIGABRT,
};

use windows_sys::Win32::System::{
    Threading::{
	GetCurrentThread,
	SetThreadAffinityMask,
    },
    Performance::{
	QueryPerformanceFrequency,
	QueryPerformanceCounter,
    },
};

#[allow(non_camel_case_types)]
type sigset_t = c_int;

const SIG_BLOCK: c_int = 0x0;
const SIG_UNBLOCK: c_int = 0x1;
const SIG_SETMASK: c_int = 0x2;

const SIGALARM: c_int = 14;

static SIGBLOCKED: AtomicI32 = AtomicI32::new(0);

fn sigprocmask(how: c_int, set: *const sigset_t) {
    let mut newset: sigset_t = 0;
    unsafe {
	match how {
	    SIG_BLOCK => newset &= !*set,
	    SIG_UNBLOCK => newset |= *set,
	    SIG_SETMASK => newset = *set,
	    _ => {},
	}
	// Global is accessed directly
	SIGBLOCKED.swap(newset, Ordering::Relaxed);
    }
}

fn siggetblocked() -> sigset_t {
    SIGBLOCKED.load(Ordering::Relaxed)
}

fn sigemptyset(set: &mut sigset_t) {
    *set = 0;
}

fn sigaddset(set: &mut sigset_t, sig: i32) {
    *set |= sig;
}

pub fn block_signals() {
    let mut block_alarm: sigset_t = 0;
    sigemptyset(&mut block_alarm);
    //sigaddset(&mut block_alarm, SIGALARM);
    sigaddset(&mut block_alarm, SIGINT);
    //sigaddset(&mut block_alarm, SIGHUP);
    sigaddset(&mut block_alarm, SIGILL);
    sigaddset(&mut block_alarm, SIGSEGV);
    sigaddset(&mut block_alarm, SIGTERM);
    sigaddset(&mut block_alarm, SIGFPE);
    sigaddset(&mut block_alarm, SIGABRT);
    sigprocmask(SIG_BLOCK, &mut block_alarm);
}

pub fn unblock_signals() {
    let mut block_alarm: sigset_t = 0;
    sigemptyset(&mut block_alarm);
    //sigaddset(&mut block_alarm, SIGALARM);
    sigaddset(&mut block_alarm, SIGINT);
    //sigaddset(&mut block_alarm, SIGHUP);
    sigaddset(&mut block_alarm, SIGILL);
    sigaddset(&mut block_alarm, SIGSEGV);
    sigaddset(&mut block_alarm, SIGTERM);
    sigaddset(&mut block_alarm, SIGFPE);
    sigaddset(&mut block_alarm, SIGABRT);
    sigprocmask(SIG_UNBLOCK, &mut block_alarm);
}

const MCA_MAGIC: u32 = 0x1234;

// OS specific
#[derive(Debug)]
pub struct McaPerf {
    magic: u32,
    freq: i64,
    scale: f64,
    start: u64,
    split_start: u64,
}

impl Default for McaPerf {
    fn default() -> Self {
	Self {
	    magic: 0,
	    freq: 0,
	    scale: 0.0,
	    start: 0,
	    split_start: 0,
	}
    }
}

impl McaPerf {
    fn initialize(&mut self) {
	self.magic = MCA_MAGIC;
	// One time access to clock frequency
	unsafe {
	    let old_mask = SetThreadAffinityMask(GetCurrentThread(), 0);
	    QueryPerformanceFrequency(&mut self.freq);
	    SetThreadAffinityMask(GetCurrentThread(), old_mask);
	}
	self.scale = (self.freq as f64)/1E6; // microseconds
    }

    pub fn initialized(&self) -> bool {
	self.magic == MCA_MAGIC
    }
}

// Get starting timestamp for this process
pub fn begin_ts(ts: &mut McaTimestamp) {
    ts.perf.initialize();

    // Always collect the timestamp from the same CPU
    let mut start: i64 = 0;
    unsafe {
	let old_mask = SetThreadAffinityMask(GetCurrentThread(), 0);
	QueryPerformanceCounter(&mut start);
	SetThreadAffinityMask(GetCurrentThread(), old_mask);
    }
    ts.perf.start = start as u64;
}

// Get new starting split timestamp for this process
pub fn begin_split_ts(ts: &mut McaTimestamp) {
    if !ts.perf.initialized() {
	begin_ts(ts);
    };
    
    // Always collect the timestamp from the same CPU
    unsafe {
	let old_mask = SetThreadAffinityMask(GetCurrentThread(), 0);
	QueryPerformanceCounter(&mut (ts.perf.split_start as i64));
	SetThreadAffinityMask(GetCurrentThread(), old_mask);
    }
}

// Get most recent elapsed split time in microseconds
pub fn end_split_ts(ts: &mut McaTimestamp) -> f64 {
    let mut elapsed: f64 = 0.0;
    // Cannot process split times unless initialized
    if ts.perf.initialized() {
	if 0 < ts.perf.split_start {
	    let mut split_finish: i64 = 0;

	    // Always collect the timestamp from the same CPU
	    unsafe {
		let old_mask = SetThreadAffinityMask(GetCurrentThread(), 0);
		QueryPerformanceCounter(&mut split_finish);
		SetThreadAffinityMask(GetCurrentThread(), old_mask);
	    }

	    elapsed = ((split_finish as u64 - ts.perf.split_start) as f64) / ts.perf.scale;
	    ts.perf.split_start = split_finish as u64;
	}

	ts.split_samples += 1;
	ts.split_sum += elapsed;
    }

    elapsed
}

// Get elapsed total time in microseconds
pub fn end_ts(ts: &mut McaTimestamp) -> f64 {
    let mut elapsed: f64 = 0.0;
    // Cannot process round trip times unless initialized
    if ts.perf.initialized() {
	let mut finish: i64 = 0;
	if 0 < ts.perf.start {

	    // Always collect counters from the same CPU
	    unsafe {
		let old_mask = SetThreadAffinityMask(GetCurrentThread(), 0);
		QueryPerformanceCounter(&mut finish);
		SetThreadAffinityMask(GetCurrentThread(), old_mask);
	    }

	    elapsed = ((finish as u64 - ts.perf.start) as f64) / ts.perf.scale;
	}
    }

    elapsed
}
