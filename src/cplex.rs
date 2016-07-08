//! CPLEX problem and solver
use std::path::Path;
use std::ffi::CString;
use std::ptr;

extern crate libc;
use self::libc::{c_int, c_char};

use cplex_sys::*;
use error::{Error, PrivateErrorConstructor};
use env;
use model;

/// LP problem and solver
pub struct Problem {
    pub env: *const CPXenv,
    pub lp: *mut CPXlp,
}

pub trait ExtractableModel {
    /// Add a constraint to the model and extract the variables
    fn extract(self, &mut Problem) -> Result<(), Error>;
}

impl Problem {
    /// Create a CPLEX LP problem in the environment
    /// # Native call
    /// `CPXLcreateprob`
    pub fn new(e: &env::Env, name: &str) -> Result<Self, Error> {
        let mut status = 0 as c_int;
        let lp = unsafe { CPXLcreateprob(e.env, &mut status, str_as_ptr!(name)) };
        match status {
            0 => {
                assert!(!lp.is_null());
                Ok(Problem {
                    env: e.env,
                    lp: lp,
                })
            }
            _ => Err(Error::new(e.env, status)),
        }
    }

    /// Run CPLEX optimize
    /// # Native call
    /// `CPXLmipopt`
    pub fn solve(&mut self) -> Result<(), Error> {
        match unsafe { CPXLgetprobtype(self.env, self.lp) } {
            CPXPROB_LP => cpx_call!(CPXLlpopt, self.env, self.lp),
            CPXPROB_MILP |
            CPXPROB_FIXEDMILP => cpx_call!(CPXLmipopt, self.env, self.lp),
            _ => unimplemented!(),
        }
    }

    /// Get the numeric CPLEX status
    /// # Native call
    /// `CPXLgetstat`
    pub fn get_status(&self) -> i32 {
        unsafe { CPXLgetstat(self.env, self.lp) }
    }

    /// Get the textual representation of the CPLEX status
    /// # Native call
    /// `CPXLgetstat` and `CPXLgetstatstring`
    pub fn get_status_text(&self) -> String {
        unsafe {
            let status = self.get_status();
            let message = CString::from_vec_unchecked(Vec::with_capacity(CPXMESSAGEBUFSIZE));
            let c_msg = message.into_raw();
            CPXLgetstatstring(self.env, status, c_msg);
            CString::from_raw(c_msg).to_str().unwrap().to_string()
        }
    }

    /// Write the LP problem to a file
    /// # Native call
    /// `CPXLwriteprob`
    pub fn write_problem<P: AsRef<Path>>(&self, file: P, filetype: &str) -> Result<(), Error> {
        cpx_call!(CPXLwriteprob,
                  self.env,
                  self.lp,
                  str_as_ptr!(file.as_ref().to_string_lossy().to_mut().as_str()),
                  str_as_ptr!(filetype))
    }


    pub fn extract(&mut self, m: model::Model) -> Result<(), Error> {
        m.extract(self)
    }
}

impl Drop for Problem {
    fn drop(&mut self) {
        match unsafe { CPXLfreeprob(self.env, &mut &self.lp) } {
            0 => (),
            s => println!("{}", Error::new(self.env, s).description()),
        }
    }
}

/// Acessing CPLEX model directly
pub trait Raw {
    /// Add an empty new row, i.e. a new constraint to the model
    fn new_row(&mut self, rhs: f64, sense: model::Sense, name: Option<&str>) -> Result<(), Error>;

    /// Add an empty column, i.e. a new variable to the model
    fn new_col(&mut self,
               obj: f64,
               lb: f64,
               ub: f64,
               ctype: model::VarType,
               name: Option<&str>)
               -> Result<(), Error>;

    /// Get the current number of rows
    fn get_num_rows(&self) -> i32;

    /// Get the current number of columns
    fn get_num_cols(&self) -> i32;

    /// Set a specific coefficient int the matrix
    fn set_coefficent(&mut self, i: i32, j: i32, coef: f64) -> Result<(), Error>;

    /// Set the sense of the objective function, i.e., min or max
    fn set_objective_sense(&mut self, sense: model::Objective) -> Result<(), Error>;

    /// Retrieve the solution values for variables [begin; end]
    fn get_x(&self, begin: i32, end: i32) -> Result<Vec<f64>, Error>;
}

impl Raw for Problem {
    fn new_row(&mut self, rhs: f64, sense: model::Sense, name: Option<&str>) -> Result<(), Error> {
        let rowname: [*const c_char; 1] = [if name.is_some() {
                                               str_as_ptr!(name.unwrap())
                                           } else {
                                               ptr::null()
                                           }];
        cpx_call!(CPXLnewrows,
                  self.env,
                  self.lp,
                  1,
                  &rhs,
                  &(sense as c_char),
                  ptr::null(),
                  rowname.as_ptr())

    }

    fn new_col(&mut self,
               obj: f64,
               lb: f64,
               ub: f64,
               ctype: model::VarType,
               name: Option<&str>)
               -> Result<(), Error> {
        let colname: [*const c_char; 1] = [if name.is_some() {
                                               str_as_ptr!(name.unwrap())
                                           } else {
                                               ptr::null()
                                           }];
        cpx_call!(CPXLnewcols,
                  self.env,
                  self.lp,
                  1,
                  &obj,
                  &lb,
                  &ub,
                  &(ctype as c_char),
                  colname.as_ptr())
    }

    fn get_num_rows(&self) -> i32 {
        unsafe { CPXLgetnumrows(self.env, self.lp) }
    }

    fn get_num_cols(&self) -> i32 {
        unsafe { CPXLgetnumcols(self.env, self.lp) }
    }

    fn set_coefficent(&mut self, i: i32, j: i32, coef: f64) -> Result<(), Error> {
        cpx_call!(CPXLchgcoef, self.env, self.lp, i, j, coef)
    }

    fn get_x(&self, begin: i32, end: i32) -> Result<Vec<f64>, Error> {
        let sz = (end - begin + 1) as usize;
        let mut v = vec!(0.0; sz);
        cpx_return!(CPXLgetx, v, self.env, self.lp, v.as_mut_ptr(), begin, end)
    }

    fn set_objective_sense(&mut self, sense: model::Objective) -> Result<(), Error> {
        cpx_call!(CPXLchgobjsen, self.env, self.lp, sense as c_int)
    }
}
