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

// #[link(name = "cplex1260")]
extern "C" {
    pub fn CPXopenCPLEX(status: *mut c_int) -> *mut CPXenv;

    pub fn CPXcloseCPLEX(env: &*mut CPXenv) -> c_int;

    pub fn CPXgeterrorstring(env: *const CPXenv,
                             status: c_int,
                             message: *mut c_char)
                             -> *const c_char;

    pub fn CPXversion(env: *const CPXenv) -> *const c_char;

    pub fn CPXversionnumber(env: *const CPXenv) -> *const c_char;

    pub fn CPXsetdefaults(env: *mut CPXenv) -> c_int;

    pub fn CPXsetintparam(env: *mut CPXenv, whichparam: c_int, newvalue: c_int) -> c_int;

    pub fn CPXsetlongparam(env: *mut CPXenv, whichparam: c_int, newvalue: int64_t) -> c_int;

    pub fn CPXsetdblparam(env: *mut CPXenv, whichparam: c_int, newvalue: c_double) -> c_int;

    pub fn CPXsetstrparam(env: *mut CPXenv, whichparam: c_int, newvalue: *const c_char) -> c_int;

    pub fn CPXgetintparam(env: *const CPXenv, whichparam: c_int, value: *mut c_int) -> c_int;

    pub fn CPXgetlongparam(env: *const CPXenv, whichparam: c_int, value: *mut int64_t) -> c_int;

    pub fn CPXgetdblparam(env: *const CPXenv, whichparam: c_int, value: *mut c_double) -> c_int;

    pub fn CPXgetstrparam(env: *const CPXenv, whichparam: c_int, value: *mut c_char) -> c_int;

    pub fn CPXcreateprob(env: *const CPXenv,
                         status: *mut c_int,
                         probname: *const c_char)
                         -> *mut CPXlp;

    pub fn CPXfreeprob(env: *const CPXenv, lp: &*mut CPXlp) -> c_int;

    pub fn CPXgetnumrows(env: *const CPXenv, lp: *const CPXlp) -> c_int;

    pub fn CPXgetnumcols(env: *const CPXenv, lp: *const CPXlp) -> c_int;

    pub fn CPXnewcols(env: *const CPXenv,
                      lp: *mut CPXlp,
                      ccnt: c_int,
                      obj: *const c_double,
                      lb: *const c_double,
                      ub: *const c_double,
                      ctype: *const c_char,
                      colname: *const *const c_char)
                      -> c_int;

    pub fn CPXnewrows(env: *const CPXenv,
                      lp: *mut CPXlp,
                      rcnt: c_int,
                      rhs: *const c_double,
                      sense: *const c_char,
                      rngval: *const c_double,
                      rowname: *const *const c_char)
                      -> c_int;

    pub fn CPXchgcoef(env: *const CPXenv,
                      lp: *mut CPXlp,
                      i: c_int,
                      j: c_int,
                      newvalue: c_double)
                      -> c_int;

    pub fn CPXchgbds(env: *const CPXenv,
                     lp: *mut CPXlp,
                     cnt: c_int,
                     indices: *const c_int,
                     lu: *const c_char,
                     bd: *const c_double)
                     -> c_int;

    pub fn CPXcopylp(env: *const CPXenv,
                     lp: *mut CPXlp,
                     numcols: c_int,
                     numrows: c_int,
                     objsense: c_int,
                     objective: *const c_double,
                     rhs: *const c_double,
                     sense: *const c_char,
                     matbeg: *const int64_t,
                     matcnt: *const c_int,
                     matind: *const c_int,
                     matval: *const c_double,
                     lb: *const c_double,
                     ub: *const c_double,
                     rngval: *const c_double)
                     -> c_int;

    pub fn CPXcopyctype(env: *const CPXenv, lp: *mut CPXlp, ctype: *const c_char) -> c_int;

    pub fn CPXpresolve(env: *const CPXenv, lp: *mut CPXlp, method: c_int) -> c_int;

    pub fn CPXmipopt(env: *const CPXenv, lp: *mut CPXlp) -> c_int;

    pub fn CPXlpopt(env: *const CPXenv, lp: *mut CPXlp) -> c_int;

    pub fn CPXprimopt(env: *const CPXenv, lp: *mut CPXlp) -> c_int;

    pub fn CPXgetstat(env: *const CPXenv, lp: *const CPXlp) -> c_int;

    pub fn CPXgetstatstring(env: *const CPXenv,
                            status: c_int,
                            message: *mut c_char)
                            -> *const c_char;

    pub fn CPXgetobjval(env: *const CPXenv, lp: *mut CPXlp, objval: *mut c_double) -> c_int;

    pub fn CPXgetx(env: *const CPXenv,
                   lp: *const CPXlp,
                   x: *mut c_double,
                   begin: c_int,
                   end: c_int)
                   -> c_int;

    pub fn CPXsolution(env: *const CPXenv,
                       lp: *const CPXlp,
                       lpstat: *mut c_int,
                       objval: *mut c_int,
                       x: *mut c_double,
                       pi: *mut c_double,
                       slack: *mut c_double,
                       dj: *mut c_double)
                       -> c_int;

    pub fn CPXsolninfo(env: *const CPXenv,
                       lp: *mut CPXlp,
                       solnmethod: *mut c_int,
                       solntype: *mut c_int,
                       pfeasind: *mut c_int,
                       dfeasind: *mut c_int)
                       -> c_int;

    pub fn CPXgettime(env: *const CPXenv, timestamp: *mut c_double) -> c_int;

    pub fn CPXwriteparam(env: *const CPXenv, filename: *const c_char) -> c_int;

    pub fn CPXwriteprob(env: *const CPXenv,
                        lp: *const CPXlp,
                        filename: *const c_char,
                        filetype: *const c_char)
                        -> c_int;
}
