//! Error type
use std::ffi::CString;
use std::fmt;
use std::error;

use cplex_sys;

/// Error type
pub struct Error {
    code: i32,
    description: String,
}

impl Error {
    /// Create a new Error for specific error code
    pub fn new(env: *const cplex_sys::CPXenv, code: i32) -> Error {
        unsafe {
            let message =
                CString::from_vec_unchecked(Vec::with_capacity(cplex_sys::CPXMESSAGEBUFSIZE));
            let c_msg = message.into_raw();
            cplex_sys::CPXgeterrorstring(env, code, c_msg);
            Error {
                code: code,
                description: CString::from_raw(c_msg).to_str().unwrap().trim().to_string(),
            }
        }
    }

    /// Get the error code
    pub fn code(&self) -> i32 {
        self.code
    }

    /// Get the error description
    pub fn description(&self) -> &str {
        self.description.as_str()
    }
}

impl error::Error for Error {
    fn description(&self) -> &str {
        Error::description(self)
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.description)
    }
}

impl fmt::Debug for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.description)
    }
}
