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

impl Problem {
    /// Create a CPLEX LP problem in the environment
    /// # Native call
    /// `CPXcreateprob`
    pub fn new(e: &env::Env, name: &str) -> Result<Self, Error> {
        let mut status = 0 as c_int;
        let lp = unsafe { CPXcreateprob(e.env, &mut status, str_as_ptr!(name)) };
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
    /// `CPXmipopt`
    pub fn solve(&mut self) -> Result<(), Error> {
        cpx_call!(CPXmipopt, self.env, &mut *self.lp)
    }

    /// Get the numeric CPLEX status
    /// # Native call
    /// `CPXgetstat`
    pub fn get_status(&self) -> i32 {
        unsafe { CPXgetstat(self.env, self.lp) }
    }

    /// Get the textual representation of the CPLEX status
    /// # Native call
    /// `CPXgetstat` and `CPXgetstatstring`
    pub fn get_status_text(&self) -> String {
        unsafe {
            let status = self.get_status();
            let message = CString::from_vec_unchecked(Vec::with_capacity(CPXMESSAGEBUFSIZE));
            let c_msg = message.into_raw();
            CPXgetstatstring(self.env, status, c_msg);
            CString::from_raw(c_msg).to_str().unwrap().to_string()
        }
    }

    /// Write the LP problem to a file
    /// # Native call
    /// `CPXwriteprob`
    pub fn write_problem<P: AsRef<Path>>(&self, file: P, filetype: &str) -> Result<(), Error> {
        cpx_call!(CPXwriteprob,
                  self.env,
                  self.lp,
                  str_as_ptr!(file.as_ref().to_string_lossy().to_mut().as_str()),
                  str_as_ptr!(filetype))
    }

    #[allow(unused_variables)]
    pub fn extract(m: model::Model) -> Result<(), Error> {
        unimplemented!()
    }
}

impl Drop for Problem {
    fn drop(&mut self) {
        match unsafe { CPXfreeprob(self.env, &mut &self.lp) } {
            0 => (),
            s => println!("{}", Error::new(self.env, s).description()),
        }
    }
}

/// Acessing CPLEX model directly
pub trait Raw {
	/// Add an empty new row, i.e. a new constraint to the model
	fn new_row(&mut self, rhs: f64, sense: char, name: Option<&str>) -> Result<(), Error>;

	/// Add an empty column, i.e. a new variable to the model
	fn new_col(&mut self, obj: f64, lb: f64, ub: f64, ctype: char, name: Option<&str>) -> Result<(), Error>;

	/// Get the current number of rows
	fn get_num_rows(&self) -> i32;

	/// Get the current number of columns
	fn get_num_cols(&self) -> i32;

	/// Set a specific coefficient int the matrix
	fn set_coefficent(&mut self, i: i32, j:i32, coef: f64) -> Result<(), Error>;
}

impl Raw for Problem {
	fn new_row(&mut self, rhs: f64, sense: char, name: Option<&str>) -> Result<(), Error> {
		let rowname: [*const c_char; 1] = [ if name.is_some() {
			str_as_ptr!(name.unwrap())
		} else {
			ptr::null()
		} ];
		cpx_call!(CPXnewrows, self.env, self.lp, 1, &rhs, &(sense as c_char), ptr::null(), rowname.as_ptr())

	}

	fn new_col(&mut self, obj: f64, lb: f64, ub: f64, ctype: char, name: Option<&str>) -> Result<(), Error> {
		let colname: [*const c_char; 1] = [ if name.is_some() {
			str_as_ptr!(name.unwrap())
		} else {
			ptr::null()
		} ];
		cpx_call!(CPXnewcols, self.env, self.lp, 1, &obj, &ub, &lb, &(ctype as c_char), colname.as_ptr())
	}

	fn get_num_rows(&self) -> i32 {
		unsafe { CPXgetnumrows(self.env, self.lp) }
	}

	fn get_num_cols(&self) -> i32 {
		unsafe { CPXgetnumcols(self.env, self.lp) }
	}

	fn set_coefficent(&mut self, i: i32, j:i32, coef: f64) -> Result<(), Error>{
		cpx_call!(CPXchgcoef, self.env, self.lp, i, j, coef)
	}
}
