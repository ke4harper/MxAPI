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

// https://www.jameselford.com/blog/working-with-signals-in-rust-pt1-whats-a-signal/
#[allow(dead_code)]
fn mca_block_signals() {
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

#[allow(dead_code)]
fn mca_unblock_signals() {
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

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    fn signals() {
	mca_block_signals();
	mca_unblock_signals();
    }
}
