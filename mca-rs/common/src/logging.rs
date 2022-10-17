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

/// Logging functions

use super::*;

/// Get global debug level
#[macro_export]
macro_rules! mca_debug {
    // Accessor
    () => {{
	{
	    use std::env;
	    use std::sync::atomic::{Ordering};

	    let initialized = MCA_DEBUG_INITIALIZED.load(Ordering::Relaxed);
	    if initialized <= 0 {
		let key = "MCA_DEBUG";
		match env::var(key) {
		    Ok(val) => MCA_DEBUG.store(val.parse::<usize>().expect("$MCA_DEBUG is not a number"), Ordering::Relaxed),
		    Err(_) => MCA_DEBUG.store(0, Ordering::Relaxed),
		}
		MCA_DEBUG_INITIALIZED.store(1, Ordering::Relaxed);
	    }

	    let level = MCA_DEBUG.load(Ordering::Relaxed);
	    
	    level
	}
    }};
    // Mutator
    ($level: expr) => {{
	{
	    use std::sync::atomic::{Ordering};
	    
	    MCA_DEBUG.store($level, Ordering::Relaxed);
	    MCA_DEBUG_INITIALIZED.store(1, Ordering::Relaxed);
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
	    use thread_id;
	    use std::process;
	    
	    if $level as usize <= mca_debug!() {
		print!("/* MCA PID:{} TID:{} */   //", process::id(), thread_id::get());
	    }
	}
    }};
    // Additional message
    ($level: expr, $msg: expr) => {{
	mca_dprintf! { $level }
	if $level as usize <= mca_debug!() {
	    mca_dformat! { $msg }
	}
    }};
    // Variable formatted arguments
    ($level: expr, $fmt: expr, $($args: tt), *) => {{
	mca_dprintf! { $level }
	if $level as usize <= mca_debug!() {
	    mca_dformat! { $fmt, $($args), * }
	}
    }};
}

/// mca_set_debug_level - programmically control diagnostic reporting verbosity
pub fn mca_set_debug_level(level: usize) {
    mca_debug!(level);
}

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
    fn print_multiple_args() {
	mca_dprintf!(0, "{} {} {}", "These are args", 2, 3);
    }
    #[test]
    fn set_debug_level() {
	mca_set_debug_level(1);
	assert_eq!(1, mca_debug!());
	mca_set_debug_level(0);
    }
}
