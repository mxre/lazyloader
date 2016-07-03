//! CPLEX environment
use std::path::Path;
use std::ptr;
use std::ffi::CStr;
use std::ffi::CString;

extern crate libc;
use self::libc::c_int;

use cplex_sys::*;
use error::Error;
use param::*;
use cplex::Problem;

/// CPLEX environment
pub struct Env {
    env: *mut CPXenv,
}

impl Env {
    /// Create a new CPLEX environment
    /// # Native call
    /// `CPXopenCPLEX`
    pub fn new() -> Result<Env, Error> {
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

    /// Create a CPLEX LP problem in the environment
    /// # Native call
    /// `CPXcreateprob`
    pub fn create_problem(&self, name: &str) -> Result<Problem, Error> {
        let mut status = 0 as c_int;
        let lp = unsafe { CPXcreateprob(self.env, &mut status, str_as_ptr!(name)) };
        match status {
            0 => {
                assert!(!lp.is_null());
                Ok(Problem {
                    env: self.env,
                    lp: lp,
                })
            }
            _ => Err(Error::new(self.env, status)),
        }
    }

    /// Set CPLEX default parameters
    /// # Native call
    /// `CPXsetdefaults`
    pub fn set_default_param(&self) -> Result<(), Error> {
        cpx_call!(CPXsetdefaults, self.env, )
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
    pub fn version(&mut self) -> &str {
        ptr_as_str!(CPXversion(self.env))
    }
}

impl Drop for Env {
    fn drop(&mut self) {
        let status = unsafe { CPXcloseCPLEX(&mut &self.env) };
        match status {
            0 => (),
            _ => println!("{}", Error::new(ptr::null(), status).description()),
        }
    }
}

impl IntParam for Env {
    fn set_param(&self, what: IntParameter, value: i32) -> Result<(), Error> {
        cpx_call!(CPXsetintparam, self.env, IntParameter::into(what), value)
    }

    fn get_param(&self, what: IntParameter) -> Result<i32, Error> {
        let mut value: i32 = 0;
        cpx_return!(CPXgetintparam,
                    value,
                    self.env,
                    IntParameter::into(what),
                    &mut value)
    }
}

impl DblParam for Env {
    fn set_param(&self, what: DblParameter, value: f64) -> Result<(), Error> {
        cpx_call!(CPXsetdblparam, self.env, DblParameter::into(what), value)
    }

    fn get_param(&self, what: DblParameter) -> Result<f64, Error> {
        let mut value: f64 = 0.0;
        cpx_return!(CPXgetdblparam,
                    value,
                    self.env,
                    DblParameter::into(what),
                    &mut value)
    }
}
