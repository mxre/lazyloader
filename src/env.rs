//! CPLEX environment
use std::path::Path;
use std::ptr;
use std::ffi::{CStr, CString};
use std::ops::FnMut;
use std::marker::Send;

extern crate libc;
use self::libc::{c_int, c_void, c_double};

use cplex_sys::*;
use error::{Error, PrivateErrorConstructor};
use cplex::Raw;
use callback::*;

/// CPLEX environment
pub struct Env {
    env: *mut CPXenv,
    cb: Box<CallbackEnvelope>,
    dispose: bool,
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
                // set the API to the same encoding as Rust
                // this will effect all string passed through CPLEX
                cpx_safe!(CPXLsetstrparam,
                          env,
                          constants::CPX_PARAM_APIENCODING,
                          str_as_ptr!("UTF-8"));
                Ok(Env {
                    env: env,
                    cb: Default::default(),
                    dispose: true,
                })
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
    #[inline]
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

    /// Set user cut callback
    pub fn set_user_cut_callback<F>(&mut self, cb: F) -> Result<(), Error>
        where F: FnMut(&Callback) -> Action + Send + 'static
    {
        self.cb.user = Some(Box::new(cb));
        cpx_call!(CPXLsetusercutcallbackfunc,
                  self.env,
                  Some(user_callback_wrapper),
                  ptr::null_mut())
    }

    /// Unset user cut callback
    pub fn clear_user_cut_callback(&mut self) -> Result<(), Error> {
        self.cb.user = None;
        cpx_call!(CPXLsetusercutcallbackfunc, self.env, None, ptr::null_mut())
    }

    /// Set incumbent callback
    pub fn set_incumbent_callback<F>(&mut self, cb: F) -> Result<(), Error>
        where F: FnMut(&Callback, f64, &Vec<f64>) -> (bool, Action) + Send + 'static
    {
        self.cb.incumbent = Some(Box::new(cb));
        cpx_call!(CPXLsetincumbentcallbackfunc,
                  self.env,
                  Some(incumbent_callback_wrapper),
                  &*self.cb as *const _ as *mut _)
    }

    /// Unset incumbent callback
    pub fn clear_incumbent_callback(&mut self) -> Result<(), Error> {
        self.cb.incumbent = None;
        cpx_call!(CPXLsetincumbentcallbackfunc,
                  self.env,
                  None,
                  ptr::null_mut())
    }

    /// Set heuristic callback
    pub fn set_heuristic_callback<F>(&mut self, cb: F) -> Result<(), Error>
        where F: FnMut(&Callback, &mut f64, &mut Vec<f64>) -> (bool, Action) + Send + 'static
    {
        self.cb.heuristic = Some(Box::new(cb));
        cpx_call!(CPXLsetheuristiccallbackfunc,
                  self.env,
                  Some(heuristic_callback_wrapper),
                  &*self.cb as *const _ as *mut _)
    }

    /// Unset heuristic callback
    pub fn clear_heuristict_callback(&mut self) -> Result<(), Error> {
        self.cb.heuristic = None;
        cpx_call!(CPXLsetheuristiccallbackfunc,
                  self.env,
                  None,
                  ptr::null_mut())
    }
}

/// Package all handles to callback functions for the lifetime of the CPLEX environment struct
#[derive(Default)]
struct CallbackEnvelope {
    user: Option<Box<FnMut(&Callback) -> Action>>,
    incumbent: Option<Box<FnMut(&Callback, f64, &Vec<f64>) -> (bool, Action)>>,
    heuristic: Option<Box<FnMut(&Callback, &mut f64, &mut Vec<f64>) -> (bool, Action)>>,
}

/// Wrap the user callback clojure
extern "C" fn user_callback_wrapper(env: *mut CPXenv,
                                    cbdata: *mut c_void,
                                    wherefrom: c_int,
                                    cbhandle: *mut c_void,
                                    useraction_p: *mut c_int)
                                    -> c_int {
    // println!("UCB");
    let cb = Callback::from_cpx(env, cbdata, wherefrom);
    let ret = unsafe {
        (*(cbhandle as *mut CallbackEnvelope))
            .user
            .as_mut()
            .map_or(Action::Default, |v| v(&cb))
    };
    unsafe {
        *useraction_p = ret as i32;
    }
    0
}

/// Wrap the incumbent callback clojure
extern "C" fn incumbent_callback_wrapper(env: *mut CPXenv,
                                         cbdata: *mut c_void,
                                         wherefrom: c_int,
                                         cbhandle: *mut c_void,
                                         objval: c_double,
                                         x: *mut c_double,
                                         isfeas_p: *mut c_int,
                                         useraction_p: *mut c_int)
                                         -> c_int {
    //println!("ICB");
    let cb = Callback::from_cpx(env, cbdata, wherefrom);
    let cols = cb.get_problem().get_num_cols() as usize;
    let x: Vec<f64> = unsafe { Vec::from_raw_parts(x, cols, cols) };
    let ret = unsafe {
        (*(cbhandle as *mut CallbackEnvelope))
            .incumbent
            .as_mut()
            .map_or((false, Action::Default), |v| v(&cb, objval, &x))
    };
    unsafe {
        *isfeas_p = if ret.0 {
            1
        } else {
            0
        };
        *useraction_p = ret.1 as i32;
    }
    0
}

/// Wrap the heuristic callback clojure
extern "C" fn heuristic_callback_wrapper(env: *mut CPXenv,
                                         cbdata: *mut c_void,
                                         wherefrom: c_int,
                                         cbhandle: *mut c_void,
                                         objval_p: *mut c_double,
                                         x: *mut c_double,
                                         checkfeas_p: *mut c_int,
                                         useraction_p: *mut c_int)
                                         -> c_int {
    let cb = Callback::from_cpx(env, cbdata, wherefrom);
    let cols = cb.get_problem().get_num_cols() as usize;
    let mut x: Vec<f64> = unsafe { Vec::from_raw_parts(x, cols, cols) };
    let objval: &mut f64 = unsafe { &mut *objval_p };
    let ret = unsafe {
        (*(cbhandle as *mut CallbackEnvelope))
            .heuristic
            .as_mut()
            .map_or((false, Action::Default), |v| v(&cb, objval, &mut x))
    };

    unsafe {
        *checkfeas_p = if ret.0 {
            1
        } else {
            0
        };
        *useraction_p = ret.1 as i32;
    }
    0
}

impl Drop for Env {
    fn drop(&mut self) {
        if self.dispose {
            match cpx_safe!(CPXLcloseCPLEX, &mut &self.env) {
                0 => (),
                s => println!("{}", Error::new(ptr::null(), s).description()),
            }
        }
    }
}

/// Common trait for all parameters
pub trait ParameterType {
    /// Type of the paramters value, for the setter.
    /// For strings This can be `&str`
    type InType;
    /// Type of the paramters value, for the getter
    /// For strings this can be `String`
    type ReturnType;

    /// Setter for the parameter
    #[inline]
    fn set(self, env: *mut CPXenv, value: Self::InType) -> Result<(), Error>;

    /// Getter for the parameter
    #[inline]
    fn get(self, env: *const CPXenv) -> Result<Self::ReturnType, Error>;
}

/// Create callback handle
pub trait CallbackPrivate {
    /// Create a callback handle from callback supplied pointers and values
    fn from_cpx(env: *mut CPXenv, cbdata: *mut c_void, wherefrom: i32) -> Callback;
}

/// Access raw CPLEX handle
pub trait PrivateEnv {
    /// Get the pointer to the CPLEX environment
    #[inline]
    fn get_env(&self) -> *mut CPXenv;

    /// Create a new CPLEX environment object from existing pointer
    #[inline]
    fn from_cpx(env: *mut CPXenv) -> Env;
}

impl PrivateEnv for Env {
    fn get_env(&self) -> *mut CPXenv {
        self.env
    }

    fn from_cpx(env: *mut CPXenv) -> Env {
        Env {
            env: env,
            cb: Default::default(),
            dispose: false,
        }
    }
}
