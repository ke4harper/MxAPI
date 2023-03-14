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

use crc::{Crc, CRC_32_ISCSI};

const CASTAGNOLI: Crc<u32> = Crc::<u32>::new(&CRC_32_ISCSI);

/// crc32_compute_buf - computes the CRC-32 value of a buffer,
// Digest initialized with the string representation of an integer key
//
//       Computes or accumulates the CRC-32 value for a memory buffer.
//       The in_crc32 gives a previously accumulated CRC-32 value to allow
//       a CRC to be generated for multiple sequential buffer-fuls of data.
//       The in_crc32 for the first buffer must be zero.
//
//  Parameters:
//       in_crc32 - accumulated CRC-32 value, must be 0 on first call
//       buf     - buffer to compute CRC-32 value for
//
//  Returns: crc32 - computed CRC-32 value
//
#[allow(dead_code)]
pub fn crc32_compute_buf(in_crc32: u32, buf: &str) -> u32 {
    let mut digest = CASTAGNOLI.digest();
    // Convert key to bytes
    let crc32_bytes = (in_crc32 ^ 0xFFFFFFFF).to_be_bytes();
    digest.update(&crc32_bytes);
    // Convert string to bytes
    let buf_bytes = buf.as_bytes();
    digest.update(buf_bytes);

    digest.finalize()
}

#[allow(unused_imports)]
use more_asserts as ma;

#[cfg(test)]
mod tests { 

    use super::*;

    #[test]
    fn generate_key() {
	let mut key = 0;
	let buf = "0123456";
	key = crc32_compute_buf(key, buf);
	ma::assert_lt!(0, key);
    }
}
