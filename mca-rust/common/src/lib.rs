//! Common functions

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
	    use std::env;
	    use std::process;
	    use std::thread;
	    
	    let debug_str = env!("MCA_DEBUG", "$MCA_DEBUG is not set");
	    let debug = debug_str.parse::<u32>().expect("$MCA_DEBUG not a number");
	    if $level as u32 <= debug {
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
}
