//! User supplied callbacks

extern crate libc;
use self::libc::c_void;

use cplex_sys::*;
use env::CallbackPrivate;
use error::{Error, PrivateErrorConstructor};
use model::{Expr, Sense};

/// Return values of callbacks advice CPLEX on further calculations.
///
/// The meaning of the values is dependent on the type and context of
/// the callback.
pub enum Action {
    /// The default action
    Default = constants::CPX_CALLBACK_DEFAULT as isize,
    /// Abort solve, dismiss all set values
    Fail = constants::CPX_CALLBACK_FAIL as isize,
    /// Set returned values
    Set = constants::CPX_CALLBACK_SET as isize,
    /// Abort the callback loop for this node and continue with B&C
    /// This value can only be returned by the user cut callback.
    AbortLoop = constants::CPX_CALLBACK_ABORT_CUT_LOOP as isize,
}

/// Interface to CPLEX inside a callback
pub struct Callback {
    env: *const CPXenv,
    cbdata: *mut c_void,
    wherefrom: i32,
}

impl Callback {
    /// Add a cut to the model
    pub fn add_cut(&self,
                   expr: Expr,
                   sense: Sense,
                   rhs: f64,
                   purgeable: bool)
                   -> Result<(), Error> {
        let len = expr.vars.len();
        let mut ind: Vec<i32> = Vec::with_capacity(len);
        let mut val: Vec<f64> = Vec::with_capacity(len);

        for (v, c) in expr.vars {
            ind.push(v as i32);
            val.push(c);
        }

        cpx_call!(CPXLcutcallbackadd,
                  self.env,
                  self.cbdata,
                  self.wherefrom,
                  len as CPXDIM,
                  rhs,
                  sense as i32,
                  ind.as_ptr(),
                  val.as_ptr(),
                  purgeable as i32)
    }
}

impl CallbackPrivate for Callback {
    fn from_cpx(env: *const CPXenv, cbdata: *mut c_void, wherefrom: i32) -> Callback {
        Callback {
            env: env,
            cbdata: cbdata,
            wherefrom: wherefrom,
        }
    }
}
