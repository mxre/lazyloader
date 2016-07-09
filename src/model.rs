//! ILO Concert like Model building
//! Concert-like symbolic model building
//! # Example
//! ```
//! use cplex::model::{Model, Sense, Objective};
//!
//! // initialize CPLEX
//! let mut env = cplex::Env::new().unwrap();
//! let mut lp = cplex::Problem::new(&env, "test").unwrap();
//! let mut model = Model::new();
//!
//! // create variables
//! let x_0 = model.new_binary_var().set_name("x(0)");
//! let x_1 = model.new_binary_var().set_name("x(1)");
//! let x_2 = model.new_binary_var().set_name("x(2)");
//! {
//!     // work with borrowed handles in model building
//!     let x0 = &x_0;
//!     let x1 = &x_1;
//!     let x2 = &x_2;
//!
//!     model.add_ineq(0.5 * x0 + x1, Sense::GreaterThan, 1.0, Some("c(0)"))
//!          .add_ineq(2 * x1 + x0, Sense::LessThan, 2.0, Some("c(1)"))
//!          .add_ineq(3 * x2, Sense::LessThan, 2.0, Some("c(2)"))
//!          .set_objective(Objective::Max, x0 + x1);
//! }
//! // transfer ownership of the variables
//! model.add_var(x_0).add_var(x_1).add_var(x_2);
//! // copy the model to CPLEX internal coefficent matrix
//! lp.extract(model).unwrap();
//! ```
//! Note that in this example we only temporarily borrow `x0`, `x1` and `x2`

use std;
use std::ops::{Add, Mul};
use std::ptr;
use std::num::FpCategory;
use std::convert::AsRef;

extern crate libc;
use self::libc::c_char;

use cplex::{Problem, ExtractableModel, PrivateProblem};
use error::{Error, PrivateErrorConstructor};
use cplex_sys::*;

/// Types of objectives
#[derive(Clone, Hash, Copy)]
pub enum Objective {
    /// objective is a minimizer
    Min = 1,
    /// objective is a maximizer
    Max = -1,
}

/// Type of a constraint
#[derive(Clone, Hash, Copy)]
pub enum Sense {
    LessThan = 'L' as isize,
    Equal = 'E' as isize,
    GreaterThan = 'G' as isize,
}

/// Type of variables
#[derive(Clone, Hash, Copy)]
pub enum VarType {
    Numeric = 'C' as isize,
    Integer = 'I' as isize,
    Binary = 'B' as isize,
}

/// ILP model, containing objective function and set of constraints
#[derive(Clone)]
pub struct Model {
    c_cnt: usize,
    v_cnt: usize,
    vars: Vec<Var>,
    constraints: Vec<Constraint>,
    obj: Objective,
    obj_fun: Option<Expr>,
}

impl Model {
    /// Create a new model
    pub fn new() -> Self {
        Model {
            c_cnt: 0,
            v_cnt: 0,
            vars: Vec::new(),
            constraints: Vec::new(),
            obj: Objective::Min,
            obj_fun: None,
        }
    }

    /// Add an inequality to the model
    pub fn add_ineq(&mut self, expr: Expr, s: Sense, rhs: f64, name: Option<&str>) -> &mut Self {
        let cnt = self.c_cnt;
        self.c_cnt += 1;
        self.constraints.push(Constraint {
            cnt: cnt,
            sense: s,
            rhs: rhs,
            name: name.map(|s| s.to_string()),
            expr: Some(expr),
        });
        assert!(self.constraints.len() == cnt + 1);
        self
    }

    /// Add a variable to the model
    pub fn add_var(&mut self, v: Var) -> &mut Self {
        self.vars.push(v);
        self
    }

    /// Add the objective function of the model
    pub fn set_objective(&mut self, m: Objective, o: Expr) -> &mut Self {
        self.obj = m;
        self.obj_fun = Some(o);
        self
    }

    /// Create a new numeric (continous) variable
    pub fn new_numeric_var(&mut self) -> &mut Var {
        let cnt = self.v_cnt;
        self.v_cnt += 1;
        self.vars.push(Var {
            cnt: cnt,
            tp: VarType::Numeric,
            lb: std::f64::MIN,
            ub: std::f64::MAX,
            name: None,
        });
        assert!(self.vars.len() == cnt + 1);
        &mut self.vars[cnt]
    }

    /// Create a new integer (non-continous) variable
    pub fn new_integer_var(&mut self) -> &mut Var {
        let cnt = self.v_cnt;
        self.v_cnt += 1;
        self.vars.push(Var {
            cnt: cnt,
            tp: VarType::Integer,
            lb: std::f64::MIN,
            ub: std::f64::MAX,
            name: None,
        });
        assert!(self.vars.len() == cnt + 1);
        &mut self.vars[cnt]
    }

    /// Create a new binary variable
    pub fn new_binary_var(&mut self) -> Var {
        let cnt = self.v_cnt;
        self.v_cnt += 1;
        Var {
            cnt: cnt,
            tp: VarType::Binary,
            lb: 0.0,
            ub: 1.0,
            name: None,
        }
        // assert!(self.vars.len() == cnt + 1);
        // self.vars[cnt]
    }
}

impl ExtractableModel for Model {
    fn extract(self, p: &mut Problem) -> Result<(), Error> {
        let numcols = self.vars.len();
        let numrows = self.constraints.len();
        // Objective coefficent
        let mut obj = vec!(0.0; numcols);
        // first index for row
        let mut matbeg = vec!(0 as CPXNNZ; numrows);
        // number of rows for specific column
        let mut matcnt = vec!(0 as CPXDIM; numrows);
        // indices for variables
        let mut matind: Vec<CPXDIM> = Vec::new();
        // Non-Zero coefficents
        let mut matval: Vec<f64> = Vec::new();
        let mut sense = vec!('E' as c_char; numrows);
        // RHS for each constraint
        let mut rhs = vec!(0.0; numrows);
        // for range constraints
        let rng = ptr::null();
        let mut rownames: Vec<*const c_char> = vec!(ptr::null(); numrows);

        let mut colnames: Vec<*const c_char> = vec!(ptr::null(); numcols);
        let mut lb = vec!(std::f64::MIN; numcols);
        let mut ub = vec!(std::f64::MAX; numcols);
        let mut ct = vec!('C' as c_char; numcols);

        // setup columnt data
        for c in self.vars {
            // println!("{}: {:?}", c.cnt, c.name);
            colnames[c.cnt] = c.name.map_or(ptr::null(), |s| s.as_ptr() as (*const c_char));
            lb[c.cnt] = c.lb;
            ub[c.cnt] = c.ub;
            ct[c.cnt] = c.tp as c_char;
        }

        // setup row data, copy non-zero row data
        let mut matbeg_cnt = 0 as CPXNNZ;
        for c in self.constraints {
            matbeg[c.cnt] = matbeg_cnt;
            let e = match c.expr {
                None => {
                    return Err(Error::custom_error("Extracted Constraint did not contain an \
                                                    extractable Expression"))
                }
                Some(e) => e,
            };

            // println!("{}: {:?}", c.cnt, c.name);
            rownames[c.cnt] = c.name.map_or(ptr::null(), |s| s.as_ptr() as (*const c_char));
            sense[c.cnt] = c.sense as c_char;
            matcnt[c.cnt] = e.vars.len() as CPXDIM;
            matbeg_cnt += e.vars.len() as CPXNNZ;

            for (var, coef) in e.vars {
                matind.push(var as i32);
                matval.push(coef);
            }

            rhs[c.cnt] = if e.offs.classify() == FpCategory::Normal {
                c.rhs - e.offs
            } else {
                c.rhs
            };

        }
        // println!("{:?} {:?} {:?} {:?}", matbeg, matcnt, matind, matval);

        // setup objective function coefficients
        let o = match self.obj_fun {
            None => return Err(Error::custom_error("An no extractable objective function was set")),
            Some(e) => e,
        };
        for (var, coef) in o.vars {
            obj[var] = coef;
        }

        let env = p.get_env();
        let cpx = p.get_lp();
        try!(cpx_call!(CPXLcopylpwnames,
                       env,
                       cpx,
                       numcols as i32,
                       numrows as i32,
                       self.obj as i32,
                       obj.as_ptr(),
                       rhs.as_ptr(),
                       sense.as_ptr(),
                       matbeg.as_ptr(),
                       matcnt.as_ptr(),
                       matind.as_ptr(),
                       matval.as_ptr(),
                       lb.as_ptr(),
                       ub.as_ptr(),
                       rng,
                       colnames.as_ptr(),
                       rownames.as_ptr()));
        try!(cpx_call!(CPXLcopyctype, env, cpx, ct.as_ptr()));
        // try!(cpx_call!(CPXLchgobjoffset, env, cpx, o.offs));
        Ok(())
    }
}

/// A constraint is an inequality of an expression and a constant
#[derive(Clone)]
pub struct Constraint {
    cnt: usize,
    sense: Sense,
    rhs: f64,
    expr: Option<Expr>,
    name: Option<String>,
}

impl Constraint {
    pub fn set_expr(&mut self, expr: Expr) -> &mut Self {
        self.expr = Some(expr);
        self
    }

    pub fn set_sense(&mut self, sense: Sense) -> &mut Self {
        self.sense = sense;
        self
    }

    pub fn set_rhs(&mut self, rhs: f64) -> &mut Self {
        self.rhs = rhs;
        self
    }

    pub fn set_name(&mut self, name: &str) -> &mut Self {
        self.name = Some(name.to_string());
        self
    }
}

/// An expression is a sum of variables (each variable may have a coefficent)
#[derive(Clone)]
pub struct Expr {
    vars: Vec<(usize, f64)>,
    offs: f64,
}

impl Expr {
    fn from_mult(lhs: &Var, rhs: f64) -> Self {
        Expr {
            vars: vec![(lhs.cnt, rhs)],
            offs: 0.0,
        }
    }
}

impl<'a> Add<&'a Var> for Expr {
    type Output = Expr;

    fn add(self, rhs: &'a Var) -> Expr {
        let mut e = Expr {
            vars: self.vars.clone(),
            offs: self.offs,
        };
        e.vars.push((rhs.cnt, 1.0));
        e
    }
}

impl<'a> Add<Expr> for &'a Var {
    type Output = Expr;

    fn add(self, rhs: Expr) -> Expr {
        let mut e = Expr {
            vars: rhs.vars.clone(),
            offs: rhs.offs,
        };
        e.vars.push((self.cnt, 1.0));
        e
    }
}

impl<'a> Add<&'a Var> for &'a Var {
    type Output = Expr;

    fn add(self, rhs: &'a Var) -> Expr {
        Expr {
            vars: vec![(self.cnt, 1.0), (rhs.cnt, 1.0)],
            offs: 0.0,
        }
    }
}

impl Add<Expr> for Expr {
    type Output = Expr;

    fn add(self, rhs: Expr) -> Expr {
        let mut e = self.clone();
        e.vars.extend_from_slice(rhs.vars.as_slice());
        e.offs += rhs.offs;
        e
    }
}

impl<'a> Mul<&'a Var> for i32 {
    type Output = Expr;

    fn mul(self, rhs: &'a Var) -> Expr {
        Expr::from_mult(rhs, self as f64)
    }
}

impl<'a> Mul<&'a Var> for f64 {
    type Output = Expr;

    fn mul(self, rhs: &'a Var) -> Expr {
        Expr::from_mult(rhs, self)
    }
}

/// A variable of the model
#[derive(Clone)]
pub struct Var {
    /// serial number of the variable in the model, corresponds to the CPLEX model row
    cnt: usize,
    /// type of this variable
    tp: VarType,
    /// lower bound
    lb: f64,
    /// uppper bound
    ub: f64,
    /// name of the variable in the model or default name
    name: Option<String>,
}

impl Var {
    /// set the lower bound for this variable
    pub fn set_lb(mut self, lb: f64) -> Self {
        self.lb = lb;
        self
    }

    /// get the lower bound for this variable
    pub fn get_lb(&self) -> f64 {
        self.lb
    }

    /// set the upper bound for this variable
    pub fn set_ub(mut self, ub: f64) -> Self {
        self.ub = ub;
        self
    }

    /// get the upper bound for this variable
    pub fn get_ub(&self) -> f64 {
        self.ub
    }

    /// set the name of the variable
    pub fn set_name(mut self, name: &str) -> Self {
        self.name = Some(name.to_string());
        self
    }

    /// get the name of the variable
    pub fn get_name<'a>(&'a self) -> Option<&'a str> {
        self.name.as_ref().map(|s| s.as_str())
    }
}
