//! CPLEX Rust adapter crate

#![allow(dead_code)]

mod cplex_sys;

#[macro_use]
mod macros;

pub mod error;
pub mod param;
pub mod env;
pub mod cplex;

pub use error::Error;
pub use cplex::Problem;
pub use env::Env;
