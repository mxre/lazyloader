//! CPLEX Rust adapter crate

mod cplex_sys;

#[macro_use]
mod macros;

mod error;
pub mod param;
mod env;
mod cplex;
pub mod model;

pub use cplex::Raw;
pub use error::Error;
pub use cplex::Problem;
pub use env::Env;

/// Orginal C names for CPLEX constants
pub mod constants {
    pub use cplex_sys::constants::*;
}
