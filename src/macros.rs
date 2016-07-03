//! Macros for use inside this crate

/// Wrap a CPLEX call and provide a return value for the encapsulating function
#[macro_export]
macro_rules! cpx_return {
    ($func:ident, $env:expr) => {
        cpx_return!($func, $env, )
    };

    ($func:ident, $value:expr, $env:expr, $($arg:expr),*) => {{
        if cfg!(test) {
            println!("[calling {}]", stringify!($func));
        }
        match unsafe { $func( $env, $($arg),* ) } {
            0 => Ok($value),
            e => Err(Error::new($env, e)),
        }
    }};
}

/// Wrap a CPLEX call and with no return value in the encapsulating function.
///
/// This will simply call `cpx_return!` with `()` as the return type.
#[macro_export]
macro_rules! cpx_call {
    ($func:ident, $env:expr) => {
        cpx_call!($func, $env, )
    };

    ($func:ident, $env:expr, $($arg:expr),*) => {
        cpx_return!($func, (), $env, $($arg),*)
    };
}

/// Shorthand to convert a char* to a &str
///
/// This will panic on UTF8 errors
#[macro_export]
macro_rules! ptr_as_str {
    ($cstring:expr) => {
        unsafe {
            CStr::from_ptr($cstring).to_str().unwrap()
        }
    };
}

/// Shorthand to convert a &str a char*
///
/// This will panic on UTF8 errors
#[macro_export]
macro_rules! str_as_ptr {
    ($str:expr) => {
       CString::new($str).unwrap().as_ptr()
    };
}
