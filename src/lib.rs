//! CPLEX Rust adapter crate

#![allow(dead_code)]

mod cplex_sys;

#[macro_use]
mod macros;

pub use param::*;
pub use error::*;
pub use cplex::*;
pub use env::*;

pub mod error;
pub mod param;
pub mod env;
pub mod cplex;
