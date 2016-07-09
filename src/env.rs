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
    /// `CPXLopenCPLEX`
    pub fn new() -> Result<Self, Error> {
        let mut status = 0 as c_int;
        let env = cpx_safe!(CPXLopenCPLEX, &mut status);
        match status {
            0 => {
                assert!(!env.is_null());
                cpx_safe!(CPXLsetstrparam,
                          env,
                          CPX_PARAM_APIENCODING,
                          str_as_ptr!("UTF-8"));
                Ok(Env { env: env })
            }
            _ => Err(Error::new(ptr::null(), status)),
        }
    }

    /// Set CPLEX default parameters
    /// # Native call
    /// `CPXLsetdefaults`
    pub fn set_default_param(&self) -> Result<(), Error> {
        cpx_call!(CPXLsetdefaults, self.env)
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
    /// `CPXLwriteparam`
    pub fn write_param<P: AsRef<Path>>(&self, file: P) -> Result<(), Error> {
        cpx_call!(CPXLwriteparam,
                  self.env,
                  str_as_ptr!(file.as_ref().to_str().unwrap()))
    }

    /// Get the CPLEX version as a string
    /// # Native call
    /// `CPXLversion`
    #[allow(unused_unsafe)]
    pub fn version(&self) -> &str {
        ptr_as_str!(cpx_safe!(CPXLversion, self.env))
    }

    /// Set a logfile for all CPLEX output
    /// # Native call
    /// `CPXLfopen` and `CPXLsetlogfile`
    pub fn set_logfile<P: AsRef<Path>>(&self, file: P) -> Result<(), Error> {
        let fp = cpx_safe!(CPXLfopen,
                           str_as_ptr!(file.as_ref().to_str().unwrap()),
                           str_as_ptr!("w"));
        if fp.is_null() {
            Err(Error::custom_error("Could not open/create logfile"))
        } else {
            cpx_call!(CPXLsetlogfile, self.env, fp)
        }
    }
}

impl Drop for Env {
    fn drop(&mut self) {
        match cpx_safe!(CPXLcloseCPLEX, &mut &self.env) {
            0 => (),
            s => println!("{}", Error::new(ptr::null(), s).description()),
        }
    }
}
