//! CPLEX Rust adapter crate

#[macro_use]
mod macros;

#[cfg(feature = "cpx_expose")]
pub mod cplex_sys;
#[cfg(feature = "cpx_expose")]
pub mod env;
#[cfg(feature = "cpx_expose")]
pub mod cplex;

#[cfg(not(feature = "cpx_expose"))]
mod cplex_sys;
#[cfg(not(feature = "cpx_expose"))]
mod env;
#[cfg(not(feature = "cpx_expose"))]
mod cplex;

mod error;
pub mod param;
pub mod callback;
pub mod model;

pub use cplex::Raw;
pub use error::Error;
pub use cplex::Problem;
pub use env::Env;

/// Orginal C names for CPLEX constants
pub mod constants {
    pub use cplex_sys::constants::*;
}
