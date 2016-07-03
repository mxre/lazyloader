//! CPLEX Callable C-Library

extern crate libc;
use self::libc::{c_char, c_int, c_double, int64_t};

pub enum CPXenv {}

pub enum CPXlp {}

pub const CPXMESSAGEBUFSIZE: usize = 1024;

pub const CPX_PARAM_TILIM: c_int = 1039;
pub const CPX_PARAM_THREADS: c_int = 1067;

pub const CPX_PARAM_EPAGAP: c_int = 2008;
pub const CPX_PARAM_EPGAP: c_int = 2009;
pub const CPX_PARAM_MIPDISPLAY: c_int = 2012;

//#[link(name = "cplex1260")]
extern "C" {
    pub fn CPXopenCPLEX(status: *mut c_int) -> *mut CPXenv;

    pub fn CPXcloseCPLEX(env: &*mut CPXenv) -> c_int;

    pub fn CPXgeterrorstring(env: *const CPXenv,
                             status: c_int,
                             message: *mut c_char)
                             -> *const c_char;

    pub fn CPXversion(env: *const CPXenv) -> *const c_char;

    pub fn CPXversionnumber(env: *const CPXenv) -> *const c_char;

    pub fn CPXsetdefaults(env: *mut CPXenv) -> c_int;

    pub fn CPXsetintparam(env: *mut CPXenv, whichparam: c_int, newvalue: c_int) -> c_int;

    pub fn CPXsetlongparam(env: *mut CPXenv, whichparam: c_int, newvalue: int64_t) -> c_int;

    pub fn CPXsetdblparam(env: *mut CPXenv, whichparam: c_int, newvalue: c_double) -> c_int;

    pub fn CPXsetstrparam(env: *mut CPXenv, whichparam: c_int, newvalue: *mut c_char) -> c_int;

    pub fn CPXgetintparam(env: *mut CPXenv, whichparam: c_int, value: *mut c_int) -> c_int;

    pub fn CPXgetlongparam(env: *mut CPXenv, whichparam: c_int, value: *mut int64_t) -> c_int;

    pub fn CPXgetdblparam(env: *mut CPXenv, whichparam: c_int, value: *mut c_double) -> c_int;

    pub fn CPXgetstrparam(env: *mut CPXenv, whichparam: c_int, value: *mut c_char) -> c_int;

    pub fn CPXcreateprob(env: *const CPXenv,
                         status: *mut c_int,
                         probname: *const c_char)
                         -> *mut CPXlp;

    pub fn CPXfreeprob(env: *const CPXenv, lp: &*mut CPXlp) -> c_int;

    pub fn CPXgetnumrows(env: *const CPXenv, lp: *const CPXlp) -> c_int;

    pub fn CPXgetnumcols(env: *const CPXenv, lp: *const CPXlp) -> c_int;

    pub fn CPXnewcols(env: *const CPXenv,
                      lp: *mut CPXenv,
                      ccnt: c_int,
                      obj: *const c_double,
                      lb: *const c_double,
                      ub: *const c_double,
                      ctype: *const c_char,
                      colname: &*mut c_char)
                      -> c_int;

    pub fn CPXnewrows(env: *const CPXenv,
                      lp: *mut CPXenv,
                      rcnt: c_int,
                      rhs: *const c_double,
                      sense: *const c_char,
                      rngval: *const c_double,
                      rowname: &*mut c_char)
                      -> c_int;

    pub fn CPXchgcoef(env: *const CPXenv,
                      lp: *mut CPXenv,
                      i: c_int,
                      j: c_int,
                      newvalue: c_double)
                      -> c_int;

    pub fn CPXchgbds(env: *const CPXenv,
                     lp: *mut CPXenv,
                     cnt: c_int,
                     indices: *const c_int,
                     lu: *const c_char,
                     bd: *const c_double)
                     -> c_int;

    pub fn CPXcopylp(env: *const CPXenv,
                     lp: *mut CPXenv,
                     numcols: c_int,
                     numrows: c_int,
                     objsense: c_int,
                     objective: *const c_double,
                     rhs: *const c_double,
                     sense: *const c_char,
                     matbeg: *const int64_t,
                     matcnt: *const c_int,
                     matind: *const c_int,
                     matval: *const c_double,
                     lb: *const c_double,
                     ub: *const c_double,
                     rngval: *const c_double)
                     -> c_int;

    pub fn CPXcopyctype(env: *const CPXenv, lp: *mut CPXenv, ctype: *const c_char) -> c_int;

    pub fn CPXpresolve(env: *const CPXenv, lp: *mut CPXenv, method: c_int) -> c_int;

    pub fn CPXmipopt(env: *const CPXenv, lp: *mut CPXlp) -> c_int;

    pub fn CPXlpopt(env: *const CPXenv, lp: *mut CPXlp) -> c_int;

    pub fn CPXprimopt(env: *const CPXenv, lp: *mut CPXlp) -> c_int;

    pub fn CPXgetstat(env: *const CPXenv, lp: *const CPXlp) -> c_int;

    pub fn CPXgetstatstring(env: *const CPXenv,
                            status: c_int,
                            message: *mut c_char)
                            -> *const c_char;

    pub fn CPXgetobjval(env: *const CPXenv, lp: *mut CPXlp, objval: *mut c_double) -> c_int;

    pub fn CPXgetx(env: *const CPXenv,
                   lp: *const CPXlp,
                   x: *mut c_double,
                   begin: c_int,
                   end: c_int)
                   -> c_int;

    pub fn CPXsolution(env: *const CPXenv,
                       lp: *const CPXlp,
                       lpstat: *mut c_int,
                       objval: *mut c_int,
                       x: *mut c_double,
                       pi: *mut c_double,
                       slack: *mut c_double,
                       dj: *mut c_double)
                       -> c_int;

    pub fn CPXsolninfo(env: *const CPXenv,
                       lp: *mut CPXlp,
                       solnmethod: *mut c_int,
                       solntype: *mut c_int,
                       pfeasind: *mut c_int,
                       dfeasind: *mut c_int)
                       -> c_int;

    pub fn CPXgettime(env: *const CPXenv, timestamp: *mut c_double) -> c_int;

    pub fn CPXwriteparam(env: *const CPXenv, filename: *const c_char) -> c_int;

    pub fn CPXwriteprob(env: *const CPXenv,
                        lp: *const CPXlp,
                        filename: *const c_char,
                        filetype: *const c_char)
                        -> c_int;
}
