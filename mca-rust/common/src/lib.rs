//! Common functions

use std::sync::atomic::{AtomicUsize, Ordering};

// Global debug setting
static MCA_DEBUG: AtomicUsize = AtomicUsize::new(0);
static MCA_DEBUG_INITIALIZED: AtomicUsize = AtomicUsize::new(0);

/// Get global debug level
#[macro_export]
macro_rules! mca_debug {
    // Accessor
    () => {{
	{
	    use std::env;
	    
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
	    use std::process;
	    use std::thread;
	    
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

/// mca_set_debug_level - programmically control diagnostic reporting level
fn mca_set_debug_level(level: usize) {
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
    fn set_debug_level() {
	mca_set_debug_level(1);
	assert_eq!(1, mca_debug!());
    }
}
