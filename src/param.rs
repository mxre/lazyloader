//! CPLEX parameter
use cplex_sys::*;
use error::{Error, PrivateErrorConstructor};

/// Type for integer parameter
#[derive(Clone, PartialEq, Eq)]
pub struct IntParameter(i32);

/// Type for long integer parameter
#[derive(Clone, PartialEq, Eq)]
pub struct LongParameter(i32);

/// Type for boolean parameter
#[derive(Clone, PartialEq, Eq)]
pub struct BoolParameter(i32);

/// Type for numeric parameter
#[derive(Clone, PartialEq, Eq)]
pub struct DblParameter(i32);

/// Type for string parameter
#[derive(Clone, PartialEq, Eq)]
pub struct StrParameter(i32);

/// CPX_PARAM_ADVIND
pub const ADVANCE: IntParameter = IntParameter(CPX_PARAM_ADVIND);
/// CPX_PARAM_CLOCKTYPE
pub const CLOCK_TYPE: IntParameter = IntParameter(CPX_PARAM_CLOCKTYPE);
/// CPX_PARAM_THREADS
pub const THREADS: IntParameter = IntParameter(CPX_PARAM_THREADS);
/// CPX_PARAM_TILIM
pub const TIME_LIMIT: DblParameter = DblParameter(CPX_PARAM_TILIM);
/// CPX_PARAM_WORKDIR
pub const WORK_DIR: StrParameter = StrParameter(CPX_PARAM_WORKDIR);
/// CPX_PARAM_WORKMEM
pub const WORK_MEM: DblParameter = DblParameter(CPX_PARAM_WORKMEM);
/// CPX_PARAM_SCRIND
pub const SCREEN_OUTPUT: BoolParameter = BoolParameter(CPX_PARAM_SCRIND);


/// Paramter driving alogrithm emphasis
pub mod emphasis {
    use ::param::{BoolParameter, IntParameter};
    use ::cplex_sys::*;

    /// CPX_PARAM_MEMORYEMPHASIS
    pub const MEMORY: BoolParameter = BoolParameter(CPX_PARAM_MEMORYEMPHASIS);
    /// CPX_PARAM_MIPEMPHASIS
    pub const MIP: IntParameter = IntParameter(CPX_PARAM_MIPEMPHASIS);
    /// CPX_PARAM_NUMERICALEMPHASIS
    pub const NUMERICAL: BoolParameter = BoolParameter(CPX_PARAM_NUMERICALEMPHASIS);
}

/// Parameters driving the MIP algorithm
pub mod mip {
    use ::param::{IntParameter, LongParameter};
    use ::cplex_sys::*;

    /// CPX_PARAM_MIPDISPLAY
    pub const DISPLAY: IntParameter = IntParameter(CPX_PARAM_MIPDISPLAY);
    /// CPX_PARAM_MIPINTERVAL
    pub const INTERVAL: LongParameter = LongParameter(CPX_PARAM_MIPINTERVAL);

    pub mod cuts {
        use ::param::IntParameter;
        use ::cplex_sys::*;

        /// CPX_PARAM_CLIQUES
        pub const CLIQUES: IntParameter = IntParameter(CPX_PARAM_CLIQUES);
        /// CPX_PARAM_COVERS
        pub const COVERS: IntParameter = IntParameter(CPX_PARAM_COVERS);
        /// CPX_PARAM_DISJCUTS
        pub const DISJUNCITVE: IntParameter = IntParameter(CPX_PARAM_DISJCUTS);
        /// CPX_PARAM_FLOWCOVERS
        pub const FLOW_COVERS: IntParameter = IntParameter(CPX_PARAM_FLOWCOVERS);
        /// CPX_PARAM_FRACCUTS
        pub const GOMORY: IntParameter = IntParameter(CPX_PARAM_FRACCUTS);
        /// CPX_PARAM_GUBCOVERS
        pub const GUB_COVERS: IntParameter = IntParameter(CPX_PARAM_GUBCOVERS);
        /// CPX_PARAM_IMPLBD
        pub const IMPLIED: IntParameter = IntParameter(CPX_PARAM_IMPLBD);
        /// CPX_PARAM_LANDPCUTS
        pub const LIFT_PROJ: IntParameter = IntParameter(CPX_PARAM_LANDPCUTS);
        /// CPX_PARAM_MCFCUTS
        pub const MCF_CUT: IntParameter = IntParameter(CPX_PARAM_MCFCUTS);
        /// CPX_PARAM_MIRCUTS
        pub const MIR_CUT: IntParameter = IntParameter(CPX_PARAM_MIRCUTS);
        /// CPX_PARAM_FLOWPATHS
        pub const PATH_CUT: IntParameter = IntParameter(CPX_PARAM_FLOWPATHS);
        /// CPX_PARAM_ZEROHALFCUTS
        pub const ZERO_HALF_CUT: IntParameter = IntParameter(CPX_PARAM_ZEROHALFCUTS);
    }

    /// Tolerances for the MIP algorithm
    pub mod tolerances {
        use ::param::DblParameter;
        use ::cplex_sys::*;

        /// CPX_PARAM_EPAGAP
        pub const ABSOLUTE_GAP: DblParameter = DblParameter(CPX_PARAM_EPAGAP);
        /// CPX_PARAM_EPLIN
        pub const LINEARIZATION: DblParameter = DblParameter(CPX_PARAM_EPLIN);
        /// CPX_PARAM_EPINT
        pub const INTEGRALITY: DblParameter = DblParameter(CPX_PARAM_EPINT);
        /// CPX_PARAM_CUTLO
        pub const LOWER_CUTOFF: DblParameter = DblParameter(CPX_PARAM_CUTLO);
        /// CPX_PARAM_EPGAP
        pub const GAP: DblParameter = DblParameter(CPX_PARAM_EPGAP);
        /// CPX_PARAM_OBJDIF
        pub const OBJ_DIFFERENCE: DblParameter = DblParameter(CPX_PARAM_OBJDIF);
        /// CPX_PARAM_RELOBJDIF
        pub const REL_OBJ_DIFFERENCE: DblParameter = DblParameter(CPX_PARAM_RELOBJDIF);
        /// CPX_PARAM_CUTUP
        pub const UPPER_CUTOFF: DblParameter = DblParameter(CPX_PARAM_CUTUP);
    }
}

pub mod preprocessing {
    use ::param::{IntParameter, LongParameter, BoolParameter};
    use ::cplex_sys::*;

    /// CPX_PARAM_AGGIND
    pub const AGGREGATOR: IntParameter = IntParameter(CPX_PARAM_AGGIND);
    /// CPX_PARAM_BNDSTRENIND
    pub const BOUND_STRENGTH: IntParameter = IntParameter(CPX_PARAM_BNDSTRENIND);
    /// CPX_PARAM_COEREDIND
    pub const COEF_REDUCE: IntParameter = IntParameter(CPX_PARAM_COEREDIND);
    /// CPX_PARAM_DEPIND
    pub const DEPENDENCY: IntParameter = IntParameter(CPX_PARAM_DEPIND);
    /// CPX_PARAM_PREDUAL
    pub const DUAL: IntParameter = IntParameter(CPX_PARAM_PREDUAL);
    /// CPX_PARAM_AGGFILL
    pub const FILL: LongParameter = LongParameter(CPX_PARAM_AGGFILL);
    /// CPX_PARAM_PRELINEAR
    pub const LINEAR: IntParameter = IntParameter(CPX_PARAM_PRELINEAR);
    /// CPX_PARAM_PREPASS
    pub const NUM_PASS: IntParameter = IntParameter(CPX_PARAM_PREPASS);
    /// CPX_PARAM_PREIND
    pub const PRESOLVE: BoolParameter = BoolParameter(CPX_PARAM_PREIND);
    /// CPX_PARAM_CALCQCPDUALS
    pub const QCP_DUALS: IntParameter = IntParameter(CPX_PARAM_CALCQCPDUALS);
    /// CPX_PARAM_QPMAKEPSDIND
    pub const QP_MAKE_PSD: BoolParameter = BoolParameter(CPX_PARAM_QPMAKEPSDIND);
    /// CPX_PARAM_REDUCE
    pub const REDUCE: IntParameter = IntParameter(CPX_PARAM_REDUCE);
    /// CPX_PARAM_RELAXPREIND
    pub const RELAX: IntParameter = IntParameter(CPX_PARAM_RELAXPREIND);
    /// CPX_PARAM_REPEATPRESOLVE
    pub const REPEAT_PRESOLVE: IntParameter = IntParameter(CPX_PARAM_REPEATPRESOLVE);
    /// CPX_PARAM_SYMMETRY
    pub const SYMMETRY: IntParameter = IntParameter(CPX_PARAM_SYMMETRY);
}

pub mod tune {
    use ::param::{IntParameter, DblParameter};
    use ::cplex_sys::*;

    /// CPX_PARAM_TUNINGDETTILIM
    pub const DET_TIME_LIMIT: DblParameter = DblParameter(CPX_PARAM_TUNINGDETTILIM);
    /// CPX_PARAM_TUNINGDISPLAY
    pub const DISPLAY: IntParameter = IntParameter(CPX_PARAM_TUNINGDISPLAY);
    /// CPX_PARAM_TUNINGMEASURE
    pub const MEASURE: IntParameter = IntParameter(CPX_PARAM_TUNINGMEASURE);
    /// CPX_PARAM_TUNINGREPEAT
    pub const REPEAT: IntParameter = IntParameter(CPX_PARAM_TUNINGREPEAT);
    /// CPX_PARAM_TUNINGTILIM
    pub const TIME_LIMIT: DblParameter = DblParameter(CPX_PARAM_TUNINGTILIM);
}

pub mod read {
    use ::param::{IntParameter, BoolParameter, StrParameter, LongParameter};
    use ::cplex_sys::*;

    // pub const API_ENCODING: StrParameter = StrParameter(CPX_PARAM_APIENCODING);
    pub const CONSTRAINTS: IntParameter = IntParameter(CPX_PARAM_ROWREADLIM);
    pub const DATACHECK: BoolParameter = BoolParameter(CPX_PARAM_DATACHECK);
    pub const FILE_ENCODING: StrParameter = StrParameter(CPX_PARAM_FILEENCODING);
    pub const NON_ZEROS: LongParameter = LongParameter(CPX_PARAM_NZREADLIM);
    // pub const QP_NON_ZEROS: LongParameter = LongParameter(CPX_PARAM_QPNZREADLIM);
    pub const SCALE: IntParameter = IntParameter(CPX_PARAM_SCAIND);
    pub const VARIABLES: IntParameter = IntParameter(CPX_PARAM_COLREADLIM);
}

/// Common trait for all parameters
pub trait ParameterType {
    /// Type of the paramters value, for the setter.
    /// For strings This can be `&str`
    type InType;
    /// Type of the paramters value, for the getter
    /// For strings this can be `String`
    type ReturnType;

    /// Setter for the parameter
    fn set(self, env: *mut CPXenv, value: Self::InType) -> Result<(), Error>;

    /// Getter for the parameter
    fn get(self, env: *const CPXenv) -> Result<Self::ReturnType, Error>;
}

// IntParameter //////////////////////////////////////////////////////////////////////////////////
impl ParameterType for IntParameter {
    type InType = i32;
    type ReturnType = i32;

    fn set(self, env: *mut CPXenv, value: i32) -> Result<(), Error> {
        cpx_call!(CPXLsetintparam, env, Self::into(self), value)
    }

    fn get(self, env: *const CPXenv) -> Result<i32, Error> {
        let mut value: i32 = 0;
        cpx_return!(CPXLgetintparam, value, env, Self::into(self), &mut value)
    }
}

impl From<i32> for IntParameter {
    fn from(param: i32) -> Self {
        IntParameter(param)
    }
}

impl From<IntParameter> for i32 {
    fn from(param: IntParameter) -> i32 {
        param.0
    }
}

/// LongParameter ////////////////////////////////////////////////////////////////////////////////
impl ParameterType for LongParameter {
    type InType = i64;
    type ReturnType = i64;

    fn set(self, env: *mut CPXenv, value: i64) -> Result<(), Error> {
        cpx_call!(CPXLsetlongparam, env, Self::into(self), value)
    }

    fn get(self, env: *const CPXenv) -> Result<i64, Error> {
        let mut value: i64 = 0;
        cpx_return!(CPXLgetlongparam, value, env, Self::into(self), &mut value)
    }
}

impl From<i32> for LongParameter {
    fn from(param: i32) -> Self {
        LongParameter(param)
    }
}

impl From<LongParameter> for i32 {
    fn from(param: LongParameter) -> i32 {
        param.0
    }
}

// DblParameter //////////////////////////////////////////////////////////////////////////////////
impl ParameterType for DblParameter {
    type InType = f64;
    type ReturnType = f64;

    fn set(self, env: *mut CPXenv, value: f64) -> Result<(), Error> {
        cpx_call!(CPXLsetdblparam, env, Self::into(self), value)
    }

    fn get(self, env: *const CPXenv) -> Result<f64, Error> {
        let mut value: f64 = 0.0;
        cpx_return!(CPXLgetdblparam, value, env, Self::into(self), &mut value)
    }
}

impl From<i32> for DblParameter {
    fn from(param: i32) -> Self {
        DblParameter(param)
    }
}

impl From<DblParameter> for i32 {
    fn from(param: DblParameter) -> i32 {
        param.0
    }
}

// BoolParameter /////////////////////////////////////////////////////////////////////////////////
impl ParameterType for BoolParameter {
    type InType = bool;
    type ReturnType = bool;

    fn set(self, env: *mut CPXenv, value: bool) -> Result<(), Error> {
        cpx_call!(CPXLsetintparam, env, Self::into(self), value as i32)
    }

    fn get(self, env: *const CPXenv) -> Result<bool, Error> {
        let mut value: i32 = 0;
        cpx_return!(CPXLgetintparam,
                    value == 1,
                    env,
                    Self::into(self),
                    &mut value)
    }
}

impl From<i32> for BoolParameter {
    fn from(param: i32) -> Self {
        BoolParameter(param)
    }
}

impl From<BoolParameter> for i32 {
    fn from(param: BoolParameter) -> i32 {
        param.0
    }
}

// StrParameter //////////////////////////////////////////////////////////////////////////////////
use std::string::String;
use std::ffi::CString;
impl ParameterType for StrParameter {
    type InType = &'static str;
    type ReturnType = String;

    fn set<'a>(self, env: *mut CPXenv, value: &'a str) -> Result<(), Error> {
        cpx_call!(CPXLsetstrparam, env, Self::into(self), str_as_ptr!(value))
    }

    fn get(self, env: *const CPXenv) -> Result<String, Error> {
        let message = unsafe { CString::from_vec_unchecked(Vec::with_capacity(CPXMESSAGEBUFSIZE)) };
        let c_msg = message.into_raw();
        cpx_return!(CPXLgetstrparam,
                    unsafe { CString::from_raw(c_msg).to_str().unwrap().to_string() },
                    env,
                    Self::into(self),
                    c_msg)
    }
}

impl From<i32> for StrParameter {
    fn from(param: i32) -> Self {
        StrParameter(param)
    }
}

impl From<StrParameter> for i32 {
    fn from(param: StrParameter) -> i32 {
        param.0
    }
}
