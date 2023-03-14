///
/// Copyright(c) 2023, Karl Eric Harper
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

/// Catch signals to clean up shared memory and semaphore resources
pub fn signal_handler(sig: Signal) {
    
    common::block_signals();
    
    let mut mrapi_pid: MrapiUint32 = 0;
    MRAPI_PID.with(|id| {
	mrapi_pid = *id.get().unwrap();
    });
    let mut mrapi_tid: MrapiUint32 = 0;
    MRAPI_TID.with(|id| {
	mrapi_tid = *id.get().unwrap();
    });
    let mut mrapi_dindex: usize = 0;
    MRAPI_DINDEX.with(|index| {
	mrapi_dindex = *index.get().unwrap();
    });
    let mut mrapi_nindex: usize = 0;
    MRAPI_NINDEX.with(|index| {
	mrapi_nindex = *index.get().unwrap();
    });

    // Print info on which signal was caught
    eprintln!("SIGNAL: mrapi received signal[{}] pid={} tid={} dindex={} nindex={}",
	      sig, mrapi_pid, mrapi_tid, mrapi_dindex, mrapi_nindex);

    let mut sr = MrapiSemRef::new(&MrapiDatabase::global_sem(), 0, false);
    
    match sig {
	SIGSEGV | SIGABRT => {
	    // Release global semaphore if possible
	    access_database_post(&mut sr);
	},
	_other => { },
    }

    // Restore the old action
    access_database_pre(&mut sr, MRAPI_FALSE);
    let old_action = self.domains[mrapi_dindex].nodes[mrapi_nindex].signals[sig];
    access_database_post(&mut sr);
}

/// Execute atomic operation
pub fn atomic_exec(pindex: usize, op: MrapiAtomicOp) {
}

/// Catch signals to process atomic operations between address spaces
pub fn atomic_handler(sig: Signal) {
}

pub fn signal_remote_process(p: usize, mrapi_db: &mut MrapiDatabase) {
    let packed = &mrapi_db.processes[p].state.load(Ordering::Relaxed);
    let pstate: MrapiProcessState = Atom::unpack(*packed);
    kill(Pid::from_raw(pstate.pid() as i32), SIGUSR1).unwrap();
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    fn test() {
    }
}
