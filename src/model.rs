//! ILO Concert like Model building

use std;
use std::ops::{Add, Mul};
use std::ptr;

extern crate libc;
use self::libc::c_char;

use cplex::{Problem, Extractable};
use error::{Error, PrivateErrorConstructor};
use cplex_sys::*;

/// Types of objectives
#[derive(Clone, Hash)]
pub enum Objective {
    /// objective is a minimizer
    Min = 1,
    /// objective is a maximizer
    Max = -1,
}

/// Type of a constraint
#[derive(Clone, Hash)]
pub enum Sense {
    LessThan = 'L' as isize,
    Equal = 'E' as isize,
    GreaterThan = 'G' as isize,
}

#[derive(Clone, Hash)]
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

    pub fn add_ineq(&mut self, expr: Expr, s: Sense, rhs: f64, name: Option<&str>) -> &mut Self {
        let cnt = self.v_cnt;
        self.v_cnt += 1;
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
            tp: VarType::Numeric as i8,
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
            tp: VarType::Integer as i8,
            lb: std::f64::MIN,
            ub: std::f64::MAX,
            name: None,
        });
        assert!(self.vars.len() == cnt + 1);
        &mut self.vars[cnt]
    }

    /// Create a new binary variable
    pub fn new_binary_var(&mut self) -> &mut Var {
        let cnt = self.v_cnt;
        self.v_cnt += 1;
        self.vars.push(Var {
            cnt: cnt,
            tp: VarType::Binary as i8,
            lb: 0.0,
            ub: 1.0,
            name: None,
        });
        assert!(self.vars.len() == cnt + 1);
        &mut self.vars[cnt]
    }
}

impl Extractable for Model {
    fn extract(self, p: &Problem) -> Result<(), Error> {
        let numcols = self.constraints.len();
        let numrows = self.vars.len();
        // Objective coefficent
        let obj = vec!(0.0; numrows);
        // first index for row
        let matbeg = vec!(0; numcols);
        // number of rows for specific column
        let matcnt = vec!(0; numcols);
        // indices for variables
        let matind = vec!(0; numrows);
        // Non-Zero coefficents
        let matval = vec!(0.0; numcols);
        let sense = vec!('E' as c_char; numcols);
        // RHS for each constraint
        let rhs = vec!(0.0; numcols);
        // for range constraints
        let rng = ptr::null();
        let colnames: Vec<*const c_char> = vec!(ptr::null(); numcols);

        let rownames: Vec<*const c_char> = vec!(ptr::null(); numrows);
        let lb = vec!(std::f64::MIN; numrows);
        let ub = vec!(std::f64::MAX; numrows);
        let ct = vec!('C' as c_char; numrows);

        // TODO functionality

        try!(cpx_call!(CPXLcopylpwnames,
                       p.env,
                       p.lp,
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
        try!(cpx_call!(CPXLcopyctype, p.env, p.lp, ct.as_ptr()));
        try!(cpx_call!(CPXLchgobjoffset, p.env, p.lp, self.obj_fun.unwrap().offs));
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
    vars: Vec<usize>,
    coef: Vec<f64>,
    offs: f64,
}

impl Expr {
    fn from_mult(lhs: &Var, rhs: f64) -> Self {
        Expr {
            vars: vec![lhs.cnt],
            coef: vec![rhs],
            offs: 0.0,
        }
    }
}

impl Add<Var> for Expr {
    type Output = Expr;

    fn add(self, rhs: Var) -> Expr {
        let mut e = Expr {
            vars: self.vars.clone(),
            coef: self.coef.clone(),
            offs: self.offs,
        };
        e.vars.push(rhs.cnt);
        e.coef.push(1.0);
        e
    }
}

impl Add<Expr> for Expr {
    type Output = Expr;

    fn add(self, rhs: Expr) -> Expr {
        let mut e = self.clone();
        e.vars.extend_from_slice(rhs.vars.as_slice());
        e.coef.extend_from_slice(rhs.coef.as_slice());
        e.offs += rhs.offs;
        e
    }
}

impl Mul<i32> for Var {
    type Output = Expr;

    fn mul(self, rhs: i32) -> Expr {
        Expr::from_mult(&self, rhs as f64)
    }
}

impl Mul<f64> for Var {
    type Output = Expr;

    fn mul(self, rhs: f64) -> Expr {
        Expr::from_mult(&self, rhs)
    }
}

/// A variable of the model
#[derive(Clone)]
pub struct Var {
    /// serial number of the variable in the model, corresponds to the CPLEX model row
    cnt: usize,
    /// type of this variable
    tp: i8,
    /// lower bound
    lb: f64,
    /// uppper bound
    ub: f64,
    /// name of the variable in the model or default name
    name: Option<String>,
}

impl Var {
    /// set the lower bound for this variable
    pub fn set_lb(&mut self, lb: f64) -> &mut Self {
        self.lb = lb;
        self
    }

    /// get the lower bound for this variable
    pub fn get_lb(&self) -> f64 {
        self.lb
    }

    /// set the upper bound for this variable
    pub fn set_ub(&mut self, ub: f64) -> &mut Self {
        self.ub = ub;
        self
    }

    /// get the upper bound for this variable
    pub fn get_ub(&self) -> f64 {
        self.ub
    }

    /// set the name of the variable
    pub fn set_name(&mut self, name: &str) -> &mut Self {
        self.name = Some(name.to_string());
        self
    }

    /// get the name of the variable
    pub fn get_name<'a>(&'a self) -> Option<&'a str> {
        self.name.as_ref().map(|s| s.as_str())
    }
}
