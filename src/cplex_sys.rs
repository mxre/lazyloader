//! CPLEX Callable C-Library

extern crate libc;
use self::libc::{c_char, c_int, c_double, int64_t};

pub enum CPXenv {}

pub enum CPXlp {}

pub const CPXMESSAGEBUFSIZE: usize = 1024;

pub const CPX_PARAM_ADVIND: c_int = 1001;
pub const CPX_PARAM_AGGFILL: c_int = 1002;
pub const CPX_PARAM_AGGIND: c_int = 1003;
pub const CPX_PARAM_BASINTERVA: c_int = 1004;
pub const CPX_PARAM_CFILEMUL: c_int = 1005;
pub const CPX_PARAM_CLOCKTYPE: c_int = 1006;
pub const CPX_PARAM_CRAIND: c_int = 1007;
pub const CPX_PARAM_DEPIND: c_int = 1008;
pub const CPX_PARAM_DPRIIND: c_int = 1009;
pub const CPX_PARAM_PRICELIM: c_int = 1010;
pub const CPX_PARAM_EPMRK: c_int = 1013;
pub const CPX_PARAM_EPOPT: c_int = 1014;
pub const CPX_PARAM_EPPER: c_int = 1015;
pub const CPX_PARAM_EPRHS: c_int = 1016;
pub const CPX_PARAM_FASTMIP: c_int = 1017;
pub const CPX_PARAM_SIMDISPLAY: c_int = 1019;
pub const CPX_PARAM_ITLIM: c_int = 1020;
pub const CPX_PARAM_ROWREADLIM: c_int = 1021;
pub const CPX_PARAM_NETFIND: c_int = 1022;
pub const CPX_PARAM_COLREADLIM: c_int = 1023;
pub const CPX_PARAM_NZREADLIM: c_int = 1024;
pub const CPX_PARAM_OBJLLIM: c_int = 1025;
pub const CPX_PARAM_OBJULIM: c_int = 1026;
pub const CPX_PARAM_PERIND: c_int = 1027;
pub const CPX_PARAM_PERLIM: c_int = 1028;
pub const CPX_PARAM_PPRIIND: c_int = 1029;
pub const CPX_PARAM_PREIND: c_int = 1030;
pub const CPX_PARAM_REINV: c_int = 1031;
pub const CPX_PARAM_REVERSEIND: c_int = 1032;
pub const CPX_PARAM_RFILEMUL: c_int = 1033;
pub const CPX_PARAM_SCAIND: c_int = 1034;
pub const CPX_PARAM_SCRIND: c_int = 1035;
pub const CPX_PARAM_SINGLIM: c_int = 1037;
pub const CPX_PARAM_SINGTOL: c_int = 1038;
pub const CPX_PARAM_TILIM: c_int = 1039;
pub const CPX_PARAM_XXXIND: c_int = 1041;
pub const CPX_PARAM_PREDUAL: c_int = 1044;
pub const CPX_PARAM_EPOPT_H: c_int = 1049;
pub const CPX_PARAM_EPRHS_H: c_int = 1050;
pub const CPX_PARAM_PREPASS: c_int = 1052;
pub const CPX_PARAM_DATACHECK: c_int = 1056;
pub const CPX_PARAM_REDUCE: c_int = 1057;
pub const CPX_PARAM_PRELINEAR: c_int = 1058;
pub const CPX_PARAM_LPMETHOD: c_int = 1062;
pub const CPX_PARAM_QPMETHOD: c_int = 1063;
pub const CPX_PARAM_WORKDIR: c_int = 1064;
pub const CPX_PARAM_WORKMEM: c_int = 1065;
pub const CPX_PARAM_THREADS: c_int = 1067;
pub const CPX_PARAM_CONFLICTDISPLAY: c_int = 1074;
pub const CPX_PARAM_SIFTDISPLAY: c_int = 1076;
pub const CPX_PARAM_SIFTALG: c_int = 1077;
pub const CPX_PARAM_SIFTITLIM: c_int = 1078;
pub const CPX_PARAM_MPSLONGNUM: c_int = 1081;
pub const CPX_PARAM_MEMORYEMPHASIS: c_int = 1082;
pub const CPX_PARAM_NUMERICALEMPHASIS: c_int = 1083;
pub const CPX_PARAM_FEASOPTMODE: c_int = 1084;
pub const CPX_PARAM_PARALLELMODE: c_int = 1109;
pub const CPX_PARAM_TUNINGMEASURE: c_int = 1110;
pub const CPX_PARAM_TUNINGREPEAT: c_int = 1111;
pub const CPX_PARAM_TUNINGTILIM: c_int = 1112;
pub const CPX_PARAM_TUNINGDISPLAY: c_int = 1113;
pub const CPX_PARAM_WRITELEVEL: c_int = 1114;
pub const CPX_PARAM_RANDOMSEED: c_int = 1124;
pub const CPX_PARAM_DETTILIM: c_int = 1127;
pub const CPX_PARAM_FILEENCODING: c_int = 1129;
pub const CPX_PARAM_APIENCODING: c_int = 1130;
pub const CPX_PARAM_SOLUTIONTARGET: c_int = 1131;
pub const CPX_PARAM_CLONELOG: c_int = 1132;
pub const CPX_PARAM_TUNINGDETTILIM: c_int = 1139;

pub const CPX_PARAM_BRDIR: c_int = 2001;
pub const CPX_PARAM_BTTOL: c_int = 2002;
pub const CPX_PARAM_CLIQUES: c_int = 2003;
pub const CPX_PARAM_COEREDIND: c_int = 2004;
pub const CPX_PARAM_COVERS: c_int = 2005;
pub const CPX_PARAM_CUTLO: c_int = 2006;
pub const CPX_PARAM_CUTUP: c_int = 2007;
pub const CPX_PARAM_EPAGAP: c_int = 2008;
pub const CPX_PARAM_EPGAP: c_int = 2009;
pub const CPX_PARAM_EPINT: c_int = 2010;
pub const CPX_PARAM_MIPDISPLAY: c_int = 2012;
pub const CPX_PARAM_MIPINTERVAL: c_int = 2013;
pub const CPX_PARAM_INTSOLLIM: c_int = 2015;
pub const CPX_PARAM_NODEFILEIND: c_int = 2016;
pub const CPX_PARAM_NODELIM: c_int = 2017;
pub const CPX_PARAM_NODESEL: c_int = 2018;
pub const CPX_PARAM_OBJDIF: c_int = 2019;
pub const CPX_PARAM_MIPORDIND: c_int = 2020;
pub const CPX_PARAM_RELOBJDIF: c_int = 2022;
pub const CPX_PARAM_STARTALG: c_int = 2025;
pub const CPX_PARAM_SUBALG: c_int = 2026;
pub const CPX_PARAM_TRELIM: c_int = 2027;
pub const CPX_PARAM_VARSEL: c_int = 2028;
pub const CPX_PARAM_BNDSTRENIND: c_int = 2029;
pub const CPX_PARAM_HEURFREQ: c_int = 2031;
pub const CPX_PARAM_MIPORDTYPE: c_int = 2032;
pub const CPX_PARAM_CUTSFACTOR: c_int = 2033;
pub const CPX_PARAM_RELAXPREIND: c_int = 2034;
pub const CPX_PARAM_PRESLVND: c_int = 2037;
pub const CPX_PARAM_BBINTERVAL: c_int = 2039;
pub const CPX_PARAM_FLOWCOVERS: c_int = 2040;
pub const CPX_PARAM_IMPLBD: c_int = 2041;
pub const CPX_PARAM_PROBE: c_int = 2042;
pub const CPX_PARAM_GUBCOVERS: c_int = 2044;
pub const CPX_PARAM_STRONGCANDLIM: c_int = 2045;
pub const CPX_PARAM_STRONGITLIM: c_int = 2046;
pub const CPX_PARAM_FRACCAND: c_int = 2048;
pub const CPX_PARAM_FRACCUTS: c_int = 2049;
pub const CPX_PARAM_FRACPASS: c_int = 2050;
pub const CPX_PARAM_FLOWPATHS: c_int = 2051;
pub const CPX_PARAM_MIRCUTS: c_int = 2052;
pub const CPX_PARAM_DISJCUTS: c_int = 2053;
pub const CPX_PARAM_AGGCUTLIM: c_int = 2054;
pub const CPX_PARAM_MIPCBREDLP: c_int = 2055;
pub const CPX_PARAM_CUTPASS: c_int = 2056;
pub const CPX_PARAM_MIPEMPHASIS: c_int = 2058;
pub const CPX_PARAM_SYMMETRY: c_int = 2059;
pub const CPX_PARAM_DIVETYPE: c_int = 2060;
pub const CPX_PARAM_RINSHEUR: c_int = 2061;
pub const CPX_PARAM_SUBMIPNODELIM: c_int = 2062;
pub const CPX_PARAM_LBHEUR: c_int = 2063;
pub const CPX_PARAM_REPEATPRESOLVE: c_int = 2064;
pub const CPX_PARAM_PROBETIME: c_int = 2065;
pub const CPX_PARAM_POLISHTIME: c_int = 2066;
pub const CPX_PARAM_REPAIRTRIES: c_int = 2067;
pub const CPX_PARAM_EPLIN: c_int = 2068;
pub const CPX_PARAM_EPRELAX: c_int = 2073;
pub const CPX_PARAM_FPHEUR: c_int = 2098;
pub const CPX_PARAM_EACHCUTLIM: c_int = 2102;
pub const CPX_PARAM_SOLNPOOLCAPACITY: c_int = 2103;
pub const CPX_PARAM_SOLNPOOLREPLACE: c_int = 2104;
pub const CPX_PARAM_SOLNPOOLGAP: c_int = 2105;
pub const CPX_PARAM_SOLNPOOLAGAP: c_int = 2106;
pub const CPX_PARAM_SOLNPOOLINTENSITY: c_int = 2107;
pub const CPX_PARAM_POPULATELIM: c_int = 2108;
pub const CPX_PARAM_MIPSEARCH: c_int = 2109;
pub const CPX_PARAM_MIQCPSTRAT: c_int = 2110;
pub const CPX_PARAM_ZEROHALFCUTS: c_int = 2111;
pub const CPX_PARAM_POLISHAFTEREPAGAP: c_int = 2126;
pub const CPX_PARAM_POLISHAFTEREPGAP: c_int = 2127;
pub const CPX_PARAM_POLISHAFTERNODE: c_int = 2128;
pub const CPX_PARAM_POLISHAFTERINTSOL: c_int = 2129;
pub const CPX_PARAM_POLISHAFTERTIME: c_int = 2130;
pub const CPX_PARAM_MCFCUTS: c_int = 2134;
pub const CPX_PARAM_MIPKAPPASTATS: c_int = 2137;
pub const CPX_PARAM_AUXROOTTHREADS: c_int = 2139;
pub const CPX_PARAM_INTSOLFILEPREFIX: c_int = 2143;
pub const CPX_PARAM_PROBEDETTIME: c_int = 2150;
pub const CPX_PARAM_POLISHAFTERDETTIME: c_int = 2151;
pub const CPX_PARAM_LANDPCUTS: c_int = 2152;
pub const CPX_PARAM_RAMPUPDURATION: c_int = 2163;
pub const CPX_PARAM_RAMPUPDETTILIM: c_int = 2164;
pub const CPX_PARAM_RAMPUPTILIM: c_int = 2165;

pub const CPX_PARAM_CALCQCPDUALS: c_int = 4003;
pub const CPX_PARAM_QPMAKEPSDIND: c_int = 4010;

pub const CPXPROB_LP: c_int = 0;
pub const CPXPROB_MILP: c_int = 1;
pub const CPXPROB_FIXEDMILP: c_int = 3;
pub const CPXPROB_QP: c_int = 5;
pub const CPXPROB_MIQP: c_int = 7;
pub const CPXPROB_FIXEDMIQP: c_int = 8;
pub const CPXPROB_QCP: c_int = 10;
pub const CPXPROB_MIQCP: c_int = 11;

/// CPLEX header version inficates the version of the header definitions
pub const CPX_VERSION: c_int = 12060200;
pub const CPX_VERSION_VERSION: c_int = 12;
pub const CPX_VERSION_RELEASE: c_int = 6;
pub const CPX_VERSION_MODIFICATION: c_int = 2;
pub const CPX_VERSION_FIX: c_int = 0;

/// CPLEX infinity bound numbers bigger than this are treated as infinity
pub const CPX_INFBOUND: c_double = 1.0E+20;

#[cfg(target_pointer_width = "64")]
pub type CPXNNZ = int64_t;

#[cfg(target_pointer_width = "64")]
pub type CPXCNT = int64_t;

#[cfg(target_pointer_width = "64")]
pub type CPXDIM = c_int;

// #[link(name = "cplex1260")]
extern "C" {
    pub fn CPXLopenCPLEX(status: *mut c_int) -> *mut CPXenv;

    pub fn CPXLcloseCPLEX(env: &*mut CPXenv) -> c_int;

    pub fn CPXLgeterrorstring(env: *const CPXenv,
                              status: c_int,
                              message: *mut c_char)
                              -> *const c_char;

    pub fn CPXLversion(env: *const CPXenv) -> *const c_char;

    pub fn CPXLversionnumber(env: *const CPXenv) -> *const c_char;

    pub fn CPXLsetdefaults(env: *mut CPXenv) -> c_int;

    pub fn CPXLsetintparam(env: *mut CPXenv, whichparam: c_int, newvalue: c_int) -> c_int;

    pub fn CPXLsetlongparam(env: *mut CPXenv, whichparam: c_int, newvalue: int64_t) -> c_int;

    pub fn CPXLsetdblparam(env: *mut CPXenv, whichparam: c_int, newvalue: c_double) -> c_int;

    pub fn CPXLsetstrparam(env: *mut CPXenv, whichparam: c_int, newvalue: *const c_char) -> c_int;

    pub fn CPXLgetintparam(env: *const CPXenv, whichparam: c_int, value: *mut c_int) -> c_int;

    pub fn CPXLgetlongparam(env: *const CPXenv, whichparam: c_int, value: *mut int64_t) -> c_int;

    pub fn CPXLgetdblparam(env: *const CPXenv, whichparam: c_int, value: *mut c_double) -> c_int;

    pub fn CPXLgetstrparam(env: *const CPXenv, whichparam: c_int, value: *mut c_char) -> c_int;

    pub fn CPXLcreateprob(env: *const CPXenv,
                          status: *mut c_int,
                          probname: *const c_char)
                          -> *mut CPXlp;

    pub fn CPXLfreeprob(env: *const CPXenv, lp: &*mut CPXlp) -> c_int;

    pub fn CPXLgetnumrows(env: *const CPXenv, lp: *const CPXlp) -> CPXDIM;

    pub fn CPXLgetnumcols(env: *const CPXenv, lp: *const CPXlp) -> CPXDIM;

    pub fn CPXLnewcols(env: *const CPXenv,
                       lp: *mut CPXlp,
                       ccnt: CPXCNT,
                       obj: *const c_double,
                       lb: *const c_double,
                       ub: *const c_double,
                       ctype: *const c_char,
                       colname: *const *const c_char)
                       -> c_int;

    pub fn CPXLnewrows(env: *const CPXenv,
                       lp: *mut CPXlp,
                       rcnt: CPXDIM,
                       rhs: *const c_double,
                       sense: *const c_char,
                       rngval: *const c_double,
                       rowname: *const *const c_char)
                       -> c_int;

    pub fn CPXLchgcoef(env: *const CPXenv,
                       lp: *mut CPXlp,
                       i: CPXDIM,
                       j: CPXDIM,
                       newvalue: c_double)
                       -> c_int;

    pub fn CPXLchgbds(env: *const CPXenv,
                      lp: *mut CPXlp,
                      cnt: CPXDIM,
                      indices: *const CPXDIM,
                      lu: *const c_char,
                      bd: *const c_double)
                      -> c_int;

    pub fn CPXLchgobjoffset(env: *const CPXenv, lp: *mut CPXlp, offset: f64) -> c_int;

    pub fn CPXLchgobjsen(env: *const CPXenv, lp: *mut CPXlp, max_or_min: c_int) -> c_int;

    pub fn CPXLcopylpwnames(env: *const CPXenv,
                            lp: *mut CPXlp,
                            numcols: CPXDIM,
                            numrows: CPXDIM,
                            objsense: c_int,
                            objective: *const c_double,
                            rhs: *const c_double,
                            sense: *const c_char,
                            matbeg: *const CPXNNZ,
                            matcnt: *const CPXDIM,
                            matind: *const CPXDIM,
                            matval: *const c_double,
                            lb: *const c_double,
                            ub: *const c_double,
                            rngval: *const c_double,
                            colname: *const *const c_char,
                            rowname: *const *const c_char)
                            -> c_int;

    pub fn CPXLcopyctype(env: *const CPXenv, lp: *mut CPXlp, ctype: *const c_char) -> c_int;

    pub fn CPXLpresolve(env: *const CPXenv, lp: *mut CPXlp, method: c_int) -> c_int;

    pub fn CPXLgetprobtype(env: *const CPXenv, lp: *mut CPXlp) -> c_int;

    pub fn CPXLmipopt(env: *const CPXenv, lp: *mut CPXlp) -> c_int;

    pub fn CPXLlpopt(env: *const CPXenv, lp: *mut CPXlp) -> c_int;

    pub fn CPXLprimopt(env: *const CPXenv, lp: *mut CPXlp) -> c_int;

    pub fn CPXLgetstat(env: *const CPXenv, lp: *const CPXlp) -> c_int;

    pub fn CPXLgetstatstring(env: *const CPXenv,
                             status: c_int,
                             message: *mut c_char)
                             -> *const c_char;

    pub fn CPXLgetobjval(env: *const CPXenv, lp: *mut CPXlp, objval: *mut c_double) -> c_int;

    pub fn CPXLgetx(env: *const CPXenv,
                    lp: *const CPXlp,
                    x: *mut c_double,
                    begin: CPXDIM,
                    end: CPXDIM)
                    -> c_int;

    pub fn CPXLsolution(env: *const CPXenv,
                        lp: *const CPXlp,
                        lpstat: *mut c_int,
                        objval: *mut c_double,
                        x: *mut c_double,
                        pi: *mut c_double,
                        slack: *mut c_double,
                        dj: *mut c_double)
                        -> c_int;

    pub fn CPXLsolninfo(env: *const CPXenv,
                        lp: *mut CPXlp,
                        solnmethod: *mut c_int,
                        solntype: *mut c_int,
                        pfeasind: *mut c_int,
                        dfeasind: *mut c_int)
                        -> c_int;

    pub fn CPXLgettime(env: *const CPXenv, timestamp: *mut c_double) -> c_int;

    pub fn CPXLwriteparam(env: *const CPXenv, filename: *const c_char) -> c_int;

    pub fn CPXLwriteprob(env: *const CPXenv,
                         lp: *const CPXlp,
                         filename: *const c_char,
                         filetype: *const c_char)
                         -> c_int;
}
