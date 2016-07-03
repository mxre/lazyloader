//! CPLEX parameter
use cplex_sys::*;
use error::Error;

/// Type for integer parameter
pub struct IntParameter(i32);
/// Type for numeric parameter
pub struct DblParameter(i32);
/// Type for string parameter
pub struct StrParameter(i32);

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

/// CPX_PARAM_THREADS
pub const PARAM_THREADS: IntParameter = IntParameter(CPX_PARAM_THREADS);
/// CPX_PARAM_TILIM
pub const PARAM_TIMELIMIT: DblParameter = DblParameter(CPX_PARAM_TILIM);

/// CPX_PARAM_MIPDISPLAY
pub const PARAM_MIP_DISPLAY: IntParameter = IntParameter(CPX_PARAM_MIPDISPLAY);
/// CPX_PARAM_EPAGAP
pub const PARAM_MIP_TOLERANCES_ABSMIPGAP: DblParameter = DblParameter(CPX_PARAM_EPAGAP);
/// CPX_PARAM_EPGAP
pub const PARAM_MIP_TOLERANCES_MIPGAP: DblParameter = DblParameter(CPX_PARAM_EPGAP);

/// Gets and Sets double parameters
/// # Native calls
/// `CPXsetdblparam` and `CPXgetdblparam`
pub trait DblParam {
    fn set_param(&self, what: DblParameter, value: f64) -> Result<(), Error>;
    fn get_param(&self, what: DblParameter) -> Result<f64, Error>;
}

/// Gets and Sets integer parameters
/// # Native calls
/// `CPXsetintparam` and `CPXgetintparam`
pub trait IntParam {
    fn set_param(&self, what: IntParameter, value: i32) -> Result<(), Error>;
    fn get_param(&self, what: IntParameter) -> Result<i32, Error>;
}

/// Gets and Sets string parameters
/// # Native calls
/// `CPXsetstrparam` and `CPXgetstrparam`
pub trait StrParam {
    fn set_param(&self, what: StrParameter, value: &str) -> Result<(), Error>;
    fn get_param(&self, what: StrParameter) -> Result<&str, Error>;
}
