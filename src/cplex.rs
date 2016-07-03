//! CPLEX problem and solver
use std::path::Path;
use std::ffi::CString;
use cplex_sys::*;
use error::Error;

/// LP problem and solver
pub struct Problem {
    pub env: *const CPXenv,
    pub lp: *mut CPXlp,
}

impl Problem {
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
}

impl Drop for Problem {
    fn drop(&mut self) {
        let status = unsafe { CPXfreeprob(self.env, &mut &self.lp) };
        match status {
            0 => (),
            _ => println!("{}", Error::new(self.env, status).description()),
        }
    }
}
