//! Macros for use inside this crate

/// Wrap a CPLEX call and provide a return value for the encapsulating function
#[macro_export]
macro_rules! cpx_return(
  ($func:ident, $value:expr, $env:expr, $($arg:expr),*) => (
    {
// println!("[calling {}]", stringify!($func));
      let ret = unsafe { $func( $env, $($arg),* ) };
      match ret {
        0 => Ok($value),
        _ => Err(Error::new($env, ret)),
      }
    }
  )
);

/// Wrap a CPLEX call and with no return value in the encapsulating function.
///
/// This will simply call `cpx_return!` with `()` as the return type.
#[macro_export]
macro_rules! cpx_call(
  ($func:ident, $env:expr, $($arg:expr),*) => (
    {
       cpx_return!($func, (), $env, $($arg),*)
    }
  )
);

/// Shorthand to convert a char* to a &str
///
/// This will panic on UTF8 errors
#[macro_export]
macro_rules! ptr_as_str(
  ($cstring:expr) => (
    unsafe {
      CStr::from_ptr($cstring).to_str().unwrap()
    }
  )
);

/// Shorthand to convert a &str a char*
///
/// This will panic on UTF8 errors
#[macro_export]
macro_rules! str_as_ptr(
  ($str:expr) => (
    {
       CString::new($str).unwrap().as_ptr()
    }
  )
);
