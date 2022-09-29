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
pub type McaInt128 = i128;

pub type McaUint = usize;
pub type McaUint8 = u8;
pub type McaUint16 = u16;
pub type McaUint32 = u32;
pub type McaUint64 = u64;
pub type McaUint128 = u128;

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

pub const MCA_HANDLE_MASK: McaUint128 = 0xC0000000; // not conflict with Windows, Linux or VxWorks handles
pub const MCA_MAX_DOMAINS: McaUint = 2;
pub const MCA_MAX_NODES: McaUint = 16;
pub const MCA_MAX_PROCESSES: McaUint = 16;

pub const MCA_MAX_REQUESTS: McaUint = 1024;
pub const MCA_MAX_CPUS: McaUint = 16;

pub mod logging;
pub mod crc;
pub mod profiling;
pub mod signals;
