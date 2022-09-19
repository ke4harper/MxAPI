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

pub mod logging;
pub mod profiling;

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
#[allow(dead_code)]
fn crc32_compute_buf(_in_crc32: u32, _buf: &u8, _buf_len: usize) -> u32 {
    println!("/// ERROR: replace crc32_compute_buf with native crc [3.0.0]");

    0
}

// https://www.jameselford.com/blog/working-with-signals-in-rust-pt1-whats-a-signal/
#[allow(dead_code)]
fn mca_block_signals() {
    println!("/// ERROR: replace mca_block_signals functionaliity with native signal_hook");
}

#[allow(dead_code)]
fn mca_unblock_signals() {
    println!("/// ERROR: replace mca_unblock_signals functionality with native signal_hook");
}
