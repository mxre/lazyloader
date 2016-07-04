//! CPLEX environment
use std::path::Path;
use std::ptr;
use std::ffi::{CStr, CString};

extern crate libc;
use self::libc::c_int;

use cplex_sys::*;
use error::{Error, PrivateErrorConstructor};
use param::ParameterType;

/// CPLEX environment
pub struct Env {
    pub env: *mut CPXenv,
}

impl Env {
    /// Create a new CPLEX environment
    /// # Native call
    /// `CPXopenCPLEX`
    pub fn new() -> Result<Self, Error> {
        let mut status = 0 as c_int;
        let env = unsafe { CPXopenCPLEX(&mut status) };
        match status {
            0 => {
                assert!(!env.is_null());
                Ok(Env { env: env })
            }
            _ => Err(Error::new(ptr::null(), status)),
        }
    }

    /// Set CPLEX default parameters
    /// # Native call
    /// `CPXsetdefaults`
    pub fn set_default_param(&self) -> Result<(), Error> {
        cpx_call!(CPXsetdefaults, self.env)
    }

    /// Set a CPLEX parameter
    pub fn set_param<T: ParameterType>(&mut self, what: T, value: T::InType) -> Result<(), Error> {
        what.set(self.env, value)
    }

    /// Get a CPLEX parameter
    pub fn get_param<T: ParameterType>(&self, what: T) -> Result<T::ReturnType, Error> {
        what.get(self.env)
    }

    /// Write all set parametes into a file
    /// # Native call
    /// `CPXwriteparam`
    pub fn write_param<P: AsRef<Path>>(&self, file: P) -> Result<(), Error> {
        cpx_call!(CPXwriteparam,
                  self.env,
                  str_as_ptr!(file.as_ref().to_string_lossy().to_mut().as_str()))
    }

    /// Get the CPLEX version as a string
    /// # Native call
    /// `CPXversion`
    pub fn version(&self) -> &str {
        ptr_as_str!(CPXversion(self.env))
    }
}

impl Drop for Env {
    fn drop(&mut self) {
        match unsafe { CPXcloseCPLEX(&mut &self.env) } {
            0 => (),
            s => println!("{}", Error::new(ptr::null(), s).description()),
        }
    }
}
