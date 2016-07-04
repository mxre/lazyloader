//! ILO Concert like Model building

#![allow(unused_variables)]
use std;
use std::ops::{Add, Mul};

/// ILP model, containing objective function and set of constraints
#[derive(Clone)]
pub struct Model {

}

impl Model {
	/// Create a new model
    pub fn new() -> Self {
        Model {}
    }

    /// Add a constraint to the model and extract the variables
    pub fn add(&mut self, c: Constraint) -> &mut Self {
        self
    }

    /// Add the objective function of the model
    pub fn set_objective(m: Objective, o: Expr) {
    }

    /// Create a new numeric (continous) variable
    pub fn new_numeric_var() -> Var {
        Var {
            tp: 'C' as i8,
            lb: std::f64::MIN,
            ub: std::f64::MAX,
            name: String::new(),
        }
    }

    /// Create a new integer (non-continous) variable
    pub fn new_integer_var() -> Var {
        Var {
            tp: 'I' as i8,
            lb: std::f64::MIN,
            ub: std::f64::MAX,
            name: String::new(),
        }
    }

    /// Create a new binary variable
    pub fn new_binary_var() -> Var {
        Var {
            tp: 'B' as i8,
            lb: 0.0,
            ub: 1.0,
            name: String::new(),
        }
    }

}

/// Types of objectives
#[derive(Clone)]
pub enum Objective {
	/// objective is a minimizer
    Min = 1,
    /// objective is a maximizer
    Max = -1,
}

/// A constraint is an inequality of an expression and a constant
#[derive(Clone)]
pub struct Constraint {
    sense: Sense,
    rhs: f64,
    expr: Option<Expr>,
    name: String,
}

/// Type of a constraint
#[derive(Clone)]
pub enum Sense {
	LessThan = 'L' as isize,
	Equal = 'E' as isize,
	GreaterThan = 'G' as isize,
}

impl Constraint {
	pub fn new() -> Self {
		Constraint {
			sense: Sense::Equal,
			rhs: 0.0,
			name: String::new(),
			expr: None,
		}
	}

	pub fn set_expr(&mut self, expr: Expr) -> &mut Self {
		self.expr = Some(expr);
		self
	}

	pub fn set_sense(&mut self, sense: Sense) -> &mut Self {
		self.sense = sense;
		self
	}

	pub fn set_rhs (&mut self, rhs: f64) -> &mut Self {
		self.rhs = rhs;
		self
	}

	pub fn set_name(&mut self, name: &str) -> &mut Self {
		self.name = name.to_string();
		self
	}
}

/// An expression is a sum of variables (each variable may have a coefficent)
#[derive(Clone)]
pub struct Expr {
}

impl Add<Var> for Expr {
    type Output = Expr;

    fn add(self, rhs: Var) -> Expr {
        self
    }
}

/// A variable of the model
#[derive(Clone)]
pub struct Var {
    tp: i8,
    lb: f64,
    ub: f64,
    name: String,
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
        self.name = name.to_string();
        self
    }

    /// get the name of the variable
    pub fn get_name<'a>(&'a self) -> &'a str {
        self.name.as_str()
    }
}

impl Mul<i32> for Var {
	type Output = Var;

	fn mul(self, rhs: i32) -> Var {
		self
	}
}
