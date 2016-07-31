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
// for GCC
#ifdef _WIN64 
#define _LP64 1
#endif

#include "cplexx.h"
#include <lazyloader.h>
// remove for windows some of the macros
#undef CPXPUBLIC
#define CPXPUBLIC
#undef CPXPUBVARARGS
#define CPXPUBVARARGS

/* Debugging macros */
#ifdef _MSC_VER
#define PRINT_DEBUG(fmt, ...) if (debug_enabled) {     fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, __VA_ARGS__ ); }
#define PRINT_ERR(fmt, ...)     fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, __VA_ARGS__ );
#else
#define PRINT_DEBUG(fmt, args ...) if (debug_enabled) {     fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, ## args ); }
#define PRINT_ERR(fmt, args ...)     fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, ## args );
#endif

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

void* get_lazy_handle() {
	return handle;
}

void set_lazy_handle(void* handle) {
	handle = handle;
}

static void exit_lazy_loader() {
    if (handle != NULL)
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        handle = NULL;
}

#ifdef _WIN32
#define SYMBOL(sym) GetProcAddress(handle, sym)
#else
#define SYMBOL(sym) dlsym(handle, sym)
#endif

#ifdef _WIN32
#define FREE FreeLibrary(handle)
#else
#define FREE dlclose(handle)
#endif

int test_library(int min_version) {
    if (handle == NULL) {
        return 1;
    }
    void* symbol = NULL;
    symbol = SYMBOL("CPXopenCPLEX");
    if (symbol == NULL) {
        PRINT_DEBUG("Library is missing CPXopenCPLEX\n");
        FREE;
        handle = NULL;
        return 2;
    }
    int status = 0;
    void* env = ((void* (*) (int*)) (symbol)) (&status);
    if (env == NULL || status != 0) {
        PRINT_DEBUG("Could not open a CPLEX environment\nCPLEX error: %d\n", status);
        FREE;
        handle = NULL;
        return 3;
    }
    symbol = SYMBOL("CPXversionnumber");
    int ret = 0;
    int version = 0;
    if (symbol == NULL) {
        PRINT_DEBUG("Library is missing CPXversionnumber, fallback\n");
        symbol = SYMBOL("CPXversion");
        if (symbol == NULL) {
            PRINT_DEBUG("Library is missing CPXversion\n");
            FREE;
            handle = NULL;
            return 2;
        }
        const char* verstring =
        ((const char* (*) (void*)) (symbol)) (env);
        if (verstring == NULL) {
            PRINT_DEBUG("Could not retrieve a version from the library\n");
            ret = 3;
        }
        int major, minor, micro, patch;
        if (sscanf(verstring, "%d.%d.%d.%d", &major, &minor, &micro, &patch) != 4) {
            PRINT_DEBUG("Could not retrieve a version from the library\nCPLEX error: %d\n", status);
            ret = 3;
        } else {
            version = major * 1000000 + minor * 10000 + micro * 100 + patch;
        }
    } else {
        status = ((int (*) (void*, int*)) (symbol)) (env, &version);
        if (status != 0) {
            PRINT_DEBUG("Could not retrieve a version from the library\nCPLEX error: %d\n", status);
            ret = 3;
        }
    }
    if (ret == 0) {
        PRINT_DEBUG("Loaded CPLEX library version: %d.%d.%d.%d\n",
                    (version / 1000000)      ,
                    (version / 10000  ) % 100,
                    (version / 100    ) % 100,
                    (version          ) % 100);
        if (version < min_version) {
            PRINT_DEBUG("Library version marked as too old\n");
            ret = 4;
        }
    }
    symbol = SYMBOL("CPXcloseCPLEX");
    if (symbol == NULL) {
        PRINT_DEBUG("Library is missing CPXcloseCPLEX\n");
        FREE;
        handle = NULL;
        return 2;
    }
    ((int (*) (void**)) (symbol)) (&env);
    if (ret != 0) {
        FREE;
        handle = NULL;
    }
    return ret;
}

int try_lazy_load(const char* library, int min_version) {
    const char *dbg = getenv("LAZYCPLEX_DEBUG");
    debug_enabled = (dbg != NULL && strlen(dbg) > 0);

    if (handle != NULL) {
        PRINT_DEBUG("The library is already initialized.\n");
        return 0;
    }

    PRINT_DEBUG("Trying to load %s\n", library);
#ifdef _WIN32
    handle = LoadLibrary(library);
#else
    handle = dlopen(library, RTLD_LAZY);
#endif
    if (handle == NULL)
        return 1;
    else {
        int ret = test_library(min_version);
        if (ret == 0) {
            PRINT_DEBUG("Success!\n");
            atexit(exit_lazy_loader);
        }
        return ret;
    }
}

/* Searches and loads the actual library */
int initialize_lazy_loader(int min_version) {
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
        if (try_lazy_load(libnames[i], min_version) == 0) {
            break;
        }
    }

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

CPXCHANNELptr CPXPUBLIC (*native_CPXaddchannel) (CPXENVptr env) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddcols) (CPXCENVptr env, CPXLPptr lp, int ccnt, int nzcnt, double const *obj, int const *cmatbeg, int const *cmatind, double const *cmatval, double const *lb, double const *ub, char **colname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddfpdest) (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddfuncdest) (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, const char *)) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddrows) (CPXCENVptr env, CPXLPptr lp, int ccnt, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **colname, char **rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbasicpresolve) (CPXCENVptr env, CPXLPptr lp, double *redlb, double *redub, int *rstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbinvacol) (CPXCENVptr env, CPXCLPptr lp, int j, double *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbinvarow) (CPXCENVptr env, CPXCLPptr lp, int i, double *z) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbinvcol) (CPXCENVptr env, CPXCLPptr lp, int j, double *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbinvrow) (CPXCENVptr env, CPXCLPptr lp, int i, double *y) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXboundsa) (CPXCENVptr env, CPXCLPptr lp, int begin, int end, double *lblower, double *lbupper, double *ublower, double *ubupper) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbtran) (CPXCENVptr env, CPXCLPptr lp, double *y) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcheckdfeas) (CPXCENVptr env, CPXLPptr lp, int *infeas_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcheckpfeas) (CPXCENVptr env, CPXLPptr lp, int *infeas_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchecksoln) (CPXCENVptr env, CPXLPptr lp, int *lpstatus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgbds) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *lu, double const *bd) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgcoef) (CPXCENVptr env, CPXLPptr lp, int i, int j, double newvalue) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgcoeflist) (CPXCENVptr env, CPXLPptr lp, int numcoefs, int const *rowlist, int const *collist, double const *vallist) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgcolname) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char **newname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgname) (CPXCENVptr env, CPXLPptr lp, int key, int ij, char const *newname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgobj) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgobjoffset) (CPXCENVptr env, CPXLPptr lp, double offset) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgobjsen) (CPXCENVptr env, CPXLPptr lp, int maxormin) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgprobname) (CPXCENVptr env, CPXLPptr lp, char const *probname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgprobtype) (CPXCENVptr env, CPXLPptr lp, int type) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgprobtypesolnpool) (CPXCENVptr env, CPXLPptr lp, int type, int soln) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgrhs) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgrngval) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgrowname) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char **newname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgsense) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *sense) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcleanup) (CPXCENVptr env, CPXLPptr lp, double eps) = NULL;
CPXLIBAPI CPXLPptr CPXPUBLIC (*native_CPXcloneprob) (CPXCENVptr env, CPXCLPptr lp, int *status_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcloseCPLEX) (CPXENVptr * env_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXclpwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcompletelp) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopybase) (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopybasednorms) (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *dnorm) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopydnorms) (CPXCENVptr env, CPXLPptr lp, double const *norm, int const *head, int len) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopylp) (CPXCENVptr env, CPXLPptr lp, int numcols, int numrows, int objsense, double const *objective, double const *rhs, char const *sense, int const *matbeg, int const *matcnt, int const *matind, double const *matval, double const *lb, double const *ub, double const *rngval) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopylpwnames) (CPXCENVptr env, CPXLPptr lp, int numcols, int numrows, int objsense, double const *objective, double const *rhs, char const *sense, int const *matbeg, int const *matcnt, int const *matind, double const *matval, double const *lb, double const *ub, double const *rngval, char **colname, char **rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopynettolp) (CPXCENVptr env, CPXLPptr lp, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopyobjname) (CPXCENVptr env, CPXLPptr lp, char const *objname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopypartialbase) (CPXCENVptr env, CPXLPptr lp, int ccnt, int const *cindices, int const *cstat, int rcnt, int const *rindices, int const *rstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopypnorms) (CPXCENVptr env, CPXLPptr lp, double const *cnorm, double const *rnorm, int len) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopyprotected) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopystart) (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *cprim, double const *rprim, double const *cdual, double const *rdual) = NULL;
CPXLIBAPI CPXLPptr CPXPUBLIC (*native_CPXcreateprob) (CPXCENVptr env, int *status_p, char const *probname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcrushform) (CPXCENVptr env, CPXCLPptr lp, int len, int const *ind, double const *val, int *plen_p, double *poffset_p, int *pind, double *pval) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcrushpi) (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *prepi) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcrushx) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *prex) = NULL;
int CPXPUBLIC (*native_CPXdelchannel) (CPXENVptr env, CPXCHANNELptr * channel_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelcols) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelfpdest) (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelfuncdest) (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, const char *)) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelnames) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelrows) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelsetcols) (CPXCENVptr env, CPXLPptr lp, int *delstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelsetrows) (CPXCENVptr env, CPXLPptr lp, int *delstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdeserializercreate) (CPXDESERIALIZERptr * deser_p, CPXLONG size, void const *buffer) = NULL;
CPXLIBAPI void CPXPUBLIC (*native_CPXdeserializerdestroy) (CPXDESERIALIZERptr deser) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXdeserializerleft) (CPXCDESERIALIZERptr deser) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdisconnectchannel) (CPXCENVptr env, CPXCHANNELptr channel) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdjfrompi) (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *dj) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdperwrite) (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdratio) (CPXCENVptr env, CPXLPptr lp, int *indices, int cnt, double *downratio, double *upratio, int *downenter, int *upenter, int *downstatus, int *upstatus) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdualfarkas) (CPXCENVptr env, CPXCLPptr lp, double *y, double *proof_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdualopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdualwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, double *objshift_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXembwrite) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXfclose) (CPXFILEptr stream) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXfeasopt) (CPXCENVptr env, CPXLPptr lp, double const *rhs, double const *rng, double const *lb, double const *ub) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXfeasoptext) (CPXCENVptr env, CPXLPptr lp, int grpcnt, int concnt, double const *grppref, int const *grpbeg, int const *grpind, char const *grptype) = NULL;
CPXLIBAPI void CPXPUBLIC (*native_CPXfinalize) (void) = NULL;
int CPXPUBLIC (*native_CPXfindiis) (CPXCENVptr env, CPXLPptr lp, int *iisnumrows_p, int *iisnumcols_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXflushchannel) (CPXCENVptr env, CPXCHANNELptr channel) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXflushstdchannels) (CPXCENVptr env) = NULL;
CPXLIBAPI CPXFILEptr CPXPUBLIC (*native_CPXfopen) (char const *filename_str, char const *type_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXfputs) (char const *s_str, CPXFILEptr stream) = NULL;
void CPXPUBLIC (*native_CPXfree) (void *ptr) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXfreeparenv) (CPXENVptr env, CPXENVptr * child_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXfreepresolve) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXfreeprob) (CPXCENVptr env, CPXLPptr * lp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXftran) (CPXCENVptr env, CPXCLPptr lp, double *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetax) (CPXCENVptr env, CPXCLPptr lp, double *x, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetbaritcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetbase) (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetbasednorms) (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat, double *dnorm) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetbhead) (CPXCENVptr env, CPXCLPptr lp, int *head, double *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbackinfo) (CPXCENVptr env, void *cbdata, int wherefrom, int whichinfo, void *result_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetchannels) (CPXCENVptr env, CPXCHANNELptr * cpxresults_p, CPXCHANNELptr * cpxwarning_p, CPXCHANNELptr * cpxerror_p, CPXCHANNELptr * cpxlog_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetchgparam) (CPXCENVptr env, int *cnt_p, int *paramnum, int pspace, int *surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcoef) (CPXCENVptr env, CPXCLPptr lp, int i, int j, double *coef_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcolindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcolinfeas) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcolname) (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, int storespace, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcols) (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *cmatbeg, int *cmatind, double *cmatval, int cmatspace, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetconflict) (CPXCENVptr env, CPXCLPptr lp, int *confstat_p, int *rowind, int *rowbdstat, int *confnumrows_p, int *colind, int *colbdstat, int *confnumcols_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetconflictext) (CPXCENVptr env, CPXCLPptr lp, int *grpstat, int beg, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcrossdexchcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcrossdpushcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcrosspexchcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcrossppushcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetdblparam) (CPXCENVptr env, int whichparam, double *value_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetdblquality) (CPXCENVptr env, CPXCLPptr lp, double *quality_p, int what) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetdettime) (CPXCENVptr env, double *dettimestamp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetdj) (CPXCENVptr env, CPXCLPptr lp, double *dj, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetdnorms) (CPXCENVptr env, CPXCLPptr lp, double *norm, int *head, int *len_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetdsbcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXCCHARptr CPXPUBLIC (*native_CPXgeterrorstring) (CPXCENVptr env, int errcode, char *buffer_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetgrad) (CPXCENVptr env, CPXCLPptr lp, int j, int *head, double *y) = NULL;
int CPXPUBLIC (*native_CPXgetiis) (CPXCENVptr env, CPXCLPptr lp, int *iisstat_p, int *rowind, int *rowbdstat, int *iisnumrows_p, int *colind, int *colbdstat, int *iisnumcols_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetijdiv) (CPXCENVptr env, CPXCLPptr lp, int *idiv_p, int *jdiv_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetijrow) (CPXCENVptr env, CPXCLPptr lp, int i, int j, int *row_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetintparam) (CPXCENVptr env, int whichparam, CPXINT * value_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetintquality) (CPXCENVptr env, CPXCLPptr lp, int *quality_p, int what) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetitcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetlb) (CPXCENVptr env, CPXCLPptr lp, double *lb, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetlogfile) (CPXCENVptr env, CPXFILEptr * logfile_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetlongparam) (CPXCENVptr env, int whichparam, CPXLONG * value_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetlpcallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetmethod) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnetcallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumcols) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumcores) (CPXCENVptr env, int *numcores_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumnz) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumrows) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetobj) (CPXCENVptr env, CPXCLPptr lp, double *obj, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetobjname) (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetobjoffset) (CPXCENVptr env, CPXCLPptr lp, double *objoffset_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetobjsen) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetobjval) (CPXCENVptr env, CPXCLPptr lp, double *objval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetparamname) (CPXCENVptr env, int whichparam, char *name_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetparamnum) (CPXCENVptr env, char const *name_str, int *whichparam_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetparamtype) (CPXCENVptr env, int whichparam, int *paramtype) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetphase1cnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetpi) (CPXCENVptr env, CPXCLPptr lp, double *pi, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetpnorms) (CPXCENVptr env, CPXCLPptr lp, double *cnorm, double *rnorm, int *len_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetprestat) (CPXCENVptr env, CPXCLPptr lp, int *prestat_p, int *pcstat, int *prstat, int *ocstat, int *orstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetprobname) (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetprobtype) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetprotected) (CPXCENVptr env, CPXCLPptr lp, int *cnt_p, int *indices, int pspace, int *surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetpsbcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetray) (CPXCENVptr env, CPXCLPptr lp, double *z) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetredlp) (CPXCENVptr env, CPXCLPptr lp, CPXCLPptr * redlp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetrhs) (CPXCENVptr env, CPXCLPptr lp, double *rhs, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetrngval) (CPXCENVptr env, CPXCLPptr lp, double *rngval, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetrowindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetrowinfeas) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetrowname) (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, int storespace, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetrows) (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *rmatbeg, int *rmatind, double *rmatval, int rmatspace, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsense) (CPXCENVptr env, CPXCLPptr lp, char *sense, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsiftitcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsiftphase1cnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetslack) (CPXCENVptr env, CPXCLPptr lp, double *slack, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpooldblquality) (CPXCENVptr env, CPXCLPptr lp, int soln, double *quality_p, int what) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolintquality) (CPXCENVptr env, CPXCLPptr lp, int soln, int *quality_p, int what) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetstat) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXCHARptr CPXPUBLIC (*native_CPXgetstatstring) (CPXCENVptr env, int statind, char *buffer_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetstrparam) (CPXCENVptr env, int whichparam, char *value_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgettime) (CPXCENVptr env, double *timestamp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgettuningcallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetub) (CPXCENVptr env, CPXCLPptr lp, double *ub, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetweight) (CPXCENVptr env, CPXCLPptr lp, int rcnt, int const *rmatbeg, int const *rmatind, double const *rmatval, double *weight, int dpriind) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetx) (CPXCENVptr env, CPXCLPptr lp, double *x, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXhybnetopt) (CPXCENVptr env, CPXLPptr lp, int method) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXinfodblparam) (CPXCENVptr env, int whichparam, double *defvalue_p, double *minvalue_p, double *maxvalue_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXinfointparam) (CPXCENVptr env, int whichparam, CPXINT * defvalue_p, CPXINT * minvalue_p, CPXINT * maxvalue_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXinfolongparam) (CPXCENVptr env, int whichparam, CPXLONG * defvalue_p, CPXLONG * minvalue_p, CPXLONG * maxvalue_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXinfostrparam) (CPXCENVptr env, int whichparam, char *defvalue_str) = NULL;
CPXLIBAPI void CPXPUBLIC (*native_CPXinitialize) (void) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXkilldnorms) (CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXkillpnorms) (CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXlpopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXlprewrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXlpwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXVOIDptr CPXPUBLIC (*native_CPXmalloc) (size_t size) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXmbasewrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXmdleave) (CPXCENVptr env, CPXLPptr lp, int const *indices, int cnt, double *downratio, double *upratio) = NULL;
CPXVOIDptr CPXPUBLIC (*native_CPXmemcpy) (void *s1, void *s2, size_t n) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXmpsrewrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXmpswrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBVARARGS (*native_CPXmsg) (CPXCHANNELptr channel, char const *format, ...) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXmsgstr) (CPXCHANNELptr channel, char const *msg_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETextract) (CPXCENVptr env, CPXNETptr net, CPXCLPptr lp, int *colmap, int *rowmap) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXnewcols) (CPXCENVptr env, CPXLPptr lp, int ccnt, double const *obj, double const *lb, double const *ub, char const *xctype, char **colname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXnewrows) (CPXCENVptr env, CPXLPptr lp, int rcnt, double const *rhs, char const *sense, double const *rngval, char **rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXobjsa) (CPXCENVptr env, CPXCLPptr lp, int begin, int end, double *lower, double *upper) = NULL;
CPXLIBAPI CPXENVptr CPXPUBLIC (*native_CPXopenCPLEX) (int *status_p) = NULL;
CPXLIBAPI CPXENVptr CPXPUBLIC (*native_CPXopenCPLEXruntime) (int *status_p, int serialnum, char const *licenvstring_str) = NULL;
CPXLIBAPI CPXENVptr CPXPUBLIC (*native_CPXparenv) (CPXENVptr env, int *status_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXpivot) (CPXCENVptr env, CPXLPptr lp, int jenter, int jleave, int leavestat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXpivotin) (CPXCENVptr env, CPXLPptr lp, int const *rlist, int rlen) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXpivotout) (CPXCENVptr env, CPXLPptr lp, int const *clist, int clen) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXpperwrite) (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXpratio) (CPXCENVptr env, CPXLPptr lp, int *indices, int cnt, double *downratio, double *upratio, int *downleave, int *upleave, int *downleavestatus, int *upleavestatus, int *downstatus, int *upstatus) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXpreaddrows) (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXprechgobj) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXpreslvwrite) (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double *objoff_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXpresolve) (CPXCENVptr env, CPXLPptr lp, int method) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXprimopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
int CPXPUBLIC (*native_CPXputenv) (char *envsetting_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXqpdjfrompi) (CPXCENVptr env, CPXCLPptr lp, double const *pi, double const *x, double *dj) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXqpuncrushpi) (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi, double const *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXreadcopybase) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXreadcopyparam) (CPXENVptr env, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXreadcopyprob) (CPXCENVptr env, CPXLPptr lp, char const *filename_str, char const *filetype_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXreadcopysol) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
int CPXPUBLIC (*native_CPXreadcopyvec) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXVOIDptr CPXPUBLIC (*native_CPXrealloc) (void *ptr, size_t size) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXrefineconflict) (CPXCENVptr env, CPXLPptr lp, int *confnumrows_p, int *confnumcols_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXrefineconflictext) (CPXCENVptr env, CPXLPptr lp, int grpcnt, int concnt, double const *grppref, int const *grpbeg, int const *grpind, char const *grptype) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXrhssa) (CPXCENVptr env, CPXCLPptr lp, int begin, int end, double *lower, double *upper) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXrobustopt) (CPXCENVptr env, CPXLPptr lp, CPXLPptr lblp, CPXLPptr ublp, double objchg, double const *maxchg) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsavwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXserializercreate) (CPXSERIALIZERptr * ser_p) = NULL;
CPXLIBAPI void CPXPUBLIC (*native_CPXserializerdestroy) (CPXSERIALIZERptr ser) = NULL;
CPXLIBAPI CPXLONG CPXPUBLIC (*native_CPXserializerlength) (CPXCSERIALIZERptr ser) = NULL;
CPXLIBAPI void const *CPXPUBLIC (*native_CPXserializerpayload) (CPXCSERIALIZERptr ser) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetdblparam) (CPXENVptr env, int whichparam, double newvalue) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetdefaults) (CPXENVptr env) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetintparam) (CPXENVptr env, int whichparam, CPXINT newvalue) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetlogfile) (CPXENVptr env, CPXFILEptr lfile) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetlongparam) (CPXENVptr env, int whichparam, CPXLONG newvalue) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetlpcallbackfunc) (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetnetcallbackfunc) (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetphase2) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetprofcallbackfunc) (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, int, void *), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetstrparam) (CPXENVptr env, int whichparam, char const *newvalue_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetterminate) (CPXENVptr env, volatile int *terminate_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsettuningcallbackfunc) (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsiftopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXslackfromx) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *slack) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsolninfo) (CPXCENVptr env, CPXCLPptr lp, int *solnmethod_p, int *solntype_p, int *pfeasind_p, int *dfeasind_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsolution) (CPXCENVptr env, CPXCLPptr lp, int *lpstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsolwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsolwritesolnpool) (CPXCENVptr env, CPXCLPptr lp, int soln, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsolwritesolnpoolall) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXCHARptr CPXPUBLIC (*native_CPXstrcpy) (char *dest_str, char const *src_str) = NULL;
size_t CPXPUBLIC (*native_CPXstrlen) (char const *s_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXstrongbranch) (CPXCENVptr env, CPXLPptr lp, int const *indices, int cnt, double *downobj, double *upobj, int itlim) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXtightenbds) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *lu, double const *bd) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXtuneparam) (CPXENVptr env, CPXLPptr lp, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char **strval, int *tunestat_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXtuneparamprobset) (CPXENVptr env, int filecnt, char **filename, char **filetype, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char **strval, int *tunestat_p) = NULL;
int CPXPUBLIC (*native_CPXtxtsolwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXuncrushform) (CPXCENVptr env, CPXCLPptr lp, int plen, int const *pind, double const *pval, int *len_p, double *offset_p, int *ind, double *val) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXuncrushpi) (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXuncrushx) (CPXCENVptr env, CPXCLPptr lp, double *x, double const *prex) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXunscaleprob) (CPXCENVptr env, CPXLPptr lp) = NULL;
int CPXPUBLIC (*native_CPXvecwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI CPXCCHARptr CPXPUBLIC (*native_CPXversion) (CPXCENVptr env) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXversionnumber) (CPXCENVptr env, int *version_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXwriteparam) (CPXCENVptr env, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXwriteprob) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, char const *filetype_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbaropt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXhybbaropt) (CPXCENVptr env, CPXLPptr lp, int method) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddlazyconstraints) (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddmipstarts) (CPXCENVptr env, CPXLPptr lp, int mcnt, int nzcnt, int const *beg, int const *varindices, double const *values, int const *effortlevel, char **mipstartname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddsolnpooldivfilter) (CPXCENVptr env, CPXLPptr lp, double lower_bound, double upper_bound, int nzcnt, int const *ind, double const *weight, double const *refval, char const *lname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddsolnpoolrngfilter) (CPXCENVptr env, CPXLPptr lp, double lb, double ub, int nzcnt, int const *ind, double const *val, char const *lname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddsos) (CPXCENVptr env, CPXLPptr lp, int numsos, int numsosnz, char const *sostype, int const *sosbeg, int const *sosind, double const *soswt, char **sosname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddusercuts) (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **rowname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbranchcallbackbranchasCPLEX) (CPXCENVptr env, void *cbdata, int wherefrom, int num, void *userhandle, int *seqnum_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbranchcallbackbranchbds) (CPXCENVptr env, void *cbdata, int wherefrom, int cnt, int const *indices, char const *lu, double const *bd, double nodeest, void *userhandle, int *seqnum_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbranchcallbackbranchconstraints) (CPXCENVptr env, void *cbdata, int wherefrom, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, double nodeest, void *userhandle, int *seqnum_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXbranchcallbackbranchgeneral) (CPXCENVptr env, void *cbdata, int wherefrom, int varcnt, int const *varind, char const *varlu, double const *varbd, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, double nodeest, void *userhandle, int *seqnum_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcallbacksetnodeuserhandle) (CPXCENVptr env, void *cbdata, int wherefrom, int nodeindex, void *userhandle, void **olduserhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcallbacksetuserhandle) (CPXCENVptr env, void *cbdata, int wherefrom, void *userhandle, void **olduserhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgctype) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *xctype) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgmipstarts) (CPXCENVptr env, CPXLPptr lp, int mcnt, int const *mipstartindices, int nzcnt, int const *beg, int const *varindices, double const *values, int const *effortlevel) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopyctype) (CPXCENVptr env, CPXLPptr lp, char const *xctype) = NULL;
int CPXPUBLIC (*native_CPXcopymipstart) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopyorder) (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, int const *priority, int const *direction) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopysos) (CPXCENVptr env, CPXLPptr lp, int numsos, int numsosnz, char const *sostype, int const *sosbeg, int const *sosind, double const *soswt, char **sosname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcutcallbackadd) (CPXCENVptr env, void *cbdata, int wherefrom, int nzcnt, double rhs, int sense, int const *cutind, double const *cutval, int purgeable) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcutcallbackaddlocal) (CPXCENVptr env, void *cbdata, int wherefrom, int nzcnt, double rhs, int sense, int const *cutind, double const *cutval) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelindconstrs) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelmipstarts) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelsetmipstarts) (CPXCENVptr env, CPXLPptr lp, int *delstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelsetsolnpoolfilters) (CPXCENVptr env, CPXLPptr lp, int *delstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelsetsolnpoolsolns) (CPXCENVptr env, CPXLPptr lp, int *delstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelsetsos) (CPXCENVptr env, CPXLPptr lp, int *delset) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelsolnpoolfilters) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelsolnpoolsolns) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelsos) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXfltwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXfreelazyconstraints) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXfreeusercuts) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetbestobjval) (CPXCENVptr env, CPXCLPptr lp, double *objval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetbranchcallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** branchcallback_p) (CALLBACK_BRANCH_ARGS), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetbranchnosolncallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** branchnosolncallback_p) (CALLBACK_BRANCH_ARGS), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbackbranchconstraints) (CPXCENVptr env, void *cbdata, int wherefrom, int which, int *cuts_p, int *nzcnt_p, double *rhs, char *sense, int *rmatbeg, int *rmatind, double *rmatval, int rmatsz, int *surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbackctype) (CPXCENVptr env, void *cbdata, int wherefrom, char *xctype, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbackgloballb) (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbackglobalub) (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbackincumbent) (CPXCENVptr env, void *cbdata, int wherefrom, double *x, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbackindicatorinfo) (CPXCENVptr env, void *cbdata, int wherefrom, int iindex, int whichinfo, void *result_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbacklp) (CPXCENVptr env, void *cbdata, int wherefrom, CPXCLPptr * lp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbacknodeinfo) (CPXCENVptr env, void *cbdata, int wherefrom, int nodeindex, int whichinfo, void *result_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbacknodeintfeas) (CPXCENVptr env, void *cbdata, int wherefrom, int *feas, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbacknodelb) (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbacknodelp) (CPXCENVptr env, void *cbdata, int wherefrom, CPXLPptr * nodelp_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbacknodeobjval) (CPXCENVptr env, void *cbdata, int wherefrom, double *objval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbacknodestat) (CPXCENVptr env, void *cbdata, int wherefrom, int *nodestat_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbacknodeub) (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbacknodex) (CPXCENVptr env, void *cbdata, int wherefrom, double *x, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbackorder) (CPXCENVptr env, void *cbdata, int wherefrom, int *priority, int *direction, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbackpseudocosts) (CPXCENVptr env, void *cbdata, int wherefrom, double *uppc, double *downpc, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbackseqinfo) (CPXCENVptr env, void *cbdata, int wherefrom, int seqid, int whichinfo, void *result_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcallbacksosinfo) (CPXCENVptr env, void *cbdata, int wherefrom, int sosindex, int member, int whichinfo, void *result_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetctype) (CPXCENVptr env, CPXCLPptr lp, char *xctype, int begin, int end) = NULL;
int CPXPUBLIC (*native_CPXgetcutcallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** cutcallback_p) (CALLBACK_CUT_ARGS), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetcutoff) (CPXCENVptr env, CPXCLPptr lp, double *cutoff_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetdeletenodecallbackfunc) (CPXCENVptr env, void (CPXPUBLIC ** deletecallback_p) (CALLBACK_DELETENODE_ARGS), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetheuristiccallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** heuristiccallback_p) (CALLBACK_HEURISTIC_ARGS), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetincumbentcallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** incumbentcallback_p) (CALLBACK_INCUMBENT_ARGS), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetindconstr) (CPXCENVptr env, CPXCLPptr lp, int *indvar_p, int *complemented_p, int *nzcnt_p, double *rhs_p, char *sense_p, int *linind, double *linval, int space, int *surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetindconstrindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetindconstrinfeas) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetindconstrname) (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetindconstrslack) (CPXCENVptr env, CPXCLPptr lp, double *indslack, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetinfocallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetlazyconstraintcallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** cutcallback_p) (CALLBACK_CUT_ARGS), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetmipcallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetmipitcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetmiprelgap) (CPXCENVptr env, CPXCLPptr lp, double *gap_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetmipstartindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetmipstartname) (CPXCENVptr env, CPXCLPptr lp, char **name, char *store, int storesz, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetmipstarts) (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *beg, int *varindices, double *values, int *effortlevel, int startspace, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnodecallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** nodecallback_p) (CALLBACK_NODE_ARGS), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnodecnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnodeint) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnodeleftcnt) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumbin) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumcuts) (CPXCENVptr env, CPXCLPptr lp, int cuttype, int *num_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumindconstrs) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumint) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumlazyconstraints) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnummipstarts) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumsemicont) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumsemiint) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumsos) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumusercuts) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetorder) (CPXCENVptr env, CPXCLPptr lp, int *cnt_p, int *indices, int *priority, int *direction, int ordspace, int *surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpooldivfilter) (CPXCENVptr env, CPXCLPptr lp, double *lower_cutoff_p, double *upper_cutoff_p, int *nzcnt_p, int *ind, double *val, double *refval, int space, int *surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolfilterindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolfiltername) (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolfiltertype) (CPXCENVptr env, CPXCLPptr lp, int *ftype_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolmeanobjval) (CPXCENVptr env, CPXCLPptr lp, double *meanobjval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolnumfilters) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolnumreplaced) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolnumsolns) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolobjval) (CPXCENVptr env, CPXCLPptr lp, int soln, double *objval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolqconstrslack) (CPXCENVptr env, CPXCLPptr lp, int soln, double *qcslack, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolrngfilter) (CPXCENVptr env, CPXCLPptr lp, double *lb_p, double *ub_p, int *nzcnt_p, int *ind, double *val, int space, int *surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolslack) (CPXCENVptr env, CPXCLPptr lp, int soln, double *slack, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolsolnindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolsolnname) (CPXCENVptr env, CPXCLPptr lp, char *store, int storesz, int *surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolnpoolx) (CPXCENVptr env, CPXCLPptr lp, int soln, double *x, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsolvecallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** solvecallback_p) (CALLBACK_SOLVE_ARGS), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsos) (CPXCENVptr env, CPXCLPptr lp, int *numsosnz_p, char *sostype, int *sosbeg, int *sosind, double *soswt, int sosspace, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsosindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsosinfeas) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsosname) (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, int storespace, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsubmethod) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetsubstat) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetusercutcallbackfunc) (CPXCENVptr env, int (CPXPUBLIC ** cutcallback_p) (CALLBACK_CUT_ARGS), void **cbhandle_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXindconstrslackfromx) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *indslack) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXmipopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXordread) (CPXCENVptr env, char const *filename_str, int numcols, char **colname, int *cnt_p, int *indices, int *priority, int *direction) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXordwrite) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXpopulate) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXreadcopymipstarts) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXreadcopyorder) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXreadcopysolnpoolfilters) (CPXCENVptr env, CPXLPptr lp, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXrefinemipstartconflict) (CPXCENVptr env, CPXLPptr lp, int mipstartindex, int *confnumrows_p, int *confnumcols_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXrefinemipstartconflictext) (CPXCENVptr env, CPXLPptr lp, int mipstartindex, int grpcnt, int concnt, double const *grppref, int const *grpbeg, int const *grpind, char const *grptype) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetbranchcallbackfunc) (CPXENVptr env, int (CPXPUBLIC * branchcallback) (CALLBACK_BRANCH_ARGS), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetbranchnosolncallbackfunc) (CPXENVptr env, int (CPXPUBLIC * branchnosolncallback) (CALLBACK_BRANCH_ARGS), void *cbhandle) = NULL;
int CPXPUBLIC (*native_CPXsetcutcallbackfunc) (CPXENVptr env, int (CPXPUBLIC * cutcallback) (CALLBACK_CUT_ARGS), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetdeletenodecallbackfunc) (CPXENVptr env, void (CPXPUBLIC * deletecallback) (CALLBACK_DELETENODE_ARGS), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetheuristiccallbackfunc) (CPXENVptr env, int (CPXPUBLIC * heuristiccallback) (CALLBACK_HEURISTIC_ARGS), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetincumbentcallbackfunc) (CPXENVptr env, int (CPXPUBLIC * incumbentcallback) (CALLBACK_INCUMBENT_ARGS), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetinfocallbackfunc) (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetlazyconstraintcallbackfunc) (CPXENVptr env, int (CPXPUBLIC * lazyconcallback) (CALLBACK_CUT_ARGS), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetmipcallbackfunc) (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetnodecallbackfunc) (CPXENVptr env, int (CPXPUBLIC * nodecallback) (CALLBACK_NODE_ARGS), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetsolvecallbackfunc) (CPXENVptr env, int (CPXPUBLIC * solvecallback) (CALLBACK_SOLVE_ARGS), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXsetusercutcallbackfunc) (CPXENVptr env, int (CPXPUBLIC * cutcallback) (CALLBACK_CUT_ARGS), void *cbhandle) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXwritemipstarts) (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddindconstr) (CPXCENVptr env, CPXLPptr lp, int indvar, int complemented, int nzcnt, double rhs, int sense, int const *linind, double const *linval, char const *indname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETaddarcs) (CPXCENVptr env, CPXNETptr net, int narcs, int const *fromnode, int const *tonode, double const *low, double const *up, double const *obj, char **anames) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETaddnodes) (CPXCENVptr env, CPXNETptr net, int nnodes, double const *supply, char **name) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETbasewrite) (CPXCENVptr env, CPXCNETptr net, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETchgarcname) (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, char **newname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETchgarcnodes) (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, int const *fromnode, int const *tonode) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETchgbds) (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, char const *lu, double const *bd) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETchgname) (CPXCENVptr env, CPXNETptr net, int key, int vindex, char const *name_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETchgnodename) (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, char **newname) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETchgobj) (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, double const *obj) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETchgobjsen) (CPXCENVptr env, CPXNETptr net, int maxormin) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETchgsupply) (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, double const *supply) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETcopybase) (CPXCENVptr env, CPXNETptr net, int const *astat, int const *nstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETcopynet) (CPXCENVptr env, CPXNETptr net, int objsen, int nnodes, double const *supply, char **nnames, int narcs, int const *fromnode, int const *tonode, double const *low, double const *up, double const *obj, char **anames) = NULL;
CPXLIBAPI CPXNETptr CPXPUBLIC (*native_CPXNETcreateprob) (CPXENVptr env, int *status_p, char const *name_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETdelarcs) (CPXCENVptr env, CPXNETptr net, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETdelnodes) (CPXCENVptr env, CPXNETptr net, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETdelset) (CPXCENVptr env, CPXNETptr net, int *whichnodes, int *whicharcs) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETfreeprob) (CPXENVptr env, CPXNETptr * net_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetarcindex) (CPXCENVptr env, CPXCNETptr net, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetarcname) (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, int namespc, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetarcnodes) (CPXCENVptr env, CPXCNETptr net, int *fromnode, int *tonode, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetbase) (CPXCENVptr env, CPXCNETptr net, int *astat, int *nstat) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetdj) (CPXCENVptr env, CPXCNETptr net, double *dj, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetitcnt) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetlb) (CPXCENVptr env, CPXCNETptr net, double *low, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetnodearcs) (CPXCENVptr env, CPXCNETptr net, int *arccnt_p, int *arcbeg, int *arc, int arcspace, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetnodeindex) (CPXCENVptr env, CPXCNETptr net, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetnodename) (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, int namespc, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetnumarcs) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetnumnodes) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetobj) (CPXCENVptr env, CPXCNETptr net, double *obj, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetobjsen) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetobjval) (CPXCENVptr env, CPXCNETptr net, double *objval_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetphase1cnt) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetpi) (CPXCENVptr env, CPXCNETptr net, double *pi, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetprobname) (CPXCENVptr env, CPXCNETptr net, char *buf_str, int bufspace, int *surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetslack) (CPXCENVptr env, CPXCNETptr net, double *slack, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetstat) (CPXCENVptr env, CPXCNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetsupply) (CPXCENVptr env, CPXCNETptr net, double *supply, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetub) (CPXCENVptr env, CPXCNETptr net, double *up, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETgetx) (CPXCENVptr env, CPXCNETptr net, double *x, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETprimopt) (CPXCENVptr env, CPXNETptr net) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETreadcopybase) (CPXCENVptr env, CPXNETptr net, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETreadcopyprob) (CPXCENVptr env, CPXNETptr net, char const *filename_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETsolninfo) (CPXCENVptr env, CPXCNETptr net, int *pfeasind_p, int *dfeasind_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETsolution) (CPXCENVptr env, CPXCNETptr net, int *netstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXNETwriteprob) (CPXCENVptr env, CPXCNETptr net, char const *filename_str, char const *format_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXchgqpcoef) (CPXCENVptr env, CPXLPptr lp, int i, int j, double newvalue) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopyqpsep) (CPXCENVptr env, CPXLPptr lp, double const *qsepvec) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXcopyquad) (CPXCENVptr env, CPXLPptr lp, int const *qmatbeg, int const *qmatcnt, int const *qmatind, double const *qmatval) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumqpnz) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumquad) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetqpcoef) (CPXCENVptr env, CPXCLPptr lp, int rownum, int colnum, double *coef_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetquad) (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *qmatbeg, int *qmatind, double *qmatval, int qmatspace, int *surplus_p, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXqpindefcertificate) (CPXCENVptr env, CPXCLPptr lp, double *x) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXqpopt) (CPXCENVptr env, CPXLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXaddqconstr) (CPXCENVptr env, CPXLPptr lp, int linnzcnt, int quadnzcnt, double rhs, int sense, int const *linind, double const *linval, int const *quadrow, int const *quadcol, double const *quadval, char const *lname_str) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXdelqconstrs) (CPXCENVptr env, CPXLPptr lp, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetnumqconstrs) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetqconstr) (CPXCENVptr env, CPXCLPptr lp, int *linnzcnt_p, int *quadnzcnt_p, double *rhs_p, char *sense_p, int *linind, double *linval, int linspace, int *linsurplus_p, int *quadrow, int *quadcol, double *quadval, int quadspace, int *quadsurplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetqconstrdslack) (CPXCENVptr env, CPXCLPptr lp, int qind, int *nz_p, int *ind, double *val, int space, int *surplus_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetqconstrindex) (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetqconstrinfeas) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetqconstrname) (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p, int which) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetqconstrslack) (CPXCENVptr env, CPXCLPptr lp, double *qcslack, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXgetxqxax) (CPXCENVptr env, CPXCLPptr lp, double *xqxax, int begin, int end) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXqconstrslackfromx) (CPXCENVptr env, CPXCLPptr lp, double const *x, double *qcslack) = NULL;
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
CPXLIBAPI int CPXPUBLIC (*native_CPXLchgobjoffset) (CPXCENVptr env, CPXLPptr lp, double offset) = NULL;
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
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumlazyconstraints) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI int CPXPUBLIC (*native_CPXLgetnummipstarts) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumsemicont) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumsemiint) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumsos) (CPXCENVptr env, CPXCLPptr lp) = NULL;
CPXLIBAPI CPXINT CPXPUBLIC (*native_CPXLgetnumusercuts) (CPXCENVptr env, CPXCLPptr lp) = NULL;
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

CPXCHANNELptr CPXPUBLIC CPXaddchannel (CPXENVptr env){
    if (!native_CPXaddchannel)
        native_CPXaddchannel = load_symbol("CPXaddchannel");
    return native_CPXaddchannel(env);
}
CPXLIBAPI int CPXPUBLIC CPXaddcols (CPXCENVptr env, CPXLPptr lp, int ccnt, int nzcnt, double const *obj, int const *cmatbeg, int const *cmatind, double const *cmatval, double const *lb, double const *ub, char **colname){
    if (!native_CPXaddcols)
        native_CPXaddcols = load_symbol("CPXaddcols");
    return native_CPXaddcols(env, lp, ccnt, nzcnt, obj, cmatbeg, cmatind, cmatval, lb, ub, colname);
}
CPXLIBAPI int CPXPUBLIC CPXaddfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr){
    if (!native_CPXaddfpdest)
        native_CPXaddfpdest = load_symbol("CPXaddfpdest");
    return native_CPXaddfpdest(env, channel, fileptr);
}
CPXLIBAPI int CPXPUBLIC CPXaddfuncdest (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, const char *)){
    if (!native_CPXaddfuncdest)
        native_CPXaddfuncdest = load_symbol("CPXaddfuncdest");
    return native_CPXaddfuncdest(env, channel, handle, msgfunction);
}
CPXLIBAPI int CPXPUBLIC CPXaddrows (CPXCENVptr env, CPXLPptr lp, int ccnt, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **colname, char **rowname){
    if (!native_CPXaddrows)
        native_CPXaddrows = load_symbol("CPXaddrows");
    return native_CPXaddrows(env, lp, ccnt, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, colname, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXbasicpresolve (CPXCENVptr env, CPXLPptr lp, double *redlb, double *redub, int *rstat){
    if (!native_CPXbasicpresolve)
        native_CPXbasicpresolve = load_symbol("CPXbasicpresolve");
    return native_CPXbasicpresolve(env, lp, redlb, redub, rstat);
}
CPXLIBAPI int CPXPUBLIC CPXbinvacol (CPXCENVptr env, CPXCLPptr lp, int j, double *x){
    if (!native_CPXbinvacol)
        native_CPXbinvacol = load_symbol("CPXbinvacol");
    return native_CPXbinvacol(env, lp, j, x);
}
CPXLIBAPI int CPXPUBLIC CPXbinvarow (CPXCENVptr env, CPXCLPptr lp, int i, double *z){
    if (!native_CPXbinvarow)
        native_CPXbinvarow = load_symbol("CPXbinvarow");
    return native_CPXbinvarow(env, lp, i, z);
}
CPXLIBAPI int CPXPUBLIC CPXbinvcol (CPXCENVptr env, CPXCLPptr lp, int j, double *x){
    if (!native_CPXbinvcol)
        native_CPXbinvcol = load_symbol("CPXbinvcol");
    return native_CPXbinvcol(env, lp, j, x);
}
CPXLIBAPI int CPXPUBLIC CPXbinvrow (CPXCENVptr env, CPXCLPptr lp, int i, double *y){
    if (!native_CPXbinvrow)
        native_CPXbinvrow = load_symbol("CPXbinvrow");
    return native_CPXbinvrow(env, lp, i, y);
}
CPXLIBAPI int CPXPUBLIC CPXboundsa (CPXCENVptr env, CPXCLPptr lp, int begin, int end, double *lblower, double *lbupper, double *ublower, double *ubupper){
    if (!native_CPXboundsa)
        native_CPXboundsa = load_symbol("CPXboundsa");
    return native_CPXboundsa(env, lp, begin, end, lblower, lbupper, ublower, ubupper);
}
CPXLIBAPI int CPXPUBLIC CPXbtran (CPXCENVptr env, CPXCLPptr lp, double *y){
    if (!native_CPXbtran)
        native_CPXbtran = load_symbol("CPXbtran");
    return native_CPXbtran(env, lp, y);
}
CPXLIBAPI int CPXPUBLIC CPXcheckdfeas (CPXCENVptr env, CPXLPptr lp, int *infeas_p){
    if (!native_CPXcheckdfeas)
        native_CPXcheckdfeas = load_symbol("CPXcheckdfeas");
    return native_CPXcheckdfeas(env, lp, infeas_p);
}
CPXLIBAPI int CPXPUBLIC CPXcheckpfeas (CPXCENVptr env, CPXLPptr lp, int *infeas_p){
    if (!native_CPXcheckpfeas)
        native_CPXcheckpfeas = load_symbol("CPXcheckpfeas");
    return native_CPXcheckpfeas(env, lp, infeas_p);
}
CPXLIBAPI int CPXPUBLIC CPXchecksoln (CPXCENVptr env, CPXLPptr lp, int *lpstatus_p){
    if (!native_CPXchecksoln)
        native_CPXchecksoln = load_symbol("CPXchecksoln");
    return native_CPXchecksoln(env, lp, lpstatus_p);
}
CPXLIBAPI int CPXPUBLIC CPXchgbds (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *lu, double const *bd){
    if (!native_CPXchgbds)
        native_CPXchgbds = load_symbol("CPXchgbds");
    return native_CPXchgbds(env, lp, cnt, indices, lu, bd);
}
CPXLIBAPI int CPXPUBLIC CPXchgcoef (CPXCENVptr env, CPXLPptr lp, int i, int j, double newvalue){
    if (!native_CPXchgcoef)
        native_CPXchgcoef = load_symbol("CPXchgcoef");
    return native_CPXchgcoef(env, lp, i, j, newvalue);
}
CPXLIBAPI int CPXPUBLIC CPXchgcoeflist (CPXCENVptr env, CPXLPptr lp, int numcoefs, int const *rowlist, int const *collist, double const *vallist){
    if (!native_CPXchgcoeflist)
        native_CPXchgcoeflist = load_symbol("CPXchgcoeflist");
    return native_CPXchgcoeflist(env, lp, numcoefs, rowlist, collist, vallist);
}
CPXLIBAPI int CPXPUBLIC CPXchgcolname (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char **newname){
    if (!native_CPXchgcolname)
        native_CPXchgcolname = load_symbol("CPXchgcolname");
    return native_CPXchgcolname(env, lp, cnt, indices, newname);
}
CPXLIBAPI int CPXPUBLIC CPXchgname (CPXCENVptr env, CPXLPptr lp, int key, int ij, char const *newname_str){
    if (!native_CPXchgname)
        native_CPXchgname = load_symbol("CPXchgname");
    return native_CPXchgname(env, lp, key, ij, newname_str);
}
CPXLIBAPI int CPXPUBLIC CPXchgobj (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values){
    if (!native_CPXchgobj)
        native_CPXchgobj = load_symbol("CPXchgobj");
    return native_CPXchgobj(env, lp, cnt, indices, values);
}
CPXLIBAPI int CPXPUBLIC CPXchgobjoffset (CPXCENVptr env, CPXLPptr lp, double offset){
    if (!native_CPXchgobjoffset)
        native_CPXchgobjoffset = load_symbol("CPXchgobjoffset");
    return native_CPXchgobjoffset(env, lp, offset);
}
CPXLIBAPI int CPXPUBLIC CPXchgobjsen (CPXCENVptr env, CPXLPptr lp, int maxormin){
    if (!native_CPXchgobjsen)
        native_CPXchgobjsen = load_symbol("CPXchgobjsen");
    return native_CPXchgobjsen(env, lp, maxormin);
}
CPXLIBAPI int CPXPUBLIC CPXchgprobname (CPXCENVptr env, CPXLPptr lp, char const *probname){
    if (!native_CPXchgprobname)
        native_CPXchgprobname = load_symbol("CPXchgprobname");
    return native_CPXchgprobname(env, lp, probname);
}
CPXLIBAPI int CPXPUBLIC CPXchgprobtype (CPXCENVptr env, CPXLPptr lp, int type){
    if (!native_CPXchgprobtype)
        native_CPXchgprobtype = load_symbol("CPXchgprobtype");
    return native_CPXchgprobtype(env, lp, type);
}
CPXLIBAPI int CPXPUBLIC CPXchgprobtypesolnpool (CPXCENVptr env, CPXLPptr lp, int type, int soln){
    if (!native_CPXchgprobtypesolnpool)
        native_CPXchgprobtypesolnpool = load_symbol("CPXchgprobtypesolnpool");
    return native_CPXchgprobtypesolnpool(env, lp, type, soln);
}
CPXLIBAPI int CPXPUBLIC CPXchgrhs (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values){
    if (!native_CPXchgrhs)
        native_CPXchgrhs = load_symbol("CPXchgrhs");
    return native_CPXchgrhs(env, lp, cnt, indices, values);
}
CPXLIBAPI int CPXPUBLIC CPXchgrngval (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values){
    if (!native_CPXchgrngval)
        native_CPXchgrngval = load_symbol("CPXchgrngval");
    return native_CPXchgrngval(env, lp, cnt, indices, values);
}
CPXLIBAPI int CPXPUBLIC CPXchgrowname (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char **newname){
    if (!native_CPXchgrowname)
        native_CPXchgrowname = load_symbol("CPXchgrowname");
    return native_CPXchgrowname(env, lp, cnt, indices, newname);
}
CPXLIBAPI int CPXPUBLIC CPXchgsense (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *sense){
    if (!native_CPXchgsense)
        native_CPXchgsense = load_symbol("CPXchgsense");
    return native_CPXchgsense(env, lp, cnt, indices, sense);
}
CPXLIBAPI int CPXPUBLIC CPXcleanup (CPXCENVptr env, CPXLPptr lp, double eps){
    if (!native_CPXcleanup)
        native_CPXcleanup = load_symbol("CPXcleanup");
    return native_CPXcleanup(env, lp, eps);
}
CPXLIBAPI CPXLPptr CPXPUBLIC CPXcloneprob (CPXCENVptr env, CPXCLPptr lp, int *status_p){
    if (!native_CPXcloneprob)
        native_CPXcloneprob = load_symbol("CPXcloneprob");
    return native_CPXcloneprob(env, lp, status_p);
}
CPXLIBAPI int CPXPUBLIC CPXcloseCPLEX (CPXENVptr * env_p){
    if (!native_CPXcloseCPLEX)
        native_CPXcloseCPLEX = load_symbol("CPXcloseCPLEX");
    return native_CPXcloseCPLEX(env_p);
}
CPXLIBAPI int CPXPUBLIC CPXclpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXclpwrite)
        native_CPXclpwrite = load_symbol("CPXclpwrite");
    return native_CPXclpwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXcompletelp (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXcompletelp)
        native_CPXcompletelp = load_symbol("CPXcompletelp");
    return native_CPXcompletelp(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXcopybase (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat){
    if (!native_CPXcopybase)
        native_CPXcopybase = load_symbol("CPXcopybase");
    return native_CPXcopybase(env, lp, cstat, rstat);
}
CPXLIBAPI int CPXPUBLIC CPXcopybasednorms (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *dnorm){
    if (!native_CPXcopybasednorms)
        native_CPXcopybasednorms = load_symbol("CPXcopybasednorms");
    return native_CPXcopybasednorms(env, lp, cstat, rstat, dnorm);
}
CPXLIBAPI int CPXPUBLIC CPXcopydnorms (CPXCENVptr env, CPXLPptr lp, double const *norm, int const *head, int len){
    if (!native_CPXcopydnorms)
        native_CPXcopydnorms = load_symbol("CPXcopydnorms");
    return native_CPXcopydnorms(env, lp, norm, head, len);
}
CPXLIBAPI int CPXPUBLIC CPXcopylp (CPXCENVptr env, CPXLPptr lp, int numcols, int numrows, int objsense, double const *objective, double const *rhs, char const *sense, int const *matbeg, int const *matcnt, int const *matind, double const *matval, double const *lb, double const *ub, double const *rngval){
    if (!native_CPXcopylp)
        native_CPXcopylp = load_symbol("CPXcopylp");
    return native_CPXcopylp(env, lp, numcols, numrows, objsense, objective, rhs, sense, matbeg, matcnt, matind, matval, lb, ub, rngval);
}
CPXLIBAPI int CPXPUBLIC CPXcopylpwnames (CPXCENVptr env, CPXLPptr lp, int numcols, int numrows, int objsense, double const *objective, double const *rhs, char const *sense, int const *matbeg, int const *matcnt, int const *matind, double const *matval, double const *lb, double const *ub, double const *rngval, char **colname, char **rowname){
    if (!native_CPXcopylpwnames)
        native_CPXcopylpwnames = load_symbol("CPXcopylpwnames");
    return native_CPXcopylpwnames(env, lp, numcols, numrows, objsense, objective, rhs, sense, matbeg, matcnt, matind, matval, lb, ub, rngval, colname, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXcopynettolp (CPXCENVptr env, CPXLPptr lp, CPXCNETptr net){
    if (!native_CPXcopynettolp)
        native_CPXcopynettolp = load_symbol("CPXcopynettolp");
    return native_CPXcopynettolp(env, lp, net);
}
CPXLIBAPI int CPXPUBLIC CPXcopyobjname (CPXCENVptr env, CPXLPptr lp, char const *objname_str){
    if (!native_CPXcopyobjname)
        native_CPXcopyobjname = load_symbol("CPXcopyobjname");
    return native_CPXcopyobjname(env, lp, objname_str);
}
CPXLIBAPI int CPXPUBLIC CPXcopypartialbase (CPXCENVptr env, CPXLPptr lp, int ccnt, int const *cindices, int const *cstat, int rcnt, int const *rindices, int const *rstat){
    if (!native_CPXcopypartialbase)
        native_CPXcopypartialbase = load_symbol("CPXcopypartialbase");
    return native_CPXcopypartialbase(env, lp, ccnt, cindices, cstat, rcnt, rindices, rstat);
}
CPXLIBAPI int CPXPUBLIC CPXcopypnorms (CPXCENVptr env, CPXLPptr lp, double const *cnorm, double const *rnorm, int len){
    if (!native_CPXcopypnorms)
        native_CPXcopypnorms = load_symbol("CPXcopypnorms");
    return native_CPXcopypnorms(env, lp, cnorm, rnorm, len);
}
CPXLIBAPI int CPXPUBLIC CPXcopyprotected (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices){
    if (!native_CPXcopyprotected)
        native_CPXcopyprotected = load_symbol("CPXcopyprotected");
    return native_CPXcopyprotected(env, lp, cnt, indices);
}
CPXLIBAPI int CPXPUBLIC CPXcopystart (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *cprim, double const *rprim, double const *cdual, double const *rdual){
    if (!native_CPXcopystart)
        native_CPXcopystart = load_symbol("CPXcopystart");
    return native_CPXcopystart(env, lp, cstat, rstat, cprim, rprim, cdual, rdual);
}
CPXLIBAPI CPXLPptr CPXPUBLIC CPXcreateprob (CPXCENVptr env, int *status_p, char const *probname_str){
    if (!native_CPXcreateprob)
        native_CPXcreateprob = load_symbol("CPXcreateprob");
    return native_CPXcreateprob(env, status_p, probname_str);
}
CPXLIBAPI int CPXPUBLIC CPXcrushform (CPXCENVptr env, CPXCLPptr lp, int len, int const *ind, double const *val, int *plen_p, double *poffset_p, int *pind, double *pval){
    if (!native_CPXcrushform)
        native_CPXcrushform = load_symbol("CPXcrushform");
    return native_CPXcrushform(env, lp, len, ind, val, plen_p, poffset_p, pind, pval);
}
CPXLIBAPI int CPXPUBLIC CPXcrushpi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *prepi){
    if (!native_CPXcrushpi)
        native_CPXcrushpi = load_symbol("CPXcrushpi");
    return native_CPXcrushpi(env, lp, pi, prepi);
}
CPXLIBAPI int CPXPUBLIC CPXcrushx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *prex){
    if (!native_CPXcrushx)
        native_CPXcrushx = load_symbol("CPXcrushx");
    return native_CPXcrushx(env, lp, x, prex);
}
int CPXPUBLIC CPXdelchannel (CPXENVptr env, CPXCHANNELptr * channel_p){
    if (!native_CPXdelchannel)
        native_CPXdelchannel = load_symbol("CPXdelchannel");
    return native_CPXdelchannel(env, channel_p);
}
CPXLIBAPI int CPXPUBLIC CPXdelcols (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXdelcols)
        native_CPXdelcols = load_symbol("CPXdelcols");
    return native_CPXdelcols(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXdelfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr){
    if (!native_CPXdelfpdest)
        native_CPXdelfpdest = load_symbol("CPXdelfpdest");
    return native_CPXdelfpdest(env, channel, fileptr);
}
CPXLIBAPI int CPXPUBLIC CPXdelfuncdest (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, const char *)){
    if (!native_CPXdelfuncdest)
        native_CPXdelfuncdest = load_symbol("CPXdelfuncdest");
    return native_CPXdelfuncdest(env, channel, handle, msgfunction);
}
CPXLIBAPI int CPXPUBLIC CPXdelnames (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXdelnames)
        native_CPXdelnames = load_symbol("CPXdelnames");
    return native_CPXdelnames(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXdelrows (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXdelrows)
        native_CPXdelrows = load_symbol("CPXdelrows");
    return native_CPXdelrows(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXdelsetcols (CPXCENVptr env, CPXLPptr lp, int *delstat){
    if (!native_CPXdelsetcols)
        native_CPXdelsetcols = load_symbol("CPXdelsetcols");
    return native_CPXdelsetcols(env, lp, delstat);
}
CPXLIBAPI int CPXPUBLIC CPXdelsetrows (CPXCENVptr env, CPXLPptr lp, int *delstat){
    if (!native_CPXdelsetrows)
        native_CPXdelsetrows = load_symbol("CPXdelsetrows");
    return native_CPXdelsetrows(env, lp, delstat);
}
CPXLIBAPI int CPXPUBLIC CPXdeserializercreate (CPXDESERIALIZERptr * deser_p, CPXLONG size, void const *buffer){
    if (!native_CPXdeserializercreate)
        native_CPXdeserializercreate = load_symbol("CPXdeserializercreate");
    return native_CPXdeserializercreate(deser_p, size, buffer);
}
CPXLIBAPI void CPXPUBLIC CPXdeserializerdestroy (CPXDESERIALIZERptr deser){
    if (!native_CPXdeserializerdestroy)
        native_CPXdeserializerdestroy = load_symbol("CPXdeserializerdestroy");
    return native_CPXdeserializerdestroy(deser);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXdeserializerleft (CPXCDESERIALIZERptr deser){
    if (!native_CPXdeserializerleft)
        native_CPXdeserializerleft = load_symbol("CPXdeserializerleft");
    return native_CPXdeserializerleft(deser);
}
CPXLIBAPI int CPXPUBLIC CPXdisconnectchannel (CPXCENVptr env, CPXCHANNELptr channel){
    if (!native_CPXdisconnectchannel)
        native_CPXdisconnectchannel = load_symbol("CPXdisconnectchannel");
    return native_CPXdisconnectchannel(env, channel);
}
CPXLIBAPI int CPXPUBLIC CPXdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *dj){
    if (!native_CPXdjfrompi)
        native_CPXdjfrompi = load_symbol("CPXdjfrompi");
    return native_CPXdjfrompi(env, lp, pi, dj);
}
CPXLIBAPI int CPXPUBLIC CPXdperwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon){
    if (!native_CPXdperwrite)
        native_CPXdperwrite = load_symbol("CPXdperwrite");
    return native_CPXdperwrite(env, lp, filename_str, epsilon);
}
CPXLIBAPI int CPXPUBLIC CPXdratio (CPXCENVptr env, CPXLPptr lp, int *indices, int cnt, double *downratio, double *upratio, int *downenter, int *upenter, int *downstatus, int *upstatus){
    if (!native_CPXdratio)
        native_CPXdratio = load_symbol("CPXdratio");
    return native_CPXdratio(env, lp, indices, cnt, downratio, upratio, downenter, upenter, downstatus, upstatus);
}
CPXLIBAPI int CPXPUBLIC CPXdualfarkas (CPXCENVptr env, CPXCLPptr lp, double *y, double *proof_p){
    if (!native_CPXdualfarkas)
        native_CPXdualfarkas = load_symbol("CPXdualfarkas");
    return native_CPXdualfarkas(env, lp, y, proof_p);
}
CPXLIBAPI int CPXPUBLIC CPXdualopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXdualopt)
        native_CPXdualopt = load_symbol("CPXdualopt");
    return native_CPXdualopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXdualwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, double *objshift_p){
    if (!native_CPXdualwrite)
        native_CPXdualwrite = load_symbol("CPXdualwrite");
    return native_CPXdualwrite(env, lp, filename_str, objshift_p);
}
CPXLIBAPI int CPXPUBLIC CPXembwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXembwrite)
        native_CPXembwrite = load_symbol("CPXembwrite");
    return native_CPXembwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXfclose (CPXFILEptr stream){
    if (!native_CPXfclose)
        native_CPXfclose = load_symbol("CPXfclose");
    return native_CPXfclose(stream);
}
CPXLIBAPI int CPXPUBLIC CPXfeasopt (CPXCENVptr env, CPXLPptr lp, double const *rhs, double const *rng, double const *lb, double const *ub){
    if (!native_CPXfeasopt)
        native_CPXfeasopt = load_symbol("CPXfeasopt");
    return native_CPXfeasopt(env, lp, rhs, rng, lb, ub);
}
CPXLIBAPI int CPXPUBLIC CPXfeasoptext (CPXCENVptr env, CPXLPptr lp, int grpcnt, int concnt, double const *grppref, int const *grpbeg, int const *grpind, char const *grptype){
    if (!native_CPXfeasoptext)
        native_CPXfeasoptext = load_symbol("CPXfeasoptext");
    return native_CPXfeasoptext(env, lp, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}
CPXLIBAPI void CPXPUBLIC CPXfinalize (void){
    if (!native_CPXfinalize)
        native_CPXfinalize = load_symbol("CPXfinalize");
    return native_CPXfinalize();
}
int CPXPUBLIC CPXfindiis (CPXCENVptr env, CPXLPptr lp, int *iisnumrows_p, int *iisnumcols_p){
    if (!native_CPXfindiis)
        native_CPXfindiis = load_symbol("CPXfindiis");
    return native_CPXfindiis(env, lp, iisnumrows_p, iisnumcols_p);
}
CPXLIBAPI int CPXPUBLIC CPXflushchannel (CPXCENVptr env, CPXCHANNELptr channel){
    if (!native_CPXflushchannel)
        native_CPXflushchannel = load_symbol("CPXflushchannel");
    return native_CPXflushchannel(env, channel);
}
CPXLIBAPI int CPXPUBLIC CPXflushstdchannels (CPXCENVptr env){
    if (!native_CPXflushstdchannels)
        native_CPXflushstdchannels = load_symbol("CPXflushstdchannels");
    return native_CPXflushstdchannels(env);
}
CPXLIBAPI CPXFILEptr CPXPUBLIC CPXfopen (char const *filename_str, char const *type_str){
    if (!native_CPXfopen)
        native_CPXfopen = load_symbol("CPXfopen");
    return native_CPXfopen(filename_str, type_str);
}
CPXLIBAPI int CPXPUBLIC CPXfputs (char const *s_str, CPXFILEptr stream){
    if (!native_CPXfputs)
        native_CPXfputs = load_symbol("CPXfputs");
    return native_CPXfputs(s_str, stream);
}
void CPXPUBLIC CPXfree (void *ptr){
    if (!native_CPXfree)
        native_CPXfree = load_symbol("CPXfree");
    return native_CPXfree(ptr);
}
CPXLIBAPI int CPXPUBLIC CPXfreeparenv (CPXENVptr env, CPXENVptr * child_p){
    if (!native_CPXfreeparenv)
        native_CPXfreeparenv = load_symbol("CPXfreeparenv");
    return native_CPXfreeparenv(env, child_p);
}
CPXLIBAPI int CPXPUBLIC CPXfreepresolve (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXfreepresolve)
        native_CPXfreepresolve = load_symbol("CPXfreepresolve");
    return native_CPXfreepresolve(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXfreeprob (CPXCENVptr env, CPXLPptr * lp_p){
    if (!native_CPXfreeprob)
        native_CPXfreeprob = load_symbol("CPXfreeprob");
    return native_CPXfreeprob(env, lp_p);
}
CPXLIBAPI int CPXPUBLIC CPXftran (CPXCENVptr env, CPXCLPptr lp, double *x){
    if (!native_CPXftran)
        native_CPXftran = load_symbol("CPXftran");
    return native_CPXftran(env, lp, x);
}
CPXLIBAPI int CPXPUBLIC CPXgetax (CPXCENVptr env, CPXCLPptr lp, double *x, int begin, int end){
    if (!native_CPXgetax)
        native_CPXgetax = load_symbol("CPXgetax");
    return native_CPXgetax(env, lp, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetbaritcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetbaritcnt)
        native_CPXgetbaritcnt = load_symbol("CPXgetbaritcnt");
    return native_CPXgetbaritcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetbase (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat){
    if (!native_CPXgetbase)
        native_CPXgetbase = load_symbol("CPXgetbase");
    return native_CPXgetbase(env, lp, cstat, rstat);
}
CPXLIBAPI int CPXPUBLIC CPXgetbasednorms (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat, double *dnorm){
    if (!native_CPXgetbasednorms)
        native_CPXgetbasednorms = load_symbol("CPXgetbasednorms");
    return native_CPXgetbasednorms(env, lp, cstat, rstat, dnorm);
}
CPXLIBAPI int CPXPUBLIC CPXgetbhead (CPXCENVptr env, CPXCLPptr lp, int *head, double *x){
    if (!native_CPXgetbhead)
        native_CPXgetbhead = load_symbol("CPXgetbhead");
    return native_CPXgetbhead(env, lp, head, x);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbackinfo (CPXCENVptr env, void *cbdata, int wherefrom, int whichinfo, void *result_p){
    if (!native_CPXgetcallbackinfo)
        native_CPXgetcallbackinfo = load_symbol("CPXgetcallbackinfo");
    return native_CPXgetcallbackinfo(env, cbdata, wherefrom, whichinfo, result_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetchannels (CPXCENVptr env, CPXCHANNELptr * cpxresults_p, CPXCHANNELptr * cpxwarning_p, CPXCHANNELptr * cpxerror_p, CPXCHANNELptr * cpxlog_p){
    if (!native_CPXgetchannels)
        native_CPXgetchannels = load_symbol("CPXgetchannels");
    return native_CPXgetchannels(env, cpxresults_p, cpxwarning_p, cpxerror_p, cpxlog_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetchgparam (CPXCENVptr env, int *cnt_p, int *paramnum, int pspace, int *surplus_p){
    if (!native_CPXgetchgparam)
        native_CPXgetchgparam = load_symbol("CPXgetchgparam");
    return native_CPXgetchgparam(env, cnt_p, paramnum, pspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcoef (CPXCENVptr env, CPXCLPptr lp, int i, int j, double *coef_p){
    if (!native_CPXgetcoef)
        native_CPXgetcoef = load_symbol("CPXgetcoef");
    return native_CPXgetcoef(env, lp, i, j, coef_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcolindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXgetcolindex)
        native_CPXgetcolindex = load_symbol("CPXgetcolindex");
    return native_CPXgetcolindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcolinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end){
    if (!native_CPXgetcolinfeas)
        native_CPXgetcolinfeas = load_symbol("CPXgetcolinfeas");
    return native_CPXgetcolinfeas(env, lp, x, infeasout, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcolname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, int storespace, int *surplus_p, int begin, int end){
    if (!native_CPXgetcolname)
        native_CPXgetcolname = load_symbol("CPXgetcolname");
    return native_CPXgetcolname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcols (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *cmatbeg, int *cmatind, double *cmatval, int cmatspace, int *surplus_p, int begin, int end){
    if (!native_CPXgetcols)
        native_CPXgetcols = load_symbol("CPXgetcols");
    return native_CPXgetcols(env, lp, nzcnt_p, cmatbeg, cmatind, cmatval, cmatspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetconflict (CPXCENVptr env, CPXCLPptr lp, int *confstat_p, int *rowind, int *rowbdstat, int *confnumrows_p, int *colind, int *colbdstat, int *confnumcols_p){
    if (!native_CPXgetconflict)
        native_CPXgetconflict = load_symbol("CPXgetconflict");
    return native_CPXgetconflict(env, lp, confstat_p, rowind, rowbdstat, confnumrows_p, colind, colbdstat, confnumcols_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetconflictext (CPXCENVptr env, CPXCLPptr lp, int *grpstat, int beg, int end){
    if (!native_CPXgetconflictext)
        native_CPXgetconflictext = load_symbol("CPXgetconflictext");
    return native_CPXgetconflictext(env, lp, grpstat, beg, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcrossdexchcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetcrossdexchcnt)
        native_CPXgetcrossdexchcnt = load_symbol("CPXgetcrossdexchcnt");
    return native_CPXgetcrossdexchcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetcrossdpushcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetcrossdpushcnt)
        native_CPXgetcrossdpushcnt = load_symbol("CPXgetcrossdpushcnt");
    return native_CPXgetcrossdpushcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetcrosspexchcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetcrosspexchcnt)
        native_CPXgetcrosspexchcnt = load_symbol("CPXgetcrosspexchcnt");
    return native_CPXgetcrosspexchcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetcrossppushcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetcrossppushcnt)
        native_CPXgetcrossppushcnt = load_symbol("CPXgetcrossppushcnt");
    return native_CPXgetcrossppushcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetdblparam (CPXCENVptr env, int whichparam, double *value_p){
    if (!native_CPXgetdblparam)
        native_CPXgetdblparam = load_symbol("CPXgetdblparam");
    return native_CPXgetdblparam(env, whichparam, value_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetdblquality (CPXCENVptr env, CPXCLPptr lp, double *quality_p, int what){
    if (!native_CPXgetdblquality)
        native_CPXgetdblquality = load_symbol("CPXgetdblquality");
    return native_CPXgetdblquality(env, lp, quality_p, what);
}
CPXLIBAPI int CPXPUBLIC CPXgetdettime (CPXCENVptr env, double *dettimestamp_p){
    if (!native_CPXgetdettime)
        native_CPXgetdettime = load_symbol("CPXgetdettime");
    return native_CPXgetdettime(env, dettimestamp_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetdj (CPXCENVptr env, CPXCLPptr lp, double *dj, int begin, int end){
    if (!native_CPXgetdj)
        native_CPXgetdj = load_symbol("CPXgetdj");
    return native_CPXgetdj(env, lp, dj, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetdnorms (CPXCENVptr env, CPXCLPptr lp, double *norm, int *head, int *len_p){
    if (!native_CPXgetdnorms)
        native_CPXgetdnorms = load_symbol("CPXgetdnorms");
    return native_CPXgetdnorms(env, lp, norm, head, len_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetdsbcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetdsbcnt)
        native_CPXgetdsbcnt = load_symbol("CPXgetdsbcnt");
    return native_CPXgetdsbcnt(env, lp);
}
CPXLIBAPI CPXCCHARptr CPXPUBLIC CPXgeterrorstring (CPXCENVptr env, int errcode, char *buffer_str){
    if (!native_CPXgeterrorstring)
        native_CPXgeterrorstring = load_symbol("CPXgeterrorstring");
    return native_CPXgeterrorstring(env, errcode, buffer_str);
}
CPXLIBAPI int CPXPUBLIC CPXgetgrad (CPXCENVptr env, CPXCLPptr lp, int j, int *head, double *y){
    if (!native_CPXgetgrad)
        native_CPXgetgrad = load_symbol("CPXgetgrad");
    return native_CPXgetgrad(env, lp, j, head, y);
}
int CPXPUBLIC CPXgetiis (CPXCENVptr env, CPXCLPptr lp, int *iisstat_p, int *rowind, int *rowbdstat, int *iisnumrows_p, int *colind, int *colbdstat, int *iisnumcols_p){
    if (!native_CPXgetiis)
        native_CPXgetiis = load_symbol("CPXgetiis");
    return native_CPXgetiis(env, lp, iisstat_p, rowind, rowbdstat, iisnumrows_p, colind, colbdstat, iisnumcols_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetijdiv (CPXCENVptr env, CPXCLPptr lp, int *idiv_p, int *jdiv_p){
    if (!native_CPXgetijdiv)
        native_CPXgetijdiv = load_symbol("CPXgetijdiv");
    return native_CPXgetijdiv(env, lp, idiv_p, jdiv_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetijrow (CPXCENVptr env, CPXCLPptr lp, int i, int j, int *row_p){
    if (!native_CPXgetijrow)
        native_CPXgetijrow = load_symbol("CPXgetijrow");
    return native_CPXgetijrow(env, lp, i, j, row_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetintparam (CPXCENVptr env, int whichparam, CPXINT * value_p){
    if (!native_CPXgetintparam)
        native_CPXgetintparam = load_symbol("CPXgetintparam");
    return native_CPXgetintparam(env, whichparam, value_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetintquality (CPXCENVptr env, CPXCLPptr lp, int *quality_p, int what){
    if (!native_CPXgetintquality)
        native_CPXgetintquality = load_symbol("CPXgetintquality");
    return native_CPXgetintquality(env, lp, quality_p, what);
}
CPXLIBAPI int CPXPUBLIC CPXgetitcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetitcnt)
        native_CPXgetitcnt = load_symbol("CPXgetitcnt");
    return native_CPXgetitcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetlb (CPXCENVptr env, CPXCLPptr lp, double *lb, int begin, int end){
    if (!native_CPXgetlb)
        native_CPXgetlb = load_symbol("CPXgetlb");
    return native_CPXgetlb(env, lp, lb, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetlogfile (CPXCENVptr env, CPXFILEptr * logfile_p){
    if (!native_CPXgetlogfile)
        native_CPXgetlogfile = load_symbol("CPXgetlogfile");
    return native_CPXgetlogfile(env, logfile_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetlongparam (CPXCENVptr env, int whichparam, CPXLONG * value_p){
    if (!native_CPXgetlongparam)
        native_CPXgetlongparam = load_symbol("CPXgetlongparam");
    return native_CPXgetlongparam(env, whichparam, value_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetlpcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p){
    if (!native_CPXgetlpcallbackfunc)
        native_CPXgetlpcallbackfunc = load_symbol("CPXgetlpcallbackfunc");
    return native_CPXgetlpcallbackfunc(env, callback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetmethod (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetmethod)
        native_CPXgetmethod = load_symbol("CPXgetmethod");
    return native_CPXgetmethod(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnetcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p){
    if (!native_CPXgetnetcallbackfunc)
        native_CPXgetnetcallbackfunc = load_symbol("CPXgetnetcallbackfunc");
    return native_CPXgetnetcallbackfunc(env, callback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumcols (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumcols)
        native_CPXgetnumcols = load_symbol("CPXgetnumcols");
    return native_CPXgetnumcols(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumcores (CPXCENVptr env, int *numcores_p){
    if (!native_CPXgetnumcores)
        native_CPXgetnumcores = load_symbol("CPXgetnumcores");
    return native_CPXgetnumcores(env, numcores_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumnz (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumnz)
        native_CPXgetnumnz = load_symbol("CPXgetnumnz");
    return native_CPXgetnumnz(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumrows (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumrows)
        native_CPXgetnumrows = load_symbol("CPXgetnumrows");
    return native_CPXgetnumrows(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetobj (CPXCENVptr env, CPXCLPptr lp, double *obj, int begin, int end){
    if (!native_CPXgetobj)
        native_CPXgetobj = load_symbol("CPXgetobj");
    return native_CPXgetobj(env, lp, obj, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetobjname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p){
    if (!native_CPXgetobjname)
        native_CPXgetobjname = load_symbol("CPXgetobjname");
    return native_CPXgetobjname(env, lp, buf_str, bufspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetobjoffset (CPXCENVptr env, CPXCLPptr lp, double *objoffset_p){
    if (!native_CPXgetobjoffset)
        native_CPXgetobjoffset = load_symbol("CPXgetobjoffset");
    return native_CPXgetobjoffset(env, lp, objoffset_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetobjsen (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetobjsen)
        native_CPXgetobjsen = load_symbol("CPXgetobjsen");
    return native_CPXgetobjsen(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p){
    if (!native_CPXgetobjval)
        native_CPXgetobjval = load_symbol("CPXgetobjval");
    return native_CPXgetobjval(env, lp, objval_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetparamname (CPXCENVptr env, int whichparam, char *name_str){
    if (!native_CPXgetparamname)
        native_CPXgetparamname = load_symbol("CPXgetparamname");
    return native_CPXgetparamname(env, whichparam, name_str);
}
CPXLIBAPI int CPXPUBLIC CPXgetparamnum (CPXCENVptr env, char const *name_str, int *whichparam_p){
    if (!native_CPXgetparamnum)
        native_CPXgetparamnum = load_symbol("CPXgetparamnum");
    return native_CPXgetparamnum(env, name_str, whichparam_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetparamtype (CPXCENVptr env, int whichparam, int *paramtype){
    if (!native_CPXgetparamtype)
        native_CPXgetparamtype = load_symbol("CPXgetparamtype");
    return native_CPXgetparamtype(env, whichparam, paramtype);
}
CPXLIBAPI int CPXPUBLIC CPXgetphase1cnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetphase1cnt)
        native_CPXgetphase1cnt = load_symbol("CPXgetphase1cnt");
    return native_CPXgetphase1cnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetpi (CPXCENVptr env, CPXCLPptr lp, double *pi, int begin, int end){
    if (!native_CPXgetpi)
        native_CPXgetpi = load_symbol("CPXgetpi");
    return native_CPXgetpi(env, lp, pi, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetpnorms (CPXCENVptr env, CPXCLPptr lp, double *cnorm, double *rnorm, int *len_p){
    if (!native_CPXgetpnorms)
        native_CPXgetpnorms = load_symbol("CPXgetpnorms");
    return native_CPXgetpnorms(env, lp, cnorm, rnorm, len_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetprestat (CPXCENVptr env, CPXCLPptr lp, int *prestat_p, int *pcstat, int *prstat, int *ocstat, int *orstat){
    if (!native_CPXgetprestat)
        native_CPXgetprestat = load_symbol("CPXgetprestat");
    return native_CPXgetprestat(env, lp, prestat_p, pcstat, prstat, ocstat, orstat);
}
CPXLIBAPI int CPXPUBLIC CPXgetprobname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p){
    if (!native_CPXgetprobname)
        native_CPXgetprobname = load_symbol("CPXgetprobname");
    return native_CPXgetprobname(env, lp, buf_str, bufspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetprobtype (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetprobtype)
        native_CPXgetprobtype = load_symbol("CPXgetprobtype");
    return native_CPXgetprobtype(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetprotected (CPXCENVptr env, CPXCLPptr lp, int *cnt_p, int *indices, int pspace, int *surplus_p){
    if (!native_CPXgetprotected)
        native_CPXgetprotected = load_symbol("CPXgetprotected");
    return native_CPXgetprotected(env, lp, cnt_p, indices, pspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetpsbcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetpsbcnt)
        native_CPXgetpsbcnt = load_symbol("CPXgetpsbcnt");
    return native_CPXgetpsbcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetray (CPXCENVptr env, CPXCLPptr lp, double *z){
    if (!native_CPXgetray)
        native_CPXgetray = load_symbol("CPXgetray");
    return native_CPXgetray(env, lp, z);
}
CPXLIBAPI int CPXPUBLIC CPXgetredlp (CPXCENVptr env, CPXCLPptr lp, CPXCLPptr * redlp_p){
    if (!native_CPXgetredlp)
        native_CPXgetredlp = load_symbol("CPXgetredlp");
    return native_CPXgetredlp(env, lp, redlp_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetrhs (CPXCENVptr env, CPXCLPptr lp, double *rhs, int begin, int end){
    if (!native_CPXgetrhs)
        native_CPXgetrhs = load_symbol("CPXgetrhs");
    return native_CPXgetrhs(env, lp, rhs, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetrngval (CPXCENVptr env, CPXCLPptr lp, double *rngval, int begin, int end){
    if (!native_CPXgetrngval)
        native_CPXgetrngval = load_symbol("CPXgetrngval");
    return native_CPXgetrngval(env, lp, rngval, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetrowindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXgetrowindex)
        native_CPXgetrowindex = load_symbol("CPXgetrowindex");
    return native_CPXgetrowindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetrowinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end){
    if (!native_CPXgetrowinfeas)
        native_CPXgetrowinfeas = load_symbol("CPXgetrowinfeas");
    return native_CPXgetrowinfeas(env, lp, x, infeasout, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetrowname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, int storespace, int *surplus_p, int begin, int end){
    if (!native_CPXgetrowname)
        native_CPXgetrowname = load_symbol("CPXgetrowname");
    return native_CPXgetrowname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetrows (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *rmatbeg, int *rmatind, double *rmatval, int rmatspace, int *surplus_p, int begin, int end){
    if (!native_CPXgetrows)
        native_CPXgetrows = load_symbol("CPXgetrows");
    return native_CPXgetrows(env, lp, nzcnt_p, rmatbeg, rmatind, rmatval, rmatspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetsense (CPXCENVptr env, CPXCLPptr lp, char *sense, int begin, int end){
    if (!native_CPXgetsense)
        native_CPXgetsense = load_symbol("CPXgetsense");
    return native_CPXgetsense(env, lp, sense, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetsiftitcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetsiftitcnt)
        native_CPXgetsiftitcnt = load_symbol("CPXgetsiftitcnt");
    return native_CPXgetsiftitcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetsiftphase1cnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetsiftphase1cnt)
        native_CPXgetsiftphase1cnt = load_symbol("CPXgetsiftphase1cnt");
    return native_CPXgetsiftphase1cnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetslack (CPXCENVptr env, CPXCLPptr lp, double *slack, int begin, int end){
    if (!native_CPXgetslack)
        native_CPXgetslack = load_symbol("CPXgetslack");
    return native_CPXgetslack(env, lp, slack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpooldblquality (CPXCENVptr env, CPXCLPptr lp, int soln, double *quality_p, int what){
    if (!native_CPXgetsolnpooldblquality)
        native_CPXgetsolnpooldblquality = load_symbol("CPXgetsolnpooldblquality");
    return native_CPXgetsolnpooldblquality(env, lp, soln, quality_p, what);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolintquality (CPXCENVptr env, CPXCLPptr lp, int soln, int *quality_p, int what){
    if (!native_CPXgetsolnpoolintquality)
        native_CPXgetsolnpoolintquality = load_symbol("CPXgetsolnpoolintquality");
    return native_CPXgetsolnpoolintquality(env, lp, soln, quality_p, what);
}
CPXLIBAPI int CPXPUBLIC CPXgetstat (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetstat)
        native_CPXgetstat = load_symbol("CPXgetstat");
    return native_CPXgetstat(env, lp);
}
CPXLIBAPI CPXCHARptr CPXPUBLIC CPXgetstatstring (CPXCENVptr env, int statind, char *buffer_str){
    if (!native_CPXgetstatstring)
        native_CPXgetstatstring = load_symbol("CPXgetstatstring");
    return native_CPXgetstatstring(env, statind, buffer_str);
}
CPXLIBAPI int CPXPUBLIC CPXgetstrparam (CPXCENVptr env, int whichparam, char *value_str){
    if (!native_CPXgetstrparam)
        native_CPXgetstrparam = load_symbol("CPXgetstrparam");
    return native_CPXgetstrparam(env, whichparam, value_str);
}
CPXLIBAPI int CPXPUBLIC CPXgettime (CPXCENVptr env, double *timestamp_p){
    if (!native_CPXgettime)
        native_CPXgettime = load_symbol("CPXgettime");
    return native_CPXgettime(env, timestamp_p);
}
CPXLIBAPI int CPXPUBLIC CPXgettuningcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p){
    if (!native_CPXgettuningcallbackfunc)
        native_CPXgettuningcallbackfunc = load_symbol("CPXgettuningcallbackfunc");
    return native_CPXgettuningcallbackfunc(env, callback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetub (CPXCENVptr env, CPXCLPptr lp, double *ub, int begin, int end){
    if (!native_CPXgetub)
        native_CPXgetub = load_symbol("CPXgetub");
    return native_CPXgetub(env, lp, ub, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetweight (CPXCENVptr env, CPXCLPptr lp, int rcnt, int const *rmatbeg, int const *rmatind, double const *rmatval, double *weight, int dpriind){
    if (!native_CPXgetweight)
        native_CPXgetweight = load_symbol("CPXgetweight");
    return native_CPXgetweight(env, lp, rcnt, rmatbeg, rmatind, rmatval, weight, dpriind);
}
CPXLIBAPI int CPXPUBLIC CPXgetx (CPXCENVptr env, CPXCLPptr lp, double *x, int begin, int end){
    if (!native_CPXgetx)
        native_CPXgetx = load_symbol("CPXgetx");
    return native_CPXgetx(env, lp, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXhybnetopt (CPXCENVptr env, CPXLPptr lp, int method){
    if (!native_CPXhybnetopt)
        native_CPXhybnetopt = load_symbol("CPXhybnetopt");
    return native_CPXhybnetopt(env, lp, method);
}
CPXLIBAPI int CPXPUBLIC CPXinfodblparam (CPXCENVptr env, int whichparam, double *defvalue_p, double *minvalue_p, double *maxvalue_p){
    if (!native_CPXinfodblparam)
        native_CPXinfodblparam = load_symbol("CPXinfodblparam");
    return native_CPXinfodblparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}
CPXLIBAPI int CPXPUBLIC CPXinfointparam (CPXCENVptr env, int whichparam, CPXINT * defvalue_p, CPXINT * minvalue_p, CPXINT * maxvalue_p){
    if (!native_CPXinfointparam)
        native_CPXinfointparam = load_symbol("CPXinfointparam");
    return native_CPXinfointparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}
CPXLIBAPI int CPXPUBLIC CPXinfolongparam (CPXCENVptr env, int whichparam, CPXLONG * defvalue_p, CPXLONG * minvalue_p, CPXLONG * maxvalue_p){
    if (!native_CPXinfolongparam)
        native_CPXinfolongparam = load_symbol("CPXinfolongparam");
    return native_CPXinfolongparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}
CPXLIBAPI int CPXPUBLIC CPXinfostrparam (CPXCENVptr env, int whichparam, char *defvalue_str){
    if (!native_CPXinfostrparam)
        native_CPXinfostrparam = load_symbol("CPXinfostrparam");
    return native_CPXinfostrparam(env, whichparam, defvalue_str);
}
CPXLIBAPI void CPXPUBLIC CPXinitialize (void){
    if (!native_CPXinitialize)
        native_CPXinitialize = load_symbol("CPXinitialize");
    return native_CPXinitialize();
}
CPXLIBAPI int CPXPUBLIC CPXkilldnorms (CPXLPptr lp){
    if (!native_CPXkilldnorms)
        native_CPXkilldnorms = load_symbol("CPXkilldnorms");
    return native_CPXkilldnorms(lp);
}
CPXLIBAPI int CPXPUBLIC CPXkillpnorms (CPXLPptr lp){
    if (!native_CPXkillpnorms)
        native_CPXkillpnorms = load_symbol("CPXkillpnorms");
    return native_CPXkillpnorms(lp);
}
CPXLIBAPI int CPXPUBLIC CPXlpopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXlpopt)
        native_CPXlpopt = load_symbol("CPXlpopt");
    return native_CPXlpopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXlprewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXlprewrite)
        native_CPXlprewrite = load_symbol("CPXlprewrite");
    return native_CPXlprewrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXlpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXlpwrite)
        native_CPXlpwrite = load_symbol("CPXlpwrite");
    return native_CPXlpwrite(env, lp, filename_str);
}
CPXVOIDptr CPXPUBLIC CPXmalloc (size_t size){
    if (!native_CPXmalloc)
        native_CPXmalloc = load_symbol("CPXmalloc");
    return native_CPXmalloc(size);
}
CPXLIBAPI int CPXPUBLIC CPXmbasewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXmbasewrite)
        native_CPXmbasewrite = load_symbol("CPXmbasewrite");
    return native_CPXmbasewrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXmdleave (CPXCENVptr env, CPXLPptr lp, int const *indices, int cnt, double *downratio, double *upratio){
    if (!native_CPXmdleave)
        native_CPXmdleave = load_symbol("CPXmdleave");
    return native_CPXmdleave(env, lp, indices, cnt, downratio, upratio);
}
CPXVOIDptr CPXPUBLIC CPXmemcpy (void *s1, void *s2, size_t n){
    if (!native_CPXmemcpy)
        native_CPXmemcpy = load_symbol("CPXmemcpy");
    return native_CPXmemcpy(s1, s2, n);
}
CPXLIBAPI int CPXPUBLIC CPXmpsrewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXmpsrewrite)
        native_CPXmpsrewrite = load_symbol("CPXmpsrewrite");
    return native_CPXmpsrewrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXmpswrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXmpswrite)
        native_CPXmpswrite = load_symbol("CPXmpswrite");
    return native_CPXmpswrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBVARARGS CPXmsg (CPXCHANNELptr channel, char const *format, ...){
    if (!native_CPXmsg)
        native_CPXmsg = load_symbol("CPXmsg");
    return native_CPXmsg(channel, format);
}
CPXLIBAPI int CPXPUBLIC CPXmsgstr (CPXCHANNELptr channel, char const *msg_str){
    if (!native_CPXmsgstr)
        native_CPXmsgstr = load_symbol("CPXmsgstr");
    return native_CPXmsgstr(channel, msg_str);
}
CPXLIBAPI int CPXPUBLIC CPXNETextract (CPXCENVptr env, CPXNETptr net, CPXCLPptr lp, int *colmap, int *rowmap){
    if (!native_CPXNETextract)
        native_CPXNETextract = load_symbol("CPXNETextract");
    return native_CPXNETextract(env, net, lp, colmap, rowmap);
}
CPXLIBAPI int CPXPUBLIC CPXnewcols (CPXCENVptr env, CPXLPptr lp, int ccnt, double const *obj, double const *lb, double const *ub, char const *xctype, char **colname){
    if (!native_CPXnewcols)
        native_CPXnewcols = load_symbol("CPXnewcols");
    return native_CPXnewcols(env, lp, ccnt, obj, lb, ub, xctype, colname);
}
CPXLIBAPI int CPXPUBLIC CPXnewrows (CPXCENVptr env, CPXLPptr lp, int rcnt, double const *rhs, char const *sense, double const *rngval, char **rowname){
    if (!native_CPXnewrows)
        native_CPXnewrows = load_symbol("CPXnewrows");
    return native_CPXnewrows(env, lp, rcnt, rhs, sense, rngval, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXobjsa (CPXCENVptr env, CPXCLPptr lp, int begin, int end, double *lower, double *upper){
    if (!native_CPXobjsa)
        native_CPXobjsa = load_symbol("CPXobjsa");
    return native_CPXobjsa(env, lp, begin, end, lower, upper);
}
CPXLIBAPI CPXENVptr CPXPUBLIC CPXopenCPLEX (int *status_p){
    if (!native_CPXopenCPLEX)
        native_CPXopenCPLEX = load_symbol("CPXopenCPLEX");
    return native_CPXopenCPLEX(status_p);
}
CPXLIBAPI CPXENVptr CPXPUBLIC CPXopenCPLEXruntime (int *status_p, int serialnum, char const *licenvstring_str){
    if (!native_CPXopenCPLEXruntime)
        native_CPXopenCPLEXruntime = load_symbol("CPXopenCPLEXruntime");
    return native_CPXopenCPLEXruntime(status_p, serialnum, licenvstring_str);
}
CPXLIBAPI CPXENVptr CPXPUBLIC CPXparenv (CPXENVptr env, int *status_p){
    if (!native_CPXparenv)
        native_CPXparenv = load_symbol("CPXparenv");
    return native_CPXparenv(env, status_p);
}
CPXLIBAPI int CPXPUBLIC CPXpivot (CPXCENVptr env, CPXLPptr lp, int jenter, int jleave, int leavestat){
    if (!native_CPXpivot)
        native_CPXpivot = load_symbol("CPXpivot");
    return native_CPXpivot(env, lp, jenter, jleave, leavestat);
}
CPXLIBAPI int CPXPUBLIC CPXpivotin (CPXCENVptr env, CPXLPptr lp, int const *rlist, int rlen){
    if (!native_CPXpivotin)
        native_CPXpivotin = load_symbol("CPXpivotin");
    return native_CPXpivotin(env, lp, rlist, rlen);
}
CPXLIBAPI int CPXPUBLIC CPXpivotout (CPXCENVptr env, CPXLPptr lp, int const *clist, int clen){
    if (!native_CPXpivotout)
        native_CPXpivotout = load_symbol("CPXpivotout");
    return native_CPXpivotout(env, lp, clist, clen);
}
CPXLIBAPI int CPXPUBLIC CPXpperwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon){
    if (!native_CPXpperwrite)
        native_CPXpperwrite = load_symbol("CPXpperwrite");
    return native_CPXpperwrite(env, lp, filename_str, epsilon);
}
CPXLIBAPI int CPXPUBLIC CPXpratio (CPXCENVptr env, CPXLPptr lp, int *indices, int cnt, double *downratio, double *upratio, int *downleave, int *upleave, int *downleavestatus, int *upleavestatus, int *downstatus, int *upstatus){
    if (!native_CPXpratio)
        native_CPXpratio = load_symbol("CPXpratio");
    return native_CPXpratio(env, lp, indices, cnt, downratio, upratio, downleave, upleave, downleavestatus, upleavestatus, downstatus, upstatus);
}
CPXLIBAPI int CPXPUBLIC CPXpreaddrows (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **rowname){
    if (!native_CPXpreaddrows)
        native_CPXpreaddrows = load_symbol("CPXpreaddrows");
    return native_CPXpreaddrows(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXprechgobj (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values){
    if (!native_CPXprechgobj)
        native_CPXprechgobj = load_symbol("CPXprechgobj");
    return native_CPXprechgobj(env, lp, cnt, indices, values);
}
CPXLIBAPI int CPXPUBLIC CPXpreslvwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double *objoff_p){
    if (!native_CPXpreslvwrite)
        native_CPXpreslvwrite = load_symbol("CPXpreslvwrite");
    return native_CPXpreslvwrite(env, lp, filename_str, objoff_p);
}
CPXLIBAPI int CPXPUBLIC CPXpresolve (CPXCENVptr env, CPXLPptr lp, int method){
    if (!native_CPXpresolve)
        native_CPXpresolve = load_symbol("CPXpresolve");
    return native_CPXpresolve(env, lp, method);
}
CPXLIBAPI int CPXPUBLIC CPXprimopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXprimopt)
        native_CPXprimopt = load_symbol("CPXprimopt");
    return native_CPXprimopt(env, lp);
}
int CPXPUBLIC CPXputenv (char *envsetting_str){
    if (!native_CPXputenv)
        native_CPXputenv = load_symbol("CPXputenv");
    return native_CPXputenv(envsetting_str);
}
CPXLIBAPI int CPXPUBLIC CPXqpdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double const *x, double *dj){
    if (!native_CPXqpdjfrompi)
        native_CPXqpdjfrompi = load_symbol("CPXqpdjfrompi");
    return native_CPXqpdjfrompi(env, lp, pi, x, dj);
}
CPXLIBAPI int CPXPUBLIC CPXqpuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi, double const *x){
    if (!native_CPXqpuncrushpi)
        native_CPXqpuncrushpi = load_symbol("CPXqpuncrushpi");
    return native_CPXqpuncrushpi(env, lp, pi, prepi, x);
}
CPXLIBAPI int CPXPUBLIC CPXreadcopybase (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXreadcopybase)
        native_CPXreadcopybase = load_symbol("CPXreadcopybase");
    return native_CPXreadcopybase(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXreadcopyparam (CPXENVptr env, char const *filename_str){
    if (!native_CPXreadcopyparam)
        native_CPXreadcopyparam = load_symbol("CPXreadcopyparam");
    return native_CPXreadcopyparam(env, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXreadcopyprob (CPXCENVptr env, CPXLPptr lp, char const *filename_str, char const *filetype_str){
    if (!native_CPXreadcopyprob)
        native_CPXreadcopyprob = load_symbol("CPXreadcopyprob");
    return native_CPXreadcopyprob(env, lp, filename_str, filetype_str);
}
CPXLIBAPI int CPXPUBLIC CPXreadcopysol (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXreadcopysol)
        native_CPXreadcopysol = load_symbol("CPXreadcopysol");
    return native_CPXreadcopysol(env, lp, filename_str);
}
int CPXPUBLIC CPXreadcopyvec (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXreadcopyvec)
        native_CPXreadcopyvec = load_symbol("CPXreadcopyvec");
    return native_CPXreadcopyvec(env, lp, filename_str);
}
CPXVOIDptr CPXPUBLIC CPXrealloc (void *ptr, size_t size){
    if (!native_CPXrealloc)
        native_CPXrealloc = load_symbol("CPXrealloc");
    return native_CPXrealloc(ptr, size);
}
CPXLIBAPI int CPXPUBLIC CPXrefineconflict (CPXCENVptr env, CPXLPptr lp, int *confnumrows_p, int *confnumcols_p){
    if (!native_CPXrefineconflict)
        native_CPXrefineconflict = load_symbol("CPXrefineconflict");
    return native_CPXrefineconflict(env, lp, confnumrows_p, confnumcols_p);
}
CPXLIBAPI int CPXPUBLIC CPXrefineconflictext (CPXCENVptr env, CPXLPptr lp, int grpcnt, int concnt, double const *grppref, int const *grpbeg, int const *grpind, char const *grptype){
    if (!native_CPXrefineconflictext)
        native_CPXrefineconflictext = load_symbol("CPXrefineconflictext");
    return native_CPXrefineconflictext(env, lp, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}
CPXLIBAPI int CPXPUBLIC CPXrhssa (CPXCENVptr env, CPXCLPptr lp, int begin, int end, double *lower, double *upper){
    if (!native_CPXrhssa)
        native_CPXrhssa = load_symbol("CPXrhssa");
    return native_CPXrhssa(env, lp, begin, end, lower, upper);
}
CPXLIBAPI int CPXPUBLIC CPXrobustopt (CPXCENVptr env, CPXLPptr lp, CPXLPptr lblp, CPXLPptr ublp, double objchg, double const *maxchg){
    if (!native_CPXrobustopt)
        native_CPXrobustopt = load_symbol("CPXrobustopt");
    return native_CPXrobustopt(env, lp, lblp, ublp, objchg, maxchg);
}
CPXLIBAPI int CPXPUBLIC CPXsavwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXsavwrite)
        native_CPXsavwrite = load_symbol("CPXsavwrite");
    return native_CPXsavwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXserializercreate (CPXSERIALIZERptr * ser_p){
    if (!native_CPXserializercreate)
        native_CPXserializercreate = load_symbol("CPXserializercreate");
    return native_CPXserializercreate(ser_p);
}
CPXLIBAPI void CPXPUBLIC CPXserializerdestroy (CPXSERIALIZERptr ser){
    if (!native_CPXserializerdestroy)
        native_CPXserializerdestroy = load_symbol("CPXserializerdestroy");
    return native_CPXserializerdestroy(ser);
}
CPXLIBAPI CPXLONG CPXPUBLIC CPXserializerlength (CPXCSERIALIZERptr ser){
    if (!native_CPXserializerlength)
        native_CPXserializerlength = load_symbol("CPXserializerlength");
    return native_CPXserializerlength(ser);
}
CPXLIBAPI void const *CPXPUBLIC CPXserializerpayload (CPXCSERIALIZERptr ser){
    if (!native_CPXserializerpayload)
        native_CPXserializerpayload = load_symbol("CPXserializerpayload");
    return native_CPXserializerpayload(ser);
}
CPXLIBAPI int CPXPUBLIC CPXsetdblparam (CPXENVptr env, int whichparam, double newvalue){
    if (!native_CPXsetdblparam)
        native_CPXsetdblparam = load_symbol("CPXsetdblparam");
    return native_CPXsetdblparam(env, whichparam, newvalue);
}
CPXLIBAPI int CPXPUBLIC CPXsetdefaults (CPXENVptr env){
    if (!native_CPXsetdefaults)
        native_CPXsetdefaults = load_symbol("CPXsetdefaults");
    return native_CPXsetdefaults(env);
}
CPXLIBAPI int CPXPUBLIC CPXsetintparam (CPXENVptr env, int whichparam, CPXINT newvalue){
    if (!native_CPXsetintparam)
        native_CPXsetintparam = load_symbol("CPXsetintparam");
    return native_CPXsetintparam(env, whichparam, newvalue);
}
CPXLIBAPI int CPXPUBLIC CPXsetlogfile (CPXENVptr env, CPXFILEptr lfile){
    if (!native_CPXsetlogfile)
        native_CPXsetlogfile = load_symbol("CPXsetlogfile");
    return native_CPXsetlogfile(env, lfile);
}
CPXLIBAPI int CPXPUBLIC CPXsetlongparam (CPXENVptr env, int whichparam, CPXLONG newvalue){
    if (!native_CPXsetlongparam)
        native_CPXsetlongparam = load_symbol("CPXsetlongparam");
    return native_CPXsetlongparam(env, whichparam, newvalue);
}
CPXLIBAPI int CPXPUBLIC CPXsetlpcallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle){
    if (!native_CPXsetlpcallbackfunc)
        native_CPXsetlpcallbackfunc = load_symbol("CPXsetlpcallbackfunc");
    return native_CPXsetlpcallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetnetcallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle){
    if (!native_CPXsetnetcallbackfunc)
        native_CPXsetnetcallbackfunc = load_symbol("CPXsetnetcallbackfunc");
    return native_CPXsetnetcallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetphase2 (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXsetphase2)
        native_CPXsetphase2 = load_symbol("CPXsetphase2");
    return native_CPXsetphase2(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXsetprofcallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, int, void *), void *cbhandle){
    if (!native_CPXsetprofcallbackfunc)
        native_CPXsetprofcallbackfunc = load_symbol("CPXsetprofcallbackfunc");
    return native_CPXsetprofcallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetstrparam (CPXENVptr env, int whichparam, char const *newvalue_str){
    if (!native_CPXsetstrparam)
        native_CPXsetstrparam = load_symbol("CPXsetstrparam");
    return native_CPXsetstrparam(env, whichparam, newvalue_str);
}
CPXLIBAPI int CPXPUBLIC CPXsetterminate (CPXENVptr env, volatile int *terminate_p){
    if (!native_CPXsetterminate)
        native_CPXsetterminate = load_symbol("CPXsetterminate");
    return native_CPXsetterminate(env, terminate_p);
}
CPXLIBAPI int CPXPUBLIC CPXsettuningcallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle){
    if (!native_CPXsettuningcallbackfunc)
        native_CPXsettuningcallbackfunc = load_symbol("CPXsettuningcallbackfunc");
    return native_CPXsettuningcallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsiftopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXsiftopt)
        native_CPXsiftopt = load_symbol("CPXsiftopt");
    return native_CPXsiftopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *slack){
    if (!native_CPXslackfromx)
        native_CPXslackfromx = load_symbol("CPXslackfromx");
    return native_CPXslackfromx(env, lp, x, slack);
}
CPXLIBAPI int CPXPUBLIC CPXsolninfo (CPXCENVptr env, CPXCLPptr lp, int *solnmethod_p, int *solntype_p, int *pfeasind_p, int *dfeasind_p){
    if (!native_CPXsolninfo)
        native_CPXsolninfo = load_symbol("CPXsolninfo");
    return native_CPXsolninfo(env, lp, solnmethod_p, solntype_p, pfeasind_p, dfeasind_p);
}
CPXLIBAPI int CPXPUBLIC CPXsolution (CPXCENVptr env, CPXCLPptr lp, int *lpstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj){
    if (!native_CPXsolution)
        native_CPXsolution = load_symbol("CPXsolution");
    return native_CPXsolution(env, lp, lpstat_p, objval_p, x, pi, slack, dj);
}
CPXLIBAPI int CPXPUBLIC CPXsolwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXsolwrite)
        native_CPXsolwrite = load_symbol("CPXsolwrite");
    return native_CPXsolwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXsolwritesolnpool (CPXCENVptr env, CPXCLPptr lp, int soln, char const *filename_str){
    if (!native_CPXsolwritesolnpool)
        native_CPXsolwritesolnpool = load_symbol("CPXsolwritesolnpool");
    return native_CPXsolwritesolnpool(env, lp, soln, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXsolwritesolnpoolall (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXsolwritesolnpoolall)
        native_CPXsolwritesolnpoolall = load_symbol("CPXsolwritesolnpoolall");
    return native_CPXsolwritesolnpoolall(env, lp, filename_str);
}
CPXCHARptr CPXPUBLIC CPXstrcpy (char *dest_str, char const *src_str){
    if (!native_CPXstrcpy)
        native_CPXstrcpy = load_symbol("CPXstrcpy");
    return native_CPXstrcpy(dest_str, src_str);
}
size_t CPXPUBLIC CPXstrlen (char const *s_str){
    if (!native_CPXstrlen)
        native_CPXstrlen = load_symbol("CPXstrlen");
    return native_CPXstrlen(s_str);
}
CPXLIBAPI int CPXPUBLIC CPXstrongbranch (CPXCENVptr env, CPXLPptr lp, int const *indices, int cnt, double *downobj, double *upobj, int itlim){
    if (!native_CPXstrongbranch)
        native_CPXstrongbranch = load_symbol("CPXstrongbranch");
    return native_CPXstrongbranch(env, lp, indices, cnt, downobj, upobj, itlim);
}
CPXLIBAPI int CPXPUBLIC CPXtightenbds (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *lu, double const *bd){
    if (!native_CPXtightenbds)
        native_CPXtightenbds = load_symbol("CPXtightenbds");
    return native_CPXtightenbds(env, lp, cnt, indices, lu, bd);
}
CPXLIBAPI int CPXPUBLIC CPXtuneparam (CPXENVptr env, CPXLPptr lp, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char **strval, int *tunestat_p){
    if (!native_CPXtuneparam)
        native_CPXtuneparam = load_symbol("CPXtuneparam");
    return native_CPXtuneparam(env, lp, intcnt, intnum, intval, dblcnt, dblnum, dblval, strcnt, strnum, strval, tunestat_p);
}
CPXLIBAPI int CPXPUBLIC CPXtuneparamprobset (CPXENVptr env, int filecnt, char **filename, char **filetype, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char **strval, int *tunestat_p){
    if (!native_CPXtuneparamprobset)
        native_CPXtuneparamprobset = load_symbol("CPXtuneparamprobset");
    return native_CPXtuneparamprobset(env, filecnt, filename, filetype, intcnt, intnum, intval, dblcnt, dblnum, dblval, strcnt, strnum, strval, tunestat_p);
}
int CPXPUBLIC CPXtxtsolwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXtxtsolwrite)
        native_CPXtxtsolwrite = load_symbol("CPXtxtsolwrite");
    return native_CPXtxtsolwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXuncrushform (CPXCENVptr env, CPXCLPptr lp, int plen, int const *pind, double const *pval, int *len_p, double *offset_p, int *ind, double *val){
    if (!native_CPXuncrushform)
        native_CPXuncrushform = load_symbol("CPXuncrushform");
    return native_CPXuncrushform(env, lp, plen, pind, pval, len_p, offset_p, ind, val);
}
CPXLIBAPI int CPXPUBLIC CPXuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi){
    if (!native_CPXuncrushpi)
        native_CPXuncrushpi = load_symbol("CPXuncrushpi");
    return native_CPXuncrushpi(env, lp, pi, prepi);
}
CPXLIBAPI int CPXPUBLIC CPXuncrushx (CPXCENVptr env, CPXCLPptr lp, double *x, double const *prex){
    if (!native_CPXuncrushx)
        native_CPXuncrushx = load_symbol("CPXuncrushx");
    return native_CPXuncrushx(env, lp, x, prex);
}
CPXLIBAPI int CPXPUBLIC CPXunscaleprob (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXunscaleprob)
        native_CPXunscaleprob = load_symbol("CPXunscaleprob");
    return native_CPXunscaleprob(env, lp);
}
int CPXPUBLIC CPXvecwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXvecwrite)
        native_CPXvecwrite = load_symbol("CPXvecwrite");
    return native_CPXvecwrite(env, lp, filename_str);
}
CPXLIBAPI CPXCCHARptr CPXPUBLIC CPXversion (CPXCENVptr env){
    if (!native_CPXversion)
        native_CPXversion = load_symbol("CPXversion");
    return native_CPXversion(env);
}
CPXLIBAPI int CPXPUBLIC CPXversionnumber (CPXCENVptr env, int *version_p){
    if (!native_CPXversionnumber)
        native_CPXversionnumber = load_symbol("CPXversionnumber");
    return native_CPXversionnumber(env, version_p);
}
CPXLIBAPI int CPXPUBLIC CPXwriteparam (CPXCENVptr env, char const *filename_str){
    if (!native_CPXwriteparam)
        native_CPXwriteparam = load_symbol("CPXwriteparam");
    return native_CPXwriteparam(env, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXwriteprob (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, char const *filetype_str){
    if (!native_CPXwriteprob)
        native_CPXwriteprob = load_symbol("CPXwriteprob");
    return native_CPXwriteprob(env, lp, filename_str, filetype_str);
}
CPXLIBAPI int CPXPUBLIC CPXbaropt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXbaropt)
        native_CPXbaropt = load_symbol("CPXbaropt");
    return native_CPXbaropt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXhybbaropt (CPXCENVptr env, CPXLPptr lp, int method){
    if (!native_CPXhybbaropt)
        native_CPXhybbaropt = load_symbol("CPXhybbaropt");
    return native_CPXhybbaropt(env, lp, method);
}
CPXLIBAPI int CPXPUBLIC CPXaddlazyconstraints (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **rowname){
    if (!native_CPXaddlazyconstraints)
        native_CPXaddlazyconstraints = load_symbol("CPXaddlazyconstraints");
    return native_CPXaddlazyconstraints(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXaddmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt, int nzcnt, int const *beg, int const *varindices, double const *values, int const *effortlevel, char **mipstartname){
    if (!native_CPXaddmipstarts)
        native_CPXaddmipstarts = load_symbol("CPXaddmipstarts");
    return native_CPXaddmipstarts(env, lp, mcnt, nzcnt, beg, varindices, values, effortlevel, mipstartname);
}
CPXLIBAPI int CPXPUBLIC CPXaddsolnpooldivfilter (CPXCENVptr env, CPXLPptr lp, double lower_bound, double upper_bound, int nzcnt, int const *ind, double const *weight, double const *refval, char const *lname_str){
    if (!native_CPXaddsolnpooldivfilter)
        native_CPXaddsolnpooldivfilter = load_symbol("CPXaddsolnpooldivfilter");
    return native_CPXaddsolnpooldivfilter(env, lp, lower_bound, upper_bound, nzcnt, ind, weight, refval, lname_str);
}
CPXLIBAPI int CPXPUBLIC CPXaddsolnpoolrngfilter (CPXCENVptr env, CPXLPptr lp, double lb, double ub, int nzcnt, int const *ind, double const *val, char const *lname_str){
    if (!native_CPXaddsolnpoolrngfilter)
        native_CPXaddsolnpoolrngfilter = load_symbol("CPXaddsolnpoolrngfilter");
    return native_CPXaddsolnpoolrngfilter(env, lp, lb, ub, nzcnt, ind, val, lname_str);
}
CPXLIBAPI int CPXPUBLIC CPXaddsos (CPXCENVptr env, CPXLPptr lp, int numsos, int numsosnz, char const *sostype, int const *sosbeg, int const *sosind, double const *soswt, char **sosname){
    if (!native_CPXaddsos)
        native_CPXaddsos = load_symbol("CPXaddsos");
    return native_CPXaddsos(env, lp, numsos, numsosnz, sostype, sosbeg, sosind, soswt, sosname);
}
CPXLIBAPI int CPXPUBLIC CPXaddusercuts (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **rowname){
    if (!native_CPXaddusercuts)
        native_CPXaddusercuts = load_symbol("CPXaddusercuts");
    return native_CPXaddusercuts(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}
CPXLIBAPI int CPXPUBLIC CPXbranchcallbackbranchasCPLEX (CPXCENVptr env, void *cbdata, int wherefrom, int num, void *userhandle, int *seqnum_p){
    if (!native_CPXbranchcallbackbranchasCPLEX)
        native_CPXbranchcallbackbranchasCPLEX = load_symbol("CPXbranchcallbackbranchasCPLEX");
    return native_CPXbranchcallbackbranchasCPLEX(env, cbdata, wherefrom, num, userhandle, seqnum_p);
}
CPXLIBAPI int CPXPUBLIC CPXbranchcallbackbranchbds (CPXCENVptr env, void *cbdata, int wherefrom, int cnt, int const *indices, char const *lu, double const *bd, double nodeest, void *userhandle, int *seqnum_p){
    if (!native_CPXbranchcallbackbranchbds)
        native_CPXbranchcallbackbranchbds = load_symbol("CPXbranchcallbackbranchbds");
    return native_CPXbranchcallbackbranchbds(env, cbdata, wherefrom, cnt, indices, lu, bd, nodeest, userhandle, seqnum_p);
}
CPXLIBAPI int CPXPUBLIC CPXbranchcallbackbranchconstraints (CPXCENVptr env, void *cbdata, int wherefrom, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, double nodeest, void *userhandle, int *seqnum_p){
    if (!native_CPXbranchcallbackbranchconstraints)
        native_CPXbranchcallbackbranchconstraints = load_symbol("CPXbranchcallbackbranchconstraints");
    return native_CPXbranchcallbackbranchconstraints(env, cbdata, wherefrom, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, nodeest, userhandle, seqnum_p);
}
CPXLIBAPI int CPXPUBLIC CPXbranchcallbackbranchgeneral (CPXCENVptr env, void *cbdata, int wherefrom, int varcnt, int const *varind, char const *varlu, double const *varbd, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, double nodeest, void *userhandle, int *seqnum_p){
    if (!native_CPXbranchcallbackbranchgeneral)
        native_CPXbranchcallbackbranchgeneral = load_symbol("CPXbranchcallbackbranchgeneral");
    return native_CPXbranchcallbackbranchgeneral(env, cbdata, wherefrom, varcnt, varind, varlu, varbd, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, nodeest, userhandle, seqnum_p);
}
CPXLIBAPI int CPXPUBLIC CPXcallbacksetnodeuserhandle (CPXCENVptr env, void *cbdata, int wherefrom, int nodeindex, void *userhandle, void **olduserhandle_p){
    if (!native_CPXcallbacksetnodeuserhandle)
        native_CPXcallbacksetnodeuserhandle = load_symbol("CPXcallbacksetnodeuserhandle");
    return native_CPXcallbacksetnodeuserhandle(env, cbdata, wherefrom, nodeindex, userhandle, olduserhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXcallbacksetuserhandle (CPXCENVptr env, void *cbdata, int wherefrom, void *userhandle, void **olduserhandle_p){
    if (!native_CPXcallbacksetuserhandle)
        native_CPXcallbacksetuserhandle = load_symbol("CPXcallbacksetuserhandle");
    return native_CPXcallbacksetuserhandle(env, cbdata, wherefrom, userhandle, olduserhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXchgctype (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *xctype){
    if (!native_CPXchgctype)
        native_CPXchgctype = load_symbol("CPXchgctype");
    return native_CPXchgctype(env, lp, cnt, indices, xctype);
}
CPXLIBAPI int CPXPUBLIC CPXchgmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt, int const *mipstartindices, int nzcnt, int const *beg, int const *varindices, double const *values, int const *effortlevel){
    if (!native_CPXchgmipstarts)
        native_CPXchgmipstarts = load_symbol("CPXchgmipstarts");
    return native_CPXchgmipstarts(env, lp, mcnt, mipstartindices, nzcnt, beg, varindices, values, effortlevel);
}
CPXLIBAPI int CPXPUBLIC CPXcopyctype (CPXCENVptr env, CPXLPptr lp, char const *xctype){
    if (!native_CPXcopyctype)
        native_CPXcopyctype = load_symbol("CPXcopyctype");
    return native_CPXcopyctype(env, lp, xctype);
}
int CPXPUBLIC CPXcopymipstart (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values){
    if (!native_CPXcopymipstart)
        native_CPXcopymipstart = load_symbol("CPXcopymipstart");
    return native_CPXcopymipstart(env, lp, cnt, indices, values);
}
CPXLIBAPI int CPXPUBLIC CPXcopyorder (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, int const *priority, int const *direction){
    if (!native_CPXcopyorder)
        native_CPXcopyorder = load_symbol("CPXcopyorder");
    return native_CPXcopyorder(env, lp, cnt, indices, priority, direction);
}
CPXLIBAPI int CPXPUBLIC CPXcopysos (CPXCENVptr env, CPXLPptr lp, int numsos, int numsosnz, char const *sostype, int const *sosbeg, int const *sosind, double const *soswt, char **sosname){
    if (!native_CPXcopysos)
        native_CPXcopysos = load_symbol("CPXcopysos");
    return native_CPXcopysos(env, lp, numsos, numsosnz, sostype, sosbeg, sosind, soswt, sosname);
}
CPXLIBAPI int CPXPUBLIC CPXcutcallbackadd (CPXCENVptr env, void *cbdata, int wherefrom, int nzcnt, double rhs, int sense, int const *cutind, double const *cutval, int purgeable){
    if (!native_CPXcutcallbackadd)
        native_CPXcutcallbackadd = load_symbol("CPXcutcallbackadd");
    return native_CPXcutcallbackadd(env, cbdata, wherefrom, nzcnt, rhs, sense, cutind, cutval, purgeable);
}
CPXLIBAPI int CPXPUBLIC CPXcutcallbackaddlocal (CPXCENVptr env, void *cbdata, int wherefrom, int nzcnt, double rhs, int sense, int const *cutind, double const *cutval){
    if (!native_CPXcutcallbackaddlocal)
        native_CPXcutcallbackaddlocal = load_symbol("CPXcutcallbackaddlocal");
    return native_CPXcutcallbackaddlocal(env, cbdata, wherefrom, nzcnt, rhs, sense, cutind, cutval);
}
CPXLIBAPI int CPXPUBLIC CPXdelindconstrs (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXdelindconstrs)
        native_CPXdelindconstrs = load_symbol("CPXdelindconstrs");
    return native_CPXdelindconstrs(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXdelmipstarts (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXdelmipstarts)
        native_CPXdelmipstarts = load_symbol("CPXdelmipstarts");
    return native_CPXdelmipstarts(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXdelsetmipstarts (CPXCENVptr env, CPXLPptr lp, int *delstat){
    if (!native_CPXdelsetmipstarts)
        native_CPXdelsetmipstarts = load_symbol("CPXdelsetmipstarts");
    return native_CPXdelsetmipstarts(env, lp, delstat);
}
CPXLIBAPI int CPXPUBLIC CPXdelsetsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int *delstat){
    if (!native_CPXdelsetsolnpoolfilters)
        native_CPXdelsetsolnpoolfilters = load_symbol("CPXdelsetsolnpoolfilters");
    return native_CPXdelsetsolnpoolfilters(env, lp, delstat);
}
CPXLIBAPI int CPXPUBLIC CPXdelsetsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int *delstat){
    if (!native_CPXdelsetsolnpoolsolns)
        native_CPXdelsetsolnpoolsolns = load_symbol("CPXdelsetsolnpoolsolns");
    return native_CPXdelsetsolnpoolsolns(env, lp, delstat);
}
CPXLIBAPI int CPXPUBLIC CPXdelsetsos (CPXCENVptr env, CPXLPptr lp, int *delset){
    if (!native_CPXdelsetsos)
        native_CPXdelsetsos = load_symbol("CPXdelsetsos");
    return native_CPXdelsetsos(env, lp, delset);
}
CPXLIBAPI int CPXPUBLIC CPXdelsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXdelsolnpoolfilters)
        native_CPXdelsolnpoolfilters = load_symbol("CPXdelsolnpoolfilters");
    return native_CPXdelsolnpoolfilters(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXdelsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXdelsolnpoolsolns)
        native_CPXdelsolnpoolsolns = load_symbol("CPXdelsolnpoolsolns");
    return native_CPXdelsolnpoolsolns(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXdelsos (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXdelsos)
        native_CPXdelsos = load_symbol("CPXdelsos");
    return native_CPXdelsos(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXfltwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXfltwrite)
        native_CPXfltwrite = load_symbol("CPXfltwrite");
    return native_CPXfltwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXfreelazyconstraints (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXfreelazyconstraints)
        native_CPXfreelazyconstraints = load_symbol("CPXfreelazyconstraints");
    return native_CPXfreelazyconstraints(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXfreeusercuts (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXfreeusercuts)
        native_CPXfreeusercuts = load_symbol("CPXfreeusercuts");
    return native_CPXfreeusercuts(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetbestobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p){
    if (!native_CPXgetbestobjval)
        native_CPXgetbestobjval = load_symbol("CPXgetbestobjval");
    return native_CPXgetbestobjval(env, lp, objval_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetbranchcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** branchcallback_p) (CALLBACK_BRANCH_ARGS), void **cbhandle_p){
    if (!native_CPXgetbranchcallbackfunc)
        native_CPXgetbranchcallbackfunc = load_symbol("CPXgetbranchcallbackfunc");
    return native_CPXgetbranchcallbackfunc(env, branchcallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetbranchnosolncallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** branchnosolncallback_p) (CALLBACK_BRANCH_ARGS), void **cbhandle_p){
    if (!native_CPXgetbranchnosolncallbackfunc)
        native_CPXgetbranchnosolncallbackfunc = load_symbol("CPXgetbranchnosolncallbackfunc");
    return native_CPXgetbranchnosolncallbackfunc(env, branchnosolncallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbackbranchconstraints (CPXCENVptr env, void *cbdata, int wherefrom, int which, int *cuts_p, int *nzcnt_p, double *rhs, char *sense, int *rmatbeg, int *rmatind, double *rmatval, int rmatsz, int *surplus_p){
    if (!native_CPXgetcallbackbranchconstraints)
        native_CPXgetcallbackbranchconstraints = load_symbol("CPXgetcallbackbranchconstraints");
    return native_CPXgetcallbackbranchconstraints(env, cbdata, wherefrom, which, cuts_p, nzcnt_p, rhs, sense, rmatbeg, rmatind, rmatval, rmatsz, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbackctype (CPXCENVptr env, void *cbdata, int wherefrom, char *xctype, int begin, int end){
    if (!native_CPXgetcallbackctype)
        native_CPXgetcallbackctype = load_symbol("CPXgetcallbackctype");
    return native_CPXgetcallbackctype(env, cbdata, wherefrom, xctype, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbackgloballb (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, int begin, int end){
    if (!native_CPXgetcallbackgloballb)
        native_CPXgetcallbackgloballb = load_symbol("CPXgetcallbackgloballb");
    return native_CPXgetcallbackgloballb(env, cbdata, wherefrom, lb, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbackglobalub (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, int begin, int end){
    if (!native_CPXgetcallbackglobalub)
        native_CPXgetcallbackglobalub = load_symbol("CPXgetcallbackglobalub");
    return native_CPXgetcallbackglobalub(env, cbdata, wherefrom, ub, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbackincumbent (CPXCENVptr env, void *cbdata, int wherefrom, double *x, int begin, int end){
    if (!native_CPXgetcallbackincumbent)
        native_CPXgetcallbackincumbent = load_symbol("CPXgetcallbackincumbent");
    return native_CPXgetcallbackincumbent(env, cbdata, wherefrom, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbackindicatorinfo (CPXCENVptr env, void *cbdata, int wherefrom, int iindex, int whichinfo, void *result_p){
    if (!native_CPXgetcallbackindicatorinfo)
        native_CPXgetcallbackindicatorinfo = load_symbol("CPXgetcallbackindicatorinfo");
    return native_CPXgetcallbackindicatorinfo(env, cbdata, wherefrom, iindex, whichinfo, result_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbacklp (CPXCENVptr env, void *cbdata, int wherefrom, CPXCLPptr * lp_p){
    if (!native_CPXgetcallbacklp)
        native_CPXgetcallbacklp = load_symbol("CPXgetcallbacklp");
    return native_CPXgetcallbacklp(env, cbdata, wherefrom, lp_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeinfo (CPXCENVptr env, void *cbdata, int wherefrom, int nodeindex, int whichinfo, void *result_p){
    if (!native_CPXgetcallbacknodeinfo)
        native_CPXgetcallbacknodeinfo = load_symbol("CPXgetcallbacknodeinfo");
    return native_CPXgetcallbacknodeinfo(env, cbdata, wherefrom, nodeindex, whichinfo, result_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeintfeas (CPXCENVptr env, void *cbdata, int wherefrom, int *feas, int begin, int end){
    if (!native_CPXgetcallbacknodeintfeas)
        native_CPXgetcallbacknodeintfeas = load_symbol("CPXgetcallbacknodeintfeas");
    return native_CPXgetcallbacknodeintfeas(env, cbdata, wherefrom, feas, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodelb (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, int begin, int end){
    if (!native_CPXgetcallbacknodelb)
        native_CPXgetcallbacknodelb = load_symbol("CPXgetcallbacknodelb");
    return native_CPXgetcallbacknodelb(env, cbdata, wherefrom, lb, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodelp (CPXCENVptr env, void *cbdata, int wherefrom, CPXLPptr * nodelp_p){
    if (!native_CPXgetcallbacknodelp)
        native_CPXgetcallbacknodelp = load_symbol("CPXgetcallbacknodelp");
    return native_CPXgetcallbacknodelp(env, cbdata, wherefrom, nodelp_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeobjval (CPXCENVptr env, void *cbdata, int wherefrom, double *objval_p){
    if (!native_CPXgetcallbacknodeobjval)
        native_CPXgetcallbacknodeobjval = load_symbol("CPXgetcallbacknodeobjval");
    return native_CPXgetcallbacknodeobjval(env, cbdata, wherefrom, objval_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodestat (CPXCENVptr env, void *cbdata, int wherefrom, int *nodestat_p){
    if (!native_CPXgetcallbacknodestat)
        native_CPXgetcallbacknodestat = load_symbol("CPXgetcallbacknodestat");
    return native_CPXgetcallbacknodestat(env, cbdata, wherefrom, nodestat_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeub (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, int begin, int end){
    if (!native_CPXgetcallbacknodeub)
        native_CPXgetcallbacknodeub = load_symbol("CPXgetcallbacknodeub");
    return native_CPXgetcallbacknodeub(env, cbdata, wherefrom, ub, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodex (CPXCENVptr env, void *cbdata, int wherefrom, double *x, int begin, int end){
    if (!native_CPXgetcallbacknodex)
        native_CPXgetcallbacknodex = load_symbol("CPXgetcallbacknodex");
    return native_CPXgetcallbacknodex(env, cbdata, wherefrom, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbackorder (CPXCENVptr env, void *cbdata, int wherefrom, int *priority, int *direction, int begin, int end){
    if (!native_CPXgetcallbackorder)
        native_CPXgetcallbackorder = load_symbol("CPXgetcallbackorder");
    return native_CPXgetcallbackorder(env, cbdata, wherefrom, priority, direction, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbackpseudocosts (CPXCENVptr env, void *cbdata, int wherefrom, double *uppc, double *downpc, int begin, int end){
    if (!native_CPXgetcallbackpseudocosts)
        native_CPXgetcallbackpseudocosts = load_symbol("CPXgetcallbackpseudocosts");
    return native_CPXgetcallbackpseudocosts(env, cbdata, wherefrom, uppc, downpc, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbackseqinfo (CPXCENVptr env, void *cbdata, int wherefrom, int seqid, int whichinfo, void *result_p){
    if (!native_CPXgetcallbackseqinfo)
        native_CPXgetcallbackseqinfo = load_symbol("CPXgetcallbackseqinfo");
    return native_CPXgetcallbackseqinfo(env, cbdata, wherefrom, seqid, whichinfo, result_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcallbacksosinfo (CPXCENVptr env, void *cbdata, int wherefrom, int sosindex, int member, int whichinfo, void *result_p){
    if (!native_CPXgetcallbacksosinfo)
        native_CPXgetcallbacksosinfo = load_symbol("CPXgetcallbacksosinfo");
    return native_CPXgetcallbacksosinfo(env, cbdata, wherefrom, sosindex, member, whichinfo, result_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetctype (CPXCENVptr env, CPXCLPptr lp, char *xctype, int begin, int end){
    if (!native_CPXgetctype)
        native_CPXgetctype = load_symbol("CPXgetctype");
    return native_CPXgetctype(env, lp, xctype, begin, end);
}
int CPXPUBLIC CPXgetcutcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** cutcallback_p) (CALLBACK_CUT_ARGS), void **cbhandle_p){
    if (!native_CPXgetcutcallbackfunc)
        native_CPXgetcutcallbackfunc = load_symbol("CPXgetcutcallbackfunc");
    return native_CPXgetcutcallbackfunc(env, cutcallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetcutoff (CPXCENVptr env, CPXCLPptr lp, double *cutoff_p){
    if (!native_CPXgetcutoff)
        native_CPXgetcutoff = load_symbol("CPXgetcutoff");
    return native_CPXgetcutoff(env, lp, cutoff_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetdeletenodecallbackfunc (CPXCENVptr env, void (CPXPUBLIC ** deletecallback_p) (CALLBACK_DELETENODE_ARGS), void **cbhandle_p){
    if (!native_CPXgetdeletenodecallbackfunc)
        native_CPXgetdeletenodecallbackfunc = load_symbol("CPXgetdeletenodecallbackfunc");
    return native_CPXgetdeletenodecallbackfunc(env, deletecallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetheuristiccallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** heuristiccallback_p) (CALLBACK_HEURISTIC_ARGS), void **cbhandle_p){
    if (!native_CPXgetheuristiccallbackfunc)
        native_CPXgetheuristiccallbackfunc = load_symbol("CPXgetheuristiccallbackfunc");
    return native_CPXgetheuristiccallbackfunc(env, heuristiccallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetincumbentcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** incumbentcallback_p) (CALLBACK_INCUMBENT_ARGS), void **cbhandle_p){
    if (!native_CPXgetincumbentcallbackfunc)
        native_CPXgetincumbentcallbackfunc = load_symbol("CPXgetincumbentcallbackfunc");
    return native_CPXgetincumbentcallbackfunc(env, incumbentcallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetindconstr (CPXCENVptr env, CPXCLPptr lp, int *indvar_p, int *complemented_p, int *nzcnt_p, double *rhs_p, char *sense_p, int *linind, double *linval, int space, int *surplus_p, int which){
    if (!native_CPXgetindconstr)
        native_CPXgetindconstr = load_symbol("CPXgetindconstr");
    return native_CPXgetindconstr(env, lp, indvar_p, complemented_p, nzcnt_p, rhs_p, sense_p, linind, linval, space, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXgetindconstrindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXgetindconstrindex)
        native_CPXgetindconstrindex = load_symbol("CPXgetindconstrindex");
    return native_CPXgetindconstrindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetindconstrinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end){
    if (!native_CPXgetindconstrinfeas)
        native_CPXgetindconstrinfeas = load_symbol("CPXgetindconstrinfeas");
    return native_CPXgetindconstrinfeas(env, lp, x, infeasout, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetindconstrname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p, int which){
    if (!native_CPXgetindconstrname)
        native_CPXgetindconstrname = load_symbol("CPXgetindconstrname");
    return native_CPXgetindconstrname(env, lp, buf_str, bufspace, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXgetindconstrslack (CPXCENVptr env, CPXCLPptr lp, double *indslack, int begin, int end){
    if (!native_CPXgetindconstrslack)
        native_CPXgetindconstrslack = load_symbol("CPXgetindconstrslack");
    return native_CPXgetindconstrslack(env, lp, indslack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetinfocallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p){
    if (!native_CPXgetinfocallbackfunc)
        native_CPXgetinfocallbackfunc = load_symbol("CPXgetinfocallbackfunc");
    return native_CPXgetinfocallbackfunc(env, callback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetlazyconstraintcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** cutcallback_p) (CALLBACK_CUT_ARGS), void **cbhandle_p){
    if (!native_CPXgetlazyconstraintcallbackfunc)
        native_CPXgetlazyconstraintcallbackfunc = load_symbol("CPXgetlazyconstraintcallbackfunc");
    return native_CPXgetlazyconstraintcallbackfunc(env, cutcallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetmipcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p){
    if (!native_CPXgetmipcallbackfunc)
        native_CPXgetmipcallbackfunc = load_symbol("CPXgetmipcallbackfunc");
    return native_CPXgetmipcallbackfunc(env, callback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetmipitcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetmipitcnt)
        native_CPXgetmipitcnt = load_symbol("CPXgetmipitcnt");
    return native_CPXgetmipitcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetmiprelgap (CPXCENVptr env, CPXCLPptr lp, double *gap_p){
    if (!native_CPXgetmiprelgap)
        native_CPXgetmiprelgap = load_symbol("CPXgetmiprelgap");
    return native_CPXgetmiprelgap(env, lp, gap_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetmipstartindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXgetmipstartindex)
        native_CPXgetmipstartindex = load_symbol("CPXgetmipstartindex");
    return native_CPXgetmipstartindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetmipstartname (CPXCENVptr env, CPXCLPptr lp, char **name, char *store, int storesz, int *surplus_p, int begin, int end){
    if (!native_CPXgetmipstartname)
        native_CPXgetmipstartname = load_symbol("CPXgetmipstartname");
    return native_CPXgetmipstartname(env, lp, name, store, storesz, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetmipstarts (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *beg, int *varindices, double *values, int *effortlevel, int startspace, int *surplus_p, int begin, int end){
    if (!native_CPXgetmipstarts)
        native_CPXgetmipstarts = load_symbol("CPXgetmipstarts");
    return native_CPXgetmipstarts(env, lp, nzcnt_p, beg, varindices, values, effortlevel, startspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetnodecallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** nodecallback_p) (CALLBACK_NODE_ARGS), void **cbhandle_p){
    if (!native_CPXgetnodecallbackfunc)
        native_CPXgetnodecallbackfunc = load_symbol("CPXgetnodecallbackfunc");
    return native_CPXgetnodecallbackfunc(env, nodecallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetnodecnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnodecnt)
        native_CPXgetnodecnt = load_symbol("CPXgetnodecnt");
    return native_CPXgetnodecnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnodeint (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnodeint)
        native_CPXgetnodeint = load_symbol("CPXgetnodeint");
    return native_CPXgetnodeint(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnodeleftcnt (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnodeleftcnt)
        native_CPXgetnodeleftcnt = load_symbol("CPXgetnodeleftcnt");
    return native_CPXgetnodeleftcnt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumbin (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumbin)
        native_CPXgetnumbin = load_symbol("CPXgetnumbin");
    return native_CPXgetnumbin(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumcuts (CPXCENVptr env, CPXCLPptr lp, int cuttype, int *num_p){
    if (!native_CPXgetnumcuts)
        native_CPXgetnumcuts = load_symbol("CPXgetnumcuts");
    return native_CPXgetnumcuts(env, lp, cuttype, num_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumindconstrs (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumindconstrs)
        native_CPXgetnumindconstrs = load_symbol("CPXgetnumindconstrs");
    return native_CPXgetnumindconstrs(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumint (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumint)
        native_CPXgetnumint = load_symbol("CPXgetnumint");
    return native_CPXgetnumint(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumlazyconstraints (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumlazyconstraints)
        native_CPXgetnumlazyconstraints = load_symbol("CPXgetnumlazyconstraints");
    return native_CPXgetnumlazyconstraints(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnummipstarts (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnummipstarts)
        native_CPXgetnummipstarts = load_symbol("CPXgetnummipstarts");
    return native_CPXgetnummipstarts(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumsemicont (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumsemicont)
        native_CPXgetnumsemicont = load_symbol("CPXgetnumsemicont");
    return native_CPXgetnumsemicont(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumsemiint (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumsemiint)
        native_CPXgetnumsemiint = load_symbol("CPXgetnumsemiint");
    return native_CPXgetnumsemiint(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumsos (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumsos)
        native_CPXgetnumsos = load_symbol("CPXgetnumsos");
    return native_CPXgetnumsos(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumusercuts (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumusercuts)
        native_CPXgetnumusercuts = load_symbol("CPXgetnumusercuts");
    return native_CPXgetnumusercuts(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetorder (CPXCENVptr env, CPXCLPptr lp, int *cnt_p, int *indices, int *priority, int *direction, int ordspace, int *surplus_p){
    if (!native_CPXgetorder)
        native_CPXgetorder = load_symbol("CPXgetorder");
    return native_CPXgetorder(env, lp, cnt_p, indices, priority, direction, ordspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpooldivfilter (CPXCENVptr env, CPXCLPptr lp, double *lower_cutoff_p, double *upper_cutoff_p, int *nzcnt_p, int *ind, double *val, double *refval, int space, int *surplus_p, int which){
    if (!native_CPXgetsolnpooldivfilter)
        native_CPXgetsolnpooldivfilter = load_symbol("CPXgetsolnpooldivfilter");
    return native_CPXgetsolnpooldivfilter(env, lp, lower_cutoff_p, upper_cutoff_p, nzcnt_p, ind, val, refval, space, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolfilterindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXgetsolnpoolfilterindex)
        native_CPXgetsolnpoolfilterindex = load_symbol("CPXgetsolnpoolfilterindex");
    return native_CPXgetsolnpoolfilterindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolfiltername (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p, int which){
    if (!native_CPXgetsolnpoolfiltername)
        native_CPXgetsolnpoolfiltername = load_symbol("CPXgetsolnpoolfiltername");
    return native_CPXgetsolnpoolfiltername(env, lp, buf_str, bufspace, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolfiltertype (CPXCENVptr env, CPXCLPptr lp, int *ftype_p, int which){
    if (!native_CPXgetsolnpoolfiltertype)
        native_CPXgetsolnpoolfiltertype = load_symbol("CPXgetsolnpoolfiltertype");
    return native_CPXgetsolnpoolfiltertype(env, lp, ftype_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolmeanobjval (CPXCENVptr env, CPXCLPptr lp, double *meanobjval_p){
    if (!native_CPXgetsolnpoolmeanobjval)
        native_CPXgetsolnpoolmeanobjval = load_symbol("CPXgetsolnpoolmeanobjval");
    return native_CPXgetsolnpoolmeanobjval(env, lp, meanobjval_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolnumfilters (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetsolnpoolnumfilters)
        native_CPXgetsolnpoolnumfilters = load_symbol("CPXgetsolnpoolnumfilters");
    return native_CPXgetsolnpoolnumfilters(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolnumreplaced (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetsolnpoolnumreplaced)
        native_CPXgetsolnpoolnumreplaced = load_symbol("CPXgetsolnpoolnumreplaced");
    return native_CPXgetsolnpoolnumreplaced(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolnumsolns (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetsolnpoolnumsolns)
        native_CPXgetsolnpoolnumsolns = load_symbol("CPXgetsolnpoolnumsolns");
    return native_CPXgetsolnpoolnumsolns(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolobjval (CPXCENVptr env, CPXCLPptr lp, int soln, double *objval_p){
    if (!native_CPXgetsolnpoolobjval)
        native_CPXgetsolnpoolobjval = load_symbol("CPXgetsolnpoolobjval");
    return native_CPXgetsolnpoolobjval(env, lp, soln, objval_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolqconstrslack (CPXCENVptr env, CPXCLPptr lp, int soln, double *qcslack, int begin, int end){
    if (!native_CPXgetsolnpoolqconstrslack)
        native_CPXgetsolnpoolqconstrslack = load_symbol("CPXgetsolnpoolqconstrslack");
    return native_CPXgetsolnpoolqconstrslack(env, lp, soln, qcslack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolrngfilter (CPXCENVptr env, CPXCLPptr lp, double *lb_p, double *ub_p, int *nzcnt_p, int *ind, double *val, int space, int *surplus_p, int which){
    if (!native_CPXgetsolnpoolrngfilter)
        native_CPXgetsolnpoolrngfilter = load_symbol("CPXgetsolnpoolrngfilter");
    return native_CPXgetsolnpoolrngfilter(env, lp, lb_p, ub_p, nzcnt_p, ind, val, space, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolslack (CPXCENVptr env, CPXCLPptr lp, int soln, double *slack, int begin, int end){
    if (!native_CPXgetsolnpoolslack)
        native_CPXgetsolnpoolslack = load_symbol("CPXgetsolnpoolslack");
    return native_CPXgetsolnpoolslack(env, lp, soln, slack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolsolnindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXgetsolnpoolsolnindex)
        native_CPXgetsolnpoolsolnindex = load_symbol("CPXgetsolnpoolsolnindex");
    return native_CPXgetsolnpoolsolnindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolsolnname (CPXCENVptr env, CPXCLPptr lp, char *store, int storesz, int *surplus_p, int which){
    if (!native_CPXgetsolnpoolsolnname)
        native_CPXgetsolnpoolsolnname = load_symbol("CPXgetsolnpoolsolnname");
    return native_CPXgetsolnpoolsolnname(env, lp, store, storesz, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolx (CPXCENVptr env, CPXCLPptr lp, int soln, double *x, int begin, int end){
    if (!native_CPXgetsolnpoolx)
        native_CPXgetsolnpoolx = load_symbol("CPXgetsolnpoolx");
    return native_CPXgetsolnpoolx(env, lp, soln, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetsolvecallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** solvecallback_p) (CALLBACK_SOLVE_ARGS), void **cbhandle_p){
    if (!native_CPXgetsolvecallbackfunc)
        native_CPXgetsolvecallbackfunc = load_symbol("CPXgetsolvecallbackfunc");
    return native_CPXgetsolvecallbackfunc(env, solvecallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetsos (CPXCENVptr env, CPXCLPptr lp, int *numsosnz_p, char *sostype, int *sosbeg, int *sosind, double *soswt, int sosspace, int *surplus_p, int begin, int end){
    if (!native_CPXgetsos)
        native_CPXgetsos = load_symbol("CPXgetsos");
    return native_CPXgetsos(env, lp, numsosnz_p, sostype, sosbeg, sosind, soswt, sosspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetsosindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXgetsosindex)
        native_CPXgetsosindex = load_symbol("CPXgetsosindex");
    return native_CPXgetsosindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetsosinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end){
    if (!native_CPXgetsosinfeas)
        native_CPXgetsosinfeas = load_symbol("CPXgetsosinfeas");
    return native_CPXgetsosinfeas(env, lp, x, infeasout, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetsosname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, int storespace, int *surplus_p, int begin, int end){
    if (!native_CPXgetsosname)
        native_CPXgetsosname = load_symbol("CPXgetsosname");
    return native_CPXgetsosname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetsubmethod (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetsubmethod)
        native_CPXgetsubmethod = load_symbol("CPXgetsubmethod");
    return native_CPXgetsubmethod(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetsubstat (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetsubstat)
        native_CPXgetsubstat = load_symbol("CPXgetsubstat");
    return native_CPXgetsubstat(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetusercutcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** cutcallback_p) (CALLBACK_CUT_ARGS), void **cbhandle_p){
    if (!native_CPXgetusercutcallbackfunc)
        native_CPXgetusercutcallbackfunc = load_symbol("CPXgetusercutcallbackfunc");
    return native_CPXgetusercutcallbackfunc(env, cutcallback_p, cbhandle_p);
}
CPXLIBAPI int CPXPUBLIC CPXindconstrslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *indslack){
    if (!native_CPXindconstrslackfromx)
        native_CPXindconstrslackfromx = load_symbol("CPXindconstrslackfromx");
    return native_CPXindconstrslackfromx(env, lp, x, indslack);
}
CPXLIBAPI int CPXPUBLIC CPXmipopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXmipopt)
        native_CPXmipopt = load_symbol("CPXmipopt");
    return native_CPXmipopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXordread (CPXCENVptr env, char const *filename_str, int numcols, char **colname, int *cnt_p, int *indices, int *priority, int *direction){
    if (!native_CPXordread)
        native_CPXordread = load_symbol("CPXordread");
    return native_CPXordread(env, filename_str, numcols, colname, cnt_p, indices, priority, direction);
}
CPXLIBAPI int CPXPUBLIC CPXordwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str){
    if (!native_CPXordwrite)
        native_CPXordwrite = load_symbol("CPXordwrite");
    return native_CPXordwrite(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXpopulate (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXpopulate)
        native_CPXpopulate = load_symbol("CPXpopulate");
    return native_CPXpopulate(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXreadcopymipstarts (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXreadcopymipstarts)
        native_CPXreadcopymipstarts = load_symbol("CPXreadcopymipstarts");
    return native_CPXreadcopymipstarts(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXreadcopyorder (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXreadcopyorder)
        native_CPXreadcopyorder = load_symbol("CPXreadcopyorder");
    return native_CPXreadcopyorder(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXreadcopysolnpoolfilters (CPXCENVptr env, CPXLPptr lp, char const *filename_str){
    if (!native_CPXreadcopysolnpoolfilters)
        native_CPXreadcopysolnpoolfilters = load_symbol("CPXreadcopysolnpoolfilters");
    return native_CPXreadcopysolnpoolfilters(env, lp, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXrefinemipstartconflict (CPXCENVptr env, CPXLPptr lp, int mipstartindex, int *confnumrows_p, int *confnumcols_p){
    if (!native_CPXrefinemipstartconflict)
        native_CPXrefinemipstartconflict = load_symbol("CPXrefinemipstartconflict");
    return native_CPXrefinemipstartconflict(env, lp, mipstartindex, confnumrows_p, confnumcols_p);
}
CPXLIBAPI int CPXPUBLIC CPXrefinemipstartconflictext (CPXCENVptr env, CPXLPptr lp, int mipstartindex, int grpcnt, int concnt, double const *grppref, int const *grpbeg, int const *grpind, char const *grptype){
    if (!native_CPXrefinemipstartconflictext)
        native_CPXrefinemipstartconflictext = load_symbol("CPXrefinemipstartconflictext");
    return native_CPXrefinemipstartconflictext(env, lp, mipstartindex, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}
CPXLIBAPI int CPXPUBLIC CPXsetbranchcallbackfunc (CPXENVptr env, int (CPXPUBLIC * branchcallback) (CALLBACK_BRANCH_ARGS), void *cbhandle){
    if (!native_CPXsetbranchcallbackfunc)
        native_CPXsetbranchcallbackfunc = load_symbol("CPXsetbranchcallbackfunc");
    return native_CPXsetbranchcallbackfunc(env, branchcallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetbranchnosolncallbackfunc (CPXENVptr env, int (CPXPUBLIC * branchnosolncallback) (CALLBACK_BRANCH_ARGS), void *cbhandle){
    if (!native_CPXsetbranchnosolncallbackfunc)
        native_CPXsetbranchnosolncallbackfunc = load_symbol("CPXsetbranchnosolncallbackfunc");
    return native_CPXsetbranchnosolncallbackfunc(env, branchnosolncallback, cbhandle);
}
int CPXPUBLIC CPXsetcutcallbackfunc (CPXENVptr env, int (CPXPUBLIC * cutcallback) (CALLBACK_CUT_ARGS), void *cbhandle){
    if (!native_CPXsetcutcallbackfunc)
        native_CPXsetcutcallbackfunc = load_symbol("CPXsetcutcallbackfunc");
    return native_CPXsetcutcallbackfunc(env, cutcallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetdeletenodecallbackfunc (CPXENVptr env, void (CPXPUBLIC * deletecallback) (CALLBACK_DELETENODE_ARGS), void *cbhandle){
    if (!native_CPXsetdeletenodecallbackfunc)
        native_CPXsetdeletenodecallbackfunc = load_symbol("CPXsetdeletenodecallbackfunc");
    return native_CPXsetdeletenodecallbackfunc(env, deletecallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetheuristiccallbackfunc (CPXENVptr env, int (CPXPUBLIC * heuristiccallback) (CALLBACK_HEURISTIC_ARGS), void *cbhandle){
    if (!native_CPXsetheuristiccallbackfunc)
        native_CPXsetheuristiccallbackfunc = load_symbol("CPXsetheuristiccallbackfunc");
    return native_CPXsetheuristiccallbackfunc(env, heuristiccallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetincumbentcallbackfunc (CPXENVptr env, int (CPXPUBLIC * incumbentcallback) (CALLBACK_INCUMBENT_ARGS), void *cbhandle){
    if (!native_CPXsetincumbentcallbackfunc)
        native_CPXsetincumbentcallbackfunc = load_symbol("CPXsetincumbentcallbackfunc");
    return native_CPXsetincumbentcallbackfunc(env, incumbentcallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetinfocallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle){
    if (!native_CPXsetinfocallbackfunc)
        native_CPXsetinfocallbackfunc = load_symbol("CPXsetinfocallbackfunc");
    return native_CPXsetinfocallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetlazyconstraintcallbackfunc (CPXENVptr env, int (CPXPUBLIC * lazyconcallback) (CALLBACK_CUT_ARGS), void *cbhandle){
    if (!native_CPXsetlazyconstraintcallbackfunc)
        native_CPXsetlazyconstraintcallbackfunc = load_symbol("CPXsetlazyconstraintcallbackfunc");
    return native_CPXsetlazyconstraintcallbackfunc(env, lazyconcallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetmipcallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle){
    if (!native_CPXsetmipcallbackfunc)
        native_CPXsetmipcallbackfunc = load_symbol("CPXsetmipcallbackfunc");
    return native_CPXsetmipcallbackfunc(env, callback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetnodecallbackfunc (CPXENVptr env, int (CPXPUBLIC * nodecallback) (CALLBACK_NODE_ARGS), void *cbhandle){
    if (!native_CPXsetnodecallbackfunc)
        native_CPXsetnodecallbackfunc = load_symbol("CPXsetnodecallbackfunc");
    return native_CPXsetnodecallbackfunc(env, nodecallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetsolvecallbackfunc (CPXENVptr env, int (CPXPUBLIC * solvecallback) (CALLBACK_SOLVE_ARGS), void *cbhandle){
    if (!native_CPXsetsolvecallbackfunc)
        native_CPXsetsolvecallbackfunc = load_symbol("CPXsetsolvecallbackfunc");
    return native_CPXsetsolvecallbackfunc(env, solvecallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXsetusercutcallbackfunc (CPXENVptr env, int (CPXPUBLIC * cutcallback) (CALLBACK_CUT_ARGS), void *cbhandle){
    if (!native_CPXsetusercutcallbackfunc)
        native_CPXsetusercutcallbackfunc = load_symbol("CPXsetusercutcallbackfunc");
    return native_CPXsetusercutcallbackfunc(env, cutcallback, cbhandle);
}
CPXLIBAPI int CPXPUBLIC CPXwritemipstarts (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, int begin, int end){
    if (!native_CPXwritemipstarts)
        native_CPXwritemipstarts = load_symbol("CPXwritemipstarts");
    return native_CPXwritemipstarts(env, lp, filename_str, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXaddindconstr (CPXCENVptr env, CPXLPptr lp, int indvar, int complemented, int nzcnt, double rhs, int sense, int const *linind, double const *linval, char const *indname_str){
    if (!native_CPXaddindconstr)
        native_CPXaddindconstr = load_symbol("CPXaddindconstr");
    return native_CPXaddindconstr(env, lp, indvar, complemented, nzcnt, rhs, sense, linind, linval, indname_str);
}
CPXLIBAPI int CPXPUBLIC CPXNETaddarcs (CPXCENVptr env, CPXNETptr net, int narcs, int const *fromnode, int const *tonode, double const *low, double const *up, double const *obj, char **anames){
    if (!native_CPXNETaddarcs)
        native_CPXNETaddarcs = load_symbol("CPXNETaddarcs");
    return native_CPXNETaddarcs(env, net, narcs, fromnode, tonode, low, up, obj, anames);
}
CPXLIBAPI int CPXPUBLIC CPXNETaddnodes (CPXCENVptr env, CPXNETptr net, int nnodes, double const *supply, char **name){
    if (!native_CPXNETaddnodes)
        native_CPXNETaddnodes = load_symbol("CPXNETaddnodes");
    return native_CPXNETaddnodes(env, net, nnodes, supply, name);
}
CPXLIBAPI int CPXPUBLIC CPXNETbasewrite (CPXCENVptr env, CPXCNETptr net, char const *filename_str){
    if (!native_CPXNETbasewrite)
        native_CPXNETbasewrite = load_symbol("CPXNETbasewrite");
    return native_CPXNETbasewrite(env, net, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXNETchgarcname (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, char **newname){
    if (!native_CPXNETchgarcname)
        native_CPXNETchgarcname = load_symbol("CPXNETchgarcname");
    return native_CPXNETchgarcname(env, net, cnt, indices, newname);
}
CPXLIBAPI int CPXPUBLIC CPXNETchgarcnodes (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, int const *fromnode, int const *tonode){
    if (!native_CPXNETchgarcnodes)
        native_CPXNETchgarcnodes = load_symbol("CPXNETchgarcnodes");
    return native_CPXNETchgarcnodes(env, net, cnt, indices, fromnode, tonode);
}
CPXLIBAPI int CPXPUBLIC CPXNETchgbds (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, char const *lu, double const *bd){
    if (!native_CPXNETchgbds)
        native_CPXNETchgbds = load_symbol("CPXNETchgbds");
    return native_CPXNETchgbds(env, net, cnt, indices, lu, bd);
}
CPXLIBAPI int CPXPUBLIC CPXNETchgname (CPXCENVptr env, CPXNETptr net, int key, int vindex, char const *name_str){
    if (!native_CPXNETchgname)
        native_CPXNETchgname = load_symbol("CPXNETchgname");
    return native_CPXNETchgname(env, net, key, vindex, name_str);
}
CPXLIBAPI int CPXPUBLIC CPXNETchgnodename (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, char **newname){
    if (!native_CPXNETchgnodename)
        native_CPXNETchgnodename = load_symbol("CPXNETchgnodename");
    return native_CPXNETchgnodename(env, net, cnt, indices, newname);
}
CPXLIBAPI int CPXPUBLIC CPXNETchgobj (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, double const *obj){
    if (!native_CPXNETchgobj)
        native_CPXNETchgobj = load_symbol("CPXNETchgobj");
    return native_CPXNETchgobj(env, net, cnt, indices, obj);
}
CPXLIBAPI int CPXPUBLIC CPXNETchgobjsen (CPXCENVptr env, CPXNETptr net, int maxormin){
    if (!native_CPXNETchgobjsen)
        native_CPXNETchgobjsen = load_symbol("CPXNETchgobjsen");
    return native_CPXNETchgobjsen(env, net, maxormin);
}
CPXLIBAPI int CPXPUBLIC CPXNETchgsupply (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, double const *supply){
    if (!native_CPXNETchgsupply)
        native_CPXNETchgsupply = load_symbol("CPXNETchgsupply");
    return native_CPXNETchgsupply(env, net, cnt, indices, supply);
}
CPXLIBAPI int CPXPUBLIC CPXNETcopybase (CPXCENVptr env, CPXNETptr net, int const *astat, int const *nstat){
    if (!native_CPXNETcopybase)
        native_CPXNETcopybase = load_symbol("CPXNETcopybase");
    return native_CPXNETcopybase(env, net, astat, nstat);
}
CPXLIBAPI int CPXPUBLIC CPXNETcopynet (CPXCENVptr env, CPXNETptr net, int objsen, int nnodes, double const *supply, char **nnames, int narcs, int const *fromnode, int const *tonode, double const *low, double const *up, double const *obj, char **anames){
    if (!native_CPXNETcopynet)
        native_CPXNETcopynet = load_symbol("CPXNETcopynet");
    return native_CPXNETcopynet(env, net, objsen, nnodes, supply, nnames, narcs, fromnode, tonode, low, up, obj, anames);
}
CPXLIBAPI CPXNETptr CPXPUBLIC CPXNETcreateprob (CPXENVptr env, int *status_p, char const *name_str){
    if (!native_CPXNETcreateprob)
        native_CPXNETcreateprob = load_symbol("CPXNETcreateprob");
    return native_CPXNETcreateprob(env, status_p, name_str);
}
CPXLIBAPI int CPXPUBLIC CPXNETdelarcs (CPXCENVptr env, CPXNETptr net, int begin, int end){
    if (!native_CPXNETdelarcs)
        native_CPXNETdelarcs = load_symbol("CPXNETdelarcs");
    return native_CPXNETdelarcs(env, net, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETdelnodes (CPXCENVptr env, CPXNETptr net, int begin, int end){
    if (!native_CPXNETdelnodes)
        native_CPXNETdelnodes = load_symbol("CPXNETdelnodes");
    return native_CPXNETdelnodes(env, net, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETdelset (CPXCENVptr env, CPXNETptr net, int *whichnodes, int *whicharcs){
    if (!native_CPXNETdelset)
        native_CPXNETdelset = load_symbol("CPXNETdelset");
    return native_CPXNETdelset(env, net, whichnodes, whicharcs);
}
CPXLIBAPI int CPXPUBLIC CPXNETfreeprob (CPXENVptr env, CPXNETptr * net_p){
    if (!native_CPXNETfreeprob)
        native_CPXNETfreeprob = load_symbol("CPXNETfreeprob");
    return native_CPXNETfreeprob(env, net_p);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetarcindex (CPXCENVptr env, CPXCNETptr net, char const *lname_str, int *index_p){
    if (!native_CPXNETgetarcindex)
        native_CPXNETgetarcindex = load_symbol("CPXNETgetarcindex");
    return native_CPXNETgetarcindex(env, net, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetarcname (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, int namespc, int *surplus_p, int begin, int end){
    if (!native_CPXNETgetarcname)
        native_CPXNETgetarcname = load_symbol("CPXNETgetarcname");
    return native_CPXNETgetarcname(env, net, nnames, namestore, namespc, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetarcnodes (CPXCENVptr env, CPXCNETptr net, int *fromnode, int *tonode, int begin, int end){
    if (!native_CPXNETgetarcnodes)
        native_CPXNETgetarcnodes = load_symbol("CPXNETgetarcnodes");
    return native_CPXNETgetarcnodes(env, net, fromnode, tonode, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetbase (CPXCENVptr env, CPXCNETptr net, int *astat, int *nstat){
    if (!native_CPXNETgetbase)
        native_CPXNETgetbase = load_symbol("CPXNETgetbase");
    return native_CPXNETgetbase(env, net, astat, nstat);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetdj (CPXCENVptr env, CPXCNETptr net, double *dj, int begin, int end){
    if (!native_CPXNETgetdj)
        native_CPXNETgetdj = load_symbol("CPXNETgetdj");
    return native_CPXNETgetdj(env, net, dj, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetitcnt (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXNETgetitcnt)
        native_CPXNETgetitcnt = load_symbol("CPXNETgetitcnt");
    return native_CPXNETgetitcnt(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetlb (CPXCENVptr env, CPXCNETptr net, double *low, int begin, int end){
    if (!native_CPXNETgetlb)
        native_CPXNETgetlb = load_symbol("CPXNETgetlb");
    return native_CPXNETgetlb(env, net, low, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetnodearcs (CPXCENVptr env, CPXCNETptr net, int *arccnt_p, int *arcbeg, int *arc, int arcspace, int *surplus_p, int begin, int end){
    if (!native_CPXNETgetnodearcs)
        native_CPXNETgetnodearcs = load_symbol("CPXNETgetnodearcs");
    return native_CPXNETgetnodearcs(env, net, arccnt_p, arcbeg, arc, arcspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetnodeindex (CPXCENVptr env, CPXCNETptr net, char const *lname_str, int *index_p){
    if (!native_CPXNETgetnodeindex)
        native_CPXNETgetnodeindex = load_symbol("CPXNETgetnodeindex");
    return native_CPXNETgetnodeindex(env, net, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetnodename (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, int namespc, int *surplus_p, int begin, int end){
    if (!native_CPXNETgetnodename)
        native_CPXNETgetnodename = load_symbol("CPXNETgetnodename");
    return native_CPXNETgetnodename(env, net, nnames, namestore, namespc, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetnumarcs (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXNETgetnumarcs)
        native_CPXNETgetnumarcs = load_symbol("CPXNETgetnumarcs");
    return native_CPXNETgetnumarcs(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetnumnodes (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXNETgetnumnodes)
        native_CPXNETgetnumnodes = load_symbol("CPXNETgetnumnodes");
    return native_CPXNETgetnumnodes(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetobj (CPXCENVptr env, CPXCNETptr net, double *obj, int begin, int end){
    if (!native_CPXNETgetobj)
        native_CPXNETgetobj = load_symbol("CPXNETgetobj");
    return native_CPXNETgetobj(env, net, obj, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetobjsen (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXNETgetobjsen)
        native_CPXNETgetobjsen = load_symbol("CPXNETgetobjsen");
    return native_CPXNETgetobjsen(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetobjval (CPXCENVptr env, CPXCNETptr net, double *objval_p){
    if (!native_CPXNETgetobjval)
        native_CPXNETgetobjval = load_symbol("CPXNETgetobjval");
    return native_CPXNETgetobjval(env, net, objval_p);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetphase1cnt (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXNETgetphase1cnt)
        native_CPXNETgetphase1cnt = load_symbol("CPXNETgetphase1cnt");
    return native_CPXNETgetphase1cnt(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetpi (CPXCENVptr env, CPXCNETptr net, double *pi, int begin, int end){
    if (!native_CPXNETgetpi)
        native_CPXNETgetpi = load_symbol("CPXNETgetpi");
    return native_CPXNETgetpi(env, net, pi, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetprobname (CPXCENVptr env, CPXCNETptr net, char *buf_str, int bufspace, int *surplus_p){
    if (!native_CPXNETgetprobname)
        native_CPXNETgetprobname = load_symbol("CPXNETgetprobname");
    return native_CPXNETgetprobname(env, net, buf_str, bufspace, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetslack (CPXCENVptr env, CPXCNETptr net, double *slack, int begin, int end){
    if (!native_CPXNETgetslack)
        native_CPXNETgetslack = load_symbol("CPXNETgetslack");
    return native_CPXNETgetslack(env, net, slack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetstat (CPXCENVptr env, CPXCNETptr net){
    if (!native_CPXNETgetstat)
        native_CPXNETgetstat = load_symbol("CPXNETgetstat");
    return native_CPXNETgetstat(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetsupply (CPXCENVptr env, CPXCNETptr net, double *supply, int begin, int end){
    if (!native_CPXNETgetsupply)
        native_CPXNETgetsupply = load_symbol("CPXNETgetsupply");
    return native_CPXNETgetsupply(env, net, supply, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetub (CPXCENVptr env, CPXCNETptr net, double *up, int begin, int end){
    if (!native_CPXNETgetub)
        native_CPXNETgetub = load_symbol("CPXNETgetub");
    return native_CPXNETgetub(env, net, up, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETgetx (CPXCENVptr env, CPXCNETptr net, double *x, int begin, int end){
    if (!native_CPXNETgetx)
        native_CPXNETgetx = load_symbol("CPXNETgetx");
    return native_CPXNETgetx(env, net, x, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXNETprimopt (CPXCENVptr env, CPXNETptr net){
    if (!native_CPXNETprimopt)
        native_CPXNETprimopt = load_symbol("CPXNETprimopt");
    return native_CPXNETprimopt(env, net);
}
CPXLIBAPI int CPXPUBLIC CPXNETreadcopybase (CPXCENVptr env, CPXNETptr net, char const *filename_str){
    if (!native_CPXNETreadcopybase)
        native_CPXNETreadcopybase = load_symbol("CPXNETreadcopybase");
    return native_CPXNETreadcopybase(env, net, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXNETreadcopyprob (CPXCENVptr env, CPXNETptr net, char const *filename_str){
    if (!native_CPXNETreadcopyprob)
        native_CPXNETreadcopyprob = load_symbol("CPXNETreadcopyprob");
    return native_CPXNETreadcopyprob(env, net, filename_str);
}
CPXLIBAPI int CPXPUBLIC CPXNETsolninfo (CPXCENVptr env, CPXCNETptr net, int *pfeasind_p, int *dfeasind_p){
    if (!native_CPXNETsolninfo)
        native_CPXNETsolninfo = load_symbol("CPXNETsolninfo");
    return native_CPXNETsolninfo(env, net, pfeasind_p, dfeasind_p);
}
CPXLIBAPI int CPXPUBLIC CPXNETsolution (CPXCENVptr env, CPXCNETptr net, int *netstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj){
    if (!native_CPXNETsolution)
        native_CPXNETsolution = load_symbol("CPXNETsolution");
    return native_CPXNETsolution(env, net, netstat_p, objval_p, x, pi, slack, dj);
}
CPXLIBAPI int CPXPUBLIC CPXNETwriteprob (CPXCENVptr env, CPXCNETptr net, char const *filename_str, char const *format_str){
    if (!native_CPXNETwriteprob)
        native_CPXNETwriteprob = load_symbol("CPXNETwriteprob");
    return native_CPXNETwriteprob(env, net, filename_str, format_str);
}
CPXLIBAPI int CPXPUBLIC CPXchgqpcoef (CPXCENVptr env, CPXLPptr lp, int i, int j, double newvalue){
    if (!native_CPXchgqpcoef)
        native_CPXchgqpcoef = load_symbol("CPXchgqpcoef");
    return native_CPXchgqpcoef(env, lp, i, j, newvalue);
}
CPXLIBAPI int CPXPUBLIC CPXcopyqpsep (CPXCENVptr env, CPXLPptr lp, double const *qsepvec){
    if (!native_CPXcopyqpsep)
        native_CPXcopyqpsep = load_symbol("CPXcopyqpsep");
    return native_CPXcopyqpsep(env, lp, qsepvec);
}
CPXLIBAPI int CPXPUBLIC CPXcopyquad (CPXCENVptr env, CPXLPptr lp, int const *qmatbeg, int const *qmatcnt, int const *qmatind, double const *qmatval){
    if (!native_CPXcopyquad)
        native_CPXcopyquad = load_symbol("CPXcopyquad");
    return native_CPXcopyquad(env, lp, qmatbeg, qmatcnt, qmatind, qmatval);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumqpnz (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumqpnz)
        native_CPXgetnumqpnz = load_symbol("CPXgetnumqpnz");
    return native_CPXgetnumqpnz(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumquad (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumquad)
        native_CPXgetnumquad = load_symbol("CPXgetnumquad");
    return native_CPXgetnumquad(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetqpcoef (CPXCENVptr env, CPXCLPptr lp, int rownum, int colnum, double *coef_p){
    if (!native_CPXgetqpcoef)
        native_CPXgetqpcoef = load_symbol("CPXgetqpcoef");
    return native_CPXgetqpcoef(env, lp, rownum, colnum, coef_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetquad (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *qmatbeg, int *qmatind, double *qmatval, int qmatspace, int *surplus_p, int begin, int end){
    if (!native_CPXgetquad)
        native_CPXgetquad = load_symbol("CPXgetquad");
    return native_CPXgetquad(env, lp, nzcnt_p, qmatbeg, qmatind, qmatval, qmatspace, surplus_p, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXqpindefcertificate (CPXCENVptr env, CPXCLPptr lp, double *x){
    if (!native_CPXqpindefcertificate)
        native_CPXqpindefcertificate = load_symbol("CPXqpindefcertificate");
    return native_CPXqpindefcertificate(env, lp, x);
}
CPXLIBAPI int CPXPUBLIC CPXqpopt (CPXCENVptr env, CPXLPptr lp){
    if (!native_CPXqpopt)
        native_CPXqpopt = load_symbol("CPXqpopt");
    return native_CPXqpopt(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXaddqconstr (CPXCENVptr env, CPXLPptr lp, int linnzcnt, int quadnzcnt, double rhs, int sense, int const *linind, double const *linval, int const *quadrow, int const *quadcol, double const *quadval, char const *lname_str){
    if (!native_CPXaddqconstr)
        native_CPXaddqconstr = load_symbol("CPXaddqconstr");
    return native_CPXaddqconstr(env, lp, linnzcnt, quadnzcnt, rhs, sense, linind, linval, quadrow, quadcol, quadval, lname_str);
}
CPXLIBAPI int CPXPUBLIC CPXdelqconstrs (CPXCENVptr env, CPXLPptr lp, int begin, int end){
    if (!native_CPXdelqconstrs)
        native_CPXdelqconstrs = load_symbol("CPXdelqconstrs");
    return native_CPXdelqconstrs(env, lp, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetnumqconstrs (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXgetnumqconstrs)
        native_CPXgetnumqconstrs = load_symbol("CPXgetnumqconstrs");
    return native_CPXgetnumqconstrs(env, lp);
}
CPXLIBAPI int CPXPUBLIC CPXgetqconstr (CPXCENVptr env, CPXCLPptr lp, int *linnzcnt_p, int *quadnzcnt_p, double *rhs_p, char *sense_p, int *linind, double *linval, int linspace, int *linsurplus_p, int *quadrow, int *quadcol, double *quadval, int quadspace, int *quadsurplus_p, int which){
    if (!native_CPXgetqconstr)
        native_CPXgetqconstr = load_symbol("CPXgetqconstr");
    return native_CPXgetqconstr(env, lp, linnzcnt_p, quadnzcnt_p, rhs_p, sense_p, linind, linval, linspace, linsurplus_p, quadrow, quadcol, quadval, quadspace, quadsurplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXgetqconstrdslack (CPXCENVptr env, CPXCLPptr lp, int qind, int *nz_p, int *ind, double *val, int space, int *surplus_p){
    if (!native_CPXgetqconstrdslack)
        native_CPXgetqconstrdslack = load_symbol("CPXgetqconstrdslack");
    return native_CPXgetqconstrdslack(env, lp, qind, nz_p, ind, val, space, surplus_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetqconstrindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p){
    if (!native_CPXgetqconstrindex)
        native_CPXgetqconstrindex = load_symbol("CPXgetqconstrindex");
    return native_CPXgetqconstrindex(env, lp, lname_str, index_p);
}
CPXLIBAPI int CPXPUBLIC CPXgetqconstrinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end){
    if (!native_CPXgetqconstrinfeas)
        native_CPXgetqconstrinfeas = load_symbol("CPXgetqconstrinfeas");
    return native_CPXgetqconstrinfeas(env, lp, x, infeasout, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetqconstrname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p, int which){
    if (!native_CPXgetqconstrname)
        native_CPXgetqconstrname = load_symbol("CPXgetqconstrname");
    return native_CPXgetqconstrname(env, lp, buf_str, bufspace, surplus_p, which);
}
CPXLIBAPI int CPXPUBLIC CPXgetqconstrslack (CPXCENVptr env, CPXCLPptr lp, double *qcslack, int begin, int end){
    if (!native_CPXgetqconstrslack)
        native_CPXgetqconstrslack = load_symbol("CPXgetqconstrslack");
    return native_CPXgetqconstrslack(env, lp, qcslack, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXgetxqxax (CPXCENVptr env, CPXCLPptr lp, double *xqxax, int begin, int end){
    if (!native_CPXgetxqxax)
        native_CPXgetxqxax = load_symbol("CPXgetxqxax");
    return native_CPXgetxqxax(env, lp, xqxax, begin, end);
}
CPXLIBAPI int CPXPUBLIC CPXqconstrslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *qcslack){
    if (!native_CPXqconstrslackfromx)
        native_CPXqconstrslackfromx = load_symbol("CPXqconstrslackfromx");
    return native_CPXqconstrslackfromx(env, lp, x, qcslack);
}
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
CPXLIBAPI int CPXPUBLIC CPXLchgobjoffset (CPXCENVptr env, CPXLPptr lp, double offset){
    if (!native_CPXLchgobjoffset)
        native_CPXLchgobjoffset = load_symbol("CPXLchgobjoffset");
    return native_CPXLchgobjoffset(env, lp, offset);
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
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumlazyconstraints (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumlazyconstraints)
        native_CPXLgetnumlazyconstraints = load_symbol("CPXLgetnumlazyconstraints");
    return native_CPXLgetnumlazyconstraints(env, lp);
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
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumusercuts (CPXCENVptr env, CPXCLPptr lp){
    if (!native_CPXLgetnumusercuts)
        native_CPXLgetnumusercuts = load_symbol("CPXLgetnumusercuts");
    return native_CPXLgetnumusercuts(env, lp);
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
