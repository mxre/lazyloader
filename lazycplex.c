#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// for windows create static symbols
#define BUILD_CPXSTATIC 1

#include "cplexl.h"
#include <lazyloader.h>

// remove for windows some of the macros
#undef CPXPUBLIC
#define CPXPUBLIC
#undef CPXPUBVARARGS
#define CPXPUBVARARGS

/* Debugging macros */
#ifdef _MSC_VER
#define PRINT_DEBUG(fmt, ...) if (debug_enabled) { \
    fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, __VA_ARGS__ ); }
#define PRINT_ERR(fmt, ...) \
    fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, __VA_ARGS__ );
#else
#define PRINT_DEBUG(fmt, args ...) if (debug_enabled) { \
    fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, ## args ); }
#define PRINT_ERR(fmt, args ...) \
    fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, ## args );
#endif

static bool is_debug_enabled();
static void default_failure_callback(const char* symbol, void* cb_data);
static void* load_symbol(const char* name);

/* Handle to the library */
static void* handle = NULL;

/* Called whenever the library is about to crash */
static void (*failure_callback)(const char* symbol, void* cb_data) = default_failure_callback;

static void* callback_data = NULL;

static bool debug_enabled = false;

/* Prints the failing symbol and aborts */
void default_failure_callback(const char* symbol, void* cb_data){
    PRINT_ERR ("the symbol %s could not be found!\n", symbol);
    abort();
}

static void exit_lazy_loader() {
    if (handle != NULL)
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
}

int try_lazy_load(const char* library) {
    const char *dbg = getenv("LAZYCPLEX_DEBUG");
    debug_enabled = (dbg != NULL && strlen(dbg) > 0);

    if (handle != NULL) {
        PRINT_DEBUG("The library is already initialized.\n");
        return 0;
    }
    
    PRINT_DEBUG("Trying to load %s...\n", library);
#ifdef _WIN32
    handle = LoadLibrary(library);
#else
    handle = dlopen(library, RTLD_LAZY);
#endif
    if (handle == NULL)
        return 1;
    else {
        PRINT_DEBUG("Success!\n");
        return 0;
    }
}

/* Searches and loads the actual library */
int initialize_lazy_loader() {
    const char *dbg = getenv("LAZYCPLEX_DEBUG");
    debug_enabled = (dbg != NULL && strlen(dbg) > 0);

    if (handle != NULL) {
        PRINT_DEBUG("The library is already initialized.\n");
        return 0;
    }

    int i;
    char* s = getenv("LAZYLOAD_CPLEX_DLL");
    const char* libnames[] = {
        ""
        "",
#ifdef _WIN32
        "cplex1263.dll",
        "cplex1262.dll",
        "cplex1261.dll",
        "cplex1260.dll",
        "cplex125.dll",
        "cplex124.dll",
        "cplex123.dll",
#else
        "libcplex1263.so",
        "libcplex1262.so",
        "libcplex1261.so",
        "libcplex1260.so",
        "libcplex125.so",
        "libcplex124.so",
        "libcplex123.so",
#endif
        NULL };
    if (s != NULL)
        libnames[0] = s;

    PRINT_DEBUG("Looking for a suitable library.\n");
    for (i = 0; libnames[i] != NULL; i++){
        if (strlen(libnames[i]) == 0) {
            continue;
        }
        if (try_lazy_load(libnames[i]) == 0) {
            break;
        }
    }

    atexit(exit_lazy_loader);

    if (handle == NULL) {
        PRINT_DEBUG("Library lookup failed! Suggest setting a library path manually.\n")
        return 1;
    } else {
        return 0;
    }
}

static __inline void* load_symbol(const char *name){
    void* symbol = NULL;
#ifdef _WIN32
    if (!handle || !(symbol = GetProcAddress(handle, name))) {
#else
    if (!handle || !(symbol = dlsym(handle, name))) {
#endif
        failure_callback(name, callback_data);
    } else {
        PRINT_DEBUG("successfully imported the symbol %s at %p.\n", name, symbol);
    }
    return symbol;
}

void set_lazyloader_error_callback(void (*f)(const char *err, void* cb_data), void* cb_data) {
    failure_callback = f;
    callback_data = cb_data;
}

/* imported functions */

CPXLIBAPI CPXCHANNELptr CPXPUBLIC (*native_CPXLaddchannel) (CPXENVptr env) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddcols) (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, CPXLONG nzcnt, double const *obj, CPXLONG const *cmatbeg, CPXINT const *cmatind, double const *cmatval, double const *lb, double const *ub, char const *const *colname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddfpdest) (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddfuncdest) (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, char const *)) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddrows) (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *colname, char const *const *rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbasicpresolve) (CPXCENVptr env, CPXLPptr lp, double *redlb, double *redub, int *rstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbinvacol) (CPXCENVptr env, CPXCLPptr lp, CPXINT j, double *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbinvarow) (CPXCENVptr env, CPXCLPptr lp, CPXINT i, double *z) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbinvcol) (CPXCENVptr env, CPXCLPptr lp, CPXINT j, double *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbinvrow) (CPXCENVptr env, CPXCLPptr lp, CPXINT i, double *y) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLboundsa) (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end, double *lblower, double *lbupper, double *ublower, double *ubupper) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbtran) (CPXCENVptr env, CPXCLPptr lp, double *y) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcheckdfeas) (CPXCENVptr env, CPXLPptr lp, CPXINT * infeas_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcheckpfeas) (CPXCENVptr env, CPXLPptr lp, CPXINT * infeas_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchecksoln) (CPXCENVptr env, CPXLPptr lp, int *lpstatus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgbds) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgcoef) (CPXCENVptr env, CPXLPptr lp, CPXINT i, CPXINT j, double newvalue) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgcoeflist) (CPXCENVptr env, CPXLPptr lp, CPXLONG numcoefs, CPXINT const *rowlist, CPXINT const *collist, double const *vallist) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgcolname) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *const *newname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgname) (CPXCENVptr env, CPXLPptr lp, int key, CPXINT ij, char const *newname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgobj) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgobjsen) (CPXCENVptr env, CPXLPptr lp, int maxormin) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgprobname) (CPXCENVptr env, CPXLPptr lp, char const *probname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgprobtype) (CPXCENVptr env, CPXLPptr lp, int type) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgprobtypesolnpool) (CPXCENVptr env, CPXLPptr lp, int type, int soln) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgrhs) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgrngval) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgrowname) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *const *newname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgsense) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *sense) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcleanup) (CPXCENVptr env, CPXLPptr lp, double eps) = NULL;
CPXLIBAPI CPXLPptr CPXPUBLIC (*native_CPXLcloneprob) (CPXCENVptr env, CPXCLPptr lp, int *status_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcloseCPLEX) (CPXENVptr * env_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLclpwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcompletelp) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopybase) (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopybasednorms) (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *dnorm) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopydnorms) (CPXCENVptr env, CPXLPptr lp, double const *norm, CPXINT const *head, CPXINT len) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopylp) (CPXCENVptr env, CPXLPptr lp, CPXINT numcols, CPXINT numrows, int objsense, double const *objective, double const *rhs, char const *sense, CPXLONG const *matbeg, CPXINT const *matcnt, CPXINT const *matind, double const *matval, double const *lb, double const *ub, double const *rngval) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopylpwnames) (CPXCENVptr env, CPXLPptr lp, CPXINT numcols, CPXINT numrows, int objsense, double const *objective, double const *rhs, char const *sense, CPXLONG const *matbeg, CPXINT const *matcnt, CPXINT const *matind, double const *matval, double const *lb, double const *ub, double const *rngval, char const *const *colname, char const *const *rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopynettolp) (CPXCENVptr env, CPXLPptr lp, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopyobjname) (CPXCENVptr env, CPXLPptr lp, char const *objname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopypartialbase) (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, CPXINT const *cindices, int const *cstat, CPXINT rcnt, CPXINT const *rindices, int const *rstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopypnorms) (CPXCENVptr env, CPXLPptr lp, double const *cnorm, double const *rnorm, CPXINT len) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopyprotected) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopystart) (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *cprim, double const *rprim, double const *cdual, double const *rdual) = NULL;
CPXLIBAPI CPXLPptr CPXPUBLIC (*native_CPXLcreateprob) (CPXCENVptr env, int *status_p, char const *probname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcrushform) (CPXCENVptr env, CPXCLPptr lp, CPXINT len, CPXINT const *ind, double const *val, CPXINT * plen_p, double *poffset_p, CPXINT * pind, double *pval) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcrushpi) (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *prepi) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcrushx) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *prex) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelchannel) (CPXENVptr env, CPXCHANNELptr * channel_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelcols) (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelfpdest) (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelfuncdest) (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, char const *)) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelnames) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelrows) (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelsetcols) (CPXCENVptr env, CPXLPptr lp, CPXINT * delstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelsetrows) (CPXCENVptr env, CPXLPptr lp, CPXINT * delstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdeserializercreate) (CPXDESERIALIZERptr * deser_p, CPXLONG size, void const *buffer) = NULL;
CPXLIBAPI void CPXPUBLIC (*native_CPXLdeserializerdestroy) (CPXDESERIALIZERptr deser) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLdeserializerleft) (CPXCDESERIALIZERptr deser) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdisconnectchannel) (CPXCENVptr env, CPXCHANNELptr channel) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdjfrompi) (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *dj) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdperwrite) (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdratio) (CPXCENVptr env, CPXLPptr lp, CPXINT * indices, CPXINT cnt, double *downratio, double *upratio, CPXINT * downenter, CPXINT * upenter, int *downstatus, int *upstatus) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdualfarkas) (CPXCENVptr env, CPXCLPptr lp, double *y, double *proof_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdualopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdualwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, double *objshift_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLembwrite) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLfclose) (CPXFILEptr stream) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLfeasopt) (CPXCENVptr env, CPXLPptr lp, double const *rhs, double const *rng, double const *lb, double const *ub) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLfeasoptext) (CPXCENVptr env, CPXLPptr lp, CPXINT grpcnt, CPXLONG concnt, double const *grppref, CPXLONG const *grpbeg, CPXINT const *grpind, char const *grptype) = NULL;
CPXLIBAPI void CPXPUBLIC (*native_CPXLfinalize) (void) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLflushchannel) (CPXCENVptr env, CPXCHANNELptr channel) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLflushstdchannels) (CPXCENVptr env) = NULL;
CPXLIBAPI CPXFILEptr CPXPUBLIC (*native_CPXLfopen) (char const *filename_str, char const *type_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLfputs) (char const *s_str, CPXFILEptr stream) = NULL;
CPXLIBAPI void CPXPUBLIC (*native_CPXLfree) (void *ptr) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLfreeparenv) (CPXENVptr env, CPXENVptr * child_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLfreepresolve) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLfreeprob) (CPXCENVptr env, CPXLPptr * lp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLftran) (CPXCENVptr env, CPXCLPptr lp, double *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetax) (CPXCENVptr env, CPXCLPptr lp, double *x, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetbaritcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetbase) (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetbasednorms) (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat, double *dnorm) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetbhead) (CPXCENVptr env, CPXCLPptr lp, CPXINT * head, double *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbackinfo) (CPXCENVptr env, void *cbdata, int wherefrom, int whichinfo, void *result_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetchannels) (CPXCENVptr env, CPXCHANNELptr * cpxresults_p, CPXCHANNELptr * cpxwarning_p, CPXCHANNELptr * cpxerror_p, CPXCHANNELptr * cpxlog_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetchgparam) (CPXCENVptr env, int *cnt_p, int *paramnum, int pspace, int *surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcoef) (CPXCENVptr env, CPXCLPptr lp, CPXINT i, CPXINT j, double *coef_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcolindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcolinfeas) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcolname) (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, CPXSIZE storespace, CPXSIZE * surplus_p, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcols) (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * cmatbeg, CPXINT * cmatind, double *cmatval, CPXLONG cmatspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetconflict) (CPXCENVptr env, CPXCLPptr lp, int *confstat_p, CPXINT * rowind, int *rowbdstat, CPXINT * confnumrows_p, CPXINT * colind, int *colbdstat, CPXINT * confnumcols_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetconflictext) (CPXCENVptr env, CPXCLPptr lp, int *grpstat, CPXLONG beg, CPXLONG end) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetcrossdexchcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetcrossdpushcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetcrosspexchcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetcrossppushcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetdblparam) (CPXCENVptr env, int whichparam, double *value_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetdblquality) (CPXCENVptr env, CPXCLPptr lp, double *quality_p, int what) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetdettime) (CPXCENVptr env, double *dettimestamp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetdj) (CPXCENVptr env, CPXCLPptr lp, double *dj, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetdnorms) (CPXCENVptr env, CPXCLPptr lp, double *norm, CPXINT * head, CPXINT * len_p) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetdsbcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXCCHARptr CPXPUBLIC (*native_CPXLgeterrorstring) (CPXCENVptr env, int errcode, char *buffer_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetgrad) (CPXCENVptr env, CPXCLPptr lp, CPXINT j, CPXINT * head, double *y) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetijdiv) (CPXCENVptr env, CPXCLPptr lp, CPXINT * idiv_p, CPXINT * jdiv_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetijrow) (CPXCENVptr env, CPXCLPptr lp, CPXINT i, CPXINT j, CPXINT * row_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetintparam) (CPXCENVptr env, int whichparam, CPXINT * value_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetintquality) (CPXCENVptr env, CPXCLPptr lp, CPXINT * quality_p, int what) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetitcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetlb) (CPXCENVptr env, CPXCLPptr lp, double *lb, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetlogfile) (CPXCENVptr env, CPXFILEptr * logfile_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetlongparam) (CPXCENVptr env, int whichparam, CPXLONG * value_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetlpcallbackfunc) (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetmethod) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetnetcallbackfunc) (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumcols) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetnumcores) (CPXCENVptr env, int *numcores_p) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetnumnz) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumrows) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetobj) (CPXCENVptr env, CPXCLPptr lp, double *obj, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetobjname) (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetobjoffset) (CPXCENVptr env, CPXCLPptr lp, double *objoffset_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetobjsen) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetobjval) (CPXCENVptr env, CPXCLPptr lp, double *objval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetparamname) (CPXCENVptr env, int whichparam, char *name_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetparamnum) (CPXCENVptr env, char const *name_str, int *whichparam_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetparamtype) (CPXCENVptr env, int whichparam, int *paramtype) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetphase1cnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetpi) (CPXCENVptr env, CPXCLPptr lp, double *pi, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetpnorms) (CPXCENVptr env, CPXCLPptr lp, double *cnorm, double *rnorm, CPXINT * len_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetprestat) (CPXCENVptr env, CPXCLPptr lp, int *prestat_p, CPXINT * pcstat, CPXINT * prstat, CPXINT * ocstat, CPXINT * orstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetprobname) (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetprobtype) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetprotected) (CPXCENVptr env, CPXCLPptr lp, CPXINT * cnt_p, CPXINT * indices, CPXINT pspace, CPXINT * surplus_p) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetpsbcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetray) (CPXCENVptr env, CPXCLPptr lp, double *z) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetredlp) (CPXCENVptr env, CPXCLPptr lp, CPXCLPptr * redlp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetrhs) (CPXCENVptr env, CPXCLPptr lp, double *rhs, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetrngval) (CPXCENVptr env, CPXCLPptr lp, double *rngval, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetrowindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetrowinfeas) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetrowname) (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, CPXSIZE storespace, CPXSIZE * surplus_p, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetrows) (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * rmatbeg, CPXINT * rmatind, double *rmatval, CPXLONG rmatspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsense) (CPXCENVptr env, CPXCLPptr lp, char *sense, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetsiftitcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetsiftphase1cnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetslack) (CPXCENVptr env, CPXCLPptr lp, double *slack, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpooldblquality) (CPXCENVptr env, CPXCLPptr lp, int soln, double *quality_p, int what) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolintquality) (CPXCENVptr env, CPXCLPptr lp, int soln, CPXINT * quality_p, int what) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetstat) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXCHARptr CPXPUBLIC (*native_CPXLgetstatstring) (CPXCENVptr env, int statind, char *buffer_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetstrparam) (CPXCENVptr env, int whichparam, char *value_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgettime) (CPXCENVptr env, double *timestamp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgettuningcallbackfunc) (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetub) (CPXCENVptr env, CPXCLPptr lp, double *ub, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetweight) (CPXCENVptr env, CPXCLPptr lp, CPXINT rcnt, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, double *weight, int dpriind) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetx) (CPXCENVptr env, CPXCLPptr lp, double *x, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLhybnetopt) (CPXCENVptr env, CPXLPptr lp, int method) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLinfodblparam) (CPXCENVptr env, int whichparam, double *defvalue_p, double *minvalue_p, double *maxvalue_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLinfointparam) (CPXCENVptr env, int whichparam, CPXINT * defvalue_p, CPXINT * minvalue_p, CPXINT * maxvalue_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLinfolongparam) (CPXCENVptr env, int whichparam, CPXLONG * defvalue_p, CPXLONG * minvalue_p, CPXLONG * maxvalue_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLinfostrparam) (CPXCENVptr env, int whichparam, char *defvalue_str) = NULL;
CPXLIBAPI void CPXPUBLIC (*native_CPXLinitialize) (void) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLkilldnorms) (CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLkillpnorms) (CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLlpopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLlprewrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLlpwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI CPXVOIDptr CPXPUBLIC (*native_CPXLmalloc) (size_t size) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLmbasewrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLmdleave) (CPXCENVptr env, CPXLPptr lp, CPXINT const *indices, CPXINT cnt, double *downratio, double *upratio) = NULL;
CPXLIBAPI CPXVOIDptr CPXPUBLIC (*native_CPXLmemcpy) (void *s1, void *s2, size_t n) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLmpsrewrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLmpswrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBVARARGS (*native_CPXLmsg) (CPXCHANNELptr channel, char const *format, ...) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLmsgstr) (CPXCHANNELptr channel, char const *msg_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETextract) (CPXCENVptr env, CPXNETptr net, CPXCLPptr lp, CPXINT * colmap, CPXINT * rowmap) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLnewcols) (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, double const *obj, double const *lb, double const *ub, char const *xctype, char const *const *colname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLnewrows) (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, double const *rhs, char const *sense, double const *rngval, char const *const *rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLobjsa) (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end, double *lower, double *upper) = NULL;
CPXLIBAPI CPXENVptr CPXPUBLIC (*native_CPXLopenCPLEX) (int *status_p) = NULL;
CPXLIBAPI CPXENVptr CPXPUBLIC (*native_CPXLopenCPLEXruntime) (int *status_p, int serialnum, char const *licenvstring_str) = NULL;
CPXLIBAPI CPXENVptr CPXPUBLIC (*native_CPXLparenv) (CPXENVptr env, int *status_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLpivot) (CPXCENVptr env, CPXLPptr lp, CPXINT jenter, CPXINT jleave, int leavestat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLpivotin) (CPXCENVptr env, CPXLPptr lp, CPXINT const *rlist, CPXINT rlen) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLpivotout) (CPXCENVptr env, CPXLPptr lp, CPXINT const *clist, CPXINT clen) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLpperwrite) (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLpratio) (CPXCENVptr env, CPXLPptr lp, CPXINT * indices, CPXINT cnt, double *downratio, double *upratio, CPXINT * downleave, CPXINT * upleave, int *downleavestatus, int *upleavestatus, int *downstatus, int *upstatus) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLpreaddrows) (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLprechgobj) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLpreslvwrite) (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double *objoff_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLpresolve) (CPXCENVptr env, CPXLPptr lp, int method) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLprimopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLputenv) (char *envsetting_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLqpdjfrompi) (CPXCENVptr env, CPXCLPptr lp, double const *pi, double const *x, double *dj) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLqpuncrushpi) (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi, double const *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLreadcopybase) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLreadcopyparam) (CPXENVptr env, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLreadcopyprob) (CPXCENVptr env, CPXLPptr lp, char const *filename_str, char const *filetype_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLreadcopysol) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI CPXVOIDptr CPXPUBLIC (*native_CPXLrealloc) (void *ptr, size_t size) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLrefineconflict) (CPXCENVptr env, CPXLPptr lp, CPXINT * confnumrows_p, CPXINT * confnumcols_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLrefineconflictext) (CPXCENVptr env, CPXLPptr lp, CPXLONG grpcnt, CPXLONG concnt, double const *grppref, CPXLONG const *grpbeg, CPXINT const *grpind, char const *grptype) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLrhssa) (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end, double *lower, double *upper) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLrobustopt) (CPXCENVptr env, CPXLPptr lp, CPXLPptr lblp, CPXLPptr ublp, double objchg, double const *maxchg) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsavwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLserializercreate) (CPXSERIALIZERptr * ser_p) = NULL;
CPXLIBAPI void CPXPUBLIC (*native_CPXLserializerdestroy) (CPXSERIALIZERptr ser) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLserializerlength) (CPXCSERIALIZERptr ser) = NULL;
CPXLIBAPI void const *CPXPUBLIC (*native_CPXLserializerpayload) (CPXCSERIALIZERptr ser) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetdblparam) (CPXENVptr env, int whichparam, double newvalue) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetdefaults) (CPXENVptr env) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetintparam) (CPXENVptr env, int whichparam, CPXINT newvalue) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetlogfile) (CPXENVptr env, CPXFILEptr lfile) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetlongparam) (CPXENVptr env, int whichparam, CPXLONG newvalue) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetlpcallbackfunc) (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetnetcallbackfunc) (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetphase2) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetprofcallbackfunc) (CPXENVptr env, CPXL_CALLBACK_PROF * callback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetstrparam) (CPXENVptr env, int whichparam, char const *newvalue_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetterminate) (CPXENVptr env, volatile int *terminate_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsettuningcallbackfunc) (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsiftopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLslackfromx) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *slack) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsolninfo) (CPXCENVptr env, CPXCLPptr lp, int *solnmethod_p, int *solntype_p, int *pfeasind_p, int *dfeasind_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsolution) (CPXCENVptr env, CPXCLPptr lp, int *lpstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsolwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsolwritesolnpool) (CPXCENVptr env, CPXCLPptr lp, int soln, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsolwritesolnpoolall) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI CPXCHARptr CPXPUBLIC (*native_CPXLstrcpy) (char *dest_str, char const *src_str) = NULL;
CPXLIBAPI size_t CPXPUBLIC (*native_CPXLstrlen) (char const *s_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLstrongbranch) (CPXCENVptr env, CPXLPptr lp, CPXINT const *indices, CPXINT cnt, double *downobj, double *upobj, CPXLONG itlim) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLtightenbds) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLtuneparam) (CPXENVptr env, CPXLPptr lp, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char const *const *strval, int *tunestat_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLtuneparamprobset) (CPXENVptr env, int filecnt, char const *const *filename, char const *const *filetype, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char const *const *strval, int *tunestat_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLuncrushform) (CPXCENVptr env, CPXCLPptr lp, CPXINT plen, CPXINT const *pind, double const *pval, CPXINT * len_p, double *offset_p, CPXINT * ind, double *val) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLuncrushpi) (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLuncrushx) (CPXCENVptr env, CPXCLPptr lp, double *x, double const *prex) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLunscaleprob) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI CPXCCHARptr CPXPUBLIC (*native_CPXLversion) (CPXCENVptr env) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLversionnumber) (CPXCENVptr env, int *version_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLwriteparam) (CPXCENVptr env, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLwriteprob) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, char const *filetype_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbaropt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLhybbaropt) (CPXCENVptr env, CPXLPptr lp, int method) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddindconstr) (CPXCENVptr env, CPXLPptr lp, CPXINT indvar, int complemented, CPXINT nzcnt, double rhs, int sense, CPXINT const *linind, double const *linval, char const *indname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgqpcoef) (CPXCENVptr env, CPXLPptr lp, CPXINT i, CPXINT j, double newvalue) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopyqpsep) (CPXCENVptr env, CPXLPptr lp, double const *qsepvec) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopyquad) (CPXCENVptr env, CPXLPptr lp, CPXLONG const *qmatbeg, CPXINT const *qmatcnt, CPXINT const *qmatind, double const *qmatval) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetnumqpnz) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumquad) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetqpcoef) (CPXCENVptr env, CPXCLPptr lp, CPXINT rownum, CPXINT colnum, double *coef_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetquad) (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * qmatbeg, CPXINT * qmatind, double *qmatval, CPXLONG qmatspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLqpindefcertificate) (CPXCENVptr env, CPXCLPptr lp, double *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLqpopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddqconstr) (CPXCENVptr env, CPXLPptr lp, CPXINT linnzcnt, CPXLONG quadnzcnt, double rhs, int sense, CPXINT const *linind, double const *linval, CPXINT const *quadrow, CPXINT const *quadcol, double const *quadval, char const *lname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelqconstrs) (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumqconstrs) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetqconstr) (CPXCENVptr env, CPXCLPptr lp, CPXINT * linnzcnt_p, CPXLONG * quadnzcnt_p, double *rhs_p, char *sense_p, CPXINT * linind, double *linval, CPXINT linspace, CPXINT * linsurplus_p, CPXINT * quadrow, CPXINT * quadcol, double *quadval, CPXLONG quadspace, CPXLONG * quadsurplus_p, CPXINT which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetqconstrdslack) (CPXCENVptr env, CPXCLPptr lp, CPXINT qind, CPXINT * nz_p, CPXINT * ind, double *val, CPXINT space, CPXINT * surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetqconstrindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetqconstrinfeas) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetqconstrname) (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p, CPXINT which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetqconstrslack) (CPXCENVptr env, CPXCLPptr lp, double *qcslack, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetxqxax) (CPXCENVptr env, CPXCLPptr lp, double *xqxax, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLqconstrslackfromx) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *qcslack) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETaddarcs) (CPXCENVptr env, CPXNETptr net, CPXINT narcs, CPXINT const *fromnode, CPXINT const *tonode, double const *low, double const *up, double const *obj, char const *const *anames) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETaddnodes) (CPXCENVptr env, CPXNETptr net, CPXINT nnodes, double const *supply, char const *const *name) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETbasewrite) (CPXCENVptr env, CPXCNETptr net, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETchgarcname) (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, char const *const *newname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETchgarcnodes) (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, CPXINT const *fromnode, CPXINT const *tonode) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETchgbds) (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETchgname) (CPXCENVptr env, CPXNETptr net, int key, CPXINT vindex, char const *name_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETchgnodename) (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, char const *const *newname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETchgobj) (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, double const *obj) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETchgobjsen) (CPXCENVptr env, CPXNETptr net, int maxormin) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETchgsupply) (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, double const *supply) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETcopybase) (CPXCENVptr env, CPXNETptr net, int const *astat, int const *nstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETcopynet) (CPXCENVptr env, CPXNETptr net, int objsen, CPXINT nnodes, double const *supply, char const *const *nnames, CPXINT narcs, CPXINT const *fromnode, CPXINT const *tonode, double const *low, double const *up, double const *obj, char const *const *anames) = NULL;
CPXLIBAPI CPXNETptr CPXPUBLIC (*native_CPXLNETcreateprob) (CPXENVptr env, int *status_p, char const *name_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETdelarcs) (CPXCENVptr env, CPXNETptr net, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETdelnodes) (CPXCENVptr env, CPXNETptr net, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETdelset) (CPXCENVptr env, CPXNETptr net, CPXINT * whichnodes, CPXINT * whicharcs) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETfreeprob) (CPXENVptr env, CPXNETptr * net_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetarcindex) (CPXCENVptr env, CPXCNETptr net, char const *lname_str, CPXINT * index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetarcname) (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, CPXSIZE namespc, CPXSIZE * surplus_p, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetarcnodes) (CPXCENVptr env, CPXCNETptr net, CPXINT * fromnode, CPXINT * tonode, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetbase) (CPXCENVptr env, CPXCNETptr net, int *astat, int *nstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetdj) (CPXCENVptr env, CPXCNETptr net, double *dj, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLNETgetitcnt) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetlb) (CPXCENVptr env, CPXCNETptr net, double *low, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetnodearcs) (CPXCENVptr env, CPXCNETptr net, CPXINT * arccnt_p, CPXINT * arcbeg, CPXINT * arc, CPXINT arcspace, CPXINT * surplus_p, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetnodeindex) (CPXCENVptr env, CPXCNETptr net, char const *lname_str, CPXINT * index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetnodename) (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, CPXSIZE namespc, CPXSIZE * surplus_p, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLNETgetnumarcs) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLNETgetnumnodes) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetobj) (CPXCENVptr env, CPXCNETptr net, double *obj, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetobjsen) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetobjval) (CPXCENVptr env, CPXCNETptr net, double *objval_p) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLNETgetphase1cnt) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetpi) (CPXCENVptr env, CPXCNETptr net, double *pi, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetprobname) (CPXCENVptr env, CPXCNETptr net, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetslack) (CPXCENVptr env, CPXCNETptr net, double *slack, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetstat) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetsupply) (CPXCENVptr env, CPXCNETptr net, double *supply, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetub) (CPXCENVptr env, CPXCNETptr net, double *up, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETgetx) (CPXCENVptr env, CPXCNETptr net, double *x, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETprimopt) (CPXCENVptr env, CPXNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETreadcopybase) (CPXCENVptr env, CPXNETptr net, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETreadcopyprob) (CPXCENVptr env, CPXNETptr net, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETsolninfo) (CPXCENVptr env, CPXCNETptr net, int *pfeasind_p, int *dfeasind_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETsolution) (CPXCENVptr env, CPXCNETptr net, int *netstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLNETwriteprob) (CPXCENVptr env, CPXCNETptr net, char const *filename_str, char const *format_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddlazyconstraints) (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddmipstarts) (CPXCENVptr env, CPXLPptr lp, int mcnt, CPXLONG nzcnt, CPXLONG const *beg, CPXINT const *varindices, double const *values, int const *effortlevel, char const *const *mipstartname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddsolnpooldivfilter) (CPXCENVptr env, CPXLPptr lp, double lower_bound, double upper_bound, CPXINT nzcnt, CPXINT const *ind, double const *weight, double const *refval, char const *lname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddsolnpoolrngfilter) (CPXCENVptr env, CPXLPptr lp, double lb, double ub, CPXINT nzcnt, CPXINT const *ind, double const *val, char const *lname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddsos) (CPXCENVptr env, CPXLPptr lp, CPXINT numsos, CPXLONG numsosnz, char const *sostype, CPXLONG const *sosbeg, CPXINT const *sosind, double const *soswt, char const *const *sosname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLaddusercuts) (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbranchcallbackbranchasCPLEX) (CPXCENVptr env, void *cbdata, int wherefrom, int num, void *userhandle, CPXLONG * seqnum_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbranchcallbackbranchbds) (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd, double nodeest, void *userhandle, CPXLONG * seqnum_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbranchcallbackbranchconstraints) (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, double nodeest, void *userhandle, CPXLONG * seqnum_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLbranchcallbackbranchgeneral) (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT varcnt, CPXINT const *varind, char const *varlu, double const *varbd, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, double nodeest, void *userhandle, CPXLONG * seqnum_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcallbacksetnodeuserhandle) (CPXCENVptr env, void *cbdata, int wherefrom, CPXLONG nodeindex, void *userhandle, void **olduserhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcallbacksetuserhandle) (CPXCENVptr env, void *cbdata, int wherefrom, void *userhandle, void **olduserhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgctype) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *xctype) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgmipstarts) (CPXCENVptr env, CPXLPptr lp, int mcnt, int const *mipstartindices, CPXLONG nzcnt, CPXLONG const *beg, CPXINT const *varindices, double const *values, int const *effortlevel) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopyctype) (CPXCENVptr env, CPXLPptr lp, char const *xctype) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopyorder) (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, CPXINT const *priority, int const *direction) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcopysos) (CPXCENVptr env, CPXLPptr lp, CPXINT numsos, CPXLONG numsosnz, char const *sostype, CPXLONG const *sosbeg, CPXINT const *sosind, double const *soswt, char const *const *sosname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcutcallbackadd) (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT nzcnt, double rhs, int sense, CPXINT const *cutind, double const *cutval, int purgeable) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLcutcallbackaddlocal) (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT nzcnt, double rhs, int sense, CPXINT const *cutind, double const *cutval) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelindconstrs) (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelmipstarts) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelsetmipstarts) (CPXCENVptr env, CPXLPptr lp, int *delstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelsetsolnpoolfilters) (CPXCENVptr env, CPXLPptr lp, int *delstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelsetsolnpoolsolns) (CPXCENVptr env, CPXLPptr lp, int *delstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelsetsos) (CPXCENVptr env, CPXLPptr lp, CPXINT * delset) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelsolnpoolfilters) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelsolnpoolsolns) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLdelsos) (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLfltwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLfreelazyconstraints) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLfreeusercuts) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetbestobjval) (CPXCENVptr env, CPXCLPptr lp, double *objval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetbranchcallbackfunc) (CPXCENVptr env, CPXL_CALLBACK_BRANCH ** branchcallback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetbranchnosolncallbackfunc) (CPXCENVptr env, CPXL_CALLBACK_BRANCH ** branchnosolncallback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbackbranchconstraints) (CPXCENVptr env, void *cbdata, int wherefrom, int which, CPXINT * cuts_p, CPXLONG * nzcnt_p, double *rhs, char *sense, CPXLONG * rmatbeg, CPXINT * rmatind, double *rmatval, CPXLONG rmatsz, CPXLONG * surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbackctype) (CPXCENVptr env, void *cbdata, int wherefrom, char *xctype, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbackgloballb) (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbackglobalub) (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbackincumbent) (CPXCENVptr env, void *cbdata, int wherefrom, double *x, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbackindicatorinfo) (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT iindex, int whichinfo, void *result_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbacklp) (CPXCENVptr env, void *cbdata, int wherefrom, CPXCLPptr * lp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbacknodeinfo) (CPXCENVptr env, void *cbdata, int wherefrom, CPXLONG nodeindex, int whichinfo, void *result_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbacknodeintfeas) (CPXCENVptr env, void *cbdata, int wherefrom, int *feas, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbacknodelb) (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbacknodelp) (CPXCENVptr env, void *cbdata, int wherefrom, CPXLPptr * nodelp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbacknodeobjval) (CPXCENVptr env, void *cbdata, int wherefrom, double *objval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbacknodestat) (CPXCENVptr env, void *cbdata, int wherefrom, int *nodestat_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbacknodeub) (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbacknodex) (CPXCENVptr env, void *cbdata, int wherefrom, double *x, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbackorder) (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT * priority, int *direction, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbackpseudocosts) (CPXCENVptr env, void *cbdata, int wherefrom, double *uppc, double *downpc, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbackseqinfo) (CPXCENVptr env, void *cbdata, int wherefrom, CPXLONG seqid, int whichinfo, void *result_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcallbacksosinfo) (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT sosindex, CPXINT member, int whichinfo, void *result_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetctype) (CPXCENVptr env, CPXCLPptr lp, char *xctype, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetcutoff) (CPXCENVptr env, CPXCLPptr lp, double *cutoff_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetdeletenodecallbackfunc) (CPXCENVptr env, CPXL_CALLBACK_DELETENODE ** deletecallback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetheuristiccallbackfunc) (CPXCENVptr env, CPXL_CALLBACK_HEURISTIC ** heuristiccallback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetincumbentcallbackfunc) (CPXCENVptr env, CPXL_CALLBACK_INCUMBENT ** incumbentcallback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetindconstr) (CPXCENVptr env, CPXCLPptr lp, CPXINT * indvar_p, int *complemented_p, CPXINT * nzcnt_p, double *rhs_p, char *sense_p, CPXINT * linind, double *linval, CPXINT space, CPXINT * surplus_p, CPXINT which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetindconstrindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetindconstrinfeas) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetindconstrname) (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p, CPXINT which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetindconstrslack) (CPXCENVptr env, CPXCLPptr lp, double *indslack, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetinfocallbackfunc) (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetlazyconstraintcallbackfunc) (CPXCENVptr env, CPXL_CALLBACK_CUT ** cutcallback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetmipcallbackfunc) (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetmipitcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetmiprelgap) (CPXCENVptr env, CPXCLPptr lp, double *gap_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetmipstartindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetmipstartname) (CPXCENVptr env, CPXCLPptr lp, char **name, char *store, CPXSIZE storesz, CPXSIZE * surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetmipstarts) (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * beg, CPXINT * varindices, double *values, int *effortlevel, CPXLONG startspace, CPXLONG * surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetnodecallbackfunc) (CPXCENVptr env, CPXL_CALLBACK_NODE ** nodecallback_p, void **cbhandle_p) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetnodecnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetnodeint) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXLgetnodeleftcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumbin) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetnumcuts) (CPXCENVptr env, CPXCLPptr lp, int cuttype, CPXINT * num_p) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumindconstrs) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumint) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetnummipstarts) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumsemicont) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumsemiint) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumsos) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetorder) (CPXCENVptr env, CPXCLPptr lp, CPXINT * cnt_p, CPXINT * indices, CPXINT * priority, int *direction, CPXINT ordspace, CPXINT * surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpooldivfilter) (CPXCENVptr env, CPXCLPptr lp, double *lower_cutoff_p, double *upper_cutoff_p, CPXINT * nzcnt_p, CPXINT * ind, double *val, double *refval, CPXINT space, CPXINT * surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolfilterindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolfiltername) (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolfiltertype) (CPXCENVptr env, CPXCLPptr lp, int *ftype_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolmeanobjval) (CPXCENVptr env, CPXCLPptr lp, double *meanobjval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolnumfilters) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolnumreplaced) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolnumsolns) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolobjval) (CPXCENVptr env, CPXCLPptr lp, int soln, double *objval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolqconstrslack) (CPXCENVptr env, CPXCLPptr lp, int soln, double *qcslack, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolrngfilter) (CPXCENVptr env, CPXCLPptr lp, double *lb_p, double *ub_p, CPXINT * nzcnt_p, CPXINT * ind, double *val, CPXINT space, CPXINT * surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolslack) (CPXCENVptr env, CPXCLPptr lp, int soln, double *slack, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolsolnindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolsolnname) (CPXCENVptr env, CPXCLPptr lp, char *store, CPXSIZE storesz, CPXSIZE * surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolnpoolx) (CPXCENVptr env, CPXCLPptr lp, int soln, double *x, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsolvecallbackfunc) (CPXCENVptr env, CPXL_CALLBACK_SOLVE ** solvecallback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsos) (CPXCENVptr env, CPXCLPptr lp, CPXLONG * numsosnz_p, char *sostype, CPXLONG * sosbeg, CPXINT * sosind, double *soswt, CPXLONG sosspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsosindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsosinfeas) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsosname) (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, CPXSIZE storespace, CPXSIZE * surplus_p, CPXINT begin, CPXINT end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsubmethod) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetsubstat) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetusercutcallbackfunc) (CPXCENVptr env, CPXL_CALLBACK_CUT ** cutcallback_p, void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLindconstrslackfromx) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *indslack) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLmipopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLordread) (CPXCENVptr env, char const *filename_str, CPXINT numcols, char const *const *colname, CPXINT * cnt_p, CPXINT * indices, CPXINT * priority, int *direction) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLordwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLpopulate) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLreadcopymipstarts) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLreadcopyorder) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLreadcopysolnpoolfilters) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLrefinemipstartconflict) (CPXCENVptr env, CPXLPptr lp, int mipstartindex, CPXINT * confnumrows_p, CPXINT * confnumcols_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLrefinemipstartconflictext) (CPXCENVptr env, CPXLPptr lp, int mipstartindex, CPXLONG grpcnt, CPXLONG concnt, double const *grppref, CPXLONG const *grpbeg, CPXINT const *grpind, char const *grptype) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetbranchcallbackfunc) (CPXENVptr env, CPXL_CALLBACK_BRANCH * branchcallback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetbranchnosolncallbackfunc) (CPXENVptr env, CPXL_CALLBACK_BRANCH * branchnosolncallback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetdeletenodecallbackfunc) (CPXENVptr env, CPXL_CALLBACK_DELETENODE * deletecallback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetheuristiccallbackfunc) (CPXENVptr env, CPXL_CALLBACK_HEURISTIC * heuristiccallback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetincumbentcallbackfunc) (CPXENVptr env, CPXL_CALLBACK_INCUMBENT * incumbentcallback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetinfocallbackfunc) (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetlazyconstraintcallbackfunc) (CPXENVptr env, CPXL_CALLBACK_CUT * lazyconcallback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetmipcallbackfunc) (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetnodecallbackfunc) (CPXENVptr env, CPXL_CALLBACK_NODE * nodecallback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetsolvecallbackfunc) (CPXENVptr env, CPXL_CALLBACK_SOLVE * solvecallback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLsetusercutcallbackfunc) (CPXENVptr env, CPXL_CALLBACK_CUT * cutcallback, void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLwritemipstarts) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, int begin, int end) = NULL;

/* hijacked functions */

CPXLIBAPI CPXCHANNELptr CPXPUBLIC CPXLaddchannel (CPXENVptr env){
    if (!native_CPXLaddchannel)
        native_CPXLaddchannel = load_symbol("CPXLaddchannel");
    return native_CPXLaddchannel(env);
}
CPXLIBAPI int CPXPUBLIC CPXLaddcols (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, CPXLONG nzcnt, double const *obj, CPXLONG const *cmatbeg, CPXINT const *cmatind, double const *cmatval, double const *lb, double const *ub, char const *const *colname){
    if (!native_CPXLaddcols)
        native_CPXLaddcols = load_symbol("CPXLaddcols");
    return native_CPXLaddcols(env, lp, ccnt, nzcnt, obj, cmatbeg, cmatind, cmatval, lb, ub, colname);
}
CPXLIBAPI int CPXPUBLIC CPXLaddfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr){
    if (!native_CPXLaddfpdest)
        native_CPXLaddfpdest = load_symbol("CPXLaddfpdest");
    return native_CPXLaddfpdest(env, channel, fileptr);
}
CPXLIBAPI int CPXPUBLIC CPXLaddfuncdest (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, char const *)){
    if (!native_CPXLaddfuncdest)
        native_CPXLaddfuncdest = load_symbol("CPXLaddfuncdest");
    return native_CPXLaddfuncdest(env, channel, handle, msgfunction);
}
CPXLIBAPI int CPXPUBLIC CPXLaddrows (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *colname, char const *const *rowname){
    if (!native_CPXLaddrows)
        native_CPXLaddrows = load_symbol("CPXLaddrows");
    return native_CPXLaddrows(env, lp, ccnt, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, colname, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXLbasicpresolve (CPXCENVptr env, CPXLPptr lp, double *redlb, double *redub, int *rstat){
    if (!native_CPXLbasicpresolve)
        native_CPXLbasicpresolve = load_symbol("CPXLbasicpresolve");
    return native_CPXLbasicpresolve(env, lp, redlb, redub, rstat);
}
CPXLIBAPI int CPXPUBLIC CPXLbinvacol (CPXCENVptr env, CPXCLPptr lp, CPXINT j, double *x){
    if (!native_CPXLbinvacol)
        native_CPXLbinvacol = load_symbol("CPXLbinvacol");
    return native_CPXLbinvacol(env, lp, j, x);
}
CPXLIBAPI int CPXPUBLIC CPXLbinvarow (CPXCENVptr env, CPXCLPptr lp, CPXINT i, double *z){
    if (!native_CPXLbinvarow)
        native_CPXLbinvarow = load_symbol("CPXLbinvarow");
    return native_CPXLbinvarow(env, lp, i, z);
}
CPXLIBAPI int CPXPUBLIC CPXLbinvcol (CPXCENVptr env, CPXCLPptr lp, CPXINT j, double *x){
    if (!native_CPXLbinvcol)
        native_CPXLbinvcol = load_symbol("CPXLbinvcol");
    return native_CPXLbinvcol(env, lp, j, x);
}
CPXLIBAPI int CPXPUBLIC CPXLbinvrow (CPXCENVptr env, CPXCLPptr lp, CPXINT i, double *y){
    if (!native_CPXLbinvrow)
        native_CPXLbinvrow = load_symbol("CPXLbinvrow");
    return native_CPXLbinvrow(env, lp, i, y);
}
CPXLIBAPI int CPXPUBLIC CPXLboundsa (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end, double *lblower, double *lbupper, double *ublower, double *ubupper){
    if (!native_CPXLboundsa)
        native_CPXLboundsa = load_symbol("CPXLboundsa");
    return native_CPXLboundsa(env, lp, begin, end, lblower, lbupper, ublower, ubupper);
}
CPXLIBAPI int CPXPUBLIC CPXLbtran (CPXCENVptr env, CPXCLPptr lp, double *y){
    if (!native_CPXLbtran)
        native_CPXLbtran = load_symbol("CPXLbtran");
    return native_CPXLbtran(env, lp, y);
}
CPXLIBAPI int CPXPUBLIC CPXLcheckdfeas (CPXCENVptr env, CPXLPptr lp, CPXINT * infeas_p){
    if (!native_CPXLcheckdfeas)
        native_CPXLcheckdfeas = load_symbol("CPXLcheckdfeas");
    return native_CPXLcheckdfeas(env, lp, infeas_p);
}
CPXLIBAPI int CPXPUBLIC CPXLcheckpfeas (CPXCENVptr env, CPXLPptr lp, CPXINT * infeas_p){
    if (!native_CPXLcheckpfeas)
        native_CPXLcheckpfeas = load_symbol("CPXLcheckpfeas");
    return native_CPXLcheckpfeas(env, lp, infeas_p);
}
CPXLIBAPI int CPXPUBLIC CPXLchecksoln (CPXCENVptr env, CPXLPptr lp, int *lpstatus_p){
    if (!native_CPXLchecksoln)
        native_CPXLchecksoln = load_symbol("CPXLchecksoln");
    return native_CPXLchecksoln(env, lp, lpstatus_p);
}
CPXLIBAPI int CPXPUBLIC CPXLchgbds (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd){
    if (!native_CPXLchgbds)
        native_CPXLchgbds = load_symbol("CPXLchgbds");
    return native_CPXLchgbds(env, lp, cnt, indices, lu, bd);
}
CPXLIBAPI int CPXPUBLIC CPXLchgcoef (CPXCENVptr env, CPXLPptr lp, CPXINT i, CPXINT j, double newvalue){
    if (!native_CPXLchgcoef)
        native_CPXLchgcoef = load_symbol("CPXLchgcoef");
    return native_CPXLchgcoef(env, lp, i, j, newvalue);
}
CPXLIBAPI int CPXPUBLIC CPXLchgcoeflist (CPXCENVptr env, CPXLPptr lp, CPXLONG numcoefs, CPXINT const *rowlist, CPXINT const *collist, double const *vallist){
    if (!native_CPXLchgcoeflist)
        native_CPXLchgcoeflist = load_symbol("CPXLchgcoeflist");
    return native_CPXLchgcoeflist(env, lp, numcoefs, rowlist, collist, vallist);
}
CPXLIBAPI int CPXPUBLIC CPXLchgcolname (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *const *newname){
    if (!native_CPXLchgcolname)
        native_CPXLchgcolname = load_symbol("CPXLchgcolname");
    return native_CPXLchgcolname(env, lp, cnt, indices, newname);
}
CPXLIBAPI int CPXPUBLIC CPXLchgname (CPXCENVptr env, CPXLPptr lp, int key, CPXINT ij, char const *newname_str){
    if (!native_CPXLchgname)
        native_CPXLchgname = load_symbol("CPXLchgname");
    return native_CPXLchgname(env, lp, key, ij, newname_str);
}
CPXLIBAPI int CPXPUBLIC CPXLchgobj (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values){
    if (!native_CPXLchgobj)
        native_CPXLchgobj = load_symbol("CPXLchgobj");
    return native_CPXLchgobj(env, lp, cnt, indices, values);
}
CPXLIBAPI int CPXPUBLIC CPXLchgobjsen (CPXCENVptr env, CPXLPptr lp, int maxormin){
    if (!native_CPXLchgobjsen)
        native_CPXLchgobjsen = load_symbol("CPXLchgobjsen");
    return native_CPXLchgobjsen(env, lp, maxormin);
}
CPXLIBAPI int CPXPUBLIC CPXLchgprobname (CPXCENVptr env, CPXLPptr lp, char const *probname){
    if (!native_CPXLchgprobname)
        native_CPXLchgprobname = load_symbol("CPXLchgprobname");
    return native_CPXLchgprobname(env, lp, probname);
}
CPXLIBAPI int CPXPUBLIC CPXLchgprobtype (CPXCENVptr env, CPXLPptr lp, int type){
    if (!native_CPXLchgprobtype)
        native_CPXLchgprobtype = load_symbol("CPXLchgprobtype");
    return native_CPXLchgprobtype(env, lp, type);
}
CPXLIBAPI int CPXPUBLIC CPXLchgprobtypesolnpool (CPXCENVptr env, CPXLPptr lp, int type, int soln){
    if (!native_CPXLchgprobtypesolnpool)
        native_CPXLchgprobtypesolnpool = load_symbol("CPXLchgprobtypesolnpool");
    return native_CPXLchgprobtypesolnpool(env, lp, type, soln);
}
CPXLIBAPI int CPXPUBLIC CPXLchgrhs (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values){
    if (!native_CPXLchgrhs)
        native_CPXLchgrhs = load_symbol("CPXLchgrhs");
    return native_CPXLchgrhs(env, lp, cnt, indices, values);
}
CPXLIBAPI int CPXPUBLIC CPXLchgrngval (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values){
    if (!native_CPXLchgrngval)
        native_CPXLchgrngval = load_symbol("CPXLchgrngval");
    return native_CPXLchgrngval(env, lp, cnt, indices, values);
}
CPXLIBAPI int CPXPUBLIC CPXLchgrowname (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *const *newname){
    if (!native_CPXLchgrowname)
        native_CPXLchgrowname = load_symbol("CPXLchgrowname");
    return native_CPXLchgrowname(env, lp, cnt, indices, newname);
}
CPXLIBAPI int CPXPUBLIC CPXLchgsense (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *sense){
    if (!native_CPXLchgsense)
        native_CPXLchgsense = load_symbol("CPXLchgsense");
    return native_CPXLchgsense(env, lp, cnt, indices, sense);
}
CPXLIBAPI int CPXPUBLIC CPXLcleanup (CPXCENVptr env, CPXLPptr lp, double eps){
    if (!native_CPXLcleanup)
        native_CPXLcleanup = load_symbol("CPXLcleanup");
    return native_CPXLcleanup(env, lp, eps);
}
CPXLIBAPI CPXLPptr CPXPUBLIC CPXLcloneprob (CPXCENVptr env, CPXCLPptr lp, int *status_p){
    if (!native_CPXLcloneprob)
        native_CPXLcloneprob = load_symbol("CPXLcloneprob");
    return native_CPXLcloneprob(env, lp, status_p);
}
CPXLIBAPI int CPXPUBLIC CPXLcloseCPLEX (CPXENVptr * env_p){
    if (!native_CPXLcloseCPLEX)
        native_CPXLcloseCPLEX = load_symbol("CPXLcloseCPLEX");
    return native_CPXLcloseCPLEX(env_p);
}
CPXLIBAPI int CPXPUBLIC CPXLclpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLclpwrite)
        native_CPXLclpwrite = load_symbol("CPXLclpwrite");
    return native_CPXLclpwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLcompletelp (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLcompletelp)
        native_CPXLcompletelp = load_symbol("CPXLcompletelp");
    return native_CPXLcompletelp(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLcopybase (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat){
    if (!native_CPXLcopybase)
        native_CPXLcopybase = load_symbol("CPXLcopybase");
    return native_CPXLcopybase(env, lp, cstat, rstat);
}
CPXLIBAPI int CPXPUBLIC CPXLcopybasednorms (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *dnorm){
    if (!native_CPXLcopybasednorms)
        native_CPXLcopybasednorms = load_symbol("CPXLcopybasednorms");
    return native_CPXLcopybasednorms(env, lp, cstat, rstat, dnorm);
}
CPXLIBAPI int CPXPUBLIC CPXLcopydnorms (CPXCENVptr env, CPXLPptr lp, double const *norm, CPXINT const *head, CPXINT len){
    if (!native_CPXLcopydnorms)
        native_CPXLcopydnorms = load_symbol("CPXLcopydnorms");
    return native_CPXLcopydnorms(env, lp, norm, head, len);
}
CPXLIBAPI int CPXPUBLIC CPXLcopylp (CPXCENVptr env, CPXLPptr lp, CPXINT numcols, CPXINT numrows, int objsense, double const *objective, double const *rhs, char const *sense, CPXLONG const *matbeg, CPXINT const *matcnt, CPXINT const *matind, double const *matval, double const *lb, double const *ub, double const *rngval){
    if (!native_CPXLcopylp)
        native_CPXLcopylp = load_symbol("CPXLcopylp");
    return native_CPXLcopylp(env, lp, numcols, numrows, objsense, objective, rhs, sense, matbeg, matcnt, matind, matval, lb, ub, rngval);
}
CPXLIBAPI int CPXPUBLIC CPXLcopylpwnames (CPXCENVptr env, CPXLPptr lp, CPXINT numcols, CPXINT numrows, int objsense, double const *objective, double const *rhs, char const *sense, CPXLONG const *matbeg, CPXINT const *matcnt, CPXINT const *matind, double const *matval, double const *lb, double const *ub, double const *rngval, char const *const *colname, char const *const *rowname){
    if (!native_CPXLcopylpwnames)
        native_CPXLcopylpwnames = load_symbol("CPXLcopylpwnames");
    return native_CPXLcopylpwnames(env, lp, numcols, numrows, objsense, objective, rhs, sense, matbeg, matcnt, matind, matval, lb, ub, rngval, colname, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXLcopynettolp (CPXCENVptr env, CPXLPptr lp, CPXCNETptr net){
    if (!native_CPXLcopynettolp)
        native_CPXLcopynettolp = load_symbol("CPXLcopynettolp");
    return native_CPXLcopynettolp(env, lp, net);
}
CPXLIBAPI int CPXPUBLIC CPXLcopyobjname (CPXCENVptr env, CPXLPptr lp, char const *objname_str){
    if (!native_CPXLcopyobjname)
        native_CPXLcopyobjname = load_symbol("CPXLcopyobjname");
    return native_CPXLcopyobjname(env, lp, objname_str);
}
CPXLIBAPI int CPXPUBLIC CPXLcopypartialbase (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, CPXINT const *cindices, int const *cstat, CPXINT rcnt, CPXINT const *rindices, int const *rstat){
    if (!native_CPXLcopypartialbase)
        native_CPXLcopypartialbase = load_symbol("CPXLcopypartialbase");
    return native_CPXLcopypartialbase(env, lp, ccnt, cindices, cstat, rcnt, rindices, rstat);
}
CPXLIBAPI int CPXPUBLIC CPXLcopypnorms (CPXCENVptr env, CPXLPptr lp, double const *cnorm, double const *rnorm, CPXINT len){
    if (!native_CPXLcopypnorms)
        native_CPXLcopypnorms = load_symbol("CPXLcopypnorms");
    return native_CPXLcopypnorms(env, lp, cnorm, rnorm, len);
}
CPXLIBAPI int CPXPUBLIC CPXLcopyprotected (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices){
    if (!native_CPXLcopyprotected)
        native_CPXLcopyprotected = load_symbol("CPXLcopyprotected");
    return native_CPXLcopyprotected(env, lp, cnt, indices);
}
CPXLIBAPI int CPXPUBLIC CPXLcopystart (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *cprim, double const *rprim, double const *cdual, double const *rdual){
    if (!native_CPXLcopystart)
        native_CPXLcopystart = load_symbol("CPXLcopystart");
    return native_CPXLcopystart(env, lp, cstat, rstat, cprim, rprim, cdual, rdual);
}
CPXLIBAPI CPXLPptr CPXPUBLIC CPXLcreateprob (CPXCENVptr env, int *status_p, char const *probname_str){
    if (!native_CPXLcreateprob)
        native_CPXLcreateprob = load_symbol("CPXLcreateprob");
    return native_CPXLcreateprob(env, status_p, probname_str);
}
CPXLIBAPI int CPXPUBLIC CPXLcrushform (CPXCENVptr env, CPXCLPptr lp, CPXINT len, CPXINT const *ind, double const *val, CPXINT * plen_p, double *poffset_p, CPXINT * pind, double *pval){
    if (!native_CPXLcrushform)
        native_CPXLcrushform = load_symbol("CPXLcrushform");
    return native_CPXLcrushform(env, lp, len, ind, val, plen_p, poffset_p, pind, pval);
}
CPXLIBAPI int CPXPUBLIC CPXLcrushpi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *prepi){
    if (!native_CPXLcrushpi)
        native_CPXLcrushpi = load_symbol("CPXLcrushpi");
    return native_CPXLcrushpi(env, lp, pi, prepi);
}
CPXLIBAPI int CPXPUBLIC CPXLcrushx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *prex){
    if (!native_CPXLcrushx)
        native_CPXLcrushx = load_symbol("CPXLcrushx");
    return native_CPXLcrushx(env, lp, x, prex);
}
CPXLIBAPI int CPXPUBLIC CPXLdelchannel (CPXENVptr env, CPXCHANNELptr * channel_p){
    if (!native_CPXLdelchannel)
        native_CPXLdelchannel = load_symbol("CPXLdelchannel");
    return native_CPXLdelchannel(env, channel_p);
}
CPXLIBAPI int CPXPUBLIC CPXLdelcols (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end){
    if (!native_CPXLdelcols)
        native_CPXLdelcols = load_symbol("CPXLdelcols");
    return native_CPXLdelcols(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLdelfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr){
    if (!native_CPXLdelfpdest)
        native_CPXLdelfpdest = load_symbol("CPXLdelfpdest");
    return native_CPXLdelfpdest(env, channel, fileptr);
}
CPXLIBAPI int CPXPUBLIC CPXLdelfuncdest (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, char const *)){
    if (!native_CPXLdelfuncdest)
        native_CPXLdelfuncdest = load_symbol("CPXLdelfuncdest");
    return native_CPXLdelfuncdest(env, channel, handle, msgfunction);
}
CPXLIBAPI int CPXPUBLIC CPXLdelnames (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLdelnames)
        native_CPXLdelnames = load_symbol("CPXLdelnames");
    return native_CPXLdelnames(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLdelrows (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end){
    if (!native_CPXLdelrows)
        native_CPXLdelrows = load_symbol("CPXLdelrows");
    return native_CPXLdelrows(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLdelsetcols (CPXCENVptr env, CPXLPptr lp, CPXINT * delstat){
    if (!native_CPXLdelsetcols)
        native_CPXLdelsetcols = load_symbol("CPXLdelsetcols");
    return native_CPXLdelsetcols(env, lp, delstat);
}
CPXLIBAPI int CPXPUBLIC CPXLdelsetrows (CPXCENVptr env, CPXLPptr lp, CPXINT * delstat){
    if (!native_CPXLdelsetrows)
        native_CPXLdelsetrows = load_symbol("CPXLdelsetrows");
    return native_CPXLdelsetrows(env, lp, delstat);
}
CPXLIBAPI int CPXPUBLIC CPXLdeserializercreate (CPXDESERIALIZERptr * deser_p, CPXLONG size, void const *buffer){
    if (!native_CPXLdeserializercreate)
        native_CPXLdeserializercreate = load_symbol("CPXLdeserializercreate");
    return native_CPXLdeserializercreate(deser_p, size, buffer);
}
CPXLIBAPI void CPXPUBLIC CPXLdeserializerdestroy (CPXDESERIALIZERptr deser){
    if (!native_CPXLdeserializerdestroy)
        native_CPXLdeserializerdestroy = load_symbol("CPXLdeserializerdestroy");
    return native_CPXLdeserializerdestroy(deser);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLdeserializerleft (CPXCDESERIALIZERptr deser){
    if (!native_CPXLdeserializerleft)
        native_CPXLdeserializerleft = load_symbol("CPXLdeserializerleft");
    return native_CPXLdeserializerleft(deser);
}
CPXLIBAPI int CPXPUBLIC CPXLdisconnectchannel (CPXCENVptr env, CPXCHANNELptr channel){
    if (!native_CPXLdisconnectchannel)
        native_CPXLdisconnectchannel = load_symbol("CPXLdisconnectchannel");
    return native_CPXLdisconnectchannel(env, channel);
}
CPXLIBAPI int CPXPUBLIC CPXLdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *dj){
    if (!native_CPXLdjfrompi)
        native_CPXLdjfrompi = load_symbol("CPXLdjfrompi");
    return native_CPXLdjfrompi(env, lp, pi, dj);
}
CPXLIBAPI int CPXPUBLIC CPXLdperwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon){
    if (!native_CPXLdperwrite)
        native_CPXLdperwrite = load_symbol("CPXLdperwrite");
    return native_CPXLdperwrite(env, lp, filename_str, epsilon);
}
CPXLIBAPI int CPXPUBLIC CPXLdratio (CPXCENVptr env, CPXLPptr lp, CPXINT * indices, CPXINT cnt, double *downratio, double *upratio, CPXINT * downenter, CPXINT * upenter, int *downstatus, int *upstatus){
    if (!native_CPXLdratio)
        native_CPXLdratio = load_symbol("CPXLdratio");
    return native_CPXLdratio(env, lp, indices, cnt, downratio, upratio, downenter, upenter, downstatus, upstatus);
}
CPXLIBAPI int CPXPUBLIC CPXLdualfarkas (CPXCENVptr env, CPXCLPptr lp, double *y, double *proof_p){
    if (!native_CPXLdualfarkas)
        native_CPXLdualfarkas = load_symbol("CPXLdualfarkas");
    return native_CPXLdualfarkas(env, lp, y, proof_p);
}
CPXLIBAPI int CPXPUBLIC CPXLdualopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLdualopt)
        native_CPXLdualopt = load_symbol("CPXLdualopt");
    return native_CPXLdualopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLdualwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, double *objshift_p){
    if (!native_CPXLdualwrite)
        native_CPXLdualwrite = load_symbol("CPXLdualwrite");
    return native_CPXLdualwrite(env, lp, filename_str, objshift_p);
}
CPXLIBAPI int CPXPUBLIC CPXLembwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXLembwrite)
        native_CPXLembwrite = load_symbol("CPXLembwrite");
    return native_CPXLembwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLfclose (CPXFILEptr stream){
    if (!native_CPXLfclose)
        native_CPXLfclose = load_symbol("CPXLfclose");
    return native_CPXLfclose(stream);
}
CPXLIBAPI int CPXPUBLIC CPXLfeasopt (CPXCENVptr env, CPXLPptr lp, double const *rhs, double const *rng, double const *lb, double const *ub){
    if (!native_CPXLfeasopt)
        native_CPXLfeasopt = load_symbol("CPXLfeasopt");
    return native_CPXLfeasopt(env, lp, rhs, rng, lb, ub);
}
CPXLIBAPI int CPXPUBLIC CPXLfeasoptext (CPXCENVptr env, CPXLPptr lp, CPXINT grpcnt, CPXLONG concnt, double const *grppref, CPXLONG const *grpbeg, CPXINT const *grpind, char const *grptype){
    if (!native_CPXLfeasoptext)
        native_CPXLfeasoptext = load_symbol("CPXLfeasoptext");
    return native_CPXLfeasoptext(env, lp, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}
CPXLIBAPI void CPXPUBLIC CPXLfinalize (void){
    if (!native_CPXLfinalize)
        native_CPXLfinalize = load_symbol("CPXLfinalize");
    return native_CPXLfinalize();
}
CPXLIBAPI int CPXPUBLIC CPXLflushchannel (CPXCENVptr env, CPXCHANNELptr channel){
    if (!native_CPXLflushchannel)
        native_CPXLflushchannel = load_symbol("CPXLflushchannel");
    return native_CPXLflushchannel(env, channel);
}
CPXLIBAPI int CPXPUBLIC CPXLflushstdchannels (CPXCENVptr env){
    if (!native_CPXLflushstdchannels)
        native_CPXLflushstdchannels = load_symbol("CPXLflushstdchannels");
    return native_CPXLflushstdchannels(env);
}
CPXLIBAPI CPXFILEptr CPXPUBLIC CPXLfopen (char const *filename_str, char const *type_str){
    if (!native_CPXLfopen)
        native_CPXLfopen = load_symbol("CPXLfopen");
    return native_CPXLfopen(filename_str, type_str);
}
CPXLIBAPI int CPXPUBLIC CPXLfputs (char const *s_str, CPXFILEptr stream){
    if (!native_CPXLfputs)
        native_CPXLfputs = load_symbol("CPXLfputs");
    return native_CPXLfputs(s_str, stream);
}
CPXLIBAPI void CPXPUBLIC CPXLfree (void *ptr){
    if (!native_CPXLfree)
        native_CPXLfree = load_symbol("CPXLfree");
    return native_CPXLfree(ptr);
}
CPXLIBAPI int CPXPUBLIC CPXLfreeparenv (CPXENVptr env, CPXENVptr * child_p){
    if (!native_CPXLfreeparenv)
        native_CPXLfreeparenv = load_symbol("CPXLfreeparenv");
    return native_CPXLfreeparenv(env, child_p);
}
CPXLIBAPI int CPXPUBLIC CPXLfreepresolve (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLfreepresolve)
        native_CPXLfreepresolve = load_symbol("CPXLfreepresolve");
    return native_CPXLfreepresolve(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLfreeprob (CPXCENVptr env, CPXLPptr * lp_p){
    if (!native_CPXLfreeprob)
        native_CPXLfreeprob = load_symbol("CPXLfreeprob");
    return native_CPXLfreeprob(env, lp_p);
}
CPXLIBAPI int CPXPUBLIC CPXLftran (CPXCENVptr env, CPXCLPptr lp, double *x){
    if (!native_CPXLftran)
        native_CPXLftran = load_symbol("CPXLftran");
    return native_CPXLftran(env, lp, x);
}
CPXLIBAPI int CPXPUBLIC CPXLgetax (CPXCENVptr env, CPXCLPptr lp, double *x, CPXINT begin, CPXINT end){
    if (!native_CPXLgetax)
        native_CPXLgetax = load_symbol("CPXLgetax");
    return native_CPXLgetax(env, lp, x, begin, end);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetbaritcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetbaritcnt)
        native_CPXLgetbaritcnt = load_symbol("CPXLgetbaritcnt");
    return native_CPXLgetbaritcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetbase (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat){
    if (!native_CPXLgetbase)
        native_CPXLgetbase = load_symbol("CPXLgetbase");
    return native_CPXLgetbase(env, lp, cstat, rstat);
}
CPXLIBAPI int CPXPUBLIC CPXLgetbasednorms (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat, double *dnorm){
    if (!native_CPXLgetbasednorms)
        native_CPXLgetbasednorms = load_symbol("CPXLgetbasednorms");
    return native_CPXLgetbasednorms(env, lp, cstat, rstat, dnorm);
}
CPXLIBAPI int CPXPUBLIC CPXLgetbhead (CPXCENVptr env, CPXCLPptr lp, CPXINT * head, double *x){
    if (!native_CPXLgetbhead)
        native_CPXLgetbhead = load_symbol("CPXLgetbhead");
    return native_CPXLgetbhead(env, lp, head, x);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackinfo (CPXCENVptr env, void *cbdata, int wherefrom, int whichinfo, void *result_p){
    if (!native_CPXLgetcallbackinfo)
        native_CPXLgetcallbackinfo = load_symbol("CPXLgetcallbackinfo");
    return native_CPXLgetcallbackinfo(env, cbdata, wherefrom, whichinfo, result_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetchannels (CPXCENVptr env, CPXCHANNELptr * cpxresults_p, CPXCHANNELptr * cpxwarning_p, CPXCHANNELptr * cpxerror_p, CPXCHANNELptr * cpxlog_p){
    if (!native_CPXLgetchannels)
        native_CPXLgetchannels = load_symbol("CPXLgetchannels");
    return native_CPXLgetchannels(env, cpxresults_p, cpxwarning_p, cpxerror_p, cpxlog_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetchgparam (CPXCENVptr env, int *cnt_p, int *paramnum, int pspace, int *surplus_p){
    if (!native_CPXLgetchgparam)
        native_CPXLgetchgparam = load_symbol("CPXLgetchgparam");
    return native_CPXLgetchgparam(env, cnt_p, paramnum, pspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcoef (CPXCENVptr env, CPXCLPptr lp, CPXINT i, CPXINT j, double *coef_p){
    if (!native_CPXLgetcoef)
        native_CPXLgetcoef = load_symbol("CPXLgetcoef");
    return native_CPXLgetcoef(env, lp, i, j, coef_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcolindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p){
    if (!native_CPXLgetcolindex)
        native_CPXLgetcolindex = load_symbol("CPXLgetcolindex");
    return native_CPXLgetcolindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcolinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcolinfeas)
        native_CPXLgetcolinfeas = load_symbol("CPXLgetcolinfeas");
    return native_CPXLgetcolinfeas(env, lp, x, infeasout, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcolname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, CPXSIZE storespace, CPXSIZE * surplus_p, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcolname)
        native_CPXLgetcolname = load_symbol("CPXLgetcolname");
    return native_CPXLgetcolname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcols (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * cmatbeg, CPXINT * cmatind, double *cmatval, CPXLONG cmatspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcols)
        native_CPXLgetcols = load_symbol("CPXLgetcols");
    return native_CPXLgetcols(env, lp, nzcnt_p, cmatbeg, cmatind, cmatval, cmatspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetconflict (CPXCENVptr env, CPXCLPptr lp, int *confstat_p, CPXINT * rowind, int *rowbdstat, CPXINT * confnumrows_p, CPXINT * colind, int *colbdstat, CPXINT * confnumcols_p){
    if (!native_CPXLgetconflict)
        native_CPXLgetconflict = load_symbol("CPXLgetconflict");
    return native_CPXLgetconflict(env, lp, confstat_p, rowind, rowbdstat, confnumrows_p, colind, colbdstat, confnumcols_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetconflictext (CPXCENVptr env, CPXCLPptr lp, int *grpstat, CPXLONG beg, CPXLONG end){
    if (!native_CPXLgetconflictext)
        native_CPXLgetconflictext = load_symbol("CPXLgetconflictext");
    return native_CPXLgetconflictext(env, lp, grpstat, beg, end);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetcrossdexchcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetcrossdexchcnt)
        native_CPXLgetcrossdexchcnt = load_symbol("CPXLgetcrossdexchcnt");
    return native_CPXLgetcrossdexchcnt(env, lp);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetcrossdpushcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetcrossdpushcnt)
        native_CPXLgetcrossdpushcnt = load_symbol("CPXLgetcrossdpushcnt");
    return native_CPXLgetcrossdpushcnt(env, lp);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetcrosspexchcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetcrosspexchcnt)
        native_CPXLgetcrosspexchcnt = load_symbol("CPXLgetcrosspexchcnt");
    return native_CPXLgetcrosspexchcnt(env, lp);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetcrossppushcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetcrossppushcnt)
        native_CPXLgetcrossppushcnt = load_symbol("CPXLgetcrossppushcnt");
    return native_CPXLgetcrossppushcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetdblparam (CPXCENVptr env, int whichparam, double *value_p){
    if (!native_CPXLgetdblparam)
        native_CPXLgetdblparam = load_symbol("CPXLgetdblparam");
    return native_CPXLgetdblparam(env, whichparam, value_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetdblquality (CPXCENVptr env, CPXCLPptr lp, double *quality_p, int what){
    if (!native_CPXLgetdblquality)
        native_CPXLgetdblquality = load_symbol("CPXLgetdblquality");
    return native_CPXLgetdblquality(env, lp, quality_p, what);
}
CPXLIBAPI int CPXPUBLIC CPXLgetdettime (CPXCENVptr env, double *dettimestamp_p){
    if (!native_CPXLgetdettime)
        native_CPXLgetdettime = load_symbol("CPXLgetdettime");
    return native_CPXLgetdettime(env, dettimestamp_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetdj (CPXCENVptr env, CPXCLPptr lp, double *dj, CPXINT begin, CPXINT end){
    if (!native_CPXLgetdj)
        native_CPXLgetdj = load_symbol("CPXLgetdj");
    return native_CPXLgetdj(env, lp, dj, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetdnorms (CPXCENVptr env, CPXCLPptr lp, double *norm, CPXINT * head, CPXINT * len_p){
    if (!native_CPXLgetdnorms)
        native_CPXLgetdnorms = load_symbol("CPXLgetdnorms");
    return native_CPXLgetdnorms(env, lp, norm, head, len_p);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetdsbcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetdsbcnt)
        native_CPXLgetdsbcnt = load_symbol("CPXLgetdsbcnt");
    return native_CPXLgetdsbcnt(env, lp);
}
CPXLIBAPI CPXCCHARptr CPXPUBLIC CPXLgeterrorstring (CPXCENVptr env, int errcode, char *buffer_str){
    if (!native_CPXLgeterrorstring)
        native_CPXLgeterrorstring = load_symbol("CPXLgeterrorstring");
    return native_CPXLgeterrorstring(env, errcode, buffer_str);
}
CPXLIBAPI int CPXPUBLIC CPXLgetgrad (CPXCENVptr env, CPXCLPptr lp, CPXINT j, CPXINT * head, double *y){
    if (!native_CPXLgetgrad)
        native_CPXLgetgrad = load_symbol("CPXLgetgrad");
    return native_CPXLgetgrad(env, lp, j, head, y);
}
CPXLIBAPI int CPXPUBLIC CPXLgetijdiv (CPXCENVptr env, CPXCLPptr lp, CPXINT * idiv_p, CPXINT * jdiv_p){
    if (!native_CPXLgetijdiv)
        native_CPXLgetijdiv = load_symbol("CPXLgetijdiv");
    return native_CPXLgetijdiv(env, lp, idiv_p, jdiv_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetijrow (CPXCENVptr env, CPXCLPptr lp, CPXINT i, CPXINT j, CPXINT * row_p){
    if (!native_CPXLgetijrow)
        native_CPXLgetijrow = load_symbol("CPXLgetijrow");
    return native_CPXLgetijrow(env, lp, i, j, row_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetintparam (CPXCENVptr env, int whichparam, CPXINT * value_p){
    if (!native_CPXLgetintparam)
        native_CPXLgetintparam = load_symbol("CPXLgetintparam");
    return native_CPXLgetintparam(env, whichparam, value_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetintquality (CPXCENVptr env, CPXCLPptr lp, CPXINT * quality_p, int what){
    if (!native_CPXLgetintquality)
        native_CPXLgetintquality = load_symbol("CPXLgetintquality");
    return native_CPXLgetintquality(env, lp, quality_p, what);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetitcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetitcnt)
        native_CPXLgetitcnt = load_symbol("CPXLgetitcnt");
    return native_CPXLgetitcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetlb (CPXCENVptr env, CPXCLPptr lp, double *lb, CPXINT begin, CPXINT end){
    if (!native_CPXLgetlb)
        native_CPXLgetlb = load_symbol("CPXLgetlb");
    return native_CPXLgetlb(env, lp, lb, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetlogfile (CPXCENVptr env, CPXFILEptr * logfile_p){
    if (!native_CPXLgetlogfile)
        native_CPXLgetlogfile = load_symbol("CPXLgetlogfile");
    return native_CPXLgetlogfile(env, logfile_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetlongparam (CPXCENVptr env, int whichparam, CPXLONG * value_p){
    if (!native_CPXLgetlongparam)
        native_CPXLgetlongparam = load_symbol("CPXLgetlongparam");
    return native_CPXLgetlongparam(env, whichparam, value_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetlpcallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p){
    if (!native_CPXLgetlpcallbackfunc)
        native_CPXLgetlpcallbackfunc = load_symbol("CPXLgetlpcallbackfunc");
    return native_CPXLgetlpcallbackfunc(env, callback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetmethod (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetmethod)
        native_CPXLgetmethod = load_symbol("CPXLgetmethod");
    return native_CPXLgetmethod(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetnetcallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p){
    if (!native_CPXLgetnetcallbackfunc)
        native_CPXLgetnetcallbackfunc = load_symbol("CPXLgetnetcallbackfunc");
    return native_CPXLgetnetcallbackfunc(env, callback_p, cbhandle_p);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumcols (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumcols)
        native_CPXLgetnumcols = load_symbol("CPXLgetnumcols");
    return native_CPXLgetnumcols(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetnumcores (CPXCENVptr env, int *numcores_p){
    if (!native_CPXLgetnumcores)
        native_CPXLgetnumcores = load_symbol("CPXLgetnumcores");
    return native_CPXLgetnumcores(env, numcores_p);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnumnz (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumnz)
        native_CPXLgetnumnz = load_symbol("CPXLgetnumnz");
    return native_CPXLgetnumnz(env, lp);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumrows (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumrows)
        native_CPXLgetnumrows = load_symbol("CPXLgetnumrows");
    return native_CPXLgetnumrows(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetobj (CPXCENVptr env, CPXCLPptr lp, double *obj, CPXINT begin, CPXINT end){
    if (!native_CPXLgetobj)
        native_CPXLgetobj = load_symbol("CPXLgetobj");
    return native_CPXLgetobj(env, lp, obj, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetobjname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p){
    if (!native_CPXLgetobjname)
        native_CPXLgetobjname = load_symbol("CPXLgetobjname");
    return native_CPXLgetobjname(env, lp, buf_str, bufspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetobjoffset (CPXCENVptr env, CPXCLPptr lp, double *objoffset_p){
    if (!native_CPXLgetobjoffset)
        native_CPXLgetobjoffset = load_symbol("CPXLgetobjoffset");
    return native_CPXLgetobjoffset(env, lp, objoffset_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetobjsen (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetobjsen)
        native_CPXLgetobjsen = load_symbol("CPXLgetobjsen");
    return native_CPXLgetobjsen(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p){
    if (!native_CPXLgetobjval)
        native_CPXLgetobjval = load_symbol("CPXLgetobjval");
    return native_CPXLgetobjval(env, lp, objval_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetparamname (CPXCENVptr env, int whichparam, char *name_str){
    if (!native_CPXLgetparamname)
        native_CPXLgetparamname = load_symbol("CPXLgetparamname");
    return native_CPXLgetparamname(env, whichparam, name_str);
}
CPXLIBAPI int CPXPUBLIC CPXLgetparamnum (CPXCENVptr env, char const *name_str, int *whichparam_p){
    if (!native_CPXLgetparamnum)
        native_CPXLgetparamnum = load_symbol("CPXLgetparamnum");
    return native_CPXLgetparamnum(env, name_str, whichparam_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetparamtype (CPXCENVptr env, int whichparam, int *paramtype){
    if (!native_CPXLgetparamtype)
        native_CPXLgetparamtype = load_symbol("CPXLgetparamtype");
    return native_CPXLgetparamtype(env, whichparam, paramtype);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetphase1cnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetphase1cnt)
        native_CPXLgetphase1cnt = load_symbol("CPXLgetphase1cnt");
    return native_CPXLgetphase1cnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetpi (CPXCENVptr env, CPXCLPptr lp, double *pi, CPXINT begin, CPXINT end){
    if (!native_CPXLgetpi)
        native_CPXLgetpi = load_symbol("CPXLgetpi");
    return native_CPXLgetpi(env, lp, pi, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetpnorms (CPXCENVptr env, CPXCLPptr lp, double *cnorm, double *rnorm, CPXINT * len_p){
    if (!native_CPXLgetpnorms)
        native_CPXLgetpnorms = load_symbol("CPXLgetpnorms");
    return native_CPXLgetpnorms(env, lp, cnorm, rnorm, len_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetprestat (CPXCENVptr env, CPXCLPptr lp, int *prestat_p, CPXINT * pcstat, CPXINT * prstat, CPXINT * ocstat, CPXINT * orstat){
    if (!native_CPXLgetprestat)
        native_CPXLgetprestat = load_symbol("CPXLgetprestat");
    return native_CPXLgetprestat(env, lp, prestat_p, pcstat, prstat, ocstat, orstat);
}
CPXLIBAPI int CPXPUBLIC CPXLgetprobname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p){
    if (!native_CPXLgetprobname)
        native_CPXLgetprobname = load_symbol("CPXLgetprobname");
    return native_CPXLgetprobname(env, lp, buf_str, bufspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetprobtype (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetprobtype)
        native_CPXLgetprobtype = load_symbol("CPXLgetprobtype");
    return native_CPXLgetprobtype(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetprotected (CPXCENVptr env, CPXCLPptr lp, CPXINT * cnt_p, CPXINT * indices, CPXINT pspace, CPXINT * surplus_p){
    if (!native_CPXLgetprotected)
        native_CPXLgetprotected = load_symbol("CPXLgetprotected");
    return native_CPXLgetprotected(env, lp, cnt_p, indices, pspace, surplus_p);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetpsbcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetpsbcnt)
        native_CPXLgetpsbcnt = load_symbol("CPXLgetpsbcnt");
    return native_CPXLgetpsbcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetray (CPXCENVptr env, CPXCLPptr lp, double *z){
    if (!native_CPXLgetray)
        native_CPXLgetray = load_symbol("CPXLgetray");
    return native_CPXLgetray(env, lp, z);
}
CPXLIBAPI int CPXPUBLIC CPXLgetredlp (CPXCENVptr env, CPXCLPptr lp, CPXCLPptr * redlp_p){
    if (!native_CPXLgetredlp)
        native_CPXLgetredlp = load_symbol("CPXLgetredlp");
    return native_CPXLgetredlp(env, lp, redlp_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetrhs (CPXCENVptr env, CPXCLPptr lp, double *rhs, CPXINT begin, CPXINT end){
    if (!native_CPXLgetrhs)
        native_CPXLgetrhs = load_symbol("CPXLgetrhs");
    return native_CPXLgetrhs(env, lp, rhs, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetrngval (CPXCENVptr env, CPXCLPptr lp, double *rngval, CPXINT begin, CPXINT end){
    if (!native_CPXLgetrngval)
        native_CPXLgetrngval = load_symbol("CPXLgetrngval");
    return native_CPXLgetrngval(env, lp, rngval, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetrowindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p){
    if (!native_CPXLgetrowindex)
        native_CPXLgetrowindex = load_symbol("CPXLgetrowindex");
    return native_CPXLgetrowindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetrowinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end){
    if (!native_CPXLgetrowinfeas)
        native_CPXLgetrowinfeas = load_symbol("CPXLgetrowinfeas");
    return native_CPXLgetrowinfeas(env, lp, x, infeasout, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetrowname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, CPXSIZE storespace, CPXSIZE * surplus_p, CPXINT begin, CPXINT end){
    if (!native_CPXLgetrowname)
        native_CPXLgetrowname = load_symbol("CPXLgetrowname");
    return native_CPXLgetrowname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetrows (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * rmatbeg, CPXINT * rmatind, double *rmatval, CPXLONG rmatspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end){
    if (!native_CPXLgetrows)
        native_CPXLgetrows = load_symbol("CPXLgetrows");
    return native_CPXLgetrows(env, lp, nzcnt_p, rmatbeg, rmatind, rmatval, rmatspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsense (CPXCENVptr env, CPXCLPptr lp, char *sense, CPXINT begin, CPXINT end){
    if (!native_CPXLgetsense)
        native_CPXLgetsense = load_symbol("CPXLgetsense");
    return native_CPXLgetsense(env, lp, sense, begin, end);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetsiftitcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetsiftitcnt)
        native_CPXLgetsiftitcnt = load_symbol("CPXLgetsiftitcnt");
    return native_CPXLgetsiftitcnt(env, lp);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetsiftphase1cnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetsiftphase1cnt)
        native_CPXLgetsiftphase1cnt = load_symbol("CPXLgetsiftphase1cnt");
    return native_CPXLgetsiftphase1cnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetslack (CPXCENVptr env, CPXCLPptr lp, double *slack, CPXINT begin, CPXINT end){
    if (!native_CPXLgetslack)
        native_CPXLgetslack = load_symbol("CPXLgetslack");
    return native_CPXLgetslack(env, lp, slack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpooldblquality (CPXCENVptr env, CPXCLPptr lp, int soln, double *quality_p, int what){
    if (!native_CPXLgetsolnpooldblquality)
        native_CPXLgetsolnpooldblquality = load_symbol("CPXLgetsolnpooldblquality");
    return native_CPXLgetsolnpooldblquality(env, lp, soln, quality_p, what);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolintquality (CPXCENVptr env, CPXCLPptr lp, int soln, CPXINT * quality_p, int what){
    if (!native_CPXLgetsolnpoolintquality)
        native_CPXLgetsolnpoolintquality = load_symbol("CPXLgetsolnpoolintquality");
    return native_CPXLgetsolnpoolintquality(env, lp, soln, quality_p, what);
}
CPXLIBAPI int CPXPUBLIC CPXLgetstat (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetstat)
        native_CPXLgetstat = load_symbol("CPXLgetstat");
    return native_CPXLgetstat(env, lp);
}
CPXLIBAPI CPXCHARptr CPXPUBLIC CPXLgetstatstring (CPXCENVptr env, int statind, char *buffer_str){
    if (!native_CPXLgetstatstring)
        native_CPXLgetstatstring = load_symbol("CPXLgetstatstring");
    return native_CPXLgetstatstring(env, statind, buffer_str);
}
CPXLIBAPI int CPXPUBLIC CPXLgetstrparam (CPXCENVptr env, int whichparam, char *value_str){
    if (!native_CPXLgetstrparam)
        native_CPXLgetstrparam = load_symbol("CPXLgetstrparam");
    return native_CPXLgetstrparam(env, whichparam, value_str);
}
CPXLIBAPI int CPXPUBLIC CPXLgettime (CPXCENVptr env, double *timestamp_p){
    if (!native_CPXLgettime)
        native_CPXLgettime = load_symbol("CPXLgettime");
    return native_CPXLgettime(env, timestamp_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgettuningcallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p){
    if (!native_CPXLgettuningcallbackfunc)
        native_CPXLgettuningcallbackfunc = load_symbol("CPXLgettuningcallbackfunc");
    return native_CPXLgettuningcallbackfunc(env, callback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetub (CPXCENVptr env, CPXCLPptr lp, double *ub, CPXINT begin, CPXINT end){
    if (!native_CPXLgetub)
        native_CPXLgetub = load_symbol("CPXLgetub");
    return native_CPXLgetub(env, lp, ub, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetweight (CPXCENVptr env, CPXCLPptr lp, CPXINT rcnt, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, double *weight, int dpriind){
    if (!native_CPXLgetweight)
        native_CPXLgetweight = load_symbol("CPXLgetweight");
    return native_CPXLgetweight(env, lp, rcnt, rmatbeg, rmatind, rmatval, weight, dpriind);
}
CPXLIBAPI int CPXPUBLIC CPXLgetx (CPXCENVptr env, CPXCLPptr lp, double *x, CPXINT begin, CPXINT end){
    if (!native_CPXLgetx)
        native_CPXLgetx = load_symbol("CPXLgetx");
    return native_CPXLgetx(env, lp, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLhybnetopt (CPXCENVptr env, CPXLPptr lp, int method){
    if (!native_CPXLhybnetopt)
        native_CPXLhybnetopt = load_symbol("CPXLhybnetopt");
    return native_CPXLhybnetopt(env, lp, method);
}
CPXLIBAPI int CPXPUBLIC CPXLinfodblparam (CPXCENVptr env, int whichparam, double *defvalue_p, double *minvalue_p, double *maxvalue_p){
    if (!native_CPXLinfodblparam)
        native_CPXLinfodblparam = load_symbol("CPXLinfodblparam");
    return native_CPXLinfodblparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}
CPXLIBAPI int CPXPUBLIC CPXLinfointparam (CPXCENVptr env, int whichparam, CPXINT * defvalue_p, CPXINT * minvalue_p, CPXINT * maxvalue_p){
    if (!native_CPXLinfointparam)
        native_CPXLinfointparam = load_symbol("CPXLinfointparam");
    return native_CPXLinfointparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}
CPXLIBAPI int CPXPUBLIC CPXLinfolongparam (CPXCENVptr env, int whichparam, CPXLONG * defvalue_p, CPXLONG * minvalue_p, CPXLONG * maxvalue_p){
    if (!native_CPXLinfolongparam)
        native_CPXLinfolongparam = load_symbol("CPXLinfolongparam");
    return native_CPXLinfolongparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}
CPXLIBAPI int CPXPUBLIC CPXLinfostrparam (CPXCENVptr env, int whichparam, char *defvalue_str){
    if (!native_CPXLinfostrparam)
        native_CPXLinfostrparam = load_symbol("CPXLinfostrparam");
    return native_CPXLinfostrparam(env, whichparam, defvalue_str);
}
CPXLIBAPI void CPXPUBLIC CPXLinitialize (void){
    if (!native_CPXLinitialize)
        native_CPXLinitialize = load_symbol("CPXLinitialize");
    return native_CPXLinitialize();
}
CPXLIBAPI int CPXPUBLIC CPXLkilldnorms (CPXLPptr lp){
    if (!native_CPXLkilldnorms)
        native_CPXLkilldnorms = load_symbol("CPXLkilldnorms");
    return native_CPXLkilldnorms(lp);
}
CPXLIBAPI int CPXPUBLIC CPXLkillpnorms (CPXLPptr lp){
    if (!native_CPXLkillpnorms)
        native_CPXLkillpnorms = load_symbol("CPXLkillpnorms");
    return native_CPXLkillpnorms(lp);
}
CPXLIBAPI int CPXPUBLIC CPXLlpopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLlpopt)
        native_CPXLlpopt = load_symbol("CPXLlpopt");
    return native_CPXLlpopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLlprewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLlprewrite)
        native_CPXLlprewrite = load_symbol("CPXLlprewrite");
    return native_CPXLlprewrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLlpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLlpwrite)
        native_CPXLlpwrite = load_symbol("CPXLlpwrite");
    return native_CPXLlpwrite(env, lp, filename_str);
}
CPXLIBAPI CPXVOIDptr CPXPUBLIC CPXLmalloc (size_t size){
    if (!native_CPXLmalloc)
        native_CPXLmalloc = load_symbol("CPXLmalloc");
    return native_CPXLmalloc(size);
}
CPXLIBAPI int CPXPUBLIC CPXLmbasewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLmbasewrite)
        native_CPXLmbasewrite = load_symbol("CPXLmbasewrite");
    return native_CPXLmbasewrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLmdleave (CPXCENVptr env, CPXLPptr lp, CPXINT const *indices, CPXINT cnt, double *downratio, double *upratio){
    if (!native_CPXLmdleave)
        native_CPXLmdleave = load_symbol("CPXLmdleave");
    return native_CPXLmdleave(env, lp, indices, cnt, downratio, upratio);
}
CPXLIBAPI CPXVOIDptr CPXPUBLIC CPXLmemcpy (void *s1, void *s2, size_t n){
    if (!native_CPXLmemcpy)
        native_CPXLmemcpy = load_symbol("CPXLmemcpy");
    return native_CPXLmemcpy(s1, s2, n);
}
CPXLIBAPI int CPXPUBLIC CPXLmpsrewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLmpsrewrite)
        native_CPXLmpsrewrite = load_symbol("CPXLmpsrewrite");
    return native_CPXLmpsrewrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLmpswrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLmpswrite)
        native_CPXLmpswrite = load_symbol("CPXLmpswrite");
    return native_CPXLmpswrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBVARARGS CPXLmsg (CPXCHANNELptr channel, char const *format, ...){
    if (!native_CPXLmsg)
        native_CPXLmsg = load_symbol("CPXLmsg");
    return native_CPXLmsg(channel, format);
}
CPXLIBAPI int CPXPUBLIC CPXLmsgstr (CPXCHANNELptr channel, char const *msg_str){
    if (!native_CPXLmsgstr)
        native_CPXLmsgstr = load_symbol("CPXLmsgstr");
    return native_CPXLmsgstr(channel, msg_str);
}
CPXLIBAPI int CPXPUBLIC CPXLNETextract (CPXCENVptr env, CPXNETptr net, CPXCLPptr lp, CPXINT * colmap, CPXINT * rowmap){
    if (!native_CPXLNETextract)
        native_CPXLNETextract = load_symbol("CPXLNETextract");
    return native_CPXLNETextract(env, net, lp, colmap, rowmap);
}
CPXLIBAPI int CPXPUBLIC CPXLnewcols (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, double const *obj, double const *lb, double const *ub, char const *xctype, char const *const *colname){
    if (!native_CPXLnewcols)
        native_CPXLnewcols = load_symbol("CPXLnewcols");
    return native_CPXLnewcols(env, lp, ccnt, obj, lb, ub, xctype, colname);
}
CPXLIBAPI int CPXPUBLIC CPXLnewrows (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, double const *rhs, char const *sense, double const *rngval, char const *const *rowname){
    if (!native_CPXLnewrows)
        native_CPXLnewrows = load_symbol("CPXLnewrows");
    return native_CPXLnewrows(env, lp, rcnt, rhs, sense, rngval, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXLobjsa (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end, double *lower, double *upper){
    if (!native_CPXLobjsa)
        native_CPXLobjsa = load_symbol("CPXLobjsa");
    return native_CPXLobjsa(env, lp, begin, end, lower, upper);
}
CPXLIBAPI CPXENVptr CPXPUBLIC CPXLopenCPLEX (int *status_p){
    if (!native_CPXLopenCPLEX)
        native_CPXLopenCPLEX = load_symbol("CPXLopenCPLEX");
    return native_CPXLopenCPLEX(status_p);
}
CPXLIBAPI CPXENVptr CPXPUBLIC CPXLopenCPLEXruntime (int *status_p, int serialnum, char const *licenvstring_str){
    if (!native_CPXLopenCPLEXruntime)
        native_CPXLopenCPLEXruntime = load_symbol("CPXLopenCPLEXruntime");
    return native_CPXLopenCPLEXruntime(status_p, serialnum, licenvstring_str);
}
CPXLIBAPI CPXENVptr CPXPUBLIC CPXLparenv (CPXENVptr env, int *status_p){
    if (!native_CPXLparenv)
        native_CPXLparenv = load_symbol("CPXLparenv");
    return native_CPXLparenv(env, status_p);
}
CPXLIBAPI int CPXPUBLIC CPXLpivot (CPXCENVptr env, CPXLPptr lp, CPXINT jenter, CPXINT jleave, int leavestat){
    if (!native_CPXLpivot)
        native_CPXLpivot = load_symbol("CPXLpivot");
    return native_CPXLpivot(env, lp, jenter, jleave, leavestat);
}
CPXLIBAPI int CPXPUBLIC CPXLpivotin (CPXCENVptr env, CPXLPptr lp, CPXINT const *rlist, CPXINT rlen){
    if (!native_CPXLpivotin)
        native_CPXLpivotin = load_symbol("CPXLpivotin");
    return native_CPXLpivotin(env, lp, rlist, rlen);
}
CPXLIBAPI int CPXPUBLIC CPXLpivotout (CPXCENVptr env, CPXLPptr lp, CPXINT const *clist, CPXINT clen){
    if (!native_CPXLpivotout)
        native_CPXLpivotout = load_symbol("CPXLpivotout");
    return native_CPXLpivotout(env, lp, clist, clen);
}
CPXLIBAPI int CPXPUBLIC CPXLpperwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon){
    if (!native_CPXLpperwrite)
        native_CPXLpperwrite = load_symbol("CPXLpperwrite");
    return native_CPXLpperwrite(env, lp, filename_str, epsilon);
}
CPXLIBAPI int CPXPUBLIC CPXLpratio (CPXCENVptr env, CPXLPptr lp, CPXINT * indices, CPXINT cnt, double *downratio, double *upratio, CPXINT * downleave, CPXINT * upleave, int *downleavestatus, int *upleavestatus, int *downstatus, int *upstatus){
    if (!native_CPXLpratio)
        native_CPXLpratio = load_symbol("CPXLpratio");
    return native_CPXLpratio(env, lp, indices, cnt, downratio, upratio, downleave, upleave, downleavestatus, upleavestatus, downstatus, upstatus);
}
CPXLIBAPI int CPXPUBLIC CPXLpreaddrows (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *rowname){
    if (!native_CPXLpreaddrows)
        native_CPXLpreaddrows = load_symbol("CPXLpreaddrows");
    return native_CPXLpreaddrows(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXLprechgobj (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values){
    if (!native_CPXLprechgobj)
        native_CPXLprechgobj = load_symbol("CPXLprechgobj");
    return native_CPXLprechgobj(env, lp, cnt, indices, values);
}
CPXLIBAPI int CPXPUBLIC CPXLpreslvwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double *objoff_p){
    if (!native_CPXLpreslvwrite)
        native_CPXLpreslvwrite = load_symbol("CPXLpreslvwrite");
    return native_CPXLpreslvwrite(env, lp, filename_str, objoff_p);
}
CPXLIBAPI int CPXPUBLIC CPXLpresolve (CPXCENVptr env, CPXLPptr lp, int method){
    if (!native_CPXLpresolve)
        native_CPXLpresolve = load_symbol("CPXLpresolve");
    return native_CPXLpresolve(env, lp, method);
}
CPXLIBAPI int CPXPUBLIC CPXLprimopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLprimopt)
        native_CPXLprimopt = load_symbol("CPXLprimopt");
    return native_CPXLprimopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLputenv (char *envsetting_str){
    if (!native_CPXLputenv)
        native_CPXLputenv = load_symbol("CPXLputenv");
    return native_CPXLputenv(envsetting_str);
}
CPXLIBAPI int CPXPUBLIC CPXLqpdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double const *x, double *dj){
    if (!native_CPXLqpdjfrompi)
        native_CPXLqpdjfrompi = load_symbol("CPXLqpdjfrompi");
    return native_CPXLqpdjfrompi(env, lp, pi, x, dj);
}
CPXLIBAPI int CPXPUBLIC CPXLqpuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi, double const *x){
    if (!native_CPXLqpuncrushpi)
        native_CPXLqpuncrushpi = load_symbol("CPXLqpuncrushpi");
    return native_CPXLqpuncrushpi(env, lp, pi, prepi, x);
}
CPXLIBAPI int CPXPUBLIC CPXLreadcopybase (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXLreadcopybase)
        native_CPXLreadcopybase = load_symbol("CPXLreadcopybase");
    return native_CPXLreadcopybase(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLreadcopyparam (CPXENVptr env, char const *filename_str){
    if (!native_CPXLreadcopyparam)
        native_CPXLreadcopyparam = load_symbol("CPXLreadcopyparam");
    return native_CPXLreadcopyparam(env, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLreadcopyprob (CPXCENVptr env, CPXLPptr lp, char const *filename_str, char const *filetype_str){
    if (!native_CPXLreadcopyprob)
        native_CPXLreadcopyprob = load_symbol("CPXLreadcopyprob");
    return native_CPXLreadcopyprob(env, lp, filename_str, filetype_str);
}
CPXLIBAPI int CPXPUBLIC CPXLreadcopysol (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXLreadcopysol)
        native_CPXLreadcopysol = load_symbol("CPXLreadcopysol");
    return native_CPXLreadcopysol(env, lp, filename_str);
}
CPXLIBAPI CPXVOIDptr CPXPUBLIC CPXLrealloc (void *ptr, size_t size){
    if (!native_CPXLrealloc)
        native_CPXLrealloc = load_symbol("CPXLrealloc");
    return native_CPXLrealloc(ptr, size);
}
CPXLIBAPI int CPXPUBLIC CPXLrefineconflict (CPXCENVptr env, CPXLPptr lp, CPXINT * confnumrows_p, CPXINT * confnumcols_p){
    if (!native_CPXLrefineconflict)
        native_CPXLrefineconflict = load_symbol("CPXLrefineconflict");
    return native_CPXLrefineconflict(env, lp, confnumrows_p, confnumcols_p);
}
CPXLIBAPI int CPXPUBLIC CPXLrefineconflictext (CPXCENVptr env, CPXLPptr lp, CPXLONG grpcnt, CPXLONG concnt, double const *grppref, CPXLONG const *grpbeg, CPXINT const *grpind, char const *grptype){
    if (!native_CPXLrefineconflictext)
        native_CPXLrefineconflictext = load_symbol("CPXLrefineconflictext");
    return native_CPXLrefineconflictext(env, lp, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}
CPXLIBAPI int CPXPUBLIC CPXLrhssa (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end, double *lower, double *upper){
    if (!native_CPXLrhssa)
        native_CPXLrhssa = load_symbol("CPXLrhssa");
    return native_CPXLrhssa(env, lp, begin, end, lower, upper);
}
CPXLIBAPI int CPXPUBLIC CPXLrobustopt (CPXCENVptr env, CPXLPptr lp, CPXLPptr lblp, CPXLPptr ublp, double objchg, double const *maxchg){
    if (!native_CPXLrobustopt)
        native_CPXLrobustopt = load_symbol("CPXLrobustopt");
    return native_CPXLrobustopt(env, lp, lblp, ublp, objchg, maxchg);
}
CPXLIBAPI int CPXPUBLIC CPXLsavwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLsavwrite)
        native_CPXLsavwrite = load_symbol("CPXLsavwrite");
    return native_CPXLsavwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLserializercreate (CPXSERIALIZERptr * ser_p){
    if (!native_CPXLserializercreate)
        native_CPXLserializercreate = load_symbol("CPXLserializercreate");
    return native_CPXLserializercreate(ser_p);
}
CPXLIBAPI void CPXPUBLIC CPXLserializerdestroy (CPXSERIALIZERptr ser){
    if (!native_CPXLserializerdestroy)
        native_CPXLserializerdestroy = load_symbol("CPXLserializerdestroy");
    return native_CPXLserializerdestroy(ser);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLserializerlength (CPXCSERIALIZERptr ser){
    if (!native_CPXLserializerlength)
        native_CPXLserializerlength = load_symbol("CPXLserializerlength");
    return native_CPXLserializerlength(ser);
}
CPXLIBAPI void const *CPXPUBLIC CPXLserializerpayload (CPXCSERIALIZERptr ser){
    if (!native_CPXLserializerpayload)
        native_CPXLserializerpayload = load_symbol("CPXLserializerpayload");
    return native_CPXLserializerpayload(ser);
}
CPXLIBAPI int CPXPUBLIC CPXLsetdblparam (CPXENVptr env, int whichparam, double newvalue){
    if (!native_CPXLsetdblparam)
        native_CPXLsetdblparam = load_symbol("CPXLsetdblparam");
    return native_CPXLsetdblparam(env, whichparam, newvalue);
}
CPXLIBAPI int CPXPUBLIC CPXLsetdefaults (CPXENVptr env){
    if (!native_CPXLsetdefaults)
        native_CPXLsetdefaults = load_symbol("CPXLsetdefaults");
    return native_CPXLsetdefaults(env);
}
CPXLIBAPI int CPXPUBLIC CPXLsetintparam (CPXENVptr env, int whichparam, CPXINT newvalue){
    if (!native_CPXLsetintparam)
        native_CPXLsetintparam = load_symbol("CPXLsetintparam");
    return native_CPXLsetintparam(env, whichparam, newvalue);
}
CPXLIBAPI int CPXPUBLIC CPXLsetlogfile (CPXENVptr env, CPXFILEptr lfile){
    if (!native_CPXLsetlogfile)
        native_CPXLsetlogfile = load_symbol("CPXLsetlogfile");
    return native_CPXLsetlogfile(env, lfile);
}
CPXLIBAPI int CPXPUBLIC CPXLsetlongparam (CPXENVptr env, int whichparam, CPXLONG newvalue){
    if (!native_CPXLsetlongparam)
        native_CPXLsetlongparam = load_symbol("CPXLsetlongparam");
    return native_CPXLsetlongparam(env, whichparam, newvalue);
}
CPXLIBAPI int CPXPUBLIC CPXLsetlpcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle){
    if (!native_CPXLsetlpcallbackfunc)
        native_CPXLsetlpcallbackfunc = load_symbol("CPXLsetlpcallbackfunc");
    return native_CPXLsetlpcallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetnetcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle){
    if (!native_CPXLsetnetcallbackfunc)
        native_CPXLsetnetcallbackfunc = load_symbol("CPXLsetnetcallbackfunc");
    return native_CPXLsetnetcallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetphase2 (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLsetphase2)
        native_CPXLsetphase2 = load_symbol("CPXLsetphase2");
    return native_CPXLsetphase2(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLsetprofcallbackfunc (CPXENVptr env, CPXL_CALLBACK_PROF * callback, void *cbhandle){
    if (!native_CPXLsetprofcallbackfunc)
        native_CPXLsetprofcallbackfunc = load_symbol("CPXLsetprofcallbackfunc");
    return native_CPXLsetprofcallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetstrparam (CPXENVptr env, int whichparam, char const *newvalue_str){
    if (!native_CPXLsetstrparam)
        native_CPXLsetstrparam = load_symbol("CPXLsetstrparam");
    return native_CPXLsetstrparam(env, whichparam, newvalue_str);
}
CPXLIBAPI int CPXPUBLIC CPXLsetterminate (CPXENVptr env, volatile int *terminate_p){
    if (!native_CPXLsetterminate)
        native_CPXLsetterminate = load_symbol("CPXLsetterminate");
    return native_CPXLsetterminate(env, terminate_p);
}
CPXLIBAPI int CPXPUBLIC CPXLsettuningcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle){
    if (!native_CPXLsettuningcallbackfunc)
        native_CPXLsettuningcallbackfunc = load_symbol("CPXLsettuningcallbackfunc");
    return native_CPXLsettuningcallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsiftopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLsiftopt)
        native_CPXLsiftopt = load_symbol("CPXLsiftopt");
    return native_CPXLsiftopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *slack){
    if (!native_CPXLslackfromx)
        native_CPXLslackfromx = load_symbol("CPXLslackfromx");
    return native_CPXLslackfromx(env, lp, x, slack);
}
CPXLIBAPI int CPXPUBLIC CPXLsolninfo (CPXCENVptr env, CPXCLPptr lp, int *solnmethod_p, int *solntype_p, int *pfeasind_p, int *dfeasind_p){
    if (!native_CPXLsolninfo)
        native_CPXLsolninfo = load_symbol("CPXLsolninfo");
    return native_CPXLsolninfo(env, lp, solnmethod_p, solntype_p, pfeasind_p, dfeasind_p);
}
CPXLIBAPI int CPXPUBLIC CPXLsolution (CPXCENVptr env, CPXCLPptr lp, int *lpstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj){
    if (!native_CPXLsolution)
        native_CPXLsolution = load_symbol("CPXLsolution");
    return native_CPXLsolution(env, lp, lpstat_p, objval_p, x, pi, slack, dj);
}
CPXLIBAPI int CPXPUBLIC CPXLsolwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLsolwrite)
        native_CPXLsolwrite = load_symbol("CPXLsolwrite");
    return native_CPXLsolwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLsolwritesolnpool (CPXCENVptr env, CPXCLPptr lp, int soln, char const *filename_str){
    if (!native_CPXLsolwritesolnpool)
        native_CPXLsolwritesolnpool = load_symbol("CPXLsolwritesolnpool");
    return native_CPXLsolwritesolnpool(env, lp, soln, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLsolwritesolnpoolall (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLsolwritesolnpoolall)
        native_CPXLsolwritesolnpoolall = load_symbol("CPXLsolwritesolnpoolall");
    return native_CPXLsolwritesolnpoolall(env, lp, filename_str);
}
CPXLIBAPI CPXCHARptr CPXPUBLIC CPXLstrcpy (char *dest_str, char const *src_str){
    if (!native_CPXLstrcpy)
        native_CPXLstrcpy = load_symbol("CPXLstrcpy");
    return native_CPXLstrcpy(dest_str, src_str);
}
CPXLIBAPI size_t CPXPUBLIC CPXLstrlen (char const *s_str){
    if (!native_CPXLstrlen)
        native_CPXLstrlen = load_symbol("CPXLstrlen");
    return native_CPXLstrlen(s_str);
}
CPXLIBAPI int CPXPUBLIC CPXLstrongbranch (CPXCENVptr env, CPXLPptr lp, CPXINT const *indices, CPXINT cnt, double *downobj, double *upobj, CPXLONG itlim){
    if (!native_CPXLstrongbranch)
        native_CPXLstrongbranch = load_symbol("CPXLstrongbranch");
    return native_CPXLstrongbranch(env, lp, indices, cnt, downobj, upobj, itlim);
}
CPXLIBAPI int CPXPUBLIC CPXLtightenbds (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd){
    if (!native_CPXLtightenbds)
        native_CPXLtightenbds = load_symbol("CPXLtightenbds");
    return native_CPXLtightenbds(env, lp, cnt, indices, lu, bd);
}
CPXLIBAPI int CPXPUBLIC CPXLtuneparam (CPXENVptr env, CPXLPptr lp, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char const *const *strval, int *tunestat_p){
    if (!native_CPXLtuneparam)
        native_CPXLtuneparam = load_symbol("CPXLtuneparam");
    return native_CPXLtuneparam(env, lp, intcnt, intnum, intval, dblcnt, dblnum, dblval, strcnt, strnum, strval, tunestat_p);
}
CPXLIBAPI int CPXPUBLIC CPXLtuneparamprobset (CPXENVptr env, int filecnt, char const *const *filename, char const *const *filetype, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char const *const *strval, int *tunestat_p){
    if (!native_CPXLtuneparamprobset)
        native_CPXLtuneparamprobset = load_symbol("CPXLtuneparamprobset");
    return native_CPXLtuneparamprobset(env, filecnt, filename, filetype, intcnt, intnum, intval, dblcnt, dblnum, dblval, strcnt, strnum, strval, tunestat_p);
}
CPXLIBAPI int CPXPUBLIC CPXLuncrushform (CPXCENVptr env, CPXCLPptr lp, CPXINT plen, CPXINT const *pind, double const *pval, CPXINT * len_p, double *offset_p, CPXINT * ind, double *val){
    if (!native_CPXLuncrushform)
        native_CPXLuncrushform = load_symbol("CPXLuncrushform");
    return native_CPXLuncrushform(env, lp, plen, pind, pval, len_p, offset_p, ind, val);
}
CPXLIBAPI int CPXPUBLIC CPXLuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi){
    if (!native_CPXLuncrushpi)
        native_CPXLuncrushpi = load_symbol("CPXLuncrushpi");
    return native_CPXLuncrushpi(env, lp, pi, prepi);
}
CPXLIBAPI int CPXPUBLIC CPXLuncrushx (CPXCENVptr env, CPXCLPptr lp, double *x, double const *prex){
    if (!native_CPXLuncrushx)
        native_CPXLuncrushx = load_symbol("CPXLuncrushx");
    return native_CPXLuncrushx(env, lp, x, prex);
}
CPXLIBAPI int CPXPUBLIC CPXLunscaleprob (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLunscaleprob)
        native_CPXLunscaleprob = load_symbol("CPXLunscaleprob");
    return native_CPXLunscaleprob(env, lp);
}
CPXLIBAPI CPXCCHARptr CPXPUBLIC CPXLversion (CPXCENVptr env){
    if (!native_CPXLversion)
        native_CPXLversion = load_symbol("CPXLversion");
    return native_CPXLversion(env);
}
CPXLIBAPI int CPXPUBLIC CPXLversionnumber (CPXCENVptr env, int *version_p){
    if (!native_CPXLversionnumber)
        native_CPXLversionnumber = load_symbol("CPXLversionnumber");
    return native_CPXLversionnumber(env, version_p);
}
CPXLIBAPI int CPXPUBLIC CPXLwriteparam (CPXCENVptr env, char const *filename_str){
    if (!native_CPXLwriteparam)
        native_CPXLwriteparam = load_symbol("CPXLwriteparam");
    return native_CPXLwriteparam(env, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLwriteprob (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, char const *filetype_str){
    if (!native_CPXLwriteprob)
        native_CPXLwriteprob = load_symbol("CPXLwriteprob");
    return native_CPXLwriteprob(env, lp, filename_str, filetype_str);
}
CPXLIBAPI int CPXPUBLIC CPXLbaropt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLbaropt)
        native_CPXLbaropt = load_symbol("CPXLbaropt");
    return native_CPXLbaropt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLhybbaropt (CPXCENVptr env, CPXLPptr lp, int method){
    if (!native_CPXLhybbaropt)
        native_CPXLhybbaropt = load_symbol("CPXLhybbaropt");
    return native_CPXLhybbaropt(env, lp, method);
}
CPXLIBAPI int CPXPUBLIC CPXLaddindconstr (CPXCENVptr env, CPXLPptr lp, CPXINT indvar, int complemented, CPXINT nzcnt, double rhs, int sense, CPXINT const *linind, double const *linval, char const *indname_str){
    if (!native_CPXLaddindconstr)
        native_CPXLaddindconstr = load_symbol("CPXLaddindconstr");
    return native_CPXLaddindconstr(env, lp, indvar, complemented, nzcnt, rhs, sense, linind, linval, indname_str);
}
CPXLIBAPI int CPXPUBLIC CPXLchgqpcoef (CPXCENVptr env, CPXLPptr lp, CPXINT i, CPXINT j, double newvalue){
    if (!native_CPXLchgqpcoef)
        native_CPXLchgqpcoef = load_symbol("CPXLchgqpcoef");
    return native_CPXLchgqpcoef(env, lp, i, j, newvalue);
}
CPXLIBAPI int CPXPUBLIC CPXLcopyqpsep (CPXCENVptr env, CPXLPptr lp, double const *qsepvec){
    if (!native_CPXLcopyqpsep)
        native_CPXLcopyqpsep = load_symbol("CPXLcopyqpsep");
    return native_CPXLcopyqpsep(env, lp, qsepvec);
}
CPXLIBAPI int CPXPUBLIC CPXLcopyquad (CPXCENVptr env, CPXLPptr lp, CPXLONG const *qmatbeg, CPXINT const *qmatcnt, CPXINT const *qmatind, double const *qmatval){
    if (!native_CPXLcopyquad)
        native_CPXLcopyquad = load_symbol("CPXLcopyquad");
    return native_CPXLcopyquad(env, lp, qmatbeg, qmatcnt, qmatind, qmatval);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnumqpnz (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumqpnz)
        native_CPXLgetnumqpnz = load_symbol("CPXLgetnumqpnz");
    return native_CPXLgetnumqpnz(env, lp);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumquad (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumquad)
        native_CPXLgetnumquad = load_symbol("CPXLgetnumquad");
    return native_CPXLgetnumquad(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetqpcoef (CPXCENVptr env, CPXCLPptr lp, CPXINT rownum, CPXINT colnum, double *coef_p){
    if (!native_CPXLgetqpcoef)
        native_CPXLgetqpcoef = load_symbol("CPXLgetqpcoef");
    return native_CPXLgetqpcoef(env, lp, rownum, colnum, coef_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetquad (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * qmatbeg, CPXINT * qmatind, double *qmatval, CPXLONG qmatspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end){
    if (!native_CPXLgetquad)
        native_CPXLgetquad = load_symbol("CPXLgetquad");
    return native_CPXLgetquad(env, lp, nzcnt_p, qmatbeg, qmatind, qmatval, qmatspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLqpindefcertificate (CPXCENVptr env, CPXCLPptr lp, double *x){
    if (!native_CPXLqpindefcertificate)
        native_CPXLqpindefcertificate = load_symbol("CPXLqpindefcertificate");
    return native_CPXLqpindefcertificate(env, lp, x);
}
CPXLIBAPI int CPXPUBLIC CPXLqpopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLqpopt)
        native_CPXLqpopt = load_symbol("CPXLqpopt");
    return native_CPXLqpopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLaddqconstr (CPXCENVptr env, CPXLPptr lp, CPXINT linnzcnt, CPXLONG quadnzcnt, double rhs, int sense, CPXINT const *linind, double const *linval, CPXINT const *quadrow, CPXINT const *quadcol, double const *quadval, char const *lname_str){
    if (!native_CPXLaddqconstr)
        native_CPXLaddqconstr = load_symbol("CPXLaddqconstr");
    return native_CPXLaddqconstr(env, lp, linnzcnt, quadnzcnt, rhs, sense, linind, linval, quadrow, quadcol, quadval, lname_str);
}
CPXLIBAPI int CPXPUBLIC CPXLdelqconstrs (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end){
    if (!native_CPXLdelqconstrs)
        native_CPXLdelqconstrs = load_symbol("CPXLdelqconstrs");
    return native_CPXLdelqconstrs(env, lp, begin, end);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumqconstrs (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumqconstrs)
        native_CPXLgetnumqconstrs = load_symbol("CPXLgetnumqconstrs");
    return native_CPXLgetnumqconstrs(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetqconstr (CPXCENVptr env, CPXCLPptr lp, CPXINT * linnzcnt_p, CPXLONG * quadnzcnt_p, double *rhs_p, char *sense_p, CPXINT * linind, double *linval, CPXINT linspace, CPXINT * linsurplus_p, CPXINT * quadrow, CPXINT * quadcol, double *quadval, CPXLONG quadspace, CPXLONG * quadsurplus_p, CPXINT which){
    if (!native_CPXLgetqconstr)
        native_CPXLgetqconstr = load_symbol("CPXLgetqconstr");
    return native_CPXLgetqconstr(env, lp, linnzcnt_p, quadnzcnt_p, rhs_p, sense_p, linind, linval, linspace, linsurplus_p, quadrow, quadcol, quadval, quadspace, quadsurplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXLgetqconstrdslack (CPXCENVptr env, CPXCLPptr lp, CPXINT qind, CPXINT * nz_p, CPXINT * ind, double *val, CPXINT space, CPXINT * surplus_p){
    if (!native_CPXLgetqconstrdslack)
        native_CPXLgetqconstrdslack = load_symbol("CPXLgetqconstrdslack");
    return native_CPXLgetqconstrdslack(env, lp, qind, nz_p, ind, val, space, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetqconstrindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p){
    if (!native_CPXLgetqconstrindex)
        native_CPXLgetqconstrindex = load_symbol("CPXLgetqconstrindex");
    return native_CPXLgetqconstrindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetqconstrinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end){
    if (!native_CPXLgetqconstrinfeas)
        native_CPXLgetqconstrinfeas = load_symbol("CPXLgetqconstrinfeas");
    return native_CPXLgetqconstrinfeas(env, lp, x, infeasout, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetqconstrname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p, CPXINT which){
    if (!native_CPXLgetqconstrname)
        native_CPXLgetqconstrname = load_symbol("CPXLgetqconstrname");
    return native_CPXLgetqconstrname(env, lp, buf_str, bufspace, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXLgetqconstrslack (CPXCENVptr env, CPXCLPptr lp, double *qcslack, CPXINT begin, CPXINT end){
    if (!native_CPXLgetqconstrslack)
        native_CPXLgetqconstrslack = load_symbol("CPXLgetqconstrslack");
    return native_CPXLgetqconstrslack(env, lp, qcslack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetxqxax (CPXCENVptr env, CPXCLPptr lp, double *xqxax, CPXINT begin, CPXINT end){
    if (!native_CPXLgetxqxax)
        native_CPXLgetxqxax = load_symbol("CPXLgetxqxax");
    return native_CPXLgetxqxax(env, lp, xqxax, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLqconstrslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *qcslack){
    if (!native_CPXLqconstrslackfromx)
        native_CPXLqconstrslackfromx = load_symbol("CPXLqconstrslackfromx");
    return native_CPXLqconstrslackfromx(env, lp, x, qcslack);
}
CPXLIBAPI int CPXPUBLIC CPXLNETaddarcs (CPXCENVptr env, CPXNETptr net, CPXINT narcs, CPXINT const *fromnode, CPXINT const *tonode, double const *low, double const *up, double const *obj, char const *const *anames){
    if (!native_CPXLNETaddarcs)
        native_CPXLNETaddarcs = load_symbol("CPXLNETaddarcs");
    return native_CPXLNETaddarcs(env, net, narcs, fromnode, tonode, low, up, obj, anames);
}
CPXLIBAPI int CPXPUBLIC CPXLNETaddnodes (CPXCENVptr env, CPXNETptr net, CPXINT nnodes, double const *supply, char const *const *name){
    if (!native_CPXLNETaddnodes)
        native_CPXLNETaddnodes = load_symbol("CPXLNETaddnodes");
    return native_CPXLNETaddnodes(env, net, nnodes, supply, name);
}
CPXLIBAPI int CPXPUBLIC CPXLNETbasewrite (CPXCENVptr env, CPXCNETptr net, char const *filename_str){
    if (!native_CPXLNETbasewrite)
        native_CPXLNETbasewrite = load_symbol("CPXLNETbasewrite");
    return native_CPXLNETbasewrite(env, net, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLNETchgarcname (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, char const *const *newname){
    if (!native_CPXLNETchgarcname)
        native_CPXLNETchgarcname = load_symbol("CPXLNETchgarcname");
    return native_CPXLNETchgarcname(env, net, cnt, indices, newname);
}
CPXLIBAPI int CPXPUBLIC CPXLNETchgarcnodes (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, CPXINT const *fromnode, CPXINT const *tonode){
    if (!native_CPXLNETchgarcnodes)
        native_CPXLNETchgarcnodes = load_symbol("CPXLNETchgarcnodes");
    return native_CPXLNETchgarcnodes(env, net, cnt, indices, fromnode, tonode);
}
CPXLIBAPI int CPXPUBLIC CPXLNETchgbds (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd){
    if (!native_CPXLNETchgbds)
        native_CPXLNETchgbds = load_symbol("CPXLNETchgbds");
    return native_CPXLNETchgbds(env, net, cnt, indices, lu, bd);
}
CPXLIBAPI int CPXPUBLIC CPXLNETchgname (CPXCENVptr env, CPXNETptr net, int key, CPXINT vindex, char const *name_str){
    if (!native_CPXLNETchgname)
        native_CPXLNETchgname = load_symbol("CPXLNETchgname");
    return native_CPXLNETchgname(env, net, key, vindex, name_str);
}
CPXLIBAPI int CPXPUBLIC CPXLNETchgnodename (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, char const *const *newname){
    if (!native_CPXLNETchgnodename)
        native_CPXLNETchgnodename = load_symbol("CPXLNETchgnodename");
    return native_CPXLNETchgnodename(env, net, cnt, indices, newname);
}
CPXLIBAPI int CPXPUBLIC CPXLNETchgobj (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, double const *obj){
    if (!native_CPXLNETchgobj)
        native_CPXLNETchgobj = load_symbol("CPXLNETchgobj");
    return native_CPXLNETchgobj(env, net, cnt, indices, obj);
}
CPXLIBAPI int CPXPUBLIC CPXLNETchgobjsen (CPXCENVptr env, CPXNETptr net, int maxormin){
    if (!native_CPXLNETchgobjsen)
        native_CPXLNETchgobjsen = load_symbol("CPXLNETchgobjsen");
    return native_CPXLNETchgobjsen(env, net, maxormin);
}
CPXLIBAPI int CPXPUBLIC CPXLNETchgsupply (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, double const *supply){
    if (!native_CPXLNETchgsupply)
        native_CPXLNETchgsupply = load_symbol("CPXLNETchgsupply");
    return native_CPXLNETchgsupply(env, net, cnt, indices, supply);
}
CPXLIBAPI int CPXPUBLIC CPXLNETcopybase (CPXCENVptr env, CPXNETptr net, int const *astat, int const *nstat){
    if (!native_CPXLNETcopybase)
        native_CPXLNETcopybase = load_symbol("CPXLNETcopybase");
    return native_CPXLNETcopybase(env, net, astat, nstat);
}
CPXLIBAPI int CPXPUBLIC CPXLNETcopynet (CPXCENVptr env, CPXNETptr net, int objsen, CPXINT nnodes, double const *supply, char const *const *nnames, CPXINT narcs, CPXINT const *fromnode, CPXINT const *tonode, double const *low, double const *up, double const *obj, char const *const *anames){
    if (!native_CPXLNETcopynet)
        native_CPXLNETcopynet = load_symbol("CPXLNETcopynet");
    return native_CPXLNETcopynet(env, net, objsen, nnodes, supply, nnames, narcs, fromnode, tonode, low, up, obj, anames);
}
CPXLIBAPI CPXNETptr CPXPUBLIC CPXLNETcreateprob (CPXENVptr env, int *status_p, char const *name_str){
    if (!native_CPXLNETcreateprob)
        native_CPXLNETcreateprob = load_symbol("CPXLNETcreateprob");
    return native_CPXLNETcreateprob(env, status_p, name_str);
}
CPXLIBAPI int CPXPUBLIC CPXLNETdelarcs (CPXCENVptr env, CPXNETptr net, CPXINT begin, CPXINT end){
    if (!native_CPXLNETdelarcs)
        native_CPXLNETdelarcs = load_symbol("CPXLNETdelarcs");
    return native_CPXLNETdelarcs(env, net, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETdelnodes (CPXCENVptr env, CPXNETptr net, CPXINT begin, CPXINT end){
    if (!native_CPXLNETdelnodes)
        native_CPXLNETdelnodes = load_symbol("CPXLNETdelnodes");
    return native_CPXLNETdelnodes(env, net, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETdelset (CPXCENVptr env, CPXNETptr net, CPXINT * whichnodes, CPXINT * whicharcs){
    if (!native_CPXLNETdelset)
        native_CPXLNETdelset = load_symbol("CPXLNETdelset");
    return native_CPXLNETdelset(env, net, whichnodes, whicharcs);
}
CPXLIBAPI int CPXPUBLIC CPXLNETfreeprob (CPXENVptr env, CPXNETptr * net_p){
    if (!native_CPXLNETfreeprob)
        native_CPXLNETfreeprob = load_symbol("CPXLNETfreeprob");
    return native_CPXLNETfreeprob(env, net_p);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetarcindex (CPXCENVptr env, CPXCNETptr net, char const *lname_str, CPXINT * index_p){
    if (!native_CPXLNETgetarcindex)
        native_CPXLNETgetarcindex = load_symbol("CPXLNETgetarcindex");
    return native_CPXLNETgetarcindex(env, net, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetarcname (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, CPXSIZE namespc, CPXSIZE * surplus_p, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetarcname)
        native_CPXLNETgetarcname = load_symbol("CPXLNETgetarcname");
    return native_CPXLNETgetarcname(env, net, nnames, namestore, namespc, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetarcnodes (CPXCENVptr env, CPXCNETptr net, CPXINT * fromnode, CPXINT * tonode, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetarcnodes)
        native_CPXLNETgetarcnodes = load_symbol("CPXLNETgetarcnodes");
    return native_CPXLNETgetarcnodes(env, net, fromnode, tonode, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetbase (CPXCENVptr env, CPXCNETptr net, int *astat, int *nstat){
    if (!native_CPXLNETgetbase)
        native_CPXLNETgetbase = load_symbol("CPXLNETgetbase");
    return native_CPXLNETgetbase(env, net, astat, nstat);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetdj (CPXCENVptr env, CPXCNETptr net, double *dj, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetdj)
        native_CPXLNETgetdj = load_symbol("CPXLNETgetdj");
    return native_CPXLNETgetdj(env, net, dj, begin, end);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLNETgetitcnt (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXLNETgetitcnt)
        native_CPXLNETgetitcnt = load_symbol("CPXLNETgetitcnt");
    return native_CPXLNETgetitcnt(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetlb (CPXCENVptr env, CPXCNETptr net, double *low, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetlb)
        native_CPXLNETgetlb = load_symbol("CPXLNETgetlb");
    return native_CPXLNETgetlb(env, net, low, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetnodearcs (CPXCENVptr env, CPXCNETptr net, CPXINT * arccnt_p, CPXINT * arcbeg, CPXINT * arc, CPXINT arcspace, CPXINT * surplus_p, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetnodearcs)
        native_CPXLNETgetnodearcs = load_symbol("CPXLNETgetnodearcs");
    return native_CPXLNETgetnodearcs(env, net, arccnt_p, arcbeg, arc, arcspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetnodeindex (CPXCENVptr env, CPXCNETptr net, char const *lname_str, CPXINT * index_p){
    if (!native_CPXLNETgetnodeindex)
        native_CPXLNETgetnodeindex = load_symbol("CPXLNETgetnodeindex");
    return native_CPXLNETgetnodeindex(env, net, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetnodename (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, CPXSIZE namespc, CPXSIZE * surplus_p, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetnodename)
        native_CPXLNETgetnodename = load_symbol("CPXLNETgetnodename");
    return native_CPXLNETgetnodename(env, net, nnames, namestore, namespc, surplus_p, begin, end);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLNETgetnumarcs (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXLNETgetnumarcs)
        native_CPXLNETgetnumarcs = load_symbol("CPXLNETgetnumarcs");
    return native_CPXLNETgetnumarcs(env, net);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLNETgetnumnodes (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXLNETgetnumnodes)
        native_CPXLNETgetnumnodes = load_symbol("CPXLNETgetnumnodes");
    return native_CPXLNETgetnumnodes(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetobj (CPXCENVptr env, CPXCNETptr net, double *obj, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetobj)
        native_CPXLNETgetobj = load_symbol("CPXLNETgetobj");
    return native_CPXLNETgetobj(env, net, obj, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetobjsen (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXLNETgetobjsen)
        native_CPXLNETgetobjsen = load_symbol("CPXLNETgetobjsen");
    return native_CPXLNETgetobjsen(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetobjval (CPXCENVptr env, CPXCNETptr net, double *objval_p){
    if (!native_CPXLNETgetobjval)
        native_CPXLNETgetobjval = load_symbol("CPXLNETgetobjval");
    return native_CPXLNETgetobjval(env, net, objval_p);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLNETgetphase1cnt (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXLNETgetphase1cnt)
        native_CPXLNETgetphase1cnt = load_symbol("CPXLNETgetphase1cnt");
    return native_CPXLNETgetphase1cnt(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetpi (CPXCENVptr env, CPXCNETptr net, double *pi, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetpi)
        native_CPXLNETgetpi = load_symbol("CPXLNETgetpi");
    return native_CPXLNETgetpi(env, net, pi, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetprobname (CPXCENVptr env, CPXCNETptr net, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p){
    if (!native_CPXLNETgetprobname)
        native_CPXLNETgetprobname = load_symbol("CPXLNETgetprobname");
    return native_CPXLNETgetprobname(env, net, buf_str, bufspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetslack (CPXCENVptr env, CPXCNETptr net, double *slack, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetslack)
        native_CPXLNETgetslack = load_symbol("CPXLNETgetslack");
    return native_CPXLNETgetslack(env, net, slack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetstat (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXLNETgetstat)
        native_CPXLNETgetstat = load_symbol("CPXLNETgetstat");
    return native_CPXLNETgetstat(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetsupply (CPXCENVptr env, CPXCNETptr net, double *supply, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetsupply)
        native_CPXLNETgetsupply = load_symbol("CPXLNETgetsupply");
    return native_CPXLNETgetsupply(env, net, supply, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetub (CPXCENVptr env, CPXCNETptr net, double *up, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetub)
        native_CPXLNETgetub = load_symbol("CPXLNETgetub");
    return native_CPXLNETgetub(env, net, up, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETgetx (CPXCENVptr env, CPXCNETptr net, double *x, CPXINT begin, CPXINT end){
    if (!native_CPXLNETgetx)
        native_CPXLNETgetx = load_symbol("CPXLNETgetx");
    return native_CPXLNETgetx(env, net, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLNETprimopt (CPXCENVptr env, CPXNETptr net){
    if (!native_CPXLNETprimopt)
        native_CPXLNETprimopt = load_symbol("CPXLNETprimopt");
    return native_CPXLNETprimopt(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXLNETreadcopybase (CPXCENVptr env, CPXNETptr net, char const *filename_str){
    if (!native_CPXLNETreadcopybase)
        native_CPXLNETreadcopybase = load_symbol("CPXLNETreadcopybase");
    return native_CPXLNETreadcopybase(env, net, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLNETreadcopyprob (CPXCENVptr env, CPXNETptr net, char const *filename_str){
    if (!native_CPXLNETreadcopyprob)
        native_CPXLNETreadcopyprob = load_symbol("CPXLNETreadcopyprob");
    return native_CPXLNETreadcopyprob(env, net, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLNETsolninfo (CPXCENVptr env, CPXCNETptr net, int *pfeasind_p, int *dfeasind_p){
    if (!native_CPXLNETsolninfo)
        native_CPXLNETsolninfo = load_symbol("CPXLNETsolninfo");
    return native_CPXLNETsolninfo(env, net, pfeasind_p, dfeasind_p);
}
CPXLIBAPI int CPXPUBLIC CPXLNETsolution (CPXCENVptr env, CPXCNETptr net, int *netstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj){
    if (!native_CPXLNETsolution)
        native_CPXLNETsolution = load_symbol("CPXLNETsolution");
    return native_CPXLNETsolution(env, net, netstat_p, objval_p, x, pi, slack, dj);
}
CPXLIBAPI int CPXPUBLIC CPXLNETwriteprob (CPXCENVptr env, CPXCNETptr net, char const *filename_str, char const *format_str){
    if (!native_CPXLNETwriteprob)
        native_CPXLNETwriteprob = load_symbol("CPXLNETwriteprob");
    return native_CPXLNETwriteprob(env, net, filename_str, format_str);
}
CPXLIBAPI int CPXPUBLIC CPXLaddlazyconstraints (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *rowname){
    if (!native_CPXLaddlazyconstraints)
        native_CPXLaddlazyconstraints = load_symbol("CPXLaddlazyconstraints");
    return native_CPXLaddlazyconstraints(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXLaddmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt, CPXLONG nzcnt, CPXLONG const *beg, CPXINT const *varindices, double const *values, int const *effortlevel, char const *const *mipstartname){
    if (!native_CPXLaddmipstarts)
        native_CPXLaddmipstarts = load_symbol("CPXLaddmipstarts");
    return native_CPXLaddmipstarts(env, lp, mcnt, nzcnt, beg, varindices, values, effortlevel, mipstartname);
}
CPXLIBAPI int CPXPUBLIC CPXLaddsolnpooldivfilter (CPXCENVptr env, CPXLPptr lp, double lower_bound, double upper_bound, CPXINT nzcnt, CPXINT const *ind, double const *weight, double const *refval, char const *lname_str){
    if (!native_CPXLaddsolnpooldivfilter)
        native_CPXLaddsolnpooldivfilter = load_symbol("CPXLaddsolnpooldivfilter");
    return native_CPXLaddsolnpooldivfilter(env, lp, lower_bound, upper_bound, nzcnt, ind, weight, refval, lname_str);
}
CPXLIBAPI int CPXPUBLIC CPXLaddsolnpoolrngfilter (CPXCENVptr env, CPXLPptr lp, double lb, double ub, CPXINT nzcnt, CPXINT const *ind, double const *val, char const *lname_str){
    if (!native_CPXLaddsolnpoolrngfilter)
        native_CPXLaddsolnpoolrngfilter = load_symbol("CPXLaddsolnpoolrngfilter");
    return native_CPXLaddsolnpoolrngfilter(env, lp, lb, ub, nzcnt, ind, val, lname_str);
}
CPXLIBAPI int CPXPUBLIC CPXLaddsos (CPXCENVptr env, CPXLPptr lp, CPXINT numsos, CPXLONG numsosnz, char const *sostype, CPXLONG const *sosbeg, CPXINT const *sosind, double const *soswt, char const *const *sosname){
    if (!native_CPXLaddsos)
        native_CPXLaddsos = load_symbol("CPXLaddsos");
    return native_CPXLaddsos(env, lp, numsos, numsosnz, sostype, sosbeg, sosind, soswt, sosname);
}
CPXLIBAPI int CPXPUBLIC CPXLaddusercuts (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *rowname){
    if (!native_CPXLaddusercuts)
        native_CPXLaddusercuts = load_symbol("CPXLaddusercuts");
    return native_CPXLaddusercuts(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXLbranchcallbackbranchasCPLEX (CPXCENVptr env, void *cbdata, int wherefrom, int num, void *userhandle, CPXLONG * seqnum_p){
    if (!native_CPXLbranchcallbackbranchasCPLEX)
        native_CPXLbranchcallbackbranchasCPLEX = load_symbol("CPXLbranchcallbackbranchasCPLEX");
    return native_CPXLbranchcallbackbranchasCPLEX(env, cbdata, wherefrom, num, userhandle, seqnum_p);
}
CPXLIBAPI int CPXPUBLIC CPXLbranchcallbackbranchbds (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd, double nodeest, void *userhandle, CPXLONG * seqnum_p){
    if (!native_CPXLbranchcallbackbranchbds)
        native_CPXLbranchcallbackbranchbds = load_symbol("CPXLbranchcallbackbranchbds");
    return native_CPXLbranchcallbackbranchbds(env, cbdata, wherefrom, cnt, indices, lu, bd, nodeest, userhandle, seqnum_p);
}
CPXLIBAPI int CPXPUBLIC CPXLbranchcallbackbranchconstraints (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, double nodeest, void *userhandle, CPXLONG * seqnum_p){
    if (!native_CPXLbranchcallbackbranchconstraints)
        native_CPXLbranchcallbackbranchconstraints = load_symbol("CPXLbranchcallbackbranchconstraints");
    return native_CPXLbranchcallbackbranchconstraints(env, cbdata, wherefrom, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, nodeest, userhandle, seqnum_p);
}
CPXLIBAPI int CPXPUBLIC CPXLbranchcallbackbranchgeneral (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT varcnt, CPXINT const *varind, char const *varlu, double const *varbd, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, double nodeest, void *userhandle, CPXLONG * seqnum_p){
    if (!native_CPXLbranchcallbackbranchgeneral)
        native_CPXLbranchcallbackbranchgeneral = load_symbol("CPXLbranchcallbackbranchgeneral");
    return native_CPXLbranchcallbackbranchgeneral(env, cbdata, wherefrom, varcnt, varind, varlu, varbd, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, nodeest, userhandle, seqnum_p);
}
CPXLIBAPI int CPXPUBLIC CPXLcallbacksetnodeuserhandle (CPXCENVptr env, void *cbdata, int wherefrom, CPXLONG nodeindex, void *userhandle, void **olduserhandle_p){
    if (!native_CPXLcallbacksetnodeuserhandle)
        native_CPXLcallbacksetnodeuserhandle = load_symbol("CPXLcallbacksetnodeuserhandle");
    return native_CPXLcallbacksetnodeuserhandle(env, cbdata, wherefrom, nodeindex, userhandle, olduserhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLcallbacksetuserhandle (CPXCENVptr env, void *cbdata, int wherefrom, void *userhandle, void **olduserhandle_p){
    if (!native_CPXLcallbacksetuserhandle)
        native_CPXLcallbacksetuserhandle = load_symbol("CPXLcallbacksetuserhandle");
    return native_CPXLcallbacksetuserhandle(env, cbdata, wherefrom, userhandle, olduserhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLchgctype (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *xctype){
    if (!native_CPXLchgctype)
        native_CPXLchgctype = load_symbol("CPXLchgctype");
    return native_CPXLchgctype(env, lp, cnt, indices, xctype);
}
CPXLIBAPI int CPXPUBLIC CPXLchgmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt, int const *mipstartindices, CPXLONG nzcnt, CPXLONG const *beg, CPXINT const *varindices, double const *values, int const *effortlevel){
    if (!native_CPXLchgmipstarts)
        native_CPXLchgmipstarts = load_symbol("CPXLchgmipstarts");
    return native_CPXLchgmipstarts(env, lp, mcnt, mipstartindices, nzcnt, beg, varindices, values, effortlevel);
}
CPXLIBAPI int CPXPUBLIC CPXLcopyctype (CPXCENVptr env, CPXLPptr lp, char const *xctype){
    if (!native_CPXLcopyctype)
        native_CPXLcopyctype = load_symbol("CPXLcopyctype");
    return native_CPXLcopyctype(env, lp, xctype);
}
CPXLIBAPI int CPXPUBLIC CPXLcopyorder (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, CPXINT const *priority, int const *direction){
    if (!native_CPXLcopyorder)
        native_CPXLcopyorder = load_symbol("CPXLcopyorder");
    return native_CPXLcopyorder(env, lp, cnt, indices, priority, direction);
}
CPXLIBAPI int CPXPUBLIC CPXLcopysos (CPXCENVptr env, CPXLPptr lp, CPXINT numsos, CPXLONG numsosnz, char const *sostype, CPXLONG const *sosbeg, CPXINT const *sosind, double const *soswt, char const *const *sosname){
    if (!native_CPXLcopysos)
        native_CPXLcopysos = load_symbol("CPXLcopysos");
    return native_CPXLcopysos(env, lp, numsos, numsosnz, sostype, sosbeg, sosind, soswt, sosname);
}
CPXLIBAPI int CPXPUBLIC CPXLcutcallbackadd (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT nzcnt, double rhs, int sense, CPXINT const *cutind, double const *cutval, int purgeable){
    if (!native_CPXLcutcallbackadd)
        native_CPXLcutcallbackadd = load_symbol("CPXLcutcallbackadd");
    return native_CPXLcutcallbackadd(env, cbdata, wherefrom, nzcnt, rhs, sense, cutind, cutval, purgeable);
}
CPXLIBAPI int CPXPUBLIC CPXLcutcallbackaddlocal (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT nzcnt, double rhs, int sense, CPXINT const *cutind, double const *cutval){
    if (!native_CPXLcutcallbackaddlocal)
        native_CPXLcutcallbackaddlocal = load_symbol("CPXLcutcallbackaddlocal");
    return native_CPXLcutcallbackaddlocal(env, cbdata, wherefrom, nzcnt, rhs, sense, cutind, cutval);
}
CPXLIBAPI int CPXPUBLIC CPXLdelindconstrs (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end){
    if (!native_CPXLdelindconstrs)
        native_CPXLdelindconstrs = load_symbol("CPXLdelindconstrs");
    return native_CPXLdelindconstrs(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLdelmipstarts (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXLdelmipstarts)
        native_CPXLdelmipstarts = load_symbol("CPXLdelmipstarts");
    return native_CPXLdelmipstarts(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLdelsetmipstarts (CPXCENVptr env, CPXLPptr lp, int *delstat){
    if (!native_CPXLdelsetmipstarts)
        native_CPXLdelsetmipstarts = load_symbol("CPXLdelsetmipstarts");
    return native_CPXLdelsetmipstarts(env, lp, delstat);
}
CPXLIBAPI int CPXPUBLIC CPXLdelsetsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int *delstat){
    if (!native_CPXLdelsetsolnpoolfilters)
        native_CPXLdelsetsolnpoolfilters = load_symbol("CPXLdelsetsolnpoolfilters");
    return native_CPXLdelsetsolnpoolfilters(env, lp, delstat);
}
CPXLIBAPI int CPXPUBLIC CPXLdelsetsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int *delstat){
    if (!native_CPXLdelsetsolnpoolsolns)
        native_CPXLdelsetsolnpoolsolns = load_symbol("CPXLdelsetsolnpoolsolns");
    return native_CPXLdelsetsolnpoolsolns(env, lp, delstat);
}
CPXLIBAPI int CPXPUBLIC CPXLdelsetsos (CPXCENVptr env, CPXLPptr lp, CPXINT * delset){
    if (!native_CPXLdelsetsos)
        native_CPXLdelsetsos = load_symbol("CPXLdelsetsos");
    return native_CPXLdelsetsos(env, lp, delset);
}
CPXLIBAPI int CPXPUBLIC CPXLdelsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXLdelsolnpoolfilters)
        native_CPXLdelsolnpoolfilters = load_symbol("CPXLdelsolnpoolfilters");
    return native_CPXLdelsolnpoolfilters(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLdelsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXLdelsolnpoolsolns)
        native_CPXLdelsolnpoolsolns = load_symbol("CPXLdelsolnpoolsolns");
    return native_CPXLdelsolnpoolsolns(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLdelsos (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end){
    if (!native_CPXLdelsos)
        native_CPXLdelsos = load_symbol("CPXLdelsos");
    return native_CPXLdelsos(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLfltwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLfltwrite)
        native_CPXLfltwrite = load_symbol("CPXLfltwrite");
    return native_CPXLfltwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLfreelazyconstraints (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLfreelazyconstraints)
        native_CPXLfreelazyconstraints = load_symbol("CPXLfreelazyconstraints");
    return native_CPXLfreelazyconstraints(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLfreeusercuts (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLfreeusercuts)
        native_CPXLfreeusercuts = load_symbol("CPXLfreeusercuts");
    return native_CPXLfreeusercuts(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetbestobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p){
    if (!native_CPXLgetbestobjval)
        native_CPXLgetbestobjval = load_symbol("CPXLgetbestobjval");
    return native_CPXLgetbestobjval(env, lp, objval_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetbranchcallbackfunc (CPXCENVptr env, CPXL_CALLBACK_BRANCH ** branchcallback_p, void **cbhandle_p){
    if (!native_CPXLgetbranchcallbackfunc)
        native_CPXLgetbranchcallbackfunc = load_symbol("CPXLgetbranchcallbackfunc");
    return native_CPXLgetbranchcallbackfunc(env, branchcallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetbranchnosolncallbackfunc (CPXCENVptr env, CPXL_CALLBACK_BRANCH ** branchnosolncallback_p, void **cbhandle_p){
    if (!native_CPXLgetbranchnosolncallbackfunc)
        native_CPXLgetbranchnosolncallbackfunc = load_symbol("CPXLgetbranchnosolncallbackfunc");
    return native_CPXLgetbranchnosolncallbackfunc(env, branchnosolncallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackbranchconstraints (CPXCENVptr env, void *cbdata, int wherefrom, int which, CPXINT * cuts_p, CPXLONG * nzcnt_p, double *rhs, char *sense, CPXLONG * rmatbeg, CPXINT * rmatind, double *rmatval, CPXLONG rmatsz, CPXLONG * surplus_p){
    if (!native_CPXLgetcallbackbranchconstraints)
        native_CPXLgetcallbackbranchconstraints = load_symbol("CPXLgetcallbackbranchconstraints");
    return native_CPXLgetcallbackbranchconstraints(env, cbdata, wherefrom, which, cuts_p, nzcnt_p, rhs, sense, rmatbeg, rmatind, rmatval, rmatsz, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackctype (CPXCENVptr env, void *cbdata, int wherefrom, char *xctype, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcallbackctype)
        native_CPXLgetcallbackctype = load_symbol("CPXLgetcallbackctype");
    return native_CPXLgetcallbackctype(env, cbdata, wherefrom, xctype, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackgloballb (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcallbackgloballb)
        native_CPXLgetcallbackgloballb = load_symbol("CPXLgetcallbackgloballb");
    return native_CPXLgetcallbackgloballb(env, cbdata, wherefrom, lb, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackglobalub (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcallbackglobalub)
        native_CPXLgetcallbackglobalub = load_symbol("CPXLgetcallbackglobalub");
    return native_CPXLgetcallbackglobalub(env, cbdata, wherefrom, ub, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackincumbent (CPXCENVptr env, void *cbdata, int wherefrom, double *x, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcallbackincumbent)
        native_CPXLgetcallbackincumbent = load_symbol("CPXLgetcallbackincumbent");
    return native_CPXLgetcallbackincumbent(env, cbdata, wherefrom, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackindicatorinfo (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT iindex, int whichinfo, void *result_p){
    if (!native_CPXLgetcallbackindicatorinfo)
        native_CPXLgetcallbackindicatorinfo = load_symbol("CPXLgetcallbackindicatorinfo");
    return native_CPXLgetcallbackindicatorinfo(env, cbdata, wherefrom, iindex, whichinfo, result_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacklp (CPXCENVptr env, void *cbdata, int wherefrom, CPXCLPptr * lp_p){
    if (!native_CPXLgetcallbacklp)
        native_CPXLgetcallbacklp = load_symbol("CPXLgetcallbacklp");
    return native_CPXLgetcallbacklp(env, cbdata, wherefrom, lp_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeinfo (CPXCENVptr env, void *cbdata, int wherefrom, CPXLONG nodeindex, int whichinfo, void *result_p){
    if (!native_CPXLgetcallbacknodeinfo)
        native_CPXLgetcallbacknodeinfo = load_symbol("CPXLgetcallbacknodeinfo");
    return native_CPXLgetcallbacknodeinfo(env, cbdata, wherefrom, nodeindex, whichinfo, result_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeintfeas (CPXCENVptr env, void *cbdata, int wherefrom, int *feas, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcallbacknodeintfeas)
        native_CPXLgetcallbacknodeintfeas = load_symbol("CPXLgetcallbacknodeintfeas");
    return native_CPXLgetcallbacknodeintfeas(env, cbdata, wherefrom, feas, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodelb (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcallbacknodelb)
        native_CPXLgetcallbacknodelb = load_symbol("CPXLgetcallbacknodelb");
    return native_CPXLgetcallbacknodelb(env, cbdata, wherefrom, lb, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodelp (CPXCENVptr env, void *cbdata, int wherefrom, CPXLPptr * nodelp_p){
    if (!native_CPXLgetcallbacknodelp)
        native_CPXLgetcallbacknodelp = load_symbol("CPXLgetcallbacknodelp");
    return native_CPXLgetcallbacknodelp(env, cbdata, wherefrom, nodelp_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeobjval (CPXCENVptr env, void *cbdata, int wherefrom, double *objval_p){
    if (!native_CPXLgetcallbacknodeobjval)
        native_CPXLgetcallbacknodeobjval = load_symbol("CPXLgetcallbacknodeobjval");
    return native_CPXLgetcallbacknodeobjval(env, cbdata, wherefrom, objval_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodestat (CPXCENVptr env, void *cbdata, int wherefrom, int *nodestat_p){
    if (!native_CPXLgetcallbacknodestat)
        native_CPXLgetcallbacknodestat = load_symbol("CPXLgetcallbacknodestat");
    return native_CPXLgetcallbacknodestat(env, cbdata, wherefrom, nodestat_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeub (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcallbacknodeub)
        native_CPXLgetcallbacknodeub = load_symbol("CPXLgetcallbacknodeub");
    return native_CPXLgetcallbacknodeub(env, cbdata, wherefrom, ub, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodex (CPXCENVptr env, void *cbdata, int wherefrom, double *x, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcallbacknodex)
        native_CPXLgetcallbacknodex = load_symbol("CPXLgetcallbacknodex");
    return native_CPXLgetcallbacknodex(env, cbdata, wherefrom, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackorder (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT * priority, int *direction, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcallbackorder)
        native_CPXLgetcallbackorder = load_symbol("CPXLgetcallbackorder");
    return native_CPXLgetcallbackorder(env, cbdata, wherefrom, priority, direction, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackpseudocosts (CPXCENVptr env, void *cbdata, int wherefrom, double *uppc, double *downpc, CPXINT begin, CPXINT end){
    if (!native_CPXLgetcallbackpseudocosts)
        native_CPXLgetcallbackpseudocosts = load_symbol("CPXLgetcallbackpseudocosts");
    return native_CPXLgetcallbackpseudocosts(env, cbdata, wherefrom, uppc, downpc, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackseqinfo (CPXCENVptr env, void *cbdata, int wherefrom, CPXLONG seqid, int whichinfo, void *result_p){
    if (!native_CPXLgetcallbackseqinfo)
        native_CPXLgetcallbackseqinfo = load_symbol("CPXLgetcallbackseqinfo");
    return native_CPXLgetcallbackseqinfo(env, cbdata, wherefrom, seqid, whichinfo, result_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacksosinfo (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT sosindex, CPXINT member, int whichinfo, void *result_p){
    if (!native_CPXLgetcallbacksosinfo)
        native_CPXLgetcallbacksosinfo = load_symbol("CPXLgetcallbacksosinfo");
    return native_CPXLgetcallbacksosinfo(env, cbdata, wherefrom, sosindex, member, whichinfo, result_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetctype (CPXCENVptr env, CPXCLPptr lp, char *xctype, CPXINT begin, CPXINT end){
    if (!native_CPXLgetctype)
        native_CPXLgetctype = load_symbol("CPXLgetctype");
    return native_CPXLgetctype(env, lp, xctype, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetcutoff (CPXCENVptr env, CPXCLPptr lp, double *cutoff_p){
    if (!native_CPXLgetcutoff)
        native_CPXLgetcutoff = load_symbol("CPXLgetcutoff");
    return native_CPXLgetcutoff(env, lp, cutoff_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetdeletenodecallbackfunc (CPXCENVptr env, CPXL_CALLBACK_DELETENODE ** deletecallback_p, void **cbhandle_p){
    if (!native_CPXLgetdeletenodecallbackfunc)
        native_CPXLgetdeletenodecallbackfunc = load_symbol("CPXLgetdeletenodecallbackfunc");
    return native_CPXLgetdeletenodecallbackfunc(env, deletecallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetheuristiccallbackfunc (CPXCENVptr env, CPXL_CALLBACK_HEURISTIC ** heuristiccallback_p, void **cbhandle_p){
    if (!native_CPXLgetheuristiccallbackfunc)
        native_CPXLgetheuristiccallbackfunc = load_symbol("CPXLgetheuristiccallbackfunc");
    return native_CPXLgetheuristiccallbackfunc(env, heuristiccallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetincumbentcallbackfunc (CPXCENVptr env, CPXL_CALLBACK_INCUMBENT ** incumbentcallback_p, void **cbhandle_p){
    if (!native_CPXLgetincumbentcallbackfunc)
        native_CPXLgetincumbentcallbackfunc = load_symbol("CPXLgetincumbentcallbackfunc");
    return native_CPXLgetincumbentcallbackfunc(env, incumbentcallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetindconstr (CPXCENVptr env, CPXCLPptr lp, CPXINT * indvar_p, int *complemented_p, CPXINT * nzcnt_p, double *rhs_p, char *sense_p, CPXINT * linind, double *linval, CPXINT space, CPXINT * surplus_p, CPXINT which){
    if (!native_CPXLgetindconstr)
        native_CPXLgetindconstr = load_symbol("CPXLgetindconstr");
    return native_CPXLgetindconstr(env, lp, indvar_p, complemented_p, nzcnt_p, rhs_p, sense_p, linind, linval, space, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXLgetindconstrindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p){
    if (!native_CPXLgetindconstrindex)
        native_CPXLgetindconstrindex = load_symbol("CPXLgetindconstrindex");
    return native_CPXLgetindconstrindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetindconstrinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end){
    if (!native_CPXLgetindconstrinfeas)
        native_CPXLgetindconstrinfeas = load_symbol("CPXLgetindconstrinfeas");
    return native_CPXLgetindconstrinfeas(env, lp, x, infeasout, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetindconstrname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p, CPXINT which){
    if (!native_CPXLgetindconstrname)
        native_CPXLgetindconstrname = load_symbol("CPXLgetindconstrname");
    return native_CPXLgetindconstrname(env, lp, buf_str, bufspace, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXLgetindconstrslack (CPXCENVptr env, CPXCLPptr lp, double *indslack, CPXINT begin, CPXINT end){
    if (!native_CPXLgetindconstrslack)
        native_CPXLgetindconstrslack = load_symbol("CPXLgetindconstrslack");
    return native_CPXLgetindconstrslack(env, lp, indslack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetinfocallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p){
    if (!native_CPXLgetinfocallbackfunc)
        native_CPXLgetinfocallbackfunc = load_symbol("CPXLgetinfocallbackfunc");
    return native_CPXLgetinfocallbackfunc(env, callback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetlazyconstraintcallbackfunc (CPXCENVptr env, CPXL_CALLBACK_CUT ** cutcallback_p, void **cbhandle_p){
    if (!native_CPXLgetlazyconstraintcallbackfunc)
        native_CPXLgetlazyconstraintcallbackfunc = load_symbol("CPXLgetlazyconstraintcallbackfunc");
    return native_CPXLgetlazyconstraintcallbackfunc(env, cutcallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetmipcallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p){
    if (!native_CPXLgetmipcallbackfunc)
        native_CPXLgetmipcallbackfunc = load_symbol("CPXLgetmipcallbackfunc");
    return native_CPXLgetmipcallbackfunc(env, callback_p, cbhandle_p);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetmipitcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetmipitcnt)
        native_CPXLgetmipitcnt = load_symbol("CPXLgetmipitcnt");
    return native_CPXLgetmipitcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetmiprelgap (CPXCENVptr env, CPXCLPptr lp, double *gap_p){
    if (!native_CPXLgetmiprelgap)
        native_CPXLgetmiprelgap = load_symbol("CPXLgetmiprelgap");
    return native_CPXLgetmiprelgap(env, lp, gap_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetmipstartindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXLgetmipstartindex)
        native_CPXLgetmipstartindex = load_symbol("CPXLgetmipstartindex");
    return native_CPXLgetmipstartindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetmipstartname (CPXCENVptr env, CPXCLPptr lp, char **name, char *store, CPXSIZE storesz, CPXSIZE * surplus_p, int begin, int end){
    if (!native_CPXLgetmipstartname)
        native_CPXLgetmipstartname = load_symbol("CPXLgetmipstartname");
    return native_CPXLgetmipstartname(env, lp, name, store, storesz, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetmipstarts (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * beg, CPXINT * varindices, double *values, int *effortlevel, CPXLONG startspace, CPXLONG * surplus_p, int begin, int end){
    if (!native_CPXLgetmipstarts)
        native_CPXLgetmipstarts = load_symbol("CPXLgetmipstarts");
    return native_CPXLgetmipstarts(env, lp, nzcnt_p, beg, varindices, values, effortlevel, startspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetnodecallbackfunc (CPXCENVptr env, CPXL_CALLBACK_NODE ** nodecallback_p, void **cbhandle_p){
    if (!native_CPXLgetnodecallbackfunc)
        native_CPXLgetnodecallbackfunc = load_symbol("CPXLgetnodecallbackfunc");
    return native_CPXLgetnodecallbackfunc(env, nodecallback_p, cbhandle_p);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnodecnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnodecnt)
        native_CPXLgetnodecnt = load_symbol("CPXLgetnodecnt");
    return native_CPXLgetnodecnt(env, lp);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnodeint (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnodeint)
        native_CPXLgetnodeint = load_symbol("CPXLgetnodeint");
    return native_CPXLgetnodeint(env, lp);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnodeleftcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnodeleftcnt)
        native_CPXLgetnodeleftcnt = load_symbol("CPXLgetnodeleftcnt");
    return native_CPXLgetnodeleftcnt(env, lp);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumbin (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumbin)
        native_CPXLgetnumbin = load_symbol("CPXLgetnumbin");
    return native_CPXLgetnumbin(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetnumcuts (CPXCENVptr env, CPXCLPptr lp, int cuttype, CPXINT * num_p){
    if (!native_CPXLgetnumcuts)
        native_CPXLgetnumcuts = load_symbol("CPXLgetnumcuts");
    return native_CPXLgetnumcuts(env, lp, cuttype, num_p);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumindconstrs (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumindconstrs)
        native_CPXLgetnumindconstrs = load_symbol("CPXLgetnumindconstrs");
    return native_CPXLgetnumindconstrs(env, lp);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumint (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumint)
        native_CPXLgetnumint = load_symbol("CPXLgetnumint");
    return native_CPXLgetnumint(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetnummipstarts (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnummipstarts)
        native_CPXLgetnummipstarts = load_symbol("CPXLgetnummipstarts");
    return native_CPXLgetnummipstarts(env, lp);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumsemicont (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumsemicont)
        native_CPXLgetnumsemicont = load_symbol("CPXLgetnumsemicont");
    return native_CPXLgetnumsemicont(env, lp);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumsemiint (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumsemiint)
        native_CPXLgetnumsemiint = load_symbol("CPXLgetnumsemiint");
    return native_CPXLgetnumsemiint(env, lp);
}
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumsos (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumsos)
        native_CPXLgetnumsos = load_symbol("CPXLgetnumsos");
    return native_CPXLgetnumsos(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetorder (CPXCENVptr env, CPXCLPptr lp, CPXINT * cnt_p, CPXINT * indices, CPXINT * priority, int *direction, CPXINT ordspace, CPXINT * surplus_p){
    if (!native_CPXLgetorder)
        native_CPXLgetorder = load_symbol("CPXLgetorder");
    return native_CPXLgetorder(env, lp, cnt_p, indices, priority, direction, ordspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpooldivfilter (CPXCENVptr env, CPXCLPptr lp, double *lower_cutoff_p, double *upper_cutoff_p, CPXINT * nzcnt_p, CPXINT * ind, double *val, double *refval, CPXINT space, CPXINT * surplus_p, int which){
    if (!native_CPXLgetsolnpooldivfilter)
        native_CPXLgetsolnpooldivfilter = load_symbol("CPXLgetsolnpooldivfilter");
    return native_CPXLgetsolnpooldivfilter(env, lp, lower_cutoff_p, upper_cutoff_p, nzcnt_p, ind, val, refval, space, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolfilterindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXLgetsolnpoolfilterindex)
        native_CPXLgetsolnpoolfilterindex = load_symbol("CPXLgetsolnpoolfilterindex");
    return native_CPXLgetsolnpoolfilterindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolfiltername (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p, int which){
    if (!native_CPXLgetsolnpoolfiltername)
        native_CPXLgetsolnpoolfiltername = load_symbol("CPXLgetsolnpoolfiltername");
    return native_CPXLgetsolnpoolfiltername(env, lp, buf_str, bufspace, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolfiltertype (CPXCENVptr env, CPXCLPptr lp, int *ftype_p, int which){
    if (!native_CPXLgetsolnpoolfiltertype)
        native_CPXLgetsolnpoolfiltertype = load_symbol("CPXLgetsolnpoolfiltertype");
    return native_CPXLgetsolnpoolfiltertype(env, lp, ftype_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolmeanobjval (CPXCENVptr env, CPXCLPptr lp, double *meanobjval_p){
    if (!native_CPXLgetsolnpoolmeanobjval)
        native_CPXLgetsolnpoolmeanobjval = load_symbol("CPXLgetsolnpoolmeanobjval");
    return native_CPXLgetsolnpoolmeanobjval(env, lp, meanobjval_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolnumfilters (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetsolnpoolnumfilters)
        native_CPXLgetsolnpoolnumfilters = load_symbol("CPXLgetsolnpoolnumfilters");
    return native_CPXLgetsolnpoolnumfilters(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolnumreplaced (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetsolnpoolnumreplaced)
        native_CPXLgetsolnpoolnumreplaced = load_symbol("CPXLgetsolnpoolnumreplaced");
    return native_CPXLgetsolnpoolnumreplaced(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolnumsolns (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetsolnpoolnumsolns)
        native_CPXLgetsolnpoolnumsolns = load_symbol("CPXLgetsolnpoolnumsolns");
    return native_CPXLgetsolnpoolnumsolns(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolobjval (CPXCENVptr env, CPXCLPptr lp, int soln, double *objval_p){
    if (!native_CPXLgetsolnpoolobjval)
        native_CPXLgetsolnpoolobjval = load_symbol("CPXLgetsolnpoolobjval");
    return native_CPXLgetsolnpoolobjval(env, lp, soln, objval_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolqconstrslack (CPXCENVptr env, CPXCLPptr lp, int soln, double *qcslack, CPXINT begin, CPXINT end){
    if (!native_CPXLgetsolnpoolqconstrslack)
        native_CPXLgetsolnpoolqconstrslack = load_symbol("CPXLgetsolnpoolqconstrslack");
    return native_CPXLgetsolnpoolqconstrslack(env, lp, soln, qcslack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolrngfilter (CPXCENVptr env, CPXCLPptr lp, double *lb_p, double *ub_p, CPXINT * nzcnt_p, CPXINT * ind, double *val, CPXINT space, CPXINT * surplus_p, int which){
    if (!native_CPXLgetsolnpoolrngfilter)
        native_CPXLgetsolnpoolrngfilter = load_symbol("CPXLgetsolnpoolrngfilter");
    return native_CPXLgetsolnpoolrngfilter(env, lp, lb_p, ub_p, nzcnt_p, ind, val, space, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolslack (CPXCENVptr env, CPXCLPptr lp, int soln, double *slack, CPXINT begin, CPXINT end){
    if (!native_CPXLgetsolnpoolslack)
        native_CPXLgetsolnpoolslack = load_symbol("CPXLgetsolnpoolslack");
    return native_CPXLgetsolnpoolslack(env, lp, soln, slack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolsolnindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXLgetsolnpoolsolnindex)
        native_CPXLgetsolnpoolsolnindex = load_symbol("CPXLgetsolnpoolsolnindex");
    return native_CPXLgetsolnpoolsolnindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolsolnname (CPXCENVptr env, CPXCLPptr lp, char *store, CPXSIZE storesz, CPXSIZE * surplus_p, int which){
    if (!native_CPXLgetsolnpoolsolnname)
        native_CPXLgetsolnpoolsolnname = load_symbol("CPXLgetsolnpoolsolnname");
    return native_CPXLgetsolnpoolsolnname(env, lp, store, storesz, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolx (CPXCENVptr env, CPXCLPptr lp, int soln, double *x, CPXINT begin, CPXINT end){
    if (!native_CPXLgetsolnpoolx)
        native_CPXLgetsolnpoolx = load_symbol("CPXLgetsolnpoolx");
    return native_CPXLgetsolnpoolx(env, lp, soln, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsolvecallbackfunc (CPXCENVptr env, CPXL_CALLBACK_SOLVE ** solvecallback_p, void **cbhandle_p){
    if (!native_CPXLgetsolvecallbackfunc)
        native_CPXLgetsolvecallbackfunc = load_symbol("CPXLgetsolvecallbackfunc");
    return native_CPXLgetsolvecallbackfunc(env, solvecallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsos (CPXCENVptr env, CPXCLPptr lp, CPXLONG * numsosnz_p, char *sostype, CPXLONG * sosbeg, CPXINT * sosind, double *soswt, CPXLONG sosspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end){
    if (!native_CPXLgetsos)
        native_CPXLgetsos = load_symbol("CPXLgetsos");
    return native_CPXLgetsos(env, lp, numsosnz_p, sostype, sosbeg, sosind, soswt, sosspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsosindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p){
    if (!native_CPXLgetsosindex)
        native_CPXLgetsosindex = load_symbol("CPXLgetsosindex");
    return native_CPXLgetsosindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsosinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end){
    if (!native_CPXLgetsosinfeas)
        native_CPXLgetsosinfeas = load_symbol("CPXLgetsosinfeas");
    return native_CPXLgetsosinfeas(env, lp, x, infeasout, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsosname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, CPXSIZE storespace, CPXSIZE * surplus_p, CPXINT begin, CPXINT end){
    if (!native_CPXLgetsosname)
        native_CPXLgetsosname = load_symbol("CPXLgetsosname");
    return native_CPXLgetsosname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsubmethod (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetsubmethod)
        native_CPXLgetsubmethod = load_symbol("CPXLgetsubmethod");
    return native_CPXLgetsubmethod(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetsubstat (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetsubstat)
        native_CPXLgetsubstat = load_symbol("CPXLgetsubstat");
    return native_CPXLgetsubstat(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLgetusercutcallbackfunc (CPXCENVptr env, CPXL_CALLBACK_CUT ** cutcallback_p, void **cbhandle_p){
    if (!native_CPXLgetusercutcallbackfunc)
        native_CPXLgetusercutcallbackfunc = load_symbol("CPXLgetusercutcallbackfunc");
    return native_CPXLgetusercutcallbackfunc(env, cutcallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXLindconstrslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *indslack){
    if (!native_CPXLindconstrslackfromx)
        native_CPXLindconstrslackfromx = load_symbol("CPXLindconstrslackfromx");
    return native_CPXLindconstrslackfromx(env, lp, x, indslack);
}
CPXLIBAPI int CPXPUBLIC CPXLmipopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLmipopt)
        native_CPXLmipopt = load_symbol("CPXLmipopt");
    return native_CPXLmipopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLordread (CPXCENVptr env, char const *filename_str, CPXINT numcols, char const *const *colname, CPXINT * cnt_p, CPXINT * indices, CPXINT * priority, int *direction){
    if (!native_CPXLordread)
        native_CPXLordread = load_symbol("CPXLordread");
    return native_CPXLordread(env, filename_str, numcols, colname, cnt_p, indices, priority, direction);
}
CPXLIBAPI int CPXPUBLIC CPXLordwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXLordwrite)
        native_CPXLordwrite = load_symbol("CPXLordwrite");
    return native_CPXLordwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLpopulate (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXLpopulate)
        native_CPXLpopulate = load_symbol("CPXLpopulate");
    return native_CPXLpopulate(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXLreadcopymipstarts (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXLreadcopymipstarts)
        native_CPXLreadcopymipstarts = load_symbol("CPXLreadcopymipstarts");
    return native_CPXLreadcopymipstarts(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLreadcopyorder (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXLreadcopyorder)
        native_CPXLreadcopyorder = load_symbol("CPXLreadcopyorder");
    return native_CPXLreadcopyorder(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLreadcopysolnpoolfilters (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXLreadcopysolnpoolfilters)
        native_CPXLreadcopysolnpoolfilters = load_symbol("CPXLreadcopysolnpoolfilters");
    return native_CPXLreadcopysolnpoolfilters(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXLrefinemipstartconflict (CPXCENVptr env, CPXLPptr lp, int mipstartindex, CPXINT * confnumrows_p, CPXINT * confnumcols_p){
    if (!native_CPXLrefinemipstartconflict)
        native_CPXLrefinemipstartconflict = load_symbol("CPXLrefinemipstartconflict");
    return native_CPXLrefinemipstartconflict(env, lp, mipstartindex, confnumrows_p, confnumcols_p);
}
CPXLIBAPI int CPXPUBLIC CPXLrefinemipstartconflictext (CPXCENVptr env, CPXLPptr lp, int mipstartindex, CPXLONG grpcnt, CPXLONG concnt, double const *grppref, CPXLONG const *grpbeg, CPXINT const *grpind, char const *grptype){
    if (!native_CPXLrefinemipstartconflictext)
        native_CPXLrefinemipstartconflictext = load_symbol("CPXLrefinemipstartconflictext");
    return native_CPXLrefinemipstartconflictext(env, lp, mipstartindex, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}
CPXLIBAPI int CPXPUBLIC CPXLsetbranchcallbackfunc (CPXENVptr env, CPXL_CALLBACK_BRANCH * branchcallback, void *cbhandle){
    if (!native_CPXLsetbranchcallbackfunc)
        native_CPXLsetbranchcallbackfunc = load_symbol("CPXLsetbranchcallbackfunc");
    return native_CPXLsetbranchcallbackfunc(env, branchcallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetbranchnosolncallbackfunc (CPXENVptr env, CPXL_CALLBACK_BRANCH * branchnosolncallback, void *cbhandle){
    if (!native_CPXLsetbranchnosolncallbackfunc)
        native_CPXLsetbranchnosolncallbackfunc = load_symbol("CPXLsetbranchnosolncallbackfunc");
    return native_CPXLsetbranchnosolncallbackfunc(env, branchnosolncallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetdeletenodecallbackfunc (CPXENVptr env, CPXL_CALLBACK_DELETENODE * deletecallback, void *cbhandle){
    if (!native_CPXLsetdeletenodecallbackfunc)
        native_CPXLsetdeletenodecallbackfunc = load_symbol("CPXLsetdeletenodecallbackfunc");
    return native_CPXLsetdeletenodecallbackfunc(env, deletecallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetheuristiccallbackfunc (CPXENVptr env, CPXL_CALLBACK_HEURISTIC * heuristiccallback, void *cbhandle){
    if (!native_CPXLsetheuristiccallbackfunc)
        native_CPXLsetheuristiccallbackfunc = load_symbol("CPXLsetheuristiccallbackfunc");
    return native_CPXLsetheuristiccallbackfunc(env, heuristiccallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetincumbentcallbackfunc (CPXENVptr env, CPXL_CALLBACK_INCUMBENT * incumbentcallback, void *cbhandle){
    if (!native_CPXLsetincumbentcallbackfunc)
        native_CPXLsetincumbentcallbackfunc = load_symbol("CPXLsetincumbentcallbackfunc");
    return native_CPXLsetincumbentcallbackfunc(env, incumbentcallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetinfocallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle){
    if (!native_CPXLsetinfocallbackfunc)
        native_CPXLsetinfocallbackfunc = load_symbol("CPXLsetinfocallbackfunc");
    return native_CPXLsetinfocallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetlazyconstraintcallbackfunc (CPXENVptr env, CPXL_CALLBACK_CUT * lazyconcallback, void *cbhandle){
    if (!native_CPXLsetlazyconstraintcallbackfunc)
        native_CPXLsetlazyconstraintcallbackfunc = load_symbol("CPXLsetlazyconstraintcallbackfunc");
    return native_CPXLsetlazyconstraintcallbackfunc(env, lazyconcallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetmipcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle){
    if (!native_CPXLsetmipcallbackfunc)
        native_CPXLsetmipcallbackfunc = load_symbol("CPXLsetmipcallbackfunc");
    return native_CPXLsetmipcallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetnodecallbackfunc (CPXENVptr env, CPXL_CALLBACK_NODE * nodecallback, void *cbhandle){
    if (!native_CPXLsetnodecallbackfunc)
        native_CPXLsetnodecallbackfunc = load_symbol("CPXLsetnodecallbackfunc");
    return native_CPXLsetnodecallbackfunc(env, nodecallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetsolvecallbackfunc (CPXENVptr env, CPXL_CALLBACK_SOLVE * solvecallback, void *cbhandle){
    if (!native_CPXLsetsolvecallbackfunc)
        native_CPXLsetsolvecallbackfunc = load_symbol("CPXLsetsolvecallbackfunc");
    return native_CPXLsetsolvecallbackfunc(env, solvecallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLsetusercutcallbackfunc (CPXENVptr env, CPXL_CALLBACK_CUT * cutcallback, void *cbhandle){
    if (!native_CPXLsetusercutcallbackfunc)
        native_CPXLsetusercutcallbackfunc = load_symbol("CPXLsetusercutcallbackfunc");
    return native_CPXLsetusercutcallbackfunc(env, cutcallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXLwritemipstarts (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, int begin, int end){
    if (!native_CPXLwritemipstarts)
        native_CPXLwritemipstarts = load_symbol("CPXLwritemipstarts");
    return native_CPXLwritemipstarts(env, lp, filename_str, begin, end);
}
