//! CPLEX Rust adapter crate

#[cfg(feature = "cpx_expose")]
pub mod cplex_sys;
#[cfg(not(feature = "cpx_expose"))]
mod cplex_sys;

#[macro_use]
mod macros;

mod error;
pub mod param;
mod env;
mod cplex;
pub mod callback;
pub mod model;

pub use cplex::Raw;
pub use error::Error;
pub use cplex::Problem;
pub use env::Env;

#[cfg(feature = "cpx_expose")]
pub use env::PrivateEnv;
#[cfg(feature = "cpx_expose")]
pub use cplex::PrivateProblem;

/// Orginal C names for CPLEX constants
pub mod constants {
    pub use cplex_sys::constants::*;
}
