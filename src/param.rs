//! CPLEX parameter
use cplex_sys::*;
use error::Error;

/// Type for integer parameter
pub struct IntParameter(i32);
/// Type for numeric parameter
pub struct DblParameter(i32);
/// Type for string parameter
pub struct StrParameter(i32);


/// CPX_PARAM_THREADS
pub const THREADS: IntParameter = IntParameter(CPX_PARAM_THREADS);
/// CPX_PARAM_TILIM
pub const TIMELIMIT: DblParameter = DblParameter(CPX_PARAM_TILIM);

/// Parameters driving the MIP algorithm
pub mod mip {
	use ::param::*;
	use ::cplex_sys::*;

    /// CPX_PARAM_MIPDISPLAY
    pub const DISPLAY: IntParameter = IntParameter(CPX_PARAM_MIPDISPLAY);

	/// Tolorances for the MIP algorithm
    pub mod tolerances {
		use ::param::*;
		use ::cplex_sys::*;

        /// CPX_PARAM_EPAGAP
        pub const ABSOLUTE_GAP: DblParameter = DblParameter(CPX_PARAM_EPAGAP);
        /// CPX_PARAM_EPGAP
        pub const GAP: DblParameter = DblParameter(CPX_PARAM_EPGAP);
    }
}

/// Common trait for all parameters
pub trait ParameterType {
	/// Type of the paramters value
    type ValueType;

    /// Setter for the parameter
    fn set(self, env: *mut CPXenv, value: Self::ValueType) -> Result<(), Error>;

    /// Getter for the parameter
    fn get(self, env: *const CPXenv) -> Result<Self::ValueType, Error>;
}

impl From<i32> for DblParameter {
    fn from(param: i32) -> DblParameter {
        DblParameter(param)
    }
}

impl From<DblParameter> for i32 {
    fn from(param: DblParameter) -> i32 {
        param.0
    }
}

impl From<i32> for IntParameter {
    fn from(param: i32) -> IntParameter {
        IntParameter(param)
    }
}

impl From<IntParameter> for i32 {
    fn from(param: IntParameter) -> i32 {
        param.0
    }
}

impl ParameterType for IntParameter {
    type ValueType = i32;

    fn set(self, env: *mut CPXenv, value: i32) -> Result<(), Error> {
        cpx_call!(CPXsetintparam, env, IntParameter::into(self), value)
    }

    fn get(self, env: *const CPXenv) -> Result<i32, Error> {
        let mut value: i32 = 0;
        cpx_return!(CPXgetintparam,
                    value,
                    env,
                    IntParameter::into(self),
                    &mut value)
    }
}

impl ParameterType for DblParameter {
    type ValueType = f64;

    fn set(self, env: *mut CPXenv, value: f64) -> Result<(), Error> {
        cpx_call!(CPXsetdblparam, env, DblParameter::into(self), value)
    }

    fn get(self, env: *const CPXenv) -> Result<f64, Error> {
        let mut value: f64 = 0.0;
        cpx_return!(CPXgetdblparam,
                    value,
                    env,
                    DblParameter::into(self),
                    &mut value)
    }
}
