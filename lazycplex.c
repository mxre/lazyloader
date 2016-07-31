#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "lazyloader.h"
#define DEBUG_ENVIRONMENT_VARIABLE "LAZYCPLEX_DEBUG"
#define LOADER_ENVIRONMENT_VARIABLE "LAZYLOAD_CPLEX_DLL"

static int test_library(int min_version);

static void* load_symbol(const char* name);

static void exit_lazy_loader(void);

static void default_failure_callback(const char* symbol, void* cb_data);

static void (*failure_callback)(const char* symbol, void* cb_data) = default_failure_callback;
static void* callback_data = NULL;

// debugging macros
#ifdef _MSC_VER
#define PRINT_DEBUG(fmt, ...) \
    if (debug_enabled) {\
        fprintf(stderr, "\n(%s): " fmt, __FUNCTION__, __VA_ARGS__ );\
    }
#define PRINT_ERR(fmt, ...) \
    fprintf(stderr, "\n(%s): " fmt, __FUNCTION__, __VA_ARGS__ );
#else // For GCC comaptible compilers
#define PRINT_DEBUG(fmt, args ...) \
    if (debug_enabled) {\
        fprintf(stderr, "\n(%s): " fmt, __FUNCTION__, ## args );\
    }
#define PRINT_ERR(fmt, args ...) \
    fprintf(stderr, "\n(%s): " fmt, __FUNCTION__, ## args );
#endif

// platfrom macros
#ifdef _WIN32
#define SYMBOL(name) GetProcAddress(handle, name)
#else
#define SYMBOL(name) dlsym(handle, name)
#endif
#ifdef _WIN32
#define FREE FreeLibrary(handle)
#else
#define FREE dlclose(handle)
#endif
#ifdef _WIN32
#define OPEN(library) LoadLibrary(library)
#else
#define OPEN(library) dlopen(library, RTLD_LAZY)
#endif

// handle of the loaded library
static void* handle = NULL;

static bool debug_enabled = false;

int initialize_lazy_loader(int min_version) {
    const char *dbg = getenv(DEBUG_ENVIRONMENT_VARIABLE);
    debug_enabled = (dbg != NULL && strlen(dbg) > 0);

    if (handle != NULL) {
        PRINT_DEBUG("The library is already initialized.\n");
        return 0;
    }

    int i;
    char* s = getenv(LOADER_ENVIRONMENT_VARIABLE);
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
        "cplex1263.dll",
        "cplex1262.dll",
        "cplex1261.dll",
        "cplex1260.dll",
        "cplex125.dll",
        "cplex124.dll",
        "cplex123.dll",

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

int try_lazy_load(const char* library, int min_version)
{
    const char *dbg = getenv(DEBUG_ENVIRONMENT_VARIABLE);
    debug_enabled = (dbg != NULL && strlen(dbg) > 0);

    if (handle != NULL) {
        PRINT_DEBUG("The library is already initialized.\n");
        return 0;
    }

    PRINT_DEBUG("Trying to load %s\n", library);
    handle = OPEN(library);
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

static int test_library(int min_version)
{
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

// load a symbol with the given name
static __inline void* load_symbol(const char *name)
{
    void* symbol = NULL;
    if (!handle || !(symbol = SYMBOL(name))) {
        failure_callback(name, callback_data);
    } else {
        PRINT_DEBUG("successfully imported the symbol %s\n", name);
    }
    return symbol;
}

void* get_lazy_handle()
{
	return handle;
}

void set_lazy_handle(void* handle)
{
	handle = handle;
}

static void exit_lazy_loader(void)
{
    if (handle != NULL)
        FREE;
        handle = NULL;
}

void set_lazyloader_error_callback(void (*f)(const char *err, void* cb_data), void* cb_data) {
    failure_callback = f;
    callback_data = cb_data;
}

// Prints the failing symbol and aborts
void default_failure_callback(const char* symbol, void* cb_data)
{
    PRINT_ERR("the symbol %s could not be found!\n", symbol);
    abort();
}

// for windows create static symbols
#define BUILD_CPXSTATIC 1
// for GCC
#ifdef _WIN64
#define _LP64 1
#endif
#include "cplexx.h"

// remove for windows some of the macros
#undef CPXPUBLIC
#define CPXPUBLIC
#undef CPXPUBVARARGS
#define CPXPUBVARARGS

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

// CPXaddchannel
CPXCHANNELptr CPXPUBLIC CPXaddchannel (CPXENVptr env)
{
    if (native_CPXaddchannel == NULL)
        native_CPXaddchannel = load_symbol("CPXaddchannel");
    return native_CPXaddchannel(env);
}

// CPXaddcols
CPXLIBAPI int CPXPUBLIC CPXaddcols (CPXCENVptr env, CPXLPptr lp, int ccnt, int nzcnt, double const *obj, int const *cmatbeg, int const *cmatind, double const *cmatval, double const *lb, double const *ub, char **colname)
{
    if (native_CPXaddcols == NULL)
        native_CPXaddcols = load_symbol("CPXaddcols");
    return native_CPXaddcols(env, lp, ccnt, nzcnt, obj, cmatbeg, cmatind, cmatval, lb, ub, colname);
}

// CPXaddfpdest
CPXLIBAPI int CPXPUBLIC CPXaddfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr)
{
    if (native_CPXaddfpdest == NULL)
        native_CPXaddfpdest = load_symbol("CPXaddfpdest");
    return native_CPXaddfpdest(env, channel, fileptr);
}

// CPXaddfuncdest
CPXLIBAPI int CPXPUBLIC CPXaddfuncdest (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, const char *))
{
    if (native_CPXaddfuncdest == NULL)
        native_CPXaddfuncdest = load_symbol("CPXaddfuncdest");
    return native_CPXaddfuncdest(env, channel, handle, msgfunction);
}

// CPXaddrows
CPXLIBAPI int CPXPUBLIC CPXaddrows (CPXCENVptr env, CPXLPptr lp, int ccnt, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **colname, char **rowname)
{
    if (native_CPXaddrows == NULL)
        native_CPXaddrows = load_symbol("CPXaddrows");
    return native_CPXaddrows(env, lp, ccnt, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, colname, rowname);
}

// CPXbasicpresolve
CPXLIBAPI int CPXPUBLIC CPXbasicpresolve (CPXCENVptr env, CPXLPptr lp, double *redlb, double *redub, int *rstat)
{
    if (native_CPXbasicpresolve == NULL)
        native_CPXbasicpresolve = load_symbol("CPXbasicpresolve");
    return native_CPXbasicpresolve(env, lp, redlb, redub, rstat);
}

// CPXbinvacol
CPXLIBAPI int CPXPUBLIC CPXbinvacol (CPXCENVptr env, CPXCLPptr lp, int j, double *x)
{
    if (native_CPXbinvacol == NULL)
        native_CPXbinvacol = load_symbol("CPXbinvacol");
    return native_CPXbinvacol(env, lp, j, x);
}

// CPXbinvarow
CPXLIBAPI int CPXPUBLIC CPXbinvarow (CPXCENVptr env, CPXCLPptr lp, int i, double *z)
{
    if (native_CPXbinvarow == NULL)
        native_CPXbinvarow = load_symbol("CPXbinvarow");
    return native_CPXbinvarow(env, lp, i, z);
}

// CPXbinvcol
CPXLIBAPI int CPXPUBLIC CPXbinvcol (CPXCENVptr env, CPXCLPptr lp, int j, double *x)
{
    if (native_CPXbinvcol == NULL)
        native_CPXbinvcol = load_symbol("CPXbinvcol");
    return native_CPXbinvcol(env, lp, j, x);
}

// CPXbinvrow
CPXLIBAPI int CPXPUBLIC CPXbinvrow (CPXCENVptr env, CPXCLPptr lp, int i, double *y)
{
    if (native_CPXbinvrow == NULL)
        native_CPXbinvrow = load_symbol("CPXbinvrow");
    return native_CPXbinvrow(env, lp, i, y);
}

// CPXboundsa
CPXLIBAPI int CPXPUBLIC CPXboundsa (CPXCENVptr env, CPXCLPptr lp, int begin, int end, double *lblower, double *lbupper, double *ublower, double *ubupper)
{
    if (native_CPXboundsa == NULL)
        native_CPXboundsa = load_symbol("CPXboundsa");
    return native_CPXboundsa(env, lp, begin, end, lblower, lbupper, ublower, ubupper);
}

// CPXbtran
CPXLIBAPI int CPXPUBLIC CPXbtran (CPXCENVptr env, CPXCLPptr lp, double *y)
{
    if (native_CPXbtran == NULL)
        native_CPXbtran = load_symbol("CPXbtran");
    return native_CPXbtran(env, lp, y);
}

// CPXcheckdfeas
CPXLIBAPI int CPXPUBLIC CPXcheckdfeas (CPXCENVptr env, CPXLPptr lp, int *infeas_p)
{
    if (native_CPXcheckdfeas == NULL)
        native_CPXcheckdfeas = load_symbol("CPXcheckdfeas");
    return native_CPXcheckdfeas(env, lp, infeas_p);
}

// CPXcheckpfeas
CPXLIBAPI int CPXPUBLIC CPXcheckpfeas (CPXCENVptr env, CPXLPptr lp, int *infeas_p)
{
    if (native_CPXcheckpfeas == NULL)
        native_CPXcheckpfeas = load_symbol("CPXcheckpfeas");
    return native_CPXcheckpfeas(env, lp, infeas_p);
}

// CPXchecksoln
CPXLIBAPI int CPXPUBLIC CPXchecksoln (CPXCENVptr env, CPXLPptr lp, int *lpstatus_p)
{
    if (native_CPXchecksoln == NULL)
        native_CPXchecksoln = load_symbol("CPXchecksoln");
    return native_CPXchecksoln(env, lp, lpstatus_p);
}

// CPXchgbds
CPXLIBAPI int CPXPUBLIC CPXchgbds (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *lu, double const *bd)
{
    if (native_CPXchgbds == NULL)
        native_CPXchgbds = load_symbol("CPXchgbds");
    return native_CPXchgbds(env, lp, cnt, indices, lu, bd);
}

// CPXchgcoef
CPXLIBAPI int CPXPUBLIC CPXchgcoef (CPXCENVptr env, CPXLPptr lp, int i, int j, double newvalue)
{
    if (native_CPXchgcoef == NULL)
        native_CPXchgcoef = load_symbol("CPXchgcoef");
    return native_CPXchgcoef(env, lp, i, j, newvalue);
}

// CPXchgcoeflist
CPXLIBAPI int CPXPUBLIC CPXchgcoeflist (CPXCENVptr env, CPXLPptr lp, int numcoefs, int const *rowlist, int const *collist, double const *vallist)
{
    if (native_CPXchgcoeflist == NULL)
        native_CPXchgcoeflist = load_symbol("CPXchgcoeflist");
    return native_CPXchgcoeflist(env, lp, numcoefs, rowlist, collist, vallist);
}

// CPXchgcolname
CPXLIBAPI int CPXPUBLIC CPXchgcolname (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char **newname)
{
    if (native_CPXchgcolname == NULL)
        native_CPXchgcolname = load_symbol("CPXchgcolname");
    return native_CPXchgcolname(env, lp, cnt, indices, newname);
}

// CPXchgname
CPXLIBAPI int CPXPUBLIC CPXchgname (CPXCENVptr env, CPXLPptr lp, int key, int ij, char const *newname_str)
{
    if (native_CPXchgname == NULL)
        native_CPXchgname = load_symbol("CPXchgname");
    return native_CPXchgname(env, lp, key, ij, newname_str);
}

// CPXchgobj
CPXLIBAPI int CPXPUBLIC CPXchgobj (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values)
{
    if (native_CPXchgobj == NULL)
        native_CPXchgobj = load_symbol("CPXchgobj");
    return native_CPXchgobj(env, lp, cnt, indices, values);
}

// CPXchgobjoffset
CPXLIBAPI int CPXPUBLIC CPXchgobjoffset (CPXCENVptr env, CPXLPptr lp, double offset)
{
    if (native_CPXchgobjoffset == NULL)
        native_CPXchgobjoffset = load_symbol("CPXchgobjoffset");
    return native_CPXchgobjoffset(env, lp, offset);
}

// CPXchgobjsen
CPXLIBAPI int CPXPUBLIC CPXchgobjsen (CPXCENVptr env, CPXLPptr lp, int maxormin)
{
    if (native_CPXchgobjsen == NULL)
        native_CPXchgobjsen = load_symbol("CPXchgobjsen");
    return native_CPXchgobjsen(env, lp, maxormin);
}

// CPXchgprobname
CPXLIBAPI int CPXPUBLIC CPXchgprobname (CPXCENVptr env, CPXLPptr lp, char const *probname)
{
    if (native_CPXchgprobname == NULL)
        native_CPXchgprobname = load_symbol("CPXchgprobname");
    return native_CPXchgprobname(env, lp, probname);
}

// CPXchgprobtype
CPXLIBAPI int CPXPUBLIC CPXchgprobtype (CPXCENVptr env, CPXLPptr lp, int type)
{
    if (native_CPXchgprobtype == NULL)
        native_CPXchgprobtype = load_symbol("CPXchgprobtype");
    return native_CPXchgprobtype(env, lp, type);
}

// CPXchgprobtypesolnpool
CPXLIBAPI int CPXPUBLIC CPXchgprobtypesolnpool (CPXCENVptr env, CPXLPptr lp, int type, int soln)
{
    if (native_CPXchgprobtypesolnpool == NULL)
        native_CPXchgprobtypesolnpool = load_symbol("CPXchgprobtypesolnpool");
    return native_CPXchgprobtypesolnpool(env, lp, type, soln);
}

// CPXchgrhs
CPXLIBAPI int CPXPUBLIC CPXchgrhs (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values)
{
    if (native_CPXchgrhs == NULL)
        native_CPXchgrhs = load_symbol("CPXchgrhs");
    return native_CPXchgrhs(env, lp, cnt, indices, values);
}

// CPXchgrngval
CPXLIBAPI int CPXPUBLIC CPXchgrngval (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values)
{
    if (native_CPXchgrngval == NULL)
        native_CPXchgrngval = load_symbol("CPXchgrngval");
    return native_CPXchgrngval(env, lp, cnt, indices, values);
}

// CPXchgrowname
CPXLIBAPI int CPXPUBLIC CPXchgrowname (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char **newname)
{
    if (native_CPXchgrowname == NULL)
        native_CPXchgrowname = load_symbol("CPXchgrowname");
    return native_CPXchgrowname(env, lp, cnt, indices, newname);
}

// CPXchgsense
CPXLIBAPI int CPXPUBLIC CPXchgsense (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *sense)
{
    if (native_CPXchgsense == NULL)
        native_CPXchgsense = load_symbol("CPXchgsense");
    return native_CPXchgsense(env, lp, cnt, indices, sense);
}

// CPXcleanup
CPXLIBAPI int CPXPUBLIC CPXcleanup (CPXCENVptr env, CPXLPptr lp, double eps)
{
    if (native_CPXcleanup == NULL)
        native_CPXcleanup = load_symbol("CPXcleanup");
    return native_CPXcleanup(env, lp, eps);
}

// CPXcloneprob
CPXLIBAPI CPXLPptr CPXPUBLIC CPXcloneprob (CPXCENVptr env, CPXCLPptr lp, int *status_p)
{
    if (native_CPXcloneprob == NULL)
        native_CPXcloneprob = load_symbol("CPXcloneprob");
    return native_CPXcloneprob(env, lp, status_p);
}

// CPXcloseCPLEX
CPXLIBAPI int CPXPUBLIC CPXcloseCPLEX (CPXENVptr * env_p)
{
    if (native_CPXcloseCPLEX == NULL)
        native_CPXcloseCPLEX = load_symbol("CPXcloseCPLEX");
    return native_CPXcloseCPLEX(env_p);
}

// CPXclpwrite
CPXLIBAPI int CPXPUBLIC CPXclpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXclpwrite == NULL)
        native_CPXclpwrite = load_symbol("CPXclpwrite");
    return native_CPXclpwrite(env, lp, filename_str);
}

// CPXcompletelp
CPXLIBAPI int CPXPUBLIC CPXcompletelp (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXcompletelp == NULL)
        native_CPXcompletelp = load_symbol("CPXcompletelp");
    return native_CPXcompletelp(env, lp);
}

// CPXcopybase
CPXLIBAPI int CPXPUBLIC CPXcopybase (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat)
{
    if (native_CPXcopybase == NULL)
        native_CPXcopybase = load_symbol("CPXcopybase");
    return native_CPXcopybase(env, lp, cstat, rstat);
}

// CPXcopybasednorms
CPXLIBAPI int CPXPUBLIC CPXcopybasednorms (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *dnorm)
{
    if (native_CPXcopybasednorms == NULL)
        native_CPXcopybasednorms = load_symbol("CPXcopybasednorms");
    return native_CPXcopybasednorms(env, lp, cstat, rstat, dnorm);
}

// CPXcopydnorms
CPXLIBAPI int CPXPUBLIC CPXcopydnorms (CPXCENVptr env, CPXLPptr lp, double const *norm, int const *head, int len)
{
    if (native_CPXcopydnorms == NULL)
        native_CPXcopydnorms = load_symbol("CPXcopydnorms");
    return native_CPXcopydnorms(env, lp, norm, head, len);
}

// CPXcopylp
CPXLIBAPI int CPXPUBLIC CPXcopylp (CPXCENVptr env, CPXLPptr lp, int numcols, int numrows, int objsense, double const *objective, double const *rhs, char const *sense, int const *matbeg, int const *matcnt, int const *matind, double const *matval, double const *lb, double const *ub, double const *rngval)
{
    if (native_CPXcopylp == NULL)
        native_CPXcopylp = load_symbol("CPXcopylp");
    return native_CPXcopylp(env, lp, numcols, numrows, objsense, objective, rhs, sense, matbeg, matcnt, matind, matval, lb, ub, rngval);
}

// CPXcopylpwnames
CPXLIBAPI int CPXPUBLIC CPXcopylpwnames (CPXCENVptr env, CPXLPptr lp, int numcols, int numrows, int objsense, double const *objective, double const *rhs, char const *sense, int const *matbeg, int const *matcnt, int const *matind, double const *matval, double const *lb, double const *ub, double const *rngval, char **colname, char **rowname)
{
    if (native_CPXcopylpwnames == NULL)
        native_CPXcopylpwnames = load_symbol("CPXcopylpwnames");
    return native_CPXcopylpwnames(env, lp, numcols, numrows, objsense, objective, rhs, sense, matbeg, matcnt, matind, matval, lb, ub, rngval, colname, rowname);
}

// CPXcopynettolp
CPXLIBAPI int CPXPUBLIC CPXcopynettolp (CPXCENVptr env, CPXLPptr lp, CPXCNETptr net)
{
    if (native_CPXcopynettolp == NULL)
        native_CPXcopynettolp = load_symbol("CPXcopynettolp");
    return native_CPXcopynettolp(env, lp, net);
}

// CPXcopyobjname
CPXLIBAPI int CPXPUBLIC CPXcopyobjname (CPXCENVptr env, CPXLPptr lp, char const *objname_str)
{
    if (native_CPXcopyobjname == NULL)
        native_CPXcopyobjname = load_symbol("CPXcopyobjname");
    return native_CPXcopyobjname(env, lp, objname_str);
}

// CPXcopypartialbase
CPXLIBAPI int CPXPUBLIC CPXcopypartialbase (CPXCENVptr env, CPXLPptr lp, int ccnt, int const *cindices, int const *cstat, int rcnt, int const *rindices, int const *rstat)
{
    if (native_CPXcopypartialbase == NULL)
        native_CPXcopypartialbase = load_symbol("CPXcopypartialbase");
    return native_CPXcopypartialbase(env, lp, ccnt, cindices, cstat, rcnt, rindices, rstat);
}

// CPXcopypnorms
CPXLIBAPI int CPXPUBLIC CPXcopypnorms (CPXCENVptr env, CPXLPptr lp, double const *cnorm, double const *rnorm, int len)
{
    if (native_CPXcopypnorms == NULL)
        native_CPXcopypnorms = load_symbol("CPXcopypnorms");
    return native_CPXcopypnorms(env, lp, cnorm, rnorm, len);
}

// CPXcopyprotected
CPXLIBAPI int CPXPUBLIC CPXcopyprotected (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices)
{
    if (native_CPXcopyprotected == NULL)
        native_CPXcopyprotected = load_symbol("CPXcopyprotected");
    return native_CPXcopyprotected(env, lp, cnt, indices);
}

// CPXcopystart
CPXLIBAPI int CPXPUBLIC CPXcopystart (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *cprim, double const *rprim, double const *cdual, double const *rdual)
{
    if (native_CPXcopystart == NULL)
        native_CPXcopystart = load_symbol("CPXcopystart");
    return native_CPXcopystart(env, lp, cstat, rstat, cprim, rprim, cdual, rdual);
}

// CPXcreateprob
CPXLIBAPI CPXLPptr CPXPUBLIC CPXcreateprob (CPXCENVptr env, int *status_p, char const *probname_str)
{
    if (native_CPXcreateprob == NULL)
        native_CPXcreateprob = load_symbol("CPXcreateprob");
    return native_CPXcreateprob(env, status_p, probname_str);
}

// CPXcrushform
CPXLIBAPI int CPXPUBLIC CPXcrushform (CPXCENVptr env, CPXCLPptr lp, int len, int const *ind, double const *val, int *plen_p, double *poffset_p, int *pind, double *pval)
{
    if (native_CPXcrushform == NULL)
        native_CPXcrushform = load_symbol("CPXcrushform");
    return native_CPXcrushform(env, lp, len, ind, val, plen_p, poffset_p, pind, pval);
}

// CPXcrushpi
CPXLIBAPI int CPXPUBLIC CPXcrushpi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *prepi)
{
    if (native_CPXcrushpi == NULL)
        native_CPXcrushpi = load_symbol("CPXcrushpi");
    return native_CPXcrushpi(env, lp, pi, prepi);
}

// CPXcrushx
CPXLIBAPI int CPXPUBLIC CPXcrushx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *prex)
{
    if (native_CPXcrushx == NULL)
        native_CPXcrushx = load_symbol("CPXcrushx");
    return native_CPXcrushx(env, lp, x, prex);
}

// CPXdelchannel
int CPXPUBLIC CPXdelchannel (CPXENVptr env, CPXCHANNELptr * channel_p)
{
    if (native_CPXdelchannel == NULL)
        native_CPXdelchannel = load_symbol("CPXdelchannel");
    return native_CPXdelchannel(env, channel_p);
}

// CPXdelcols
CPXLIBAPI int CPXPUBLIC CPXdelcols (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXdelcols == NULL)
        native_CPXdelcols = load_symbol("CPXdelcols");
    return native_CPXdelcols(env, lp, begin, end);
}

// CPXdelfpdest
CPXLIBAPI int CPXPUBLIC CPXdelfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr)
{
    if (native_CPXdelfpdest == NULL)
        native_CPXdelfpdest = load_symbol("CPXdelfpdest");
    return native_CPXdelfpdest(env, channel, fileptr);
}

// CPXdelfuncdest
CPXLIBAPI int CPXPUBLIC CPXdelfuncdest (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, const char *))
{
    if (native_CPXdelfuncdest == NULL)
        native_CPXdelfuncdest = load_symbol("CPXdelfuncdest");
    return native_CPXdelfuncdest(env, channel, handle, msgfunction);
}

// CPXdelnames
CPXLIBAPI int CPXPUBLIC CPXdelnames (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXdelnames == NULL)
        native_CPXdelnames = load_symbol("CPXdelnames");
    return native_CPXdelnames(env, lp);
}

// CPXdelrows
CPXLIBAPI int CPXPUBLIC CPXdelrows (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXdelrows == NULL)
        native_CPXdelrows = load_symbol("CPXdelrows");
    return native_CPXdelrows(env, lp, begin, end);
}

// CPXdelsetcols
CPXLIBAPI int CPXPUBLIC CPXdelsetcols (CPXCENVptr env, CPXLPptr lp, int *delstat)
{
    if (native_CPXdelsetcols == NULL)
        native_CPXdelsetcols = load_symbol("CPXdelsetcols");
    return native_CPXdelsetcols(env, lp, delstat);
}

// CPXdelsetrows
CPXLIBAPI int CPXPUBLIC CPXdelsetrows (CPXCENVptr env, CPXLPptr lp, int *delstat)
{
    if (native_CPXdelsetrows == NULL)
        native_CPXdelsetrows = load_symbol("CPXdelsetrows");
    return native_CPXdelsetrows(env, lp, delstat);
}

// CPXdeserializercreate
CPXLIBAPI int CPXPUBLIC CPXdeserializercreate (CPXDESERIALIZERptr * deser_p, CPXLONG size, void const *buffer)
{
    if (native_CPXdeserializercreate == NULL)
        native_CPXdeserializercreate = load_symbol("CPXdeserializercreate");
    return native_CPXdeserializercreate(deser_p, size, buffer);
}

// CPXdeserializerdestroy
CPXLIBAPI void CPXPUBLIC CPXdeserializerdestroy (CPXDESERIALIZERptr deser)
{
    if (native_CPXdeserializerdestroy == NULL)
        native_CPXdeserializerdestroy = load_symbol("CPXdeserializerdestroy");
    return native_CPXdeserializerdestroy(deser);
}

// CPXdeserializerleft
CPXLIBAPI CPXLONG CPXPUBLIC CPXdeserializerleft (CPXCDESERIALIZERptr deser)
{
    if (native_CPXdeserializerleft == NULL)
        native_CPXdeserializerleft = load_symbol("CPXdeserializerleft");
    return native_CPXdeserializerleft(deser);
}

// CPXdisconnectchannel
CPXLIBAPI int CPXPUBLIC CPXdisconnectchannel (CPXCENVptr env, CPXCHANNELptr channel)
{
    if (native_CPXdisconnectchannel == NULL)
        native_CPXdisconnectchannel = load_symbol("CPXdisconnectchannel");
    return native_CPXdisconnectchannel(env, channel);
}

// CPXdjfrompi
CPXLIBAPI int CPXPUBLIC CPXdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *dj)
{
    if (native_CPXdjfrompi == NULL)
        native_CPXdjfrompi = load_symbol("CPXdjfrompi");
    return native_CPXdjfrompi(env, lp, pi, dj);
}

// CPXdperwrite
CPXLIBAPI int CPXPUBLIC CPXdperwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon)
{
    if (native_CPXdperwrite == NULL)
        native_CPXdperwrite = load_symbol("CPXdperwrite");
    return native_CPXdperwrite(env, lp, filename_str, epsilon);
}

// CPXdratio
CPXLIBAPI int CPXPUBLIC CPXdratio (CPXCENVptr env, CPXLPptr lp, int *indices, int cnt, double *downratio, double *upratio, int *downenter, int *upenter, int *downstatus, int *upstatus)
{
    if (native_CPXdratio == NULL)
        native_CPXdratio = load_symbol("CPXdratio");
    return native_CPXdratio(env, lp, indices, cnt, downratio, upratio, downenter, upenter, downstatus, upstatus);
}

// CPXdualfarkas
CPXLIBAPI int CPXPUBLIC CPXdualfarkas (CPXCENVptr env, CPXCLPptr lp, double *y, double *proof_p)
{
    if (native_CPXdualfarkas == NULL)
        native_CPXdualfarkas = load_symbol("CPXdualfarkas");
    return native_CPXdualfarkas(env, lp, y, proof_p);
}

// CPXdualopt
CPXLIBAPI int CPXPUBLIC CPXdualopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXdualopt == NULL)
        native_CPXdualopt = load_symbol("CPXdualopt");
    return native_CPXdualopt(env, lp);
}

// CPXdualwrite
CPXLIBAPI int CPXPUBLIC CPXdualwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, double *objshift_p)
{
    if (native_CPXdualwrite == NULL)
        native_CPXdualwrite = load_symbol("CPXdualwrite");
    return native_CPXdualwrite(env, lp, filename_str, objshift_p);
}

// CPXembwrite
CPXLIBAPI int CPXPUBLIC CPXembwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXembwrite == NULL)
        native_CPXembwrite = load_symbol("CPXembwrite");
    return native_CPXembwrite(env, lp, filename_str);
}

// CPXfclose
CPXLIBAPI int CPXPUBLIC CPXfclose (CPXFILEptr stream)
{
    if (native_CPXfclose == NULL)
        native_CPXfclose = load_symbol("CPXfclose");
    return native_CPXfclose(stream);
}

// CPXfeasopt
CPXLIBAPI int CPXPUBLIC CPXfeasopt (CPXCENVptr env, CPXLPptr lp, double const *rhs, double const *rng, double const *lb, double const *ub)
{
    if (native_CPXfeasopt == NULL)
        native_CPXfeasopt = load_symbol("CPXfeasopt");
    return native_CPXfeasopt(env, lp, rhs, rng, lb, ub);
}

// CPXfeasoptext
CPXLIBAPI int CPXPUBLIC CPXfeasoptext (CPXCENVptr env, CPXLPptr lp, int grpcnt, int concnt, double const *grppref, int const *grpbeg, int const *grpind, char const *grptype)
{
    if (native_CPXfeasoptext == NULL)
        native_CPXfeasoptext = load_symbol("CPXfeasoptext");
    return native_CPXfeasoptext(env, lp, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}

// CPXfinalize
CPXLIBAPI void CPXPUBLIC CPXfinalize (void)
{
    if (native_CPXfinalize == NULL)
        native_CPXfinalize = load_symbol("CPXfinalize");
    return native_CPXfinalize();
}

// CPXfindiis
int CPXPUBLIC CPXfindiis (CPXCENVptr env, CPXLPptr lp, int *iisnumrows_p, int *iisnumcols_p)
{
    if (native_CPXfindiis == NULL)
        native_CPXfindiis = load_symbol("CPXfindiis");
    return native_CPXfindiis(env, lp, iisnumrows_p, iisnumcols_p);
}

// CPXflushchannel
CPXLIBAPI int CPXPUBLIC CPXflushchannel (CPXCENVptr env, CPXCHANNELptr channel)
{
    if (native_CPXflushchannel == NULL)
        native_CPXflushchannel = load_symbol("CPXflushchannel");
    return native_CPXflushchannel(env, channel);
}

// CPXflushstdchannels
CPXLIBAPI int CPXPUBLIC CPXflushstdchannels (CPXCENVptr env)
{
    if (native_CPXflushstdchannels == NULL)
        native_CPXflushstdchannels = load_symbol("CPXflushstdchannels");
    return native_CPXflushstdchannels(env);
}

// CPXfopen
CPXLIBAPI CPXFILEptr CPXPUBLIC CPXfopen (char const *filename_str, char const *type_str)
{
    if (native_CPXfopen == NULL)
        native_CPXfopen = load_symbol("CPXfopen");
    return native_CPXfopen(filename_str, type_str);
}

// CPXfputs
CPXLIBAPI int CPXPUBLIC CPXfputs (char const *s_str, CPXFILEptr stream)
{
    if (native_CPXfputs == NULL)
        native_CPXfputs = load_symbol("CPXfputs");
    return native_CPXfputs(s_str, stream);
}

// CPXfree
void CPXPUBLIC CPXfree (void *ptr)
{
    if (native_CPXfree == NULL)
        native_CPXfree = load_symbol("CPXfree");
    return native_CPXfree(ptr);
}

// CPXfreeparenv
CPXLIBAPI int CPXPUBLIC CPXfreeparenv (CPXENVptr env, CPXENVptr * child_p)
{
    if (native_CPXfreeparenv == NULL)
        native_CPXfreeparenv = load_symbol("CPXfreeparenv");
    return native_CPXfreeparenv(env, child_p);
}

// CPXfreepresolve
CPXLIBAPI int CPXPUBLIC CPXfreepresolve (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXfreepresolve == NULL)
        native_CPXfreepresolve = load_symbol("CPXfreepresolve");
    return native_CPXfreepresolve(env, lp);
}

// CPXfreeprob
CPXLIBAPI int CPXPUBLIC CPXfreeprob (CPXCENVptr env, CPXLPptr * lp_p)
{
    if (native_CPXfreeprob == NULL)
        native_CPXfreeprob = load_symbol("CPXfreeprob");
    return native_CPXfreeprob(env, lp_p);
}

// CPXftran
CPXLIBAPI int CPXPUBLIC CPXftran (CPXCENVptr env, CPXCLPptr lp, double *x)
{
    if (native_CPXftran == NULL)
        native_CPXftran = load_symbol("CPXftran");
    return native_CPXftran(env, lp, x);
}

// CPXgetax
CPXLIBAPI int CPXPUBLIC CPXgetax (CPXCENVptr env, CPXCLPptr lp, double *x, int begin, int end)
{
    if (native_CPXgetax == NULL)
        native_CPXgetax = load_symbol("CPXgetax");
    return native_CPXgetax(env, lp, x, begin, end);
}

// CPXgetbaritcnt
CPXLIBAPI int CPXPUBLIC CPXgetbaritcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetbaritcnt == NULL)
        native_CPXgetbaritcnt = load_symbol("CPXgetbaritcnt");
    return native_CPXgetbaritcnt(env, lp);
}

// CPXgetbase
CPXLIBAPI int CPXPUBLIC CPXgetbase (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat)
{
    if (native_CPXgetbase == NULL)
        native_CPXgetbase = load_symbol("CPXgetbase");
    return native_CPXgetbase(env, lp, cstat, rstat);
}

// CPXgetbasednorms
CPXLIBAPI int CPXPUBLIC CPXgetbasednorms (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat, double *dnorm)
{
    if (native_CPXgetbasednorms == NULL)
        native_CPXgetbasednorms = load_symbol("CPXgetbasednorms");
    return native_CPXgetbasednorms(env, lp, cstat, rstat, dnorm);
}

// CPXgetbhead
CPXLIBAPI int CPXPUBLIC CPXgetbhead (CPXCENVptr env, CPXCLPptr lp, int *head, double *x)
{
    if (native_CPXgetbhead == NULL)
        native_CPXgetbhead = load_symbol("CPXgetbhead");
    return native_CPXgetbhead(env, lp, head, x);
}

// CPXgetcallbackinfo
CPXLIBAPI int CPXPUBLIC CPXgetcallbackinfo (CPXCENVptr env, void *cbdata, int wherefrom, int whichinfo, void *result_p)
{
    if (native_CPXgetcallbackinfo == NULL)
        native_CPXgetcallbackinfo = load_symbol("CPXgetcallbackinfo");
    return native_CPXgetcallbackinfo(env, cbdata, wherefrom, whichinfo, result_p);
}

// CPXgetchannels
CPXLIBAPI int CPXPUBLIC CPXgetchannels (CPXCENVptr env, CPXCHANNELptr * cpxresults_p, CPXCHANNELptr * cpxwarning_p, CPXCHANNELptr * cpxerror_p, CPXCHANNELptr * cpxlog_p)
{
    if (native_CPXgetchannels == NULL)
        native_CPXgetchannels = load_symbol("CPXgetchannels");
    return native_CPXgetchannels(env, cpxresults_p, cpxwarning_p, cpxerror_p, cpxlog_p);
}

// CPXgetchgparam
CPXLIBAPI int CPXPUBLIC CPXgetchgparam (CPXCENVptr env, int *cnt_p, int *paramnum, int pspace, int *surplus_p)
{
    if (native_CPXgetchgparam == NULL)
        native_CPXgetchgparam = load_symbol("CPXgetchgparam");
    return native_CPXgetchgparam(env, cnt_p, paramnum, pspace, surplus_p);
}

// CPXgetcoef
CPXLIBAPI int CPXPUBLIC CPXgetcoef (CPXCENVptr env, CPXCLPptr lp, int i, int j, double *coef_p)
{
    if (native_CPXgetcoef == NULL)
        native_CPXgetcoef = load_symbol("CPXgetcoef");
    return native_CPXgetcoef(env, lp, i, j, coef_p);
}

// CPXgetcolindex
CPXLIBAPI int CPXPUBLIC CPXgetcolindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXgetcolindex == NULL)
        native_CPXgetcolindex = load_symbol("CPXgetcolindex");
    return native_CPXgetcolindex(env, lp, lname_str, index_p);
}

// CPXgetcolinfeas
CPXLIBAPI int CPXPUBLIC CPXgetcolinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end)
{
    if (native_CPXgetcolinfeas == NULL)
        native_CPXgetcolinfeas = load_symbol("CPXgetcolinfeas");
    return native_CPXgetcolinfeas(env, lp, x, infeasout, begin, end);
}

// CPXgetcolname
CPXLIBAPI int CPXPUBLIC CPXgetcolname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, int storespace, int *surplus_p, int begin, int end)
{
    if (native_CPXgetcolname == NULL)
        native_CPXgetcolname = load_symbol("CPXgetcolname");
    return native_CPXgetcolname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}

// CPXgetcols
CPXLIBAPI int CPXPUBLIC CPXgetcols (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *cmatbeg, int *cmatind, double *cmatval, int cmatspace, int *surplus_p, int begin, int end)
{
    if (native_CPXgetcols == NULL)
        native_CPXgetcols = load_symbol("CPXgetcols");
    return native_CPXgetcols(env, lp, nzcnt_p, cmatbeg, cmatind, cmatval, cmatspace, surplus_p, begin, end);
}

// CPXgetconflict
CPXLIBAPI int CPXPUBLIC CPXgetconflict (CPXCENVptr env, CPXCLPptr lp, int *confstat_p, int *rowind, int *rowbdstat, int *confnumrows_p, int *colind, int *colbdstat, int *confnumcols_p)
{
    if (native_CPXgetconflict == NULL)
        native_CPXgetconflict = load_symbol("CPXgetconflict");
    return native_CPXgetconflict(env, lp, confstat_p, rowind, rowbdstat, confnumrows_p, colind, colbdstat, confnumcols_p);
}

// CPXgetconflictext
CPXLIBAPI int CPXPUBLIC CPXgetconflictext (CPXCENVptr env, CPXCLPptr lp, int *grpstat, int beg, int end)
{
    if (native_CPXgetconflictext == NULL)
        native_CPXgetconflictext = load_symbol("CPXgetconflictext");
    return native_CPXgetconflictext(env, lp, grpstat, beg, end);
}

// CPXgetcrossdexchcnt
CPXLIBAPI int CPXPUBLIC CPXgetcrossdexchcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetcrossdexchcnt == NULL)
        native_CPXgetcrossdexchcnt = load_symbol("CPXgetcrossdexchcnt");
    return native_CPXgetcrossdexchcnt(env, lp);
}

// CPXgetcrossdpushcnt
CPXLIBAPI int CPXPUBLIC CPXgetcrossdpushcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetcrossdpushcnt == NULL)
        native_CPXgetcrossdpushcnt = load_symbol("CPXgetcrossdpushcnt");
    return native_CPXgetcrossdpushcnt(env, lp);
}

// CPXgetcrosspexchcnt
CPXLIBAPI int CPXPUBLIC CPXgetcrosspexchcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetcrosspexchcnt == NULL)
        native_CPXgetcrosspexchcnt = load_symbol("CPXgetcrosspexchcnt");
    return native_CPXgetcrosspexchcnt(env, lp);
}

// CPXgetcrossppushcnt
CPXLIBAPI int CPXPUBLIC CPXgetcrossppushcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetcrossppushcnt == NULL)
        native_CPXgetcrossppushcnt = load_symbol("CPXgetcrossppushcnt");
    return native_CPXgetcrossppushcnt(env, lp);
}

// CPXgetdblparam
CPXLIBAPI int CPXPUBLIC CPXgetdblparam (CPXCENVptr env, int whichparam, double *value_p)
{
    if (native_CPXgetdblparam == NULL)
        native_CPXgetdblparam = load_symbol("CPXgetdblparam");
    return native_CPXgetdblparam(env, whichparam, value_p);
}

// CPXgetdblquality
CPXLIBAPI int CPXPUBLIC CPXgetdblquality (CPXCENVptr env, CPXCLPptr lp, double *quality_p, int what)
{
    if (native_CPXgetdblquality == NULL)
        native_CPXgetdblquality = load_symbol("CPXgetdblquality");
    return native_CPXgetdblquality(env, lp, quality_p, what);
}

// CPXgetdettime
CPXLIBAPI int CPXPUBLIC CPXgetdettime (CPXCENVptr env, double *dettimestamp_p)
{
    if (native_CPXgetdettime == NULL)
        native_CPXgetdettime = load_symbol("CPXgetdettime");
    return native_CPXgetdettime(env, dettimestamp_p);
}

// CPXgetdj
CPXLIBAPI int CPXPUBLIC CPXgetdj (CPXCENVptr env, CPXCLPptr lp, double *dj, int begin, int end)
{
    if (native_CPXgetdj == NULL)
        native_CPXgetdj = load_symbol("CPXgetdj");
    return native_CPXgetdj(env, lp, dj, begin, end);
}

// CPXgetdnorms
CPXLIBAPI int CPXPUBLIC CPXgetdnorms (CPXCENVptr env, CPXCLPptr lp, double *norm, int *head, int *len_p)
{
    if (native_CPXgetdnorms == NULL)
        native_CPXgetdnorms = load_symbol("CPXgetdnorms");
    return native_CPXgetdnorms(env, lp, norm, head, len_p);
}

// CPXgetdsbcnt
CPXLIBAPI int CPXPUBLIC CPXgetdsbcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetdsbcnt == NULL)
        native_CPXgetdsbcnt = load_symbol("CPXgetdsbcnt");
    return native_CPXgetdsbcnt(env, lp);
}

// CPXgeterrorstring
CPXLIBAPI CPXCCHARptr CPXPUBLIC CPXgeterrorstring (CPXCENVptr env, int errcode, char *buffer_str)
{
    if (native_CPXgeterrorstring == NULL)
        native_CPXgeterrorstring = load_symbol("CPXgeterrorstring");
    return native_CPXgeterrorstring(env, errcode, buffer_str);
}

// CPXgetgrad
CPXLIBAPI int CPXPUBLIC CPXgetgrad (CPXCENVptr env, CPXCLPptr lp, int j, int *head, double *y)
{
    if (native_CPXgetgrad == NULL)
        native_CPXgetgrad = load_symbol("CPXgetgrad");
    return native_CPXgetgrad(env, lp, j, head, y);
}

// CPXgetiis
int CPXPUBLIC CPXgetiis (CPXCENVptr env, CPXCLPptr lp, int *iisstat_p, int *rowind, int *rowbdstat, int *iisnumrows_p, int *colind, int *colbdstat, int *iisnumcols_p)
{
    if (native_CPXgetiis == NULL)
        native_CPXgetiis = load_symbol("CPXgetiis");
    return native_CPXgetiis(env, lp, iisstat_p, rowind, rowbdstat, iisnumrows_p, colind, colbdstat, iisnumcols_p);
}

// CPXgetijdiv
CPXLIBAPI int CPXPUBLIC CPXgetijdiv (CPXCENVptr env, CPXCLPptr lp, int *idiv_p, int *jdiv_p)
{
    if (native_CPXgetijdiv == NULL)
        native_CPXgetijdiv = load_symbol("CPXgetijdiv");
    return native_CPXgetijdiv(env, lp, idiv_p, jdiv_p);
}

// CPXgetijrow
CPXLIBAPI int CPXPUBLIC CPXgetijrow (CPXCENVptr env, CPXCLPptr lp, int i, int j, int *row_p)
{
    if (native_CPXgetijrow == NULL)
        native_CPXgetijrow = load_symbol("CPXgetijrow");
    return native_CPXgetijrow(env, lp, i, j, row_p);
}

// CPXgetintparam
CPXLIBAPI int CPXPUBLIC CPXgetintparam (CPXCENVptr env, int whichparam, CPXINT * value_p)
{
    if (native_CPXgetintparam == NULL)
        native_CPXgetintparam = load_symbol("CPXgetintparam");
    return native_CPXgetintparam(env, whichparam, value_p);
}

// CPXgetintquality
CPXLIBAPI int CPXPUBLIC CPXgetintquality (CPXCENVptr env, CPXCLPptr lp, int *quality_p, int what)
{
    if (native_CPXgetintquality == NULL)
        native_CPXgetintquality = load_symbol("CPXgetintquality");
    return native_CPXgetintquality(env, lp, quality_p, what);
}

// CPXgetitcnt
CPXLIBAPI int CPXPUBLIC CPXgetitcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetitcnt == NULL)
        native_CPXgetitcnt = load_symbol("CPXgetitcnt");
    return native_CPXgetitcnt(env, lp);
}

// CPXgetlb
CPXLIBAPI int CPXPUBLIC CPXgetlb (CPXCENVptr env, CPXCLPptr lp, double *lb, int begin, int end)
{
    if (native_CPXgetlb == NULL)
        native_CPXgetlb = load_symbol("CPXgetlb");
    return native_CPXgetlb(env, lp, lb, begin, end);
}

// CPXgetlogfile
CPXLIBAPI int CPXPUBLIC CPXgetlogfile (CPXCENVptr env, CPXFILEptr * logfile_p)
{
    if (native_CPXgetlogfile == NULL)
        native_CPXgetlogfile = load_symbol("CPXgetlogfile");
    return native_CPXgetlogfile(env, logfile_p);
}

// CPXgetlongparam
CPXLIBAPI int CPXPUBLIC CPXgetlongparam (CPXCENVptr env, int whichparam, CPXLONG * value_p)
{
    if (native_CPXgetlongparam == NULL)
        native_CPXgetlongparam = load_symbol("CPXgetlongparam");
    return native_CPXgetlongparam(env, whichparam, value_p);
}

// CPXgetlpcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetlpcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p)
{
    if (native_CPXgetlpcallbackfunc == NULL)
        native_CPXgetlpcallbackfunc = load_symbol("CPXgetlpcallbackfunc");
    return native_CPXgetlpcallbackfunc(env, callback_p, cbhandle_p);
}

// CPXgetmethod
CPXLIBAPI int CPXPUBLIC CPXgetmethod (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetmethod == NULL)
        native_CPXgetmethod = load_symbol("CPXgetmethod");
    return native_CPXgetmethod(env, lp);
}

// CPXgetnetcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetnetcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p)
{
    if (native_CPXgetnetcallbackfunc == NULL)
        native_CPXgetnetcallbackfunc = load_symbol("CPXgetnetcallbackfunc");
    return native_CPXgetnetcallbackfunc(env, callback_p, cbhandle_p);
}

// CPXgetnumcols
CPXLIBAPI int CPXPUBLIC CPXgetnumcols (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumcols == NULL)
        native_CPXgetnumcols = load_symbol("CPXgetnumcols");
    return native_CPXgetnumcols(env, lp);
}

// CPXgetnumcores
CPXLIBAPI int CPXPUBLIC CPXgetnumcores (CPXCENVptr env, int *numcores_p)
{
    if (native_CPXgetnumcores == NULL)
        native_CPXgetnumcores = load_symbol("CPXgetnumcores");
    return native_CPXgetnumcores(env, numcores_p);
}

// CPXgetnumnz
CPXLIBAPI int CPXPUBLIC CPXgetnumnz (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumnz == NULL)
        native_CPXgetnumnz = load_symbol("CPXgetnumnz");
    return native_CPXgetnumnz(env, lp);
}

// CPXgetnumrows
CPXLIBAPI int CPXPUBLIC CPXgetnumrows (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumrows == NULL)
        native_CPXgetnumrows = load_symbol("CPXgetnumrows");
    return native_CPXgetnumrows(env, lp);
}

// CPXgetobj
CPXLIBAPI int CPXPUBLIC CPXgetobj (CPXCENVptr env, CPXCLPptr lp, double *obj, int begin, int end)
{
    if (native_CPXgetobj == NULL)
        native_CPXgetobj = load_symbol("CPXgetobj");
    return native_CPXgetobj(env, lp, obj, begin, end);
}

// CPXgetobjname
CPXLIBAPI int CPXPUBLIC CPXgetobjname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p)
{
    if (native_CPXgetobjname == NULL)
        native_CPXgetobjname = load_symbol("CPXgetobjname");
    return native_CPXgetobjname(env, lp, buf_str, bufspace, surplus_p);
}

// CPXgetobjoffset
CPXLIBAPI int CPXPUBLIC CPXgetobjoffset (CPXCENVptr env, CPXCLPptr lp, double *objoffset_p)
{
    if (native_CPXgetobjoffset == NULL)
        native_CPXgetobjoffset = load_symbol("CPXgetobjoffset");
    return native_CPXgetobjoffset(env, lp, objoffset_p);
}

// CPXgetobjsen
CPXLIBAPI int CPXPUBLIC CPXgetobjsen (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetobjsen == NULL)
        native_CPXgetobjsen = load_symbol("CPXgetobjsen");
    return native_CPXgetobjsen(env, lp);
}

// CPXgetobjval
CPXLIBAPI int CPXPUBLIC CPXgetobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p)
{
    if (native_CPXgetobjval == NULL)
        native_CPXgetobjval = load_symbol("CPXgetobjval");
    return native_CPXgetobjval(env, lp, objval_p);
}

// CPXgetparamname
CPXLIBAPI int CPXPUBLIC CPXgetparamname (CPXCENVptr env, int whichparam, char *name_str)
{
    if (native_CPXgetparamname == NULL)
        native_CPXgetparamname = load_symbol("CPXgetparamname");
    return native_CPXgetparamname(env, whichparam, name_str);
}

// CPXgetparamnum
CPXLIBAPI int CPXPUBLIC CPXgetparamnum (CPXCENVptr env, char const *name_str, int *whichparam_p)
{
    if (native_CPXgetparamnum == NULL)
        native_CPXgetparamnum = load_symbol("CPXgetparamnum");
    return native_CPXgetparamnum(env, name_str, whichparam_p);
}

// CPXgetparamtype
CPXLIBAPI int CPXPUBLIC CPXgetparamtype (CPXCENVptr env, int whichparam, int *paramtype)
{
    if (native_CPXgetparamtype == NULL)
        native_CPXgetparamtype = load_symbol("CPXgetparamtype");
    return native_CPXgetparamtype(env, whichparam, paramtype);
}

// CPXgetphase1cnt
CPXLIBAPI int CPXPUBLIC CPXgetphase1cnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetphase1cnt == NULL)
        native_CPXgetphase1cnt = load_symbol("CPXgetphase1cnt");
    return native_CPXgetphase1cnt(env, lp);
}

// CPXgetpi
CPXLIBAPI int CPXPUBLIC CPXgetpi (CPXCENVptr env, CPXCLPptr lp, double *pi, int begin, int end)
{
    if (native_CPXgetpi == NULL)
        native_CPXgetpi = load_symbol("CPXgetpi");
    return native_CPXgetpi(env, lp, pi, begin, end);
}

// CPXgetpnorms
CPXLIBAPI int CPXPUBLIC CPXgetpnorms (CPXCENVptr env, CPXCLPptr lp, double *cnorm, double *rnorm, int *len_p)
{
    if (native_CPXgetpnorms == NULL)
        native_CPXgetpnorms = load_symbol("CPXgetpnorms");
    return native_CPXgetpnorms(env, lp, cnorm, rnorm, len_p);
}

// CPXgetprestat
CPXLIBAPI int CPXPUBLIC CPXgetprestat (CPXCENVptr env, CPXCLPptr lp, int *prestat_p, int *pcstat, int *prstat, int *ocstat, int *orstat)
{
    if (native_CPXgetprestat == NULL)
        native_CPXgetprestat = load_symbol("CPXgetprestat");
    return native_CPXgetprestat(env, lp, prestat_p, pcstat, prstat, ocstat, orstat);
}

// CPXgetprobname
CPXLIBAPI int CPXPUBLIC CPXgetprobname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p)
{
    if (native_CPXgetprobname == NULL)
        native_CPXgetprobname = load_symbol("CPXgetprobname");
    return native_CPXgetprobname(env, lp, buf_str, bufspace, surplus_p);
}

// CPXgetprobtype
CPXLIBAPI int CPXPUBLIC CPXgetprobtype (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetprobtype == NULL)
        native_CPXgetprobtype = load_symbol("CPXgetprobtype");
    return native_CPXgetprobtype(env, lp);
}

// CPXgetprotected
CPXLIBAPI int CPXPUBLIC CPXgetprotected (CPXCENVptr env, CPXCLPptr lp, int *cnt_p, int *indices, int pspace, int *surplus_p)
{
    if (native_CPXgetprotected == NULL)
        native_CPXgetprotected = load_symbol("CPXgetprotected");
    return native_CPXgetprotected(env, lp, cnt_p, indices, pspace, surplus_p);
}

// CPXgetpsbcnt
CPXLIBAPI int CPXPUBLIC CPXgetpsbcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetpsbcnt == NULL)
        native_CPXgetpsbcnt = load_symbol("CPXgetpsbcnt");
    return native_CPXgetpsbcnt(env, lp);
}

// CPXgetray
CPXLIBAPI int CPXPUBLIC CPXgetray (CPXCENVptr env, CPXCLPptr lp, double *z)
{
    if (native_CPXgetray == NULL)
        native_CPXgetray = load_symbol("CPXgetray");
    return native_CPXgetray(env, lp, z);
}

// CPXgetredlp
CPXLIBAPI int CPXPUBLIC CPXgetredlp (CPXCENVptr env, CPXCLPptr lp, CPXCLPptr * redlp_p)
{
    if (native_CPXgetredlp == NULL)
        native_CPXgetredlp = load_symbol("CPXgetredlp");
    return native_CPXgetredlp(env, lp, redlp_p);
}

// CPXgetrhs
CPXLIBAPI int CPXPUBLIC CPXgetrhs (CPXCENVptr env, CPXCLPptr lp, double *rhs, int begin, int end)
{
    if (native_CPXgetrhs == NULL)
        native_CPXgetrhs = load_symbol("CPXgetrhs");
    return native_CPXgetrhs(env, lp, rhs, begin, end);
}

// CPXgetrngval
CPXLIBAPI int CPXPUBLIC CPXgetrngval (CPXCENVptr env, CPXCLPptr lp, double *rngval, int begin, int end)
{
    if (native_CPXgetrngval == NULL)
        native_CPXgetrngval = load_symbol("CPXgetrngval");
    return native_CPXgetrngval(env, lp, rngval, begin, end);
}

// CPXgetrowindex
CPXLIBAPI int CPXPUBLIC CPXgetrowindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXgetrowindex == NULL)
        native_CPXgetrowindex = load_symbol("CPXgetrowindex");
    return native_CPXgetrowindex(env, lp, lname_str, index_p);
}

// CPXgetrowinfeas
CPXLIBAPI int CPXPUBLIC CPXgetrowinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end)
{
    if (native_CPXgetrowinfeas == NULL)
        native_CPXgetrowinfeas = load_symbol("CPXgetrowinfeas");
    return native_CPXgetrowinfeas(env, lp, x, infeasout, begin, end);
}

// CPXgetrowname
CPXLIBAPI int CPXPUBLIC CPXgetrowname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, int storespace, int *surplus_p, int begin, int end)
{
    if (native_CPXgetrowname == NULL)
        native_CPXgetrowname = load_symbol("CPXgetrowname");
    return native_CPXgetrowname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}

// CPXgetrows
CPXLIBAPI int CPXPUBLIC CPXgetrows (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *rmatbeg, int *rmatind, double *rmatval, int rmatspace, int *surplus_p, int begin, int end)
{
    if (native_CPXgetrows == NULL)
        native_CPXgetrows = load_symbol("CPXgetrows");
    return native_CPXgetrows(env, lp, nzcnt_p, rmatbeg, rmatind, rmatval, rmatspace, surplus_p, begin, end);
}

// CPXgetsense
CPXLIBAPI int CPXPUBLIC CPXgetsense (CPXCENVptr env, CPXCLPptr lp, char *sense, int begin, int end)
{
    if (native_CPXgetsense == NULL)
        native_CPXgetsense = load_symbol("CPXgetsense");
    return native_CPXgetsense(env, lp, sense, begin, end);
}

// CPXgetsiftitcnt
CPXLIBAPI int CPXPUBLIC CPXgetsiftitcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetsiftitcnt == NULL)
        native_CPXgetsiftitcnt = load_symbol("CPXgetsiftitcnt");
    return native_CPXgetsiftitcnt(env, lp);
}

// CPXgetsiftphase1cnt
CPXLIBAPI int CPXPUBLIC CPXgetsiftphase1cnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetsiftphase1cnt == NULL)
        native_CPXgetsiftphase1cnt = load_symbol("CPXgetsiftphase1cnt");
    return native_CPXgetsiftphase1cnt(env, lp);
}

// CPXgetslack
CPXLIBAPI int CPXPUBLIC CPXgetslack (CPXCENVptr env, CPXCLPptr lp, double *slack, int begin, int end)
{
    if (native_CPXgetslack == NULL)
        native_CPXgetslack = load_symbol("CPXgetslack");
    return native_CPXgetslack(env, lp, slack, begin, end);
}

// CPXgetsolnpooldblquality
CPXLIBAPI int CPXPUBLIC CPXgetsolnpooldblquality (CPXCENVptr env, CPXCLPptr lp, int soln, double *quality_p, int what)
{
    if (native_CPXgetsolnpooldblquality == NULL)
        native_CPXgetsolnpooldblquality = load_symbol("CPXgetsolnpooldblquality");
    return native_CPXgetsolnpooldblquality(env, lp, soln, quality_p, what);
}

// CPXgetsolnpoolintquality
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolintquality (CPXCENVptr env, CPXCLPptr lp, int soln, int *quality_p, int what)
{
    if (native_CPXgetsolnpoolintquality == NULL)
        native_CPXgetsolnpoolintquality = load_symbol("CPXgetsolnpoolintquality");
    return native_CPXgetsolnpoolintquality(env, lp, soln, quality_p, what);
}

// CPXgetstat
CPXLIBAPI int CPXPUBLIC CPXgetstat (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetstat == NULL)
        native_CPXgetstat = load_symbol("CPXgetstat");
    return native_CPXgetstat(env, lp);
}

// CPXgetstatstring
CPXLIBAPI CPXCHARptr CPXPUBLIC CPXgetstatstring (CPXCENVptr env, int statind, char *buffer_str)
{
    if (native_CPXgetstatstring == NULL)
        native_CPXgetstatstring = load_symbol("CPXgetstatstring");
    return native_CPXgetstatstring(env, statind, buffer_str);
}

// CPXgetstrparam
CPXLIBAPI int CPXPUBLIC CPXgetstrparam (CPXCENVptr env, int whichparam, char *value_str)
{
    if (native_CPXgetstrparam == NULL)
        native_CPXgetstrparam = load_symbol("CPXgetstrparam");
    return native_CPXgetstrparam(env, whichparam, value_str);
}

// CPXgettime
CPXLIBAPI int CPXPUBLIC CPXgettime (CPXCENVptr env, double *timestamp_p)
{
    if (native_CPXgettime == NULL)
        native_CPXgettime = load_symbol("CPXgettime");
    return native_CPXgettime(env, timestamp_p);
}

// CPXgettuningcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgettuningcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p)
{
    if (native_CPXgettuningcallbackfunc == NULL)
        native_CPXgettuningcallbackfunc = load_symbol("CPXgettuningcallbackfunc");
    return native_CPXgettuningcallbackfunc(env, callback_p, cbhandle_p);
}

// CPXgetub
CPXLIBAPI int CPXPUBLIC CPXgetub (CPXCENVptr env, CPXCLPptr lp, double *ub, int begin, int end)
{
    if (native_CPXgetub == NULL)
        native_CPXgetub = load_symbol("CPXgetub");
    return native_CPXgetub(env, lp, ub, begin, end);
}

// CPXgetweight
CPXLIBAPI int CPXPUBLIC CPXgetweight (CPXCENVptr env, CPXCLPptr lp, int rcnt, int const *rmatbeg, int const *rmatind, double const *rmatval, double *weight, int dpriind)
{
    if (native_CPXgetweight == NULL)
        native_CPXgetweight = load_symbol("CPXgetweight");
    return native_CPXgetweight(env, lp, rcnt, rmatbeg, rmatind, rmatval, weight, dpriind);
}

// CPXgetx
CPXLIBAPI int CPXPUBLIC CPXgetx (CPXCENVptr env, CPXCLPptr lp, double *x, int begin, int end)
{
    if (native_CPXgetx == NULL)
        native_CPXgetx = load_symbol("CPXgetx");
    return native_CPXgetx(env, lp, x, begin, end);
}

// CPXhybnetopt
CPXLIBAPI int CPXPUBLIC CPXhybnetopt (CPXCENVptr env, CPXLPptr lp, int method)
{
    if (native_CPXhybnetopt == NULL)
        native_CPXhybnetopt = load_symbol("CPXhybnetopt");
    return native_CPXhybnetopt(env, lp, method);
}

// CPXinfodblparam
CPXLIBAPI int CPXPUBLIC CPXinfodblparam (CPXCENVptr env, int whichparam, double *defvalue_p, double *minvalue_p, double *maxvalue_p)
{
    if (native_CPXinfodblparam == NULL)
        native_CPXinfodblparam = load_symbol("CPXinfodblparam");
    return native_CPXinfodblparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}

// CPXinfointparam
CPXLIBAPI int CPXPUBLIC CPXinfointparam (CPXCENVptr env, int whichparam, CPXINT * defvalue_p, CPXINT * minvalue_p, CPXINT * maxvalue_p)
{
    if (native_CPXinfointparam == NULL)
        native_CPXinfointparam = load_symbol("CPXinfointparam");
    return native_CPXinfointparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}

// CPXinfolongparam
CPXLIBAPI int CPXPUBLIC CPXinfolongparam (CPXCENVptr env, int whichparam, CPXLONG * defvalue_p, CPXLONG * minvalue_p, CPXLONG * maxvalue_p)
{
    if (native_CPXinfolongparam == NULL)
        native_CPXinfolongparam = load_symbol("CPXinfolongparam");
    return native_CPXinfolongparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}

// CPXinfostrparam
CPXLIBAPI int CPXPUBLIC CPXinfostrparam (CPXCENVptr env, int whichparam, char *defvalue_str)
{
    if (native_CPXinfostrparam == NULL)
        native_CPXinfostrparam = load_symbol("CPXinfostrparam");
    return native_CPXinfostrparam(env, whichparam, defvalue_str);
}

// CPXinitialize
CPXLIBAPI void CPXPUBLIC CPXinitialize (void)
{
    if (native_CPXinitialize == NULL)
        native_CPXinitialize = load_symbol("CPXinitialize");
    return native_CPXinitialize();
}

// CPXkilldnorms
CPXLIBAPI int CPXPUBLIC CPXkilldnorms (CPXLPptr lp)
{
    if (native_CPXkilldnorms == NULL)
        native_CPXkilldnorms = load_symbol("CPXkilldnorms");
    return native_CPXkilldnorms(lp);
}

// CPXkillpnorms
CPXLIBAPI int CPXPUBLIC CPXkillpnorms (CPXLPptr lp)
{
    if (native_CPXkillpnorms == NULL)
        native_CPXkillpnorms = load_symbol("CPXkillpnorms");
    return native_CPXkillpnorms(lp);
}

// CPXlpopt
CPXLIBAPI int CPXPUBLIC CPXlpopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXlpopt == NULL)
        native_CPXlpopt = load_symbol("CPXlpopt");
    return native_CPXlpopt(env, lp);
}

// CPXlprewrite
CPXLIBAPI int CPXPUBLIC CPXlprewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXlprewrite == NULL)
        native_CPXlprewrite = load_symbol("CPXlprewrite");
    return native_CPXlprewrite(env, lp, filename_str);
}

// CPXlpwrite
CPXLIBAPI int CPXPUBLIC CPXlpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXlpwrite == NULL)
        native_CPXlpwrite = load_symbol("CPXlpwrite");
    return native_CPXlpwrite(env, lp, filename_str);
}

// CPXmalloc
CPXVOIDptr CPXPUBLIC CPXmalloc (size_t size)
{
    if (native_CPXmalloc == NULL)
        native_CPXmalloc = load_symbol("CPXmalloc");
    return native_CPXmalloc(size);
}

// CPXmbasewrite
CPXLIBAPI int CPXPUBLIC CPXmbasewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXmbasewrite == NULL)
        native_CPXmbasewrite = load_symbol("CPXmbasewrite");
    return native_CPXmbasewrite(env, lp, filename_str);
}

// CPXmdleave
CPXLIBAPI int CPXPUBLIC CPXmdleave (CPXCENVptr env, CPXLPptr lp, int const *indices, int cnt, double *downratio, double *upratio)
{
    if (native_CPXmdleave == NULL)
        native_CPXmdleave = load_symbol("CPXmdleave");
    return native_CPXmdleave(env, lp, indices, cnt, downratio, upratio);
}

// CPXmemcpy
CPXVOIDptr CPXPUBLIC CPXmemcpy (void *s1, void *s2, size_t n)
{
    if (native_CPXmemcpy == NULL)
        native_CPXmemcpy = load_symbol("CPXmemcpy");
    return native_CPXmemcpy(s1, s2, n);
}

// CPXmpsrewrite
CPXLIBAPI int CPXPUBLIC CPXmpsrewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXmpsrewrite == NULL)
        native_CPXmpsrewrite = load_symbol("CPXmpsrewrite");
    return native_CPXmpsrewrite(env, lp, filename_str);
}

// CPXmpswrite
CPXLIBAPI int CPXPUBLIC CPXmpswrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXmpswrite == NULL)
        native_CPXmpswrite = load_symbol("CPXmpswrite");
    return native_CPXmpswrite(env, lp, filename_str);
}

// CPXmsg
CPXLIBAPI int CPXPUBVARARGS CPXmsg (CPXCHANNELptr channel, char const *format, ...)
{
    if (native_CPXmsg == NULL)
        native_CPXmsg = load_symbol("CPXmsg");
    return native_CPXmsg(channel, format);
}

// CPXmsgstr
CPXLIBAPI int CPXPUBLIC CPXmsgstr (CPXCHANNELptr channel, char const *msg_str)
{
    if (native_CPXmsgstr == NULL)
        native_CPXmsgstr = load_symbol("CPXmsgstr");
    return native_CPXmsgstr(channel, msg_str);
}

// CPXNETextract
CPXLIBAPI int CPXPUBLIC CPXNETextract (CPXCENVptr env, CPXNETptr net, CPXCLPptr lp, int *colmap, int *rowmap)
{
    if (native_CPXNETextract == NULL)
        native_CPXNETextract = load_symbol("CPXNETextract");
    return native_CPXNETextract(env, net, lp, colmap, rowmap);
}

// CPXnewcols
CPXLIBAPI int CPXPUBLIC CPXnewcols (CPXCENVptr env, CPXLPptr lp, int ccnt, double const *obj, double const *lb, double const *ub, char const *xctype, char **colname)
{
    if (native_CPXnewcols == NULL)
        native_CPXnewcols = load_symbol("CPXnewcols");
    return native_CPXnewcols(env, lp, ccnt, obj, lb, ub, xctype, colname);
}

// CPXnewrows
CPXLIBAPI int CPXPUBLIC CPXnewrows (CPXCENVptr env, CPXLPptr lp, int rcnt, double const *rhs, char const *sense, double const *rngval, char **rowname)
{
    if (native_CPXnewrows == NULL)
        native_CPXnewrows = load_symbol("CPXnewrows");
    return native_CPXnewrows(env, lp, rcnt, rhs, sense, rngval, rowname);
}

// CPXobjsa
CPXLIBAPI int CPXPUBLIC CPXobjsa (CPXCENVptr env, CPXCLPptr lp, int begin, int end, double *lower, double *upper)
{
    if (native_CPXobjsa == NULL)
        native_CPXobjsa = load_symbol("CPXobjsa");
    return native_CPXobjsa(env, lp, begin, end, lower, upper);
}

// CPXopenCPLEX
CPXLIBAPI CPXENVptr CPXPUBLIC CPXopenCPLEX (int *status_p)
{
    if (native_CPXopenCPLEX == NULL)
        native_CPXopenCPLEX = load_symbol("CPXopenCPLEX");
    return native_CPXopenCPLEX(status_p);
}

// CPXopenCPLEXruntime
CPXLIBAPI CPXENVptr CPXPUBLIC CPXopenCPLEXruntime (int *status_p, int serialnum, char const *licenvstring_str)
{
    if (native_CPXopenCPLEXruntime == NULL)
        native_CPXopenCPLEXruntime = load_symbol("CPXopenCPLEXruntime");
    return native_CPXopenCPLEXruntime(status_p, serialnum, licenvstring_str);
}

// CPXparenv
CPXLIBAPI CPXENVptr CPXPUBLIC CPXparenv (CPXENVptr env, int *status_p)
{
    if (native_CPXparenv == NULL)
        native_CPXparenv = load_symbol("CPXparenv");
    return native_CPXparenv(env, status_p);
}

// CPXpivot
CPXLIBAPI int CPXPUBLIC CPXpivot (CPXCENVptr env, CPXLPptr lp, int jenter, int jleave, int leavestat)
{
    if (native_CPXpivot == NULL)
        native_CPXpivot = load_symbol("CPXpivot");
    return native_CPXpivot(env, lp, jenter, jleave, leavestat);
}

// CPXpivotin
CPXLIBAPI int CPXPUBLIC CPXpivotin (CPXCENVptr env, CPXLPptr lp, int const *rlist, int rlen)
{
    if (native_CPXpivotin == NULL)
        native_CPXpivotin = load_symbol("CPXpivotin");
    return native_CPXpivotin(env, lp, rlist, rlen);
}

// CPXpivotout
CPXLIBAPI int CPXPUBLIC CPXpivotout (CPXCENVptr env, CPXLPptr lp, int const *clist, int clen)
{
    if (native_CPXpivotout == NULL)
        native_CPXpivotout = load_symbol("CPXpivotout");
    return native_CPXpivotout(env, lp, clist, clen);
}

// CPXpperwrite
CPXLIBAPI int CPXPUBLIC CPXpperwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon)
{
    if (native_CPXpperwrite == NULL)
        native_CPXpperwrite = load_symbol("CPXpperwrite");
    return native_CPXpperwrite(env, lp, filename_str, epsilon);
}

// CPXpratio
CPXLIBAPI int CPXPUBLIC CPXpratio (CPXCENVptr env, CPXLPptr lp, int *indices, int cnt, double *downratio, double *upratio, int *downleave, int *upleave, int *downleavestatus, int *upleavestatus, int *downstatus, int *upstatus)
{
    if (native_CPXpratio == NULL)
        native_CPXpratio = load_symbol("CPXpratio");
    return native_CPXpratio(env, lp, indices, cnt, downratio, upratio, downleave, upleave, downleavestatus, upleavestatus, downstatus, upstatus);
}

// CPXpreaddrows
CPXLIBAPI int CPXPUBLIC CPXpreaddrows (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **rowname)
{
    if (native_CPXpreaddrows == NULL)
        native_CPXpreaddrows = load_symbol("CPXpreaddrows");
    return native_CPXpreaddrows(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}

// CPXprechgobj
CPXLIBAPI int CPXPUBLIC CPXprechgobj (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values)
{
    if (native_CPXprechgobj == NULL)
        native_CPXprechgobj = load_symbol("CPXprechgobj");
    return native_CPXprechgobj(env, lp, cnt, indices, values);
}

// CPXpreslvwrite
CPXLIBAPI int CPXPUBLIC CPXpreslvwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double *objoff_p)
{
    if (native_CPXpreslvwrite == NULL)
        native_CPXpreslvwrite = load_symbol("CPXpreslvwrite");
    return native_CPXpreslvwrite(env, lp, filename_str, objoff_p);
}

// CPXpresolve
CPXLIBAPI int CPXPUBLIC CPXpresolve (CPXCENVptr env, CPXLPptr lp, int method)
{
    if (native_CPXpresolve == NULL)
        native_CPXpresolve = load_symbol("CPXpresolve");
    return native_CPXpresolve(env, lp, method);
}

// CPXprimopt
CPXLIBAPI int CPXPUBLIC CPXprimopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXprimopt == NULL)
        native_CPXprimopt = load_symbol("CPXprimopt");
    return native_CPXprimopt(env, lp);
}

// CPXputenv
int CPXPUBLIC CPXputenv (char *envsetting_str)
{
    if (native_CPXputenv == NULL)
        native_CPXputenv = load_symbol("CPXputenv");
    return native_CPXputenv(envsetting_str);
}

// CPXqpdjfrompi
CPXLIBAPI int CPXPUBLIC CPXqpdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double const *x, double *dj)
{
    if (native_CPXqpdjfrompi == NULL)
        native_CPXqpdjfrompi = load_symbol("CPXqpdjfrompi");
    return native_CPXqpdjfrompi(env, lp, pi, x, dj);
}

// CPXqpuncrushpi
CPXLIBAPI int CPXPUBLIC CPXqpuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi, double const *x)
{
    if (native_CPXqpuncrushpi == NULL)
        native_CPXqpuncrushpi = load_symbol("CPXqpuncrushpi");
    return native_CPXqpuncrushpi(env, lp, pi, prepi, x);
}

// CPXreadcopybase
CPXLIBAPI int CPXPUBLIC CPXreadcopybase (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXreadcopybase == NULL)
        native_CPXreadcopybase = load_symbol("CPXreadcopybase");
    return native_CPXreadcopybase(env, lp, filename_str);
}

// CPXreadcopyparam
CPXLIBAPI int CPXPUBLIC CPXreadcopyparam (CPXENVptr env, char const *filename_str)
{
    if (native_CPXreadcopyparam == NULL)
        native_CPXreadcopyparam = load_symbol("CPXreadcopyparam");
    return native_CPXreadcopyparam(env, filename_str);
}

// CPXreadcopyprob
CPXLIBAPI int CPXPUBLIC CPXreadcopyprob (CPXCENVptr env, CPXLPptr lp, char const *filename_str, char const *filetype_str)
{
    if (native_CPXreadcopyprob == NULL)
        native_CPXreadcopyprob = load_symbol("CPXreadcopyprob");
    return native_CPXreadcopyprob(env, lp, filename_str, filetype_str);
}

// CPXreadcopysol
CPXLIBAPI int CPXPUBLIC CPXreadcopysol (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXreadcopysol == NULL)
        native_CPXreadcopysol = load_symbol("CPXreadcopysol");
    return native_CPXreadcopysol(env, lp, filename_str);
}

// CPXreadcopyvec
int CPXPUBLIC CPXreadcopyvec (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXreadcopyvec == NULL)
        native_CPXreadcopyvec = load_symbol("CPXreadcopyvec");
    return native_CPXreadcopyvec(env, lp, filename_str);
}

// CPXrealloc
CPXVOIDptr CPXPUBLIC CPXrealloc (void *ptr, size_t size)
{
    if (native_CPXrealloc == NULL)
        native_CPXrealloc = load_symbol("CPXrealloc");
    return native_CPXrealloc(ptr, size);
}

// CPXrefineconflict
CPXLIBAPI int CPXPUBLIC CPXrefineconflict (CPXCENVptr env, CPXLPptr lp, int *confnumrows_p, int *confnumcols_p)
{
    if (native_CPXrefineconflict == NULL)
        native_CPXrefineconflict = load_symbol("CPXrefineconflict");
    return native_CPXrefineconflict(env, lp, confnumrows_p, confnumcols_p);
}

// CPXrefineconflictext
CPXLIBAPI int CPXPUBLIC CPXrefineconflictext (CPXCENVptr env, CPXLPptr lp, int grpcnt, int concnt, double const *grppref, int const *grpbeg, int const *grpind, char const *grptype)
{
    if (native_CPXrefineconflictext == NULL)
        native_CPXrefineconflictext = load_symbol("CPXrefineconflictext");
    return native_CPXrefineconflictext(env, lp, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}

// CPXrhssa
CPXLIBAPI int CPXPUBLIC CPXrhssa (CPXCENVptr env, CPXCLPptr lp, int begin, int end, double *lower, double *upper)
{
    if (native_CPXrhssa == NULL)
        native_CPXrhssa = load_symbol("CPXrhssa");
    return native_CPXrhssa(env, lp, begin, end, lower, upper);
}

// CPXrobustopt
CPXLIBAPI int CPXPUBLIC CPXrobustopt (CPXCENVptr env, CPXLPptr lp, CPXLPptr lblp, CPXLPptr ublp, double objchg, double const *maxchg)
{
    if (native_CPXrobustopt == NULL)
        native_CPXrobustopt = load_symbol("CPXrobustopt");
    return native_CPXrobustopt(env, lp, lblp, ublp, objchg, maxchg);
}

// CPXsavwrite
CPXLIBAPI int CPXPUBLIC CPXsavwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXsavwrite == NULL)
        native_CPXsavwrite = load_symbol("CPXsavwrite");
    return native_CPXsavwrite(env, lp, filename_str);
}

// CPXserializercreate
CPXLIBAPI int CPXPUBLIC CPXserializercreate (CPXSERIALIZERptr * ser_p)
{
    if (native_CPXserializercreate == NULL)
        native_CPXserializercreate = load_symbol("CPXserializercreate");
    return native_CPXserializercreate(ser_p);
}

// CPXserializerdestroy
CPXLIBAPI void CPXPUBLIC CPXserializerdestroy (CPXSERIALIZERptr ser)
{
    if (native_CPXserializerdestroy == NULL)
        native_CPXserializerdestroy = load_symbol("CPXserializerdestroy");
    return native_CPXserializerdestroy(ser);
}

// CPXserializerlength
CPXLIBAPI CPXLONG CPXPUBLIC CPXserializerlength (CPXCSERIALIZERptr ser)
{
    if (native_CPXserializerlength == NULL)
        native_CPXserializerlength = load_symbol("CPXserializerlength");
    return native_CPXserializerlength(ser);
}

// CPXserializerpayload
CPXLIBAPI void const *CPXPUBLIC CPXserializerpayload (CPXCSERIALIZERptr ser)
{
    if (native_CPXserializerpayload == NULL)
        native_CPXserializerpayload = load_symbol("CPXserializerpayload");
    return native_CPXserializerpayload(ser);
}

// CPXsetdblparam
CPXLIBAPI int CPXPUBLIC CPXsetdblparam (CPXENVptr env, int whichparam, double newvalue)
{
    if (native_CPXsetdblparam == NULL)
        native_CPXsetdblparam = load_symbol("CPXsetdblparam");
    return native_CPXsetdblparam(env, whichparam, newvalue);
}

// CPXsetdefaults
CPXLIBAPI int CPXPUBLIC CPXsetdefaults (CPXENVptr env)
{
    if (native_CPXsetdefaults == NULL)
        native_CPXsetdefaults = load_symbol("CPXsetdefaults");
    return native_CPXsetdefaults(env);
}

// CPXsetintparam
CPXLIBAPI int CPXPUBLIC CPXsetintparam (CPXENVptr env, int whichparam, CPXINT newvalue)
{
    if (native_CPXsetintparam == NULL)
        native_CPXsetintparam = load_symbol("CPXsetintparam");
    return native_CPXsetintparam(env, whichparam, newvalue);
}

// CPXsetlogfile
CPXLIBAPI int CPXPUBLIC CPXsetlogfile (CPXENVptr env, CPXFILEptr lfile)
{
    if (native_CPXsetlogfile == NULL)
        native_CPXsetlogfile = load_symbol("CPXsetlogfile");
    return native_CPXsetlogfile(env, lfile);
}

// CPXsetlongparam
CPXLIBAPI int CPXPUBLIC CPXsetlongparam (CPXENVptr env, int whichparam, CPXLONG newvalue)
{
    if (native_CPXsetlongparam == NULL)
        native_CPXsetlongparam = load_symbol("CPXsetlongparam");
    return native_CPXsetlongparam(env, whichparam, newvalue);
}

// CPXsetlpcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetlpcallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle)
{
    if (native_CPXsetlpcallbackfunc == NULL)
        native_CPXsetlpcallbackfunc = load_symbol("CPXsetlpcallbackfunc");
    return native_CPXsetlpcallbackfunc(env, callback, cbhandle);
}

// CPXsetnetcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetnetcallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle)
{
    if (native_CPXsetnetcallbackfunc == NULL)
        native_CPXsetnetcallbackfunc = load_symbol("CPXsetnetcallbackfunc");
    return native_CPXsetnetcallbackfunc(env, callback, cbhandle);
}

// CPXsetphase2
CPXLIBAPI int CPXPUBLIC CPXsetphase2 (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXsetphase2 == NULL)
        native_CPXsetphase2 = load_symbol("CPXsetphase2");
    return native_CPXsetphase2(env, lp);
}

// CPXsetprofcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetprofcallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, int, void *), void *cbhandle)
{
    if (native_CPXsetprofcallbackfunc == NULL)
        native_CPXsetprofcallbackfunc = load_symbol("CPXsetprofcallbackfunc");
    return native_CPXsetprofcallbackfunc(env, callback, cbhandle);
}

// CPXsetstrparam
CPXLIBAPI int CPXPUBLIC CPXsetstrparam (CPXENVptr env, int whichparam, char const *newvalue_str)
{
    if (native_CPXsetstrparam == NULL)
        native_CPXsetstrparam = load_symbol("CPXsetstrparam");
    return native_CPXsetstrparam(env, whichparam, newvalue_str);
}

// CPXsetterminate
CPXLIBAPI int CPXPUBLIC CPXsetterminate (CPXENVptr env, volatile int *terminate_p)
{
    if (native_CPXsetterminate == NULL)
        native_CPXsetterminate = load_symbol("CPXsetterminate");
    return native_CPXsetterminate(env, terminate_p);
}

// CPXsettuningcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsettuningcallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle)
{
    if (native_CPXsettuningcallbackfunc == NULL)
        native_CPXsettuningcallbackfunc = load_symbol("CPXsettuningcallbackfunc");
    return native_CPXsettuningcallbackfunc(env, callback, cbhandle);
}

// CPXsiftopt
CPXLIBAPI int CPXPUBLIC CPXsiftopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXsiftopt == NULL)
        native_CPXsiftopt = load_symbol("CPXsiftopt");
    return native_CPXsiftopt(env, lp);
}

// CPXslackfromx
CPXLIBAPI int CPXPUBLIC CPXslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *slack)
{
    if (native_CPXslackfromx == NULL)
        native_CPXslackfromx = load_symbol("CPXslackfromx");
    return native_CPXslackfromx(env, lp, x, slack);
}

// CPXsolninfo
CPXLIBAPI int CPXPUBLIC CPXsolninfo (CPXCENVptr env, CPXCLPptr lp, int *solnmethod_p, int *solntype_p, int *pfeasind_p, int *dfeasind_p)
{
    if (native_CPXsolninfo == NULL)
        native_CPXsolninfo = load_symbol("CPXsolninfo");
    return native_CPXsolninfo(env, lp, solnmethod_p, solntype_p, pfeasind_p, dfeasind_p);
}

// CPXsolution
CPXLIBAPI int CPXPUBLIC CPXsolution (CPXCENVptr env, CPXCLPptr lp, int *lpstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj)
{
    if (native_CPXsolution == NULL)
        native_CPXsolution = load_symbol("CPXsolution");
    return native_CPXsolution(env, lp, lpstat_p, objval_p, x, pi, slack, dj);
}

// CPXsolwrite
CPXLIBAPI int CPXPUBLIC CPXsolwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXsolwrite == NULL)
        native_CPXsolwrite = load_symbol("CPXsolwrite");
    return native_CPXsolwrite(env, lp, filename_str);
}

// CPXsolwritesolnpool
CPXLIBAPI int CPXPUBLIC CPXsolwritesolnpool (CPXCENVptr env, CPXCLPptr lp, int soln, char const *filename_str)
{
    if (native_CPXsolwritesolnpool == NULL)
        native_CPXsolwritesolnpool = load_symbol("CPXsolwritesolnpool");
    return native_CPXsolwritesolnpool(env, lp, soln, filename_str);
}

// CPXsolwritesolnpoolall
CPXLIBAPI int CPXPUBLIC CPXsolwritesolnpoolall (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXsolwritesolnpoolall == NULL)
        native_CPXsolwritesolnpoolall = load_symbol("CPXsolwritesolnpoolall");
    return native_CPXsolwritesolnpoolall(env, lp, filename_str);
}

// CPXstrcpy
CPXCHARptr CPXPUBLIC CPXstrcpy (char *dest_str, char const *src_str)
{
    if (native_CPXstrcpy == NULL)
        native_CPXstrcpy = load_symbol("CPXstrcpy");
    return native_CPXstrcpy(dest_str, src_str);
}

// CPXstrlen
size_t CPXPUBLIC CPXstrlen (char const *s_str)
{
    if (native_CPXstrlen == NULL)
        native_CPXstrlen = load_symbol("CPXstrlen");
    return native_CPXstrlen(s_str);
}

// CPXstrongbranch
CPXLIBAPI int CPXPUBLIC CPXstrongbranch (CPXCENVptr env, CPXLPptr lp, int const *indices, int cnt, double *downobj, double *upobj, int itlim)
{
    if (native_CPXstrongbranch == NULL)
        native_CPXstrongbranch = load_symbol("CPXstrongbranch");
    return native_CPXstrongbranch(env, lp, indices, cnt, downobj, upobj, itlim);
}

// CPXtightenbds
CPXLIBAPI int CPXPUBLIC CPXtightenbds (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *lu, double const *bd)
{
    if (native_CPXtightenbds == NULL)
        native_CPXtightenbds = load_symbol("CPXtightenbds");
    return native_CPXtightenbds(env, lp, cnt, indices, lu, bd);
}

// CPXtuneparam
CPXLIBAPI int CPXPUBLIC CPXtuneparam (CPXENVptr env, CPXLPptr lp, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char **strval, int *tunestat_p)
{
    if (native_CPXtuneparam == NULL)
        native_CPXtuneparam = load_symbol("CPXtuneparam");
    return native_CPXtuneparam(env, lp, intcnt, intnum, intval, dblcnt, dblnum, dblval, strcnt, strnum, strval, tunestat_p);
}

// CPXtuneparamprobset
CPXLIBAPI int CPXPUBLIC CPXtuneparamprobset (CPXENVptr env, int filecnt, char **filename, char **filetype, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char **strval, int *tunestat_p)
{
    if (native_CPXtuneparamprobset == NULL)
        native_CPXtuneparamprobset = load_symbol("CPXtuneparamprobset");
    return native_CPXtuneparamprobset(env, filecnt, filename, filetype, intcnt, intnum, intval, dblcnt, dblnum, dblval, strcnt, strnum, strval, tunestat_p);
}

// CPXtxtsolwrite
int CPXPUBLIC CPXtxtsolwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXtxtsolwrite == NULL)
        native_CPXtxtsolwrite = load_symbol("CPXtxtsolwrite");
    return native_CPXtxtsolwrite(env, lp, filename_str);
}

// CPXuncrushform
CPXLIBAPI int CPXPUBLIC CPXuncrushform (CPXCENVptr env, CPXCLPptr lp, int plen, int const *pind, double const *pval, int *len_p, double *offset_p, int *ind, double *val)
{
    if (native_CPXuncrushform == NULL)
        native_CPXuncrushform = load_symbol("CPXuncrushform");
    return native_CPXuncrushform(env, lp, plen, pind, pval, len_p, offset_p, ind, val);
}

// CPXuncrushpi
CPXLIBAPI int CPXPUBLIC CPXuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi)
{
    if (native_CPXuncrushpi == NULL)
        native_CPXuncrushpi = load_symbol("CPXuncrushpi");
    return native_CPXuncrushpi(env, lp, pi, prepi);
}

// CPXuncrushx
CPXLIBAPI int CPXPUBLIC CPXuncrushx (CPXCENVptr env, CPXCLPptr lp, double *x, double const *prex)
{
    if (native_CPXuncrushx == NULL)
        native_CPXuncrushx = load_symbol("CPXuncrushx");
    return native_CPXuncrushx(env, lp, x, prex);
}

// CPXunscaleprob
CPXLIBAPI int CPXPUBLIC CPXunscaleprob (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXunscaleprob == NULL)
        native_CPXunscaleprob = load_symbol("CPXunscaleprob");
    return native_CPXunscaleprob(env, lp);
}

// CPXvecwrite
int CPXPUBLIC CPXvecwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXvecwrite == NULL)
        native_CPXvecwrite = load_symbol("CPXvecwrite");
    return native_CPXvecwrite(env, lp, filename_str);
}

// CPXversion
CPXLIBAPI CPXCCHARptr CPXPUBLIC CPXversion (CPXCENVptr env)
{
    if (native_CPXversion == NULL)
        native_CPXversion = load_symbol("CPXversion");
    return native_CPXversion(env);
}

// CPXversionnumber
CPXLIBAPI int CPXPUBLIC CPXversionnumber (CPXCENVptr env, int *version_p)
{
    if (native_CPXversionnumber == NULL)
        native_CPXversionnumber = load_symbol("CPXversionnumber");
    return native_CPXversionnumber(env, version_p);
}

// CPXwriteparam
CPXLIBAPI int CPXPUBLIC CPXwriteparam (CPXCENVptr env, char const *filename_str)
{
    if (native_CPXwriteparam == NULL)
        native_CPXwriteparam = load_symbol("CPXwriteparam");
    return native_CPXwriteparam(env, filename_str);
}

// CPXwriteprob
CPXLIBAPI int CPXPUBLIC CPXwriteprob (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, char const *filetype_str)
{
    if (native_CPXwriteprob == NULL)
        native_CPXwriteprob = load_symbol("CPXwriteprob");
    return native_CPXwriteprob(env, lp, filename_str, filetype_str);
}

// CPXbaropt
CPXLIBAPI int CPXPUBLIC CPXbaropt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXbaropt == NULL)
        native_CPXbaropt = load_symbol("CPXbaropt");
    return native_CPXbaropt(env, lp);
}

// CPXhybbaropt
CPXLIBAPI int CPXPUBLIC CPXhybbaropt (CPXCENVptr env, CPXLPptr lp, int method)
{
    if (native_CPXhybbaropt == NULL)
        native_CPXhybbaropt = load_symbol("CPXhybbaropt");
    return native_CPXhybbaropt(env, lp, method);
}

// CPXaddlazyconstraints
CPXLIBAPI int CPXPUBLIC CPXaddlazyconstraints (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **rowname)
{
    if (native_CPXaddlazyconstraints == NULL)
        native_CPXaddlazyconstraints = load_symbol("CPXaddlazyconstraints");
    return native_CPXaddlazyconstraints(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}

// CPXaddmipstarts
CPXLIBAPI int CPXPUBLIC CPXaddmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt, int nzcnt, int const *beg, int const *varindices, double const *values, int const *effortlevel, char **mipstartname)
{
    if (native_CPXaddmipstarts == NULL)
        native_CPXaddmipstarts = load_symbol("CPXaddmipstarts");
    return native_CPXaddmipstarts(env, lp, mcnt, nzcnt, beg, varindices, values, effortlevel, mipstartname);
}

// CPXaddsolnpooldivfilter
CPXLIBAPI int CPXPUBLIC CPXaddsolnpooldivfilter (CPXCENVptr env, CPXLPptr lp, double lower_bound, double upper_bound, int nzcnt, int const *ind, double const *weight, double const *refval, char const *lname_str)
{
    if (native_CPXaddsolnpooldivfilter == NULL)
        native_CPXaddsolnpooldivfilter = load_symbol("CPXaddsolnpooldivfilter");
    return native_CPXaddsolnpooldivfilter(env, lp, lower_bound, upper_bound, nzcnt, ind, weight, refval, lname_str);
}

// CPXaddsolnpoolrngfilter
CPXLIBAPI int CPXPUBLIC CPXaddsolnpoolrngfilter (CPXCENVptr env, CPXLPptr lp, double lb, double ub, int nzcnt, int const *ind, double const *val, char const *lname_str)
{
    if (native_CPXaddsolnpoolrngfilter == NULL)
        native_CPXaddsolnpoolrngfilter = load_symbol("CPXaddsolnpoolrngfilter");
    return native_CPXaddsolnpoolrngfilter(env, lp, lb, ub, nzcnt, ind, val, lname_str);
}

// CPXaddsos
CPXLIBAPI int CPXPUBLIC CPXaddsos (CPXCENVptr env, CPXLPptr lp, int numsos, int numsosnz, char const *sostype, int const *sosbeg, int const *sosind, double const *soswt, char **sosname)
{
    if (native_CPXaddsos == NULL)
        native_CPXaddsos = load_symbol("CPXaddsos");
    return native_CPXaddsos(env, lp, numsos, numsosnz, sostype, sosbeg, sosind, soswt, sosname);
}

// CPXaddusercuts
CPXLIBAPI int CPXPUBLIC CPXaddusercuts (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, char **rowname)
{
    if (native_CPXaddusercuts == NULL)
        native_CPXaddusercuts = load_symbol("CPXaddusercuts");
    return native_CPXaddusercuts(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}

// CPXbranchcallbackbranchasCPLEX
CPXLIBAPI int CPXPUBLIC CPXbranchcallbackbranchasCPLEX (CPXCENVptr env, void *cbdata, int wherefrom, int num, void *userhandle, int *seqnum_p)
{
    if (native_CPXbranchcallbackbranchasCPLEX == NULL)
        native_CPXbranchcallbackbranchasCPLEX = load_symbol("CPXbranchcallbackbranchasCPLEX");
    return native_CPXbranchcallbackbranchasCPLEX(env, cbdata, wherefrom, num, userhandle, seqnum_p);
}

// CPXbranchcallbackbranchbds
CPXLIBAPI int CPXPUBLIC CPXbranchcallbackbranchbds (CPXCENVptr env, void *cbdata, int wherefrom, int cnt, int const *indices, char const *lu, double const *bd, double nodeest, void *userhandle, int *seqnum_p)
{
    if (native_CPXbranchcallbackbranchbds == NULL)
        native_CPXbranchcallbackbranchbds = load_symbol("CPXbranchcallbackbranchbds");
    return native_CPXbranchcallbackbranchbds(env, cbdata, wherefrom, cnt, indices, lu, bd, nodeest, userhandle, seqnum_p);
}

// CPXbranchcallbackbranchconstraints
CPXLIBAPI int CPXPUBLIC CPXbranchcallbackbranchconstraints (CPXCENVptr env, void *cbdata, int wherefrom, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, double nodeest, void *userhandle, int *seqnum_p)
{
    if (native_CPXbranchcallbackbranchconstraints == NULL)
        native_CPXbranchcallbackbranchconstraints = load_symbol("CPXbranchcallbackbranchconstraints");
    return native_CPXbranchcallbackbranchconstraints(env, cbdata, wherefrom, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, nodeest, userhandle, seqnum_p);
}

// CPXbranchcallbackbranchgeneral
CPXLIBAPI int CPXPUBLIC CPXbranchcallbackbranchgeneral (CPXCENVptr env, void *cbdata, int wherefrom, int varcnt, int const *varind, char const *varlu, double const *varbd, int rcnt, int nzcnt, double const *rhs, char const *sense, int const *rmatbeg, int const *rmatind, double const *rmatval, double nodeest, void *userhandle, int *seqnum_p)
{
    if (native_CPXbranchcallbackbranchgeneral == NULL)
        native_CPXbranchcallbackbranchgeneral = load_symbol("CPXbranchcallbackbranchgeneral");
    return native_CPXbranchcallbackbranchgeneral(env, cbdata, wherefrom, varcnt, varind, varlu, varbd, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, nodeest, userhandle, seqnum_p);
}

// CPXcallbacksetnodeuserhandle
CPXLIBAPI int CPXPUBLIC CPXcallbacksetnodeuserhandle (CPXCENVptr env, void *cbdata, int wherefrom, int nodeindex, void *userhandle, void **olduserhandle_p)
{
    if (native_CPXcallbacksetnodeuserhandle == NULL)
        native_CPXcallbacksetnodeuserhandle = load_symbol("CPXcallbacksetnodeuserhandle");
    return native_CPXcallbacksetnodeuserhandle(env, cbdata, wherefrom, nodeindex, userhandle, olduserhandle_p);
}

// CPXcallbacksetuserhandle
CPXLIBAPI int CPXPUBLIC CPXcallbacksetuserhandle (CPXCENVptr env, void *cbdata, int wherefrom, void *userhandle, void **olduserhandle_p)
{
    if (native_CPXcallbacksetuserhandle == NULL)
        native_CPXcallbacksetuserhandle = load_symbol("CPXcallbacksetuserhandle");
    return native_CPXcallbacksetuserhandle(env, cbdata, wherefrom, userhandle, olduserhandle_p);
}

// CPXchgctype
CPXLIBAPI int CPXPUBLIC CPXchgctype (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, char const *xctype)
{
    if (native_CPXchgctype == NULL)
        native_CPXchgctype = load_symbol("CPXchgctype");
    return native_CPXchgctype(env, lp, cnt, indices, xctype);
}

// CPXchgmipstarts
CPXLIBAPI int CPXPUBLIC CPXchgmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt, int const *mipstartindices, int nzcnt, int const *beg, int const *varindices, double const *values, int const *effortlevel)
{
    if (native_CPXchgmipstarts == NULL)
        native_CPXchgmipstarts = load_symbol("CPXchgmipstarts");
    return native_CPXchgmipstarts(env, lp, mcnt, mipstartindices, nzcnt, beg, varindices, values, effortlevel);
}

// CPXcopyctype
CPXLIBAPI int CPXPUBLIC CPXcopyctype (CPXCENVptr env, CPXLPptr lp, char const *xctype)
{
    if (native_CPXcopyctype == NULL)
        native_CPXcopyctype = load_symbol("CPXcopyctype");
    return native_CPXcopyctype(env, lp, xctype);
}

// CPXcopymipstart
int CPXPUBLIC CPXcopymipstart (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, double const *values)
{
    if (native_CPXcopymipstart == NULL)
        native_CPXcopymipstart = load_symbol("CPXcopymipstart");
    return native_CPXcopymipstart(env, lp, cnt, indices, values);
}

// CPXcopyorder
CPXLIBAPI int CPXPUBLIC CPXcopyorder (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices, int const *priority, int const *direction)
{
    if (native_CPXcopyorder == NULL)
        native_CPXcopyorder = load_symbol("CPXcopyorder");
    return native_CPXcopyorder(env, lp, cnt, indices, priority, direction);
}

// CPXcopysos
CPXLIBAPI int CPXPUBLIC CPXcopysos (CPXCENVptr env, CPXLPptr lp, int numsos, int numsosnz, char const *sostype, int const *sosbeg, int const *sosind, double const *soswt, char **sosname)
{
    if (native_CPXcopysos == NULL)
        native_CPXcopysos = load_symbol("CPXcopysos");
    return native_CPXcopysos(env, lp, numsos, numsosnz, sostype, sosbeg, sosind, soswt, sosname);
}

// CPXcutcallbackadd
CPXLIBAPI int CPXPUBLIC CPXcutcallbackadd (CPXCENVptr env, void *cbdata, int wherefrom, int nzcnt, double rhs, int sense, int const *cutind, double const *cutval, int purgeable)
{
    if (native_CPXcutcallbackadd == NULL)
        native_CPXcutcallbackadd = load_symbol("CPXcutcallbackadd");
    return native_CPXcutcallbackadd(env, cbdata, wherefrom, nzcnt, rhs, sense, cutind, cutval, purgeable);
}

// CPXcutcallbackaddlocal
CPXLIBAPI int CPXPUBLIC CPXcutcallbackaddlocal (CPXCENVptr env, void *cbdata, int wherefrom, int nzcnt, double rhs, int sense, int const *cutind, double const *cutval)
{
    if (native_CPXcutcallbackaddlocal == NULL)
        native_CPXcutcallbackaddlocal = load_symbol("CPXcutcallbackaddlocal");
    return native_CPXcutcallbackaddlocal(env, cbdata, wherefrom, nzcnt, rhs, sense, cutind, cutval);
}

// CPXdelindconstrs
CPXLIBAPI int CPXPUBLIC CPXdelindconstrs (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXdelindconstrs == NULL)
        native_CPXdelindconstrs = load_symbol("CPXdelindconstrs");
    return native_CPXdelindconstrs(env, lp, begin, end);
}

// CPXdelmipstarts
CPXLIBAPI int CPXPUBLIC CPXdelmipstarts (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXdelmipstarts == NULL)
        native_CPXdelmipstarts = load_symbol("CPXdelmipstarts");
    return native_CPXdelmipstarts(env, lp, begin, end);
}

// CPXdelsetmipstarts
CPXLIBAPI int CPXPUBLIC CPXdelsetmipstarts (CPXCENVptr env, CPXLPptr lp, int *delstat)
{
    if (native_CPXdelsetmipstarts == NULL)
        native_CPXdelsetmipstarts = load_symbol("CPXdelsetmipstarts");
    return native_CPXdelsetmipstarts(env, lp, delstat);
}

// CPXdelsetsolnpoolfilters
CPXLIBAPI int CPXPUBLIC CPXdelsetsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int *delstat)
{
    if (native_CPXdelsetsolnpoolfilters == NULL)
        native_CPXdelsetsolnpoolfilters = load_symbol("CPXdelsetsolnpoolfilters");
    return native_CPXdelsetsolnpoolfilters(env, lp, delstat);
}

// CPXdelsetsolnpoolsolns
CPXLIBAPI int CPXPUBLIC CPXdelsetsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int *delstat)
{
    if (native_CPXdelsetsolnpoolsolns == NULL)
        native_CPXdelsetsolnpoolsolns = load_symbol("CPXdelsetsolnpoolsolns");
    return native_CPXdelsetsolnpoolsolns(env, lp, delstat);
}

// CPXdelsetsos
CPXLIBAPI int CPXPUBLIC CPXdelsetsos (CPXCENVptr env, CPXLPptr lp, int *delset)
{
    if (native_CPXdelsetsos == NULL)
        native_CPXdelsetsos = load_symbol("CPXdelsetsos");
    return native_CPXdelsetsos(env, lp, delset);
}

// CPXdelsolnpoolfilters
CPXLIBAPI int CPXPUBLIC CPXdelsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXdelsolnpoolfilters == NULL)
        native_CPXdelsolnpoolfilters = load_symbol("CPXdelsolnpoolfilters");
    return native_CPXdelsolnpoolfilters(env, lp, begin, end);
}

// CPXdelsolnpoolsolns
CPXLIBAPI int CPXPUBLIC CPXdelsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXdelsolnpoolsolns == NULL)
        native_CPXdelsolnpoolsolns = load_symbol("CPXdelsolnpoolsolns");
    return native_CPXdelsolnpoolsolns(env, lp, begin, end);
}

// CPXdelsos
CPXLIBAPI int CPXPUBLIC CPXdelsos (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXdelsos == NULL)
        native_CPXdelsos = load_symbol("CPXdelsos");
    return native_CPXdelsos(env, lp, begin, end);
}

// CPXfltwrite
CPXLIBAPI int CPXPUBLIC CPXfltwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXfltwrite == NULL)
        native_CPXfltwrite = load_symbol("CPXfltwrite");
    return native_CPXfltwrite(env, lp, filename_str);
}

// CPXfreelazyconstraints
CPXLIBAPI int CPXPUBLIC CPXfreelazyconstraints (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXfreelazyconstraints == NULL)
        native_CPXfreelazyconstraints = load_symbol("CPXfreelazyconstraints");
    return native_CPXfreelazyconstraints(env, lp);
}

// CPXfreeusercuts
CPXLIBAPI int CPXPUBLIC CPXfreeusercuts (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXfreeusercuts == NULL)
        native_CPXfreeusercuts = load_symbol("CPXfreeusercuts");
    return native_CPXfreeusercuts(env, lp);
}

// CPXgetbestobjval
CPXLIBAPI int CPXPUBLIC CPXgetbestobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p)
{
    if (native_CPXgetbestobjval == NULL)
        native_CPXgetbestobjval = load_symbol("CPXgetbestobjval");
    return native_CPXgetbestobjval(env, lp, objval_p);
}

// CPXgetbranchcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetbranchcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** branchcallback_p) (CALLBACK_BRANCH_ARGS), void **cbhandle_p)
{
    if (native_CPXgetbranchcallbackfunc == NULL)
        native_CPXgetbranchcallbackfunc = load_symbol("CPXgetbranchcallbackfunc");
    return native_CPXgetbranchcallbackfunc(env, branchcallback_p, cbhandle_p);
}

// CPXgetbranchnosolncallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetbranchnosolncallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** branchnosolncallback_p) (CALLBACK_BRANCH_ARGS), void **cbhandle_p)
{
    if (native_CPXgetbranchnosolncallbackfunc == NULL)
        native_CPXgetbranchnosolncallbackfunc = load_symbol("CPXgetbranchnosolncallbackfunc");
    return native_CPXgetbranchnosolncallbackfunc(env, branchnosolncallback_p, cbhandle_p);
}

// CPXgetcallbackbranchconstraints
CPXLIBAPI int CPXPUBLIC CPXgetcallbackbranchconstraints (CPXCENVptr env, void *cbdata, int wherefrom, int which, int *cuts_p, int *nzcnt_p, double *rhs, char *sense, int *rmatbeg, int *rmatind, double *rmatval, int rmatsz, int *surplus_p)
{
    if (native_CPXgetcallbackbranchconstraints == NULL)
        native_CPXgetcallbackbranchconstraints = load_symbol("CPXgetcallbackbranchconstraints");
    return native_CPXgetcallbackbranchconstraints(env, cbdata, wherefrom, which, cuts_p, nzcnt_p, rhs, sense, rmatbeg, rmatind, rmatval, rmatsz, surplus_p);
}

// CPXgetcallbackctype
CPXLIBAPI int CPXPUBLIC CPXgetcallbackctype (CPXCENVptr env, void *cbdata, int wherefrom, char *xctype, int begin, int end)
{
    if (native_CPXgetcallbackctype == NULL)
        native_CPXgetcallbackctype = load_symbol("CPXgetcallbackctype");
    return native_CPXgetcallbackctype(env, cbdata, wherefrom, xctype, begin, end);
}

// CPXgetcallbackgloballb
CPXLIBAPI int CPXPUBLIC CPXgetcallbackgloballb (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, int begin, int end)
{
    if (native_CPXgetcallbackgloballb == NULL)
        native_CPXgetcallbackgloballb = load_symbol("CPXgetcallbackgloballb");
    return native_CPXgetcallbackgloballb(env, cbdata, wherefrom, lb, begin, end);
}

// CPXgetcallbackglobalub
CPXLIBAPI int CPXPUBLIC CPXgetcallbackglobalub (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, int begin, int end)
{
    if (native_CPXgetcallbackglobalub == NULL)
        native_CPXgetcallbackglobalub = load_symbol("CPXgetcallbackglobalub");
    return native_CPXgetcallbackglobalub(env, cbdata, wherefrom, ub, begin, end);
}

// CPXgetcallbackincumbent
CPXLIBAPI int CPXPUBLIC CPXgetcallbackincumbent (CPXCENVptr env, void *cbdata, int wherefrom, double *x, int begin, int end)
{
    if (native_CPXgetcallbackincumbent == NULL)
        native_CPXgetcallbackincumbent = load_symbol("CPXgetcallbackincumbent");
    return native_CPXgetcallbackincumbent(env, cbdata, wherefrom, x, begin, end);
}

// CPXgetcallbackindicatorinfo
CPXLIBAPI int CPXPUBLIC CPXgetcallbackindicatorinfo (CPXCENVptr env, void *cbdata, int wherefrom, int iindex, int whichinfo, void *result_p)
{
    if (native_CPXgetcallbackindicatorinfo == NULL)
        native_CPXgetcallbackindicatorinfo = load_symbol("CPXgetcallbackindicatorinfo");
    return native_CPXgetcallbackindicatorinfo(env, cbdata, wherefrom, iindex, whichinfo, result_p);
}

// CPXgetcallbacklp
CPXLIBAPI int CPXPUBLIC CPXgetcallbacklp (CPXCENVptr env, void *cbdata, int wherefrom, CPXCLPptr * lp_p)
{
    if (native_CPXgetcallbacklp == NULL)
        native_CPXgetcallbacklp = load_symbol("CPXgetcallbacklp");
    return native_CPXgetcallbacklp(env, cbdata, wherefrom, lp_p);
}

// CPXgetcallbacknodeinfo
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeinfo (CPXCENVptr env, void *cbdata, int wherefrom, int nodeindex, int whichinfo, void *result_p)
{
    if (native_CPXgetcallbacknodeinfo == NULL)
        native_CPXgetcallbacknodeinfo = load_symbol("CPXgetcallbacknodeinfo");
    return native_CPXgetcallbacknodeinfo(env, cbdata, wherefrom, nodeindex, whichinfo, result_p);
}

// CPXgetcallbacknodeintfeas
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeintfeas (CPXCENVptr env, void *cbdata, int wherefrom, int *feas, int begin, int end)
{
    if (native_CPXgetcallbacknodeintfeas == NULL)
        native_CPXgetcallbacknodeintfeas = load_symbol("CPXgetcallbacknodeintfeas");
    return native_CPXgetcallbacknodeintfeas(env, cbdata, wherefrom, feas, begin, end);
}

// CPXgetcallbacknodelb
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodelb (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, int begin, int end)
{
    if (native_CPXgetcallbacknodelb == NULL)
        native_CPXgetcallbacknodelb = load_symbol("CPXgetcallbacknodelb");
    return native_CPXgetcallbacknodelb(env, cbdata, wherefrom, lb, begin, end);
}

// CPXgetcallbacknodelp
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodelp (CPXCENVptr env, void *cbdata, int wherefrom, CPXLPptr * nodelp_p)
{
    if (native_CPXgetcallbacknodelp == NULL)
        native_CPXgetcallbacknodelp = load_symbol("CPXgetcallbacknodelp");
    return native_CPXgetcallbacknodelp(env, cbdata, wherefrom, nodelp_p);
}

// CPXgetcallbacknodeobjval
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeobjval (CPXCENVptr env, void *cbdata, int wherefrom, double *objval_p)
{
    if (native_CPXgetcallbacknodeobjval == NULL)
        native_CPXgetcallbacknodeobjval = load_symbol("CPXgetcallbacknodeobjval");
    return native_CPXgetcallbacknodeobjval(env, cbdata, wherefrom, objval_p);
}

// CPXgetcallbacknodestat
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodestat (CPXCENVptr env, void *cbdata, int wherefrom, int *nodestat_p)
{
    if (native_CPXgetcallbacknodestat == NULL)
        native_CPXgetcallbacknodestat = load_symbol("CPXgetcallbacknodestat");
    return native_CPXgetcallbacknodestat(env, cbdata, wherefrom, nodestat_p);
}

// CPXgetcallbacknodeub
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeub (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, int begin, int end)
{
    if (native_CPXgetcallbacknodeub == NULL)
        native_CPXgetcallbacknodeub = load_symbol("CPXgetcallbacknodeub");
    return native_CPXgetcallbacknodeub(env, cbdata, wherefrom, ub, begin, end);
}

// CPXgetcallbacknodex
CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodex (CPXCENVptr env, void *cbdata, int wherefrom, double *x, int begin, int end)
{
    if (native_CPXgetcallbacknodex == NULL)
        native_CPXgetcallbacknodex = load_symbol("CPXgetcallbacknodex");
    return native_CPXgetcallbacknodex(env, cbdata, wherefrom, x, begin, end);
}

// CPXgetcallbackorder
CPXLIBAPI int CPXPUBLIC CPXgetcallbackorder (CPXCENVptr env, void *cbdata, int wherefrom, int *priority, int *direction, int begin, int end)
{
    if (native_CPXgetcallbackorder == NULL)
        native_CPXgetcallbackorder = load_symbol("CPXgetcallbackorder");
    return native_CPXgetcallbackorder(env, cbdata, wherefrom, priority, direction, begin, end);
}

// CPXgetcallbackpseudocosts
CPXLIBAPI int CPXPUBLIC CPXgetcallbackpseudocosts (CPXCENVptr env, void *cbdata, int wherefrom, double *uppc, double *downpc, int begin, int end)
{
    if (native_CPXgetcallbackpseudocosts == NULL)
        native_CPXgetcallbackpseudocosts = load_symbol("CPXgetcallbackpseudocosts");
    return native_CPXgetcallbackpseudocosts(env, cbdata, wherefrom, uppc, downpc, begin, end);
}

// CPXgetcallbackseqinfo
CPXLIBAPI int CPXPUBLIC CPXgetcallbackseqinfo (CPXCENVptr env, void *cbdata, int wherefrom, int seqid, int whichinfo, void *result_p)
{
    if (native_CPXgetcallbackseqinfo == NULL)
        native_CPXgetcallbackseqinfo = load_symbol("CPXgetcallbackseqinfo");
    return native_CPXgetcallbackseqinfo(env, cbdata, wherefrom, seqid, whichinfo, result_p);
}

// CPXgetcallbacksosinfo
CPXLIBAPI int CPXPUBLIC CPXgetcallbacksosinfo (CPXCENVptr env, void *cbdata, int wherefrom, int sosindex, int member, int whichinfo, void *result_p)
{
    if (native_CPXgetcallbacksosinfo == NULL)
        native_CPXgetcallbacksosinfo = load_symbol("CPXgetcallbacksosinfo");
    return native_CPXgetcallbacksosinfo(env, cbdata, wherefrom, sosindex, member, whichinfo, result_p);
}

// CPXgetctype
CPXLIBAPI int CPXPUBLIC CPXgetctype (CPXCENVptr env, CPXCLPptr lp, char *xctype, int begin, int end)
{
    if (native_CPXgetctype == NULL)
        native_CPXgetctype = load_symbol("CPXgetctype");
    return native_CPXgetctype(env, lp, xctype, begin, end);
}

// CPXgetcutcallbackfunc
int CPXPUBLIC CPXgetcutcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** cutcallback_p) (CALLBACK_CUT_ARGS), void **cbhandle_p)
{
    if (native_CPXgetcutcallbackfunc == NULL)
        native_CPXgetcutcallbackfunc = load_symbol("CPXgetcutcallbackfunc");
    return native_CPXgetcutcallbackfunc(env, cutcallback_p, cbhandle_p);
}

// CPXgetcutoff
CPXLIBAPI int CPXPUBLIC CPXgetcutoff (CPXCENVptr env, CPXCLPptr lp, double *cutoff_p)
{
    if (native_CPXgetcutoff == NULL)
        native_CPXgetcutoff = load_symbol("CPXgetcutoff");
    return native_CPXgetcutoff(env, lp, cutoff_p);
}

// CPXgetdeletenodecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetdeletenodecallbackfunc (CPXCENVptr env, void (CPXPUBLIC ** deletecallback_p) (CALLBACK_DELETENODE_ARGS), void **cbhandle_p)
{
    if (native_CPXgetdeletenodecallbackfunc == NULL)
        native_CPXgetdeletenodecallbackfunc = load_symbol("CPXgetdeletenodecallbackfunc");
    return native_CPXgetdeletenodecallbackfunc(env, deletecallback_p, cbhandle_p);
}

// CPXgetheuristiccallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetheuristiccallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** heuristiccallback_p) (CALLBACK_HEURISTIC_ARGS), void **cbhandle_p)
{
    if (native_CPXgetheuristiccallbackfunc == NULL)
        native_CPXgetheuristiccallbackfunc = load_symbol("CPXgetheuristiccallbackfunc");
    return native_CPXgetheuristiccallbackfunc(env, heuristiccallback_p, cbhandle_p);
}

// CPXgetincumbentcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetincumbentcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** incumbentcallback_p) (CALLBACK_INCUMBENT_ARGS), void **cbhandle_p)
{
    if (native_CPXgetincumbentcallbackfunc == NULL)
        native_CPXgetincumbentcallbackfunc = load_symbol("CPXgetincumbentcallbackfunc");
    return native_CPXgetincumbentcallbackfunc(env, incumbentcallback_p, cbhandle_p);
}

// CPXgetindconstr
CPXLIBAPI int CPXPUBLIC CPXgetindconstr (CPXCENVptr env, CPXCLPptr lp, int *indvar_p, int *complemented_p, int *nzcnt_p, double *rhs_p, char *sense_p, int *linind, double *linval, int space, int *surplus_p, int which)
{
    if (native_CPXgetindconstr == NULL)
        native_CPXgetindconstr = load_symbol("CPXgetindconstr");
    return native_CPXgetindconstr(env, lp, indvar_p, complemented_p, nzcnt_p, rhs_p, sense_p, linind, linval, space, surplus_p, which);
}

// CPXgetindconstrindex
CPXLIBAPI int CPXPUBLIC CPXgetindconstrindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXgetindconstrindex == NULL)
        native_CPXgetindconstrindex = load_symbol("CPXgetindconstrindex");
    return native_CPXgetindconstrindex(env, lp, lname_str, index_p);
}

// CPXgetindconstrinfeas
CPXLIBAPI int CPXPUBLIC CPXgetindconstrinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end)
{
    if (native_CPXgetindconstrinfeas == NULL)
        native_CPXgetindconstrinfeas = load_symbol("CPXgetindconstrinfeas");
    return native_CPXgetindconstrinfeas(env, lp, x, infeasout, begin, end);
}

// CPXgetindconstrname
CPXLIBAPI int CPXPUBLIC CPXgetindconstrname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p, int which)
{
    if (native_CPXgetindconstrname == NULL)
        native_CPXgetindconstrname = load_symbol("CPXgetindconstrname");
    return native_CPXgetindconstrname(env, lp, buf_str, bufspace, surplus_p, which);
}

// CPXgetindconstrslack
CPXLIBAPI int CPXPUBLIC CPXgetindconstrslack (CPXCENVptr env, CPXCLPptr lp, double *indslack, int begin, int end)
{
    if (native_CPXgetindconstrslack == NULL)
        native_CPXgetindconstrslack = load_symbol("CPXgetindconstrslack");
    return native_CPXgetindconstrslack(env, lp, indslack, begin, end);
}

// CPXgetinfocallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetinfocallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p)
{
    if (native_CPXgetinfocallbackfunc == NULL)
        native_CPXgetinfocallbackfunc = load_symbol("CPXgetinfocallbackfunc");
    return native_CPXgetinfocallbackfunc(env, callback_p, cbhandle_p);
}

// CPXgetlazyconstraintcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetlazyconstraintcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** cutcallback_p) (CALLBACK_CUT_ARGS), void **cbhandle_p)
{
    if (native_CPXgetlazyconstraintcallbackfunc == NULL)
        native_CPXgetlazyconstraintcallbackfunc = load_symbol("CPXgetlazyconstraintcallbackfunc");
    return native_CPXgetlazyconstraintcallbackfunc(env, cutcallback_p, cbhandle_p);
}

// CPXgetmipcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetmipcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *, int, void *), void **cbhandle_p)
{
    if (native_CPXgetmipcallbackfunc == NULL)
        native_CPXgetmipcallbackfunc = load_symbol("CPXgetmipcallbackfunc");
    return native_CPXgetmipcallbackfunc(env, callback_p, cbhandle_p);
}

// CPXgetmipitcnt
CPXLIBAPI int CPXPUBLIC CPXgetmipitcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetmipitcnt == NULL)
        native_CPXgetmipitcnt = load_symbol("CPXgetmipitcnt");
    return native_CPXgetmipitcnt(env, lp);
}

// CPXgetmiprelgap
CPXLIBAPI int CPXPUBLIC CPXgetmiprelgap (CPXCENVptr env, CPXCLPptr lp, double *gap_p)
{
    if (native_CPXgetmiprelgap == NULL)
        native_CPXgetmiprelgap = load_symbol("CPXgetmiprelgap");
    return native_CPXgetmiprelgap(env, lp, gap_p);
}

// CPXgetmipstartindex
CPXLIBAPI int CPXPUBLIC CPXgetmipstartindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXgetmipstartindex == NULL)
        native_CPXgetmipstartindex = load_symbol("CPXgetmipstartindex");
    return native_CPXgetmipstartindex(env, lp, lname_str, index_p);
}

// CPXgetmipstartname
CPXLIBAPI int CPXPUBLIC CPXgetmipstartname (CPXCENVptr env, CPXCLPptr lp, char **name, char *store, int storesz, int *surplus_p, int begin, int end)
{
    if (native_CPXgetmipstartname == NULL)
        native_CPXgetmipstartname = load_symbol("CPXgetmipstartname");
    return native_CPXgetmipstartname(env, lp, name, store, storesz, surplus_p, begin, end);
}

// CPXgetmipstarts
CPXLIBAPI int CPXPUBLIC CPXgetmipstarts (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *beg, int *varindices, double *values, int *effortlevel, int startspace, int *surplus_p, int begin, int end)
{
    if (native_CPXgetmipstarts == NULL)
        native_CPXgetmipstarts = load_symbol("CPXgetmipstarts");
    return native_CPXgetmipstarts(env, lp, nzcnt_p, beg, varindices, values, effortlevel, startspace, surplus_p, begin, end);
}

// CPXgetnodecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetnodecallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** nodecallback_p) (CALLBACK_NODE_ARGS), void **cbhandle_p)
{
    if (native_CPXgetnodecallbackfunc == NULL)
        native_CPXgetnodecallbackfunc = load_symbol("CPXgetnodecallbackfunc");
    return native_CPXgetnodecallbackfunc(env, nodecallback_p, cbhandle_p);
}

// CPXgetnodecnt
CPXLIBAPI int CPXPUBLIC CPXgetnodecnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnodecnt == NULL)
        native_CPXgetnodecnt = load_symbol("CPXgetnodecnt");
    return native_CPXgetnodecnt(env, lp);
}

// CPXgetnodeint
CPXLIBAPI int CPXPUBLIC CPXgetnodeint (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnodeint == NULL)
        native_CPXgetnodeint = load_symbol("CPXgetnodeint");
    return native_CPXgetnodeint(env, lp);
}

// CPXgetnodeleftcnt
CPXLIBAPI int CPXPUBLIC CPXgetnodeleftcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnodeleftcnt == NULL)
        native_CPXgetnodeleftcnt = load_symbol("CPXgetnodeleftcnt");
    return native_CPXgetnodeleftcnt(env, lp);
}

// CPXgetnumbin
CPXLIBAPI int CPXPUBLIC CPXgetnumbin (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumbin == NULL)
        native_CPXgetnumbin = load_symbol("CPXgetnumbin");
    return native_CPXgetnumbin(env, lp);
}

// CPXgetnumcuts
CPXLIBAPI int CPXPUBLIC CPXgetnumcuts (CPXCENVptr env, CPXCLPptr lp, int cuttype, int *num_p)
{
    if (native_CPXgetnumcuts == NULL)
        native_CPXgetnumcuts = load_symbol("CPXgetnumcuts");
    return native_CPXgetnumcuts(env, lp, cuttype, num_p);
}

// CPXgetnumindconstrs
CPXLIBAPI int CPXPUBLIC CPXgetnumindconstrs (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumindconstrs == NULL)
        native_CPXgetnumindconstrs = load_symbol("CPXgetnumindconstrs");
    return native_CPXgetnumindconstrs(env, lp);
}

// CPXgetnumint
CPXLIBAPI int CPXPUBLIC CPXgetnumint (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumint == NULL)
        native_CPXgetnumint = load_symbol("CPXgetnumint");
    return native_CPXgetnumint(env, lp);
}

// CPXgetnumlazyconstraints
CPXLIBAPI int CPXPUBLIC CPXgetnumlazyconstraints (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumlazyconstraints == NULL)
        native_CPXgetnumlazyconstraints = load_symbol("CPXgetnumlazyconstraints");
    return native_CPXgetnumlazyconstraints(env, lp);
}

// CPXgetnummipstarts
CPXLIBAPI int CPXPUBLIC CPXgetnummipstarts (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnummipstarts == NULL)
        native_CPXgetnummipstarts = load_symbol("CPXgetnummipstarts");
    return native_CPXgetnummipstarts(env, lp);
}

// CPXgetnumsemicont
CPXLIBAPI int CPXPUBLIC CPXgetnumsemicont (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumsemicont == NULL)
        native_CPXgetnumsemicont = load_symbol("CPXgetnumsemicont");
    return native_CPXgetnumsemicont(env, lp);
}

// CPXgetnumsemiint
CPXLIBAPI int CPXPUBLIC CPXgetnumsemiint (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumsemiint == NULL)
        native_CPXgetnumsemiint = load_symbol("CPXgetnumsemiint");
    return native_CPXgetnumsemiint(env, lp);
}

// CPXgetnumsos
CPXLIBAPI int CPXPUBLIC CPXgetnumsos (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumsos == NULL)
        native_CPXgetnumsos = load_symbol("CPXgetnumsos");
    return native_CPXgetnumsos(env, lp);
}

// CPXgetnumusercuts
CPXLIBAPI int CPXPUBLIC CPXgetnumusercuts (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumusercuts == NULL)
        native_CPXgetnumusercuts = load_symbol("CPXgetnumusercuts");
    return native_CPXgetnumusercuts(env, lp);
}

// CPXgetorder
CPXLIBAPI int CPXPUBLIC CPXgetorder (CPXCENVptr env, CPXCLPptr lp, int *cnt_p, int *indices, int *priority, int *direction, int ordspace, int *surplus_p)
{
    if (native_CPXgetorder == NULL)
        native_CPXgetorder = load_symbol("CPXgetorder");
    return native_CPXgetorder(env, lp, cnt_p, indices, priority, direction, ordspace, surplus_p);
}

// CPXgetsolnpooldivfilter
CPXLIBAPI int CPXPUBLIC CPXgetsolnpooldivfilter (CPXCENVptr env, CPXCLPptr lp, double *lower_cutoff_p, double *upper_cutoff_p, int *nzcnt_p, int *ind, double *val, double *refval, int space, int *surplus_p, int which)
{
    if (native_CPXgetsolnpooldivfilter == NULL)
        native_CPXgetsolnpooldivfilter = load_symbol("CPXgetsolnpooldivfilter");
    return native_CPXgetsolnpooldivfilter(env, lp, lower_cutoff_p, upper_cutoff_p, nzcnt_p, ind, val, refval, space, surplus_p, which);
}

// CPXgetsolnpoolfilterindex
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolfilterindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXgetsolnpoolfilterindex == NULL)
        native_CPXgetsolnpoolfilterindex = load_symbol("CPXgetsolnpoolfilterindex");
    return native_CPXgetsolnpoolfilterindex(env, lp, lname_str, index_p);
}

// CPXgetsolnpoolfiltername
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolfiltername (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p, int which)
{
    if (native_CPXgetsolnpoolfiltername == NULL)
        native_CPXgetsolnpoolfiltername = load_symbol("CPXgetsolnpoolfiltername");
    return native_CPXgetsolnpoolfiltername(env, lp, buf_str, bufspace, surplus_p, which);
}

// CPXgetsolnpoolfiltertype
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolfiltertype (CPXCENVptr env, CPXCLPptr lp, int *ftype_p, int which)
{
    if (native_CPXgetsolnpoolfiltertype == NULL)
        native_CPXgetsolnpoolfiltertype = load_symbol("CPXgetsolnpoolfiltertype");
    return native_CPXgetsolnpoolfiltertype(env, lp, ftype_p, which);
}

// CPXgetsolnpoolmeanobjval
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolmeanobjval (CPXCENVptr env, CPXCLPptr lp, double *meanobjval_p)
{
    if (native_CPXgetsolnpoolmeanobjval == NULL)
        native_CPXgetsolnpoolmeanobjval = load_symbol("CPXgetsolnpoolmeanobjval");
    return native_CPXgetsolnpoolmeanobjval(env, lp, meanobjval_p);
}

// CPXgetsolnpoolnumfilters
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolnumfilters (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetsolnpoolnumfilters == NULL)
        native_CPXgetsolnpoolnumfilters = load_symbol("CPXgetsolnpoolnumfilters");
    return native_CPXgetsolnpoolnumfilters(env, lp);
}

// CPXgetsolnpoolnumreplaced
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolnumreplaced (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetsolnpoolnumreplaced == NULL)
        native_CPXgetsolnpoolnumreplaced = load_symbol("CPXgetsolnpoolnumreplaced");
    return native_CPXgetsolnpoolnumreplaced(env, lp);
}

// CPXgetsolnpoolnumsolns
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolnumsolns (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetsolnpoolnumsolns == NULL)
        native_CPXgetsolnpoolnumsolns = load_symbol("CPXgetsolnpoolnumsolns");
    return native_CPXgetsolnpoolnumsolns(env, lp);
}

// CPXgetsolnpoolobjval
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolobjval (CPXCENVptr env, CPXCLPptr lp, int soln, double *objval_p)
{
    if (native_CPXgetsolnpoolobjval == NULL)
        native_CPXgetsolnpoolobjval = load_symbol("CPXgetsolnpoolobjval");
    return native_CPXgetsolnpoolobjval(env, lp, soln, objval_p);
}

// CPXgetsolnpoolqconstrslack
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolqconstrslack (CPXCENVptr env, CPXCLPptr lp, int soln, double *qcslack, int begin, int end)
{
    if (native_CPXgetsolnpoolqconstrslack == NULL)
        native_CPXgetsolnpoolqconstrslack = load_symbol("CPXgetsolnpoolqconstrslack");
    return native_CPXgetsolnpoolqconstrslack(env, lp, soln, qcslack, begin, end);
}

// CPXgetsolnpoolrngfilter
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolrngfilter (CPXCENVptr env, CPXCLPptr lp, double *lb_p, double *ub_p, int *nzcnt_p, int *ind, double *val, int space, int *surplus_p, int which)
{
    if (native_CPXgetsolnpoolrngfilter == NULL)
        native_CPXgetsolnpoolrngfilter = load_symbol("CPXgetsolnpoolrngfilter");
    return native_CPXgetsolnpoolrngfilter(env, lp, lb_p, ub_p, nzcnt_p, ind, val, space, surplus_p, which);
}

// CPXgetsolnpoolslack
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolslack (CPXCENVptr env, CPXCLPptr lp, int soln, double *slack, int begin, int end)
{
    if (native_CPXgetsolnpoolslack == NULL)
        native_CPXgetsolnpoolslack = load_symbol("CPXgetsolnpoolslack");
    return native_CPXgetsolnpoolslack(env, lp, soln, slack, begin, end);
}

// CPXgetsolnpoolsolnindex
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolsolnindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXgetsolnpoolsolnindex == NULL)
        native_CPXgetsolnpoolsolnindex = load_symbol("CPXgetsolnpoolsolnindex");
    return native_CPXgetsolnpoolsolnindex(env, lp, lname_str, index_p);
}

// CPXgetsolnpoolsolnname
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolsolnname (CPXCENVptr env, CPXCLPptr lp, char *store, int storesz, int *surplus_p, int which)
{
    if (native_CPXgetsolnpoolsolnname == NULL)
        native_CPXgetsolnpoolsolnname = load_symbol("CPXgetsolnpoolsolnname");
    return native_CPXgetsolnpoolsolnname(env, lp, store, storesz, surplus_p, which);
}

// CPXgetsolnpoolx
CPXLIBAPI int CPXPUBLIC CPXgetsolnpoolx (CPXCENVptr env, CPXCLPptr lp, int soln, double *x, int begin, int end)
{
    if (native_CPXgetsolnpoolx == NULL)
        native_CPXgetsolnpoolx = load_symbol("CPXgetsolnpoolx");
    return native_CPXgetsolnpoolx(env, lp, soln, x, begin, end);
}

// CPXgetsolvecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetsolvecallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** solvecallback_p) (CALLBACK_SOLVE_ARGS), void **cbhandle_p)
{
    if (native_CPXgetsolvecallbackfunc == NULL)
        native_CPXgetsolvecallbackfunc = load_symbol("CPXgetsolvecallbackfunc");
    return native_CPXgetsolvecallbackfunc(env, solvecallback_p, cbhandle_p);
}

// CPXgetsos
CPXLIBAPI int CPXPUBLIC CPXgetsos (CPXCENVptr env, CPXCLPptr lp, int *numsosnz_p, char *sostype, int *sosbeg, int *sosind, double *soswt, int sosspace, int *surplus_p, int begin, int end)
{
    if (native_CPXgetsos == NULL)
        native_CPXgetsos = load_symbol("CPXgetsos");
    return native_CPXgetsos(env, lp, numsosnz_p, sostype, sosbeg, sosind, soswt, sosspace, surplus_p, begin, end);
}

// CPXgetsosindex
CPXLIBAPI int CPXPUBLIC CPXgetsosindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXgetsosindex == NULL)
        native_CPXgetsosindex = load_symbol("CPXgetsosindex");
    return native_CPXgetsosindex(env, lp, lname_str, index_p);
}

// CPXgetsosinfeas
CPXLIBAPI int CPXPUBLIC CPXgetsosinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end)
{
    if (native_CPXgetsosinfeas == NULL)
        native_CPXgetsosinfeas = load_symbol("CPXgetsosinfeas");
    return native_CPXgetsosinfeas(env, lp, x, infeasout, begin, end);
}

// CPXgetsosname
CPXLIBAPI int CPXPUBLIC CPXgetsosname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, int storespace, int *surplus_p, int begin, int end)
{
    if (native_CPXgetsosname == NULL)
        native_CPXgetsosname = load_symbol("CPXgetsosname");
    return native_CPXgetsosname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}

// CPXgetsubmethod
CPXLIBAPI int CPXPUBLIC CPXgetsubmethod (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetsubmethod == NULL)
        native_CPXgetsubmethod = load_symbol("CPXgetsubmethod");
    return native_CPXgetsubmethod(env, lp);
}

// CPXgetsubstat
CPXLIBAPI int CPXPUBLIC CPXgetsubstat (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetsubstat == NULL)
        native_CPXgetsubstat = load_symbol("CPXgetsubstat");
    return native_CPXgetsubstat(env, lp);
}

// CPXgetusercutcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXgetusercutcallbackfunc (CPXCENVptr env, int (CPXPUBLIC ** cutcallback_p) (CALLBACK_CUT_ARGS), void **cbhandle_p)
{
    if (native_CPXgetusercutcallbackfunc == NULL)
        native_CPXgetusercutcallbackfunc = load_symbol("CPXgetusercutcallbackfunc");
    return native_CPXgetusercutcallbackfunc(env, cutcallback_p, cbhandle_p);
}

// CPXindconstrslackfromx
CPXLIBAPI int CPXPUBLIC CPXindconstrslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *indslack)
{
    if (native_CPXindconstrslackfromx == NULL)
        native_CPXindconstrslackfromx = load_symbol("CPXindconstrslackfromx");
    return native_CPXindconstrslackfromx(env, lp, x, indslack);
}

// CPXmipopt
CPXLIBAPI int CPXPUBLIC CPXmipopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXmipopt == NULL)
        native_CPXmipopt = load_symbol("CPXmipopt");
    return native_CPXmipopt(env, lp);
}

// CPXordread
CPXLIBAPI int CPXPUBLIC CPXordread (CPXCENVptr env, char const *filename_str, int numcols, char **colname, int *cnt_p, int *indices, int *priority, int *direction)
{
    if (native_CPXordread == NULL)
        native_CPXordread = load_symbol("CPXordread");
    return native_CPXordread(env, filename_str, numcols, colname, cnt_p, indices, priority, direction);
}

// CPXordwrite
CPXLIBAPI int CPXPUBLIC CPXordwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXordwrite == NULL)
        native_CPXordwrite = load_symbol("CPXordwrite");
    return native_CPXordwrite(env, lp, filename_str);
}

// CPXpopulate
CPXLIBAPI int CPXPUBLIC CPXpopulate (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXpopulate == NULL)
        native_CPXpopulate = load_symbol("CPXpopulate");
    return native_CPXpopulate(env, lp);
}

// CPXreadcopymipstarts
CPXLIBAPI int CPXPUBLIC CPXreadcopymipstarts (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXreadcopymipstarts == NULL)
        native_CPXreadcopymipstarts = load_symbol("CPXreadcopymipstarts");
    return native_CPXreadcopymipstarts(env, lp, filename_str);
}

// CPXreadcopyorder
CPXLIBAPI int CPXPUBLIC CPXreadcopyorder (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXreadcopyorder == NULL)
        native_CPXreadcopyorder = load_symbol("CPXreadcopyorder");
    return native_CPXreadcopyorder(env, lp, filename_str);
}

// CPXreadcopysolnpoolfilters
CPXLIBAPI int CPXPUBLIC CPXreadcopysolnpoolfilters (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXreadcopysolnpoolfilters == NULL)
        native_CPXreadcopysolnpoolfilters = load_symbol("CPXreadcopysolnpoolfilters");
    return native_CPXreadcopysolnpoolfilters(env, lp, filename_str);
}

// CPXrefinemipstartconflict
CPXLIBAPI int CPXPUBLIC CPXrefinemipstartconflict (CPXCENVptr env, CPXLPptr lp, int mipstartindex, int *confnumrows_p, int *confnumcols_p)
{
    if (native_CPXrefinemipstartconflict == NULL)
        native_CPXrefinemipstartconflict = load_symbol("CPXrefinemipstartconflict");
    return native_CPXrefinemipstartconflict(env, lp, mipstartindex, confnumrows_p, confnumcols_p);
}

// CPXrefinemipstartconflictext
CPXLIBAPI int CPXPUBLIC CPXrefinemipstartconflictext (CPXCENVptr env, CPXLPptr lp, int mipstartindex, int grpcnt, int concnt, double const *grppref, int const *grpbeg, int const *grpind, char const *grptype)
{
    if (native_CPXrefinemipstartconflictext == NULL)
        native_CPXrefinemipstartconflictext = load_symbol("CPXrefinemipstartconflictext");
    return native_CPXrefinemipstartconflictext(env, lp, mipstartindex, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}

// CPXsetbranchcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetbranchcallbackfunc (CPXENVptr env, int (CPXPUBLIC * branchcallback) (CALLBACK_BRANCH_ARGS), void *cbhandle)
{
    if (native_CPXsetbranchcallbackfunc == NULL)
        native_CPXsetbranchcallbackfunc = load_symbol("CPXsetbranchcallbackfunc");
    return native_CPXsetbranchcallbackfunc(env, branchcallback, cbhandle);
}

// CPXsetbranchnosolncallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetbranchnosolncallbackfunc (CPXENVptr env, int (CPXPUBLIC * branchnosolncallback) (CALLBACK_BRANCH_ARGS), void *cbhandle)
{
    if (native_CPXsetbranchnosolncallbackfunc == NULL)
        native_CPXsetbranchnosolncallbackfunc = load_symbol("CPXsetbranchnosolncallbackfunc");
    return native_CPXsetbranchnosolncallbackfunc(env, branchnosolncallback, cbhandle);
}

// CPXsetcutcallbackfunc
int CPXPUBLIC CPXsetcutcallbackfunc (CPXENVptr env, int (CPXPUBLIC * cutcallback) (CALLBACK_CUT_ARGS), void *cbhandle)
{
    if (native_CPXsetcutcallbackfunc == NULL)
        native_CPXsetcutcallbackfunc = load_symbol("CPXsetcutcallbackfunc");
    return native_CPXsetcutcallbackfunc(env, cutcallback, cbhandle);
}

// CPXsetdeletenodecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetdeletenodecallbackfunc (CPXENVptr env, void (CPXPUBLIC * deletecallback) (CALLBACK_DELETENODE_ARGS), void *cbhandle)
{
    if (native_CPXsetdeletenodecallbackfunc == NULL)
        native_CPXsetdeletenodecallbackfunc = load_symbol("CPXsetdeletenodecallbackfunc");
    return native_CPXsetdeletenodecallbackfunc(env, deletecallback, cbhandle);
}

// CPXsetheuristiccallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetheuristiccallbackfunc (CPXENVptr env, int (CPXPUBLIC * heuristiccallback) (CALLBACK_HEURISTIC_ARGS), void *cbhandle)
{
    if (native_CPXsetheuristiccallbackfunc == NULL)
        native_CPXsetheuristiccallbackfunc = load_symbol("CPXsetheuristiccallbackfunc");
    return native_CPXsetheuristiccallbackfunc(env, heuristiccallback, cbhandle);
}

// CPXsetincumbentcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetincumbentcallbackfunc (CPXENVptr env, int (CPXPUBLIC * incumbentcallback) (CALLBACK_INCUMBENT_ARGS), void *cbhandle)
{
    if (native_CPXsetincumbentcallbackfunc == NULL)
        native_CPXsetincumbentcallbackfunc = load_symbol("CPXsetincumbentcallbackfunc");
    return native_CPXsetincumbentcallbackfunc(env, incumbentcallback, cbhandle);
}

// CPXsetinfocallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetinfocallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle)
{
    if (native_CPXsetinfocallbackfunc == NULL)
        native_CPXsetinfocallbackfunc = load_symbol("CPXsetinfocallbackfunc");
    return native_CPXsetinfocallbackfunc(env, callback, cbhandle);
}

// CPXsetlazyconstraintcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetlazyconstraintcallbackfunc (CPXENVptr env, int (CPXPUBLIC * lazyconcallback) (CALLBACK_CUT_ARGS), void *cbhandle)
{
    if (native_CPXsetlazyconstraintcallbackfunc == NULL)
        native_CPXsetlazyconstraintcallbackfunc = load_symbol("CPXsetlazyconstraintcallbackfunc");
    return native_CPXsetlazyconstraintcallbackfunc(env, lazyconcallback, cbhandle);
}

// CPXsetmipcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetmipcallbackfunc (CPXENVptr env, int (CPXPUBLIC * callback) (CPXCENVptr, void *, int, void *), void *cbhandle)
{
    if (native_CPXsetmipcallbackfunc == NULL)
        native_CPXsetmipcallbackfunc = load_symbol("CPXsetmipcallbackfunc");
    return native_CPXsetmipcallbackfunc(env, callback, cbhandle);
}

// CPXsetnodecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetnodecallbackfunc (CPXENVptr env, int (CPXPUBLIC * nodecallback) (CALLBACK_NODE_ARGS), void *cbhandle)
{
    if (native_CPXsetnodecallbackfunc == NULL)
        native_CPXsetnodecallbackfunc = load_symbol("CPXsetnodecallbackfunc");
    return native_CPXsetnodecallbackfunc(env, nodecallback, cbhandle);
}

// CPXsetsolvecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetsolvecallbackfunc (CPXENVptr env, int (CPXPUBLIC * solvecallback) (CALLBACK_SOLVE_ARGS), void *cbhandle)
{
    if (native_CPXsetsolvecallbackfunc == NULL)
        native_CPXsetsolvecallbackfunc = load_symbol("CPXsetsolvecallbackfunc");
    return native_CPXsetsolvecallbackfunc(env, solvecallback, cbhandle);
}

// CPXsetusercutcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXsetusercutcallbackfunc (CPXENVptr env, int (CPXPUBLIC * cutcallback) (CALLBACK_CUT_ARGS), void *cbhandle)
{
    if (native_CPXsetusercutcallbackfunc == NULL)
        native_CPXsetusercutcallbackfunc = load_symbol("CPXsetusercutcallbackfunc");
    return native_CPXsetusercutcallbackfunc(env, cutcallback, cbhandle);
}

// CPXwritemipstarts
CPXLIBAPI int CPXPUBLIC CPXwritemipstarts (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, int begin, int end)
{
    if (native_CPXwritemipstarts == NULL)
        native_CPXwritemipstarts = load_symbol("CPXwritemipstarts");
    return native_CPXwritemipstarts(env, lp, filename_str, begin, end);
}

// CPXaddindconstr
CPXLIBAPI int CPXPUBLIC CPXaddindconstr (CPXCENVptr env, CPXLPptr lp, int indvar, int complemented, int nzcnt, double rhs, int sense, int const *linind, double const *linval, char const *indname_str)
{
    if (native_CPXaddindconstr == NULL)
        native_CPXaddindconstr = load_symbol("CPXaddindconstr");
    return native_CPXaddindconstr(env, lp, indvar, complemented, nzcnt, rhs, sense, linind, linval, indname_str);
}

// CPXNETaddarcs
CPXLIBAPI int CPXPUBLIC CPXNETaddarcs (CPXCENVptr env, CPXNETptr net, int narcs, int const *fromnode, int const *tonode, double const *low, double const *up, double const *obj, char **anames)
{
    if (native_CPXNETaddarcs == NULL)
        native_CPXNETaddarcs = load_symbol("CPXNETaddarcs");
    return native_CPXNETaddarcs(env, net, narcs, fromnode, tonode, low, up, obj, anames);
}

// CPXNETaddnodes
CPXLIBAPI int CPXPUBLIC CPXNETaddnodes (CPXCENVptr env, CPXNETptr net, int nnodes, double const *supply, char **name)
{
    if (native_CPXNETaddnodes == NULL)
        native_CPXNETaddnodes = load_symbol("CPXNETaddnodes");
    return native_CPXNETaddnodes(env, net, nnodes, supply, name);
}

// CPXNETbasewrite
CPXLIBAPI int CPXPUBLIC CPXNETbasewrite (CPXCENVptr env, CPXCNETptr net, char const *filename_str)
{
    if (native_CPXNETbasewrite == NULL)
        native_CPXNETbasewrite = load_symbol("CPXNETbasewrite");
    return native_CPXNETbasewrite(env, net, filename_str);
}

// CPXNETchgarcname
CPXLIBAPI int CPXPUBLIC CPXNETchgarcname (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, char **newname)
{
    if (native_CPXNETchgarcname == NULL)
        native_CPXNETchgarcname = load_symbol("CPXNETchgarcname");
    return native_CPXNETchgarcname(env, net, cnt, indices, newname);
}

// CPXNETchgarcnodes
CPXLIBAPI int CPXPUBLIC CPXNETchgarcnodes (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, int const *fromnode, int const *tonode)
{
    if (native_CPXNETchgarcnodes == NULL)
        native_CPXNETchgarcnodes = load_symbol("CPXNETchgarcnodes");
    return native_CPXNETchgarcnodes(env, net, cnt, indices, fromnode, tonode);
}

// CPXNETchgbds
CPXLIBAPI int CPXPUBLIC CPXNETchgbds (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, char const *lu, double const *bd)
{
    if (native_CPXNETchgbds == NULL)
        native_CPXNETchgbds = load_symbol("CPXNETchgbds");
    return native_CPXNETchgbds(env, net, cnt, indices, lu, bd);
}

// CPXNETchgname
CPXLIBAPI int CPXPUBLIC CPXNETchgname (CPXCENVptr env, CPXNETptr net, int key, int vindex, char const *name_str)
{
    if (native_CPXNETchgname == NULL)
        native_CPXNETchgname = load_symbol("CPXNETchgname");
    return native_CPXNETchgname(env, net, key, vindex, name_str);
}

// CPXNETchgnodename
CPXLIBAPI int CPXPUBLIC CPXNETchgnodename (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, char **newname)
{
    if (native_CPXNETchgnodename == NULL)
        native_CPXNETchgnodename = load_symbol("CPXNETchgnodename");
    return native_CPXNETchgnodename(env, net, cnt, indices, newname);
}

// CPXNETchgobj
CPXLIBAPI int CPXPUBLIC CPXNETchgobj (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, double const *obj)
{
    if (native_CPXNETchgobj == NULL)
        native_CPXNETchgobj = load_symbol("CPXNETchgobj");
    return native_CPXNETchgobj(env, net, cnt, indices, obj);
}

// CPXNETchgobjsen
CPXLIBAPI int CPXPUBLIC CPXNETchgobjsen (CPXCENVptr env, CPXNETptr net, int maxormin)
{
    if (native_CPXNETchgobjsen == NULL)
        native_CPXNETchgobjsen = load_symbol("CPXNETchgobjsen");
    return native_CPXNETchgobjsen(env, net, maxormin);
}

// CPXNETchgsupply
CPXLIBAPI int CPXPUBLIC CPXNETchgsupply (CPXCENVptr env, CPXNETptr net, int cnt, int const *indices, double const *supply)
{
    if (native_CPXNETchgsupply == NULL)
        native_CPXNETchgsupply = load_symbol("CPXNETchgsupply");
    return native_CPXNETchgsupply(env, net, cnt, indices, supply);
}

// CPXNETcopybase
CPXLIBAPI int CPXPUBLIC CPXNETcopybase (CPXCENVptr env, CPXNETptr net, int const *astat, int const *nstat)
{
    if (native_CPXNETcopybase == NULL)
        native_CPXNETcopybase = load_symbol("CPXNETcopybase");
    return native_CPXNETcopybase(env, net, astat, nstat);
}

// CPXNETcopynet
CPXLIBAPI int CPXPUBLIC CPXNETcopynet (CPXCENVptr env, CPXNETptr net, int objsen, int nnodes, double const *supply, char **nnames, int narcs, int const *fromnode, int const *tonode, double const *low, double const *up, double const *obj, char **anames)
{
    if (native_CPXNETcopynet == NULL)
        native_CPXNETcopynet = load_symbol("CPXNETcopynet");
    return native_CPXNETcopynet(env, net, objsen, nnodes, supply, nnames, narcs, fromnode, tonode, low, up, obj, anames);
}

// CPXNETcreateprob
CPXLIBAPI CPXNETptr CPXPUBLIC CPXNETcreateprob (CPXENVptr env, int *status_p, char const *name_str)
{
    if (native_CPXNETcreateprob == NULL)
        native_CPXNETcreateprob = load_symbol("CPXNETcreateprob");
    return native_CPXNETcreateprob(env, status_p, name_str);
}

// CPXNETdelarcs
CPXLIBAPI int CPXPUBLIC CPXNETdelarcs (CPXCENVptr env, CPXNETptr net, int begin, int end)
{
    if (native_CPXNETdelarcs == NULL)
        native_CPXNETdelarcs = load_symbol("CPXNETdelarcs");
    return native_CPXNETdelarcs(env, net, begin, end);
}

// CPXNETdelnodes
CPXLIBAPI int CPXPUBLIC CPXNETdelnodes (CPXCENVptr env, CPXNETptr net, int begin, int end)
{
    if (native_CPXNETdelnodes == NULL)
        native_CPXNETdelnodes = load_symbol("CPXNETdelnodes");
    return native_CPXNETdelnodes(env, net, begin, end);
}

// CPXNETdelset
CPXLIBAPI int CPXPUBLIC CPXNETdelset (CPXCENVptr env, CPXNETptr net, int *whichnodes, int *whicharcs)
{
    if (native_CPXNETdelset == NULL)
        native_CPXNETdelset = load_symbol("CPXNETdelset");
    return native_CPXNETdelset(env, net, whichnodes, whicharcs);
}

// CPXNETfreeprob
CPXLIBAPI int CPXPUBLIC CPXNETfreeprob (CPXENVptr env, CPXNETptr * net_p)
{
    if (native_CPXNETfreeprob == NULL)
        native_CPXNETfreeprob = load_symbol("CPXNETfreeprob");
    return native_CPXNETfreeprob(env, net_p);
}

// CPXNETgetarcindex
CPXLIBAPI int CPXPUBLIC CPXNETgetarcindex (CPXCENVptr env, CPXCNETptr net, char const *lname_str, int *index_p)
{
    if (native_CPXNETgetarcindex == NULL)
        native_CPXNETgetarcindex = load_symbol("CPXNETgetarcindex");
    return native_CPXNETgetarcindex(env, net, lname_str, index_p);
}

// CPXNETgetarcname
CPXLIBAPI int CPXPUBLIC CPXNETgetarcname (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, int namespc, int *surplus_p, int begin, int end)
{
    if (native_CPXNETgetarcname == NULL)
        native_CPXNETgetarcname = load_symbol("CPXNETgetarcname");
    return native_CPXNETgetarcname(env, net, nnames, namestore, namespc, surplus_p, begin, end);
}

// CPXNETgetarcnodes
CPXLIBAPI int CPXPUBLIC CPXNETgetarcnodes (CPXCENVptr env, CPXCNETptr net, int *fromnode, int *tonode, int begin, int end)
{
    if (native_CPXNETgetarcnodes == NULL)
        native_CPXNETgetarcnodes = load_symbol("CPXNETgetarcnodes");
    return native_CPXNETgetarcnodes(env, net, fromnode, tonode, begin, end);
}

// CPXNETgetbase
CPXLIBAPI int CPXPUBLIC CPXNETgetbase (CPXCENVptr env, CPXCNETptr net, int *astat, int *nstat)
{
    if (native_CPXNETgetbase == NULL)
        native_CPXNETgetbase = load_symbol("CPXNETgetbase");
    return native_CPXNETgetbase(env, net, astat, nstat);
}

// CPXNETgetdj
CPXLIBAPI int CPXPUBLIC CPXNETgetdj (CPXCENVptr env, CPXCNETptr net, double *dj, int begin, int end)
{
    if (native_CPXNETgetdj == NULL)
        native_CPXNETgetdj = load_symbol("CPXNETgetdj");
    return native_CPXNETgetdj(env, net, dj, begin, end);
}

// CPXNETgetitcnt
CPXLIBAPI int CPXPUBLIC CPXNETgetitcnt (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXNETgetitcnt == NULL)
        native_CPXNETgetitcnt = load_symbol("CPXNETgetitcnt");
    return native_CPXNETgetitcnt(env, net);
}

// CPXNETgetlb
CPXLIBAPI int CPXPUBLIC CPXNETgetlb (CPXCENVptr env, CPXCNETptr net, double *low, int begin, int end)
{
    if (native_CPXNETgetlb == NULL)
        native_CPXNETgetlb = load_symbol("CPXNETgetlb");
    return native_CPXNETgetlb(env, net, low, begin, end);
}

// CPXNETgetnodearcs
CPXLIBAPI int CPXPUBLIC CPXNETgetnodearcs (CPXCENVptr env, CPXCNETptr net, int *arccnt_p, int *arcbeg, int *arc, int arcspace, int *surplus_p, int begin, int end)
{
    if (native_CPXNETgetnodearcs == NULL)
        native_CPXNETgetnodearcs = load_symbol("CPXNETgetnodearcs");
    return native_CPXNETgetnodearcs(env, net, arccnt_p, arcbeg, arc, arcspace, surplus_p, begin, end);
}

// CPXNETgetnodeindex
CPXLIBAPI int CPXPUBLIC CPXNETgetnodeindex (CPXCENVptr env, CPXCNETptr net, char const *lname_str, int *index_p)
{
    if (native_CPXNETgetnodeindex == NULL)
        native_CPXNETgetnodeindex = load_symbol("CPXNETgetnodeindex");
    return native_CPXNETgetnodeindex(env, net, lname_str, index_p);
}

// CPXNETgetnodename
CPXLIBAPI int CPXPUBLIC CPXNETgetnodename (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, int namespc, int *surplus_p, int begin, int end)
{
    if (native_CPXNETgetnodename == NULL)
        native_CPXNETgetnodename = load_symbol("CPXNETgetnodename");
    return native_CPXNETgetnodename(env, net, nnames, namestore, namespc, surplus_p, begin, end);
}

// CPXNETgetnumarcs
CPXLIBAPI int CPXPUBLIC CPXNETgetnumarcs (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXNETgetnumarcs == NULL)
        native_CPXNETgetnumarcs = load_symbol("CPXNETgetnumarcs");
    return native_CPXNETgetnumarcs(env, net);
}

// CPXNETgetnumnodes
CPXLIBAPI int CPXPUBLIC CPXNETgetnumnodes (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXNETgetnumnodes == NULL)
        native_CPXNETgetnumnodes = load_symbol("CPXNETgetnumnodes");
    return native_CPXNETgetnumnodes(env, net);
}

// CPXNETgetobj
CPXLIBAPI int CPXPUBLIC CPXNETgetobj (CPXCENVptr env, CPXCNETptr net, double *obj, int begin, int end)
{
    if (native_CPXNETgetobj == NULL)
        native_CPXNETgetobj = load_symbol("CPXNETgetobj");
    return native_CPXNETgetobj(env, net, obj, begin, end);
}

// CPXNETgetobjsen
CPXLIBAPI int CPXPUBLIC CPXNETgetobjsen (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXNETgetobjsen == NULL)
        native_CPXNETgetobjsen = load_symbol("CPXNETgetobjsen");
    return native_CPXNETgetobjsen(env, net);
}

// CPXNETgetobjval
CPXLIBAPI int CPXPUBLIC CPXNETgetobjval (CPXCENVptr env, CPXCNETptr net, double *objval_p)
{
    if (native_CPXNETgetobjval == NULL)
        native_CPXNETgetobjval = load_symbol("CPXNETgetobjval");
    return native_CPXNETgetobjval(env, net, objval_p);
}

// CPXNETgetphase1cnt
CPXLIBAPI int CPXPUBLIC CPXNETgetphase1cnt (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXNETgetphase1cnt == NULL)
        native_CPXNETgetphase1cnt = load_symbol("CPXNETgetphase1cnt");
    return native_CPXNETgetphase1cnt(env, net);
}

// CPXNETgetpi
CPXLIBAPI int CPXPUBLIC CPXNETgetpi (CPXCENVptr env, CPXCNETptr net, double *pi, int begin, int end)
{
    if (native_CPXNETgetpi == NULL)
        native_CPXNETgetpi = load_symbol("CPXNETgetpi");
    return native_CPXNETgetpi(env, net, pi, begin, end);
}

// CPXNETgetprobname
CPXLIBAPI int CPXPUBLIC CPXNETgetprobname (CPXCENVptr env, CPXCNETptr net, char *buf_str, int bufspace, int *surplus_p)
{
    if (native_CPXNETgetprobname == NULL)
        native_CPXNETgetprobname = load_symbol("CPXNETgetprobname");
    return native_CPXNETgetprobname(env, net, buf_str, bufspace, surplus_p);
}

// CPXNETgetslack
CPXLIBAPI int CPXPUBLIC CPXNETgetslack (CPXCENVptr env, CPXCNETptr net, double *slack, int begin, int end)
{
    if (native_CPXNETgetslack == NULL)
        native_CPXNETgetslack = load_symbol("CPXNETgetslack");
    return native_CPXNETgetslack(env, net, slack, begin, end);
}

// CPXNETgetstat
CPXLIBAPI int CPXPUBLIC CPXNETgetstat (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXNETgetstat == NULL)
        native_CPXNETgetstat = load_symbol("CPXNETgetstat");
    return native_CPXNETgetstat(env, net);
}

// CPXNETgetsupply
CPXLIBAPI int CPXPUBLIC CPXNETgetsupply (CPXCENVptr env, CPXCNETptr net, double *supply, int begin, int end)
{
    if (native_CPXNETgetsupply == NULL)
        native_CPXNETgetsupply = load_symbol("CPXNETgetsupply");
    return native_CPXNETgetsupply(env, net, supply, begin, end);
}

// CPXNETgetub
CPXLIBAPI int CPXPUBLIC CPXNETgetub (CPXCENVptr env, CPXCNETptr net, double *up, int begin, int end)
{
    if (native_CPXNETgetub == NULL)
        native_CPXNETgetub = load_symbol("CPXNETgetub");
    return native_CPXNETgetub(env, net, up, begin, end);
}

// CPXNETgetx
CPXLIBAPI int CPXPUBLIC CPXNETgetx (CPXCENVptr env, CPXCNETptr net, double *x, int begin, int end)
{
    if (native_CPXNETgetx == NULL)
        native_CPXNETgetx = load_symbol("CPXNETgetx");
    return native_CPXNETgetx(env, net, x, begin, end);
}

// CPXNETprimopt
CPXLIBAPI int CPXPUBLIC CPXNETprimopt (CPXCENVptr env, CPXNETptr net)
{
    if (native_CPXNETprimopt == NULL)
        native_CPXNETprimopt = load_symbol("CPXNETprimopt");
    return native_CPXNETprimopt(env, net);
}

// CPXNETreadcopybase
CPXLIBAPI int CPXPUBLIC CPXNETreadcopybase (CPXCENVptr env, CPXNETptr net, char const *filename_str)
{
    if (native_CPXNETreadcopybase == NULL)
        native_CPXNETreadcopybase = load_symbol("CPXNETreadcopybase");
    return native_CPXNETreadcopybase(env, net, filename_str);
}

// CPXNETreadcopyprob
CPXLIBAPI int CPXPUBLIC CPXNETreadcopyprob (CPXCENVptr env, CPXNETptr net, char const *filename_str)
{
    if (native_CPXNETreadcopyprob == NULL)
        native_CPXNETreadcopyprob = load_symbol("CPXNETreadcopyprob");
    return native_CPXNETreadcopyprob(env, net, filename_str);
}

// CPXNETsolninfo
CPXLIBAPI int CPXPUBLIC CPXNETsolninfo (CPXCENVptr env, CPXCNETptr net, int *pfeasind_p, int *dfeasind_p)
{
    if (native_CPXNETsolninfo == NULL)
        native_CPXNETsolninfo = load_symbol("CPXNETsolninfo");
    return native_CPXNETsolninfo(env, net, pfeasind_p, dfeasind_p);
}

// CPXNETsolution
CPXLIBAPI int CPXPUBLIC CPXNETsolution (CPXCENVptr env, CPXCNETptr net, int *netstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj)
{
    if (native_CPXNETsolution == NULL)
        native_CPXNETsolution = load_symbol("CPXNETsolution");
    return native_CPXNETsolution(env, net, netstat_p, objval_p, x, pi, slack, dj);
}

// CPXNETwriteprob
CPXLIBAPI int CPXPUBLIC CPXNETwriteprob (CPXCENVptr env, CPXCNETptr net, char const *filename_str, char const *format_str)
{
    if (native_CPXNETwriteprob == NULL)
        native_CPXNETwriteprob = load_symbol("CPXNETwriteprob");
    return native_CPXNETwriteprob(env, net, filename_str, format_str);
}

// CPXchgqpcoef
CPXLIBAPI int CPXPUBLIC CPXchgqpcoef (CPXCENVptr env, CPXLPptr lp, int i, int j, double newvalue)
{
    if (native_CPXchgqpcoef == NULL)
        native_CPXchgqpcoef = load_symbol("CPXchgqpcoef");
    return native_CPXchgqpcoef(env, lp, i, j, newvalue);
}

// CPXcopyqpsep
CPXLIBAPI int CPXPUBLIC CPXcopyqpsep (CPXCENVptr env, CPXLPptr lp, double const *qsepvec)
{
    if (native_CPXcopyqpsep == NULL)
        native_CPXcopyqpsep = load_symbol("CPXcopyqpsep");
    return native_CPXcopyqpsep(env, lp, qsepvec);
}

// CPXcopyquad
CPXLIBAPI int CPXPUBLIC CPXcopyquad (CPXCENVptr env, CPXLPptr lp, int const *qmatbeg, int const *qmatcnt, int const *qmatind, double const *qmatval)
{
    if (native_CPXcopyquad == NULL)
        native_CPXcopyquad = load_symbol("CPXcopyquad");
    return native_CPXcopyquad(env, lp, qmatbeg, qmatcnt, qmatind, qmatval);
}

// CPXgetnumqpnz
CPXLIBAPI int CPXPUBLIC CPXgetnumqpnz (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumqpnz == NULL)
        native_CPXgetnumqpnz = load_symbol("CPXgetnumqpnz");
    return native_CPXgetnumqpnz(env, lp);
}

// CPXgetnumquad
CPXLIBAPI int CPXPUBLIC CPXgetnumquad (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumquad == NULL)
        native_CPXgetnumquad = load_symbol("CPXgetnumquad");
    return native_CPXgetnumquad(env, lp);
}

// CPXgetqpcoef
CPXLIBAPI int CPXPUBLIC CPXgetqpcoef (CPXCENVptr env, CPXCLPptr lp, int rownum, int colnum, double *coef_p)
{
    if (native_CPXgetqpcoef == NULL)
        native_CPXgetqpcoef = load_symbol("CPXgetqpcoef");
    return native_CPXgetqpcoef(env, lp, rownum, colnum, coef_p);
}

// CPXgetquad
CPXLIBAPI int CPXPUBLIC CPXgetquad (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p, int *qmatbeg, int *qmatind, double *qmatval, int qmatspace, int *surplus_p, int begin, int end)
{
    if (native_CPXgetquad == NULL)
        native_CPXgetquad = load_symbol("CPXgetquad");
    return native_CPXgetquad(env, lp, nzcnt_p, qmatbeg, qmatind, qmatval, qmatspace, surplus_p, begin, end);
}

// CPXqpindefcertificate
CPXLIBAPI int CPXPUBLIC CPXqpindefcertificate (CPXCENVptr env, CPXCLPptr lp, double *x)
{
    if (native_CPXqpindefcertificate == NULL)
        native_CPXqpindefcertificate = load_symbol("CPXqpindefcertificate");
    return native_CPXqpindefcertificate(env, lp, x);
}

// CPXqpopt
CPXLIBAPI int CPXPUBLIC CPXqpopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXqpopt == NULL)
        native_CPXqpopt = load_symbol("CPXqpopt");
    return native_CPXqpopt(env, lp);
}

// CPXaddqconstr
CPXLIBAPI int CPXPUBLIC CPXaddqconstr (CPXCENVptr env, CPXLPptr lp, int linnzcnt, int quadnzcnt, double rhs, int sense, int const *linind, double const *linval, int const *quadrow, int const *quadcol, double const *quadval, char const *lname_str)
{
    if (native_CPXaddqconstr == NULL)
        native_CPXaddqconstr = load_symbol("CPXaddqconstr");
    return native_CPXaddqconstr(env, lp, linnzcnt, quadnzcnt, rhs, sense, linind, linval, quadrow, quadcol, quadval, lname_str);
}

// CPXdelqconstrs
CPXLIBAPI int CPXPUBLIC CPXdelqconstrs (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXdelqconstrs == NULL)
        native_CPXdelqconstrs = load_symbol("CPXdelqconstrs");
    return native_CPXdelqconstrs(env, lp, begin, end);
}

// CPXgetnumqconstrs
CPXLIBAPI int CPXPUBLIC CPXgetnumqconstrs (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXgetnumqconstrs == NULL)
        native_CPXgetnumqconstrs = load_symbol("CPXgetnumqconstrs");
    return native_CPXgetnumqconstrs(env, lp);
}

// CPXgetqconstr
CPXLIBAPI int CPXPUBLIC CPXgetqconstr (CPXCENVptr env, CPXCLPptr lp, int *linnzcnt_p, int *quadnzcnt_p, double *rhs_p, char *sense_p, int *linind, double *linval, int linspace, int *linsurplus_p, int *quadrow, int *quadcol, double *quadval, int quadspace, int *quadsurplus_p, int which)
{
    if (native_CPXgetqconstr == NULL)
        native_CPXgetqconstr = load_symbol("CPXgetqconstr");
    return native_CPXgetqconstr(env, lp, linnzcnt_p, quadnzcnt_p, rhs_p, sense_p, linind, linval, linspace, linsurplus_p, quadrow, quadcol, quadval, quadspace, quadsurplus_p, which);
}

// CPXgetqconstrdslack
CPXLIBAPI int CPXPUBLIC CPXgetqconstrdslack (CPXCENVptr env, CPXCLPptr lp, int qind, int *nz_p, int *ind, double *val, int space, int *surplus_p)
{
    if (native_CPXgetqconstrdslack == NULL)
        native_CPXgetqconstrdslack = load_symbol("CPXgetqconstrdslack");
    return native_CPXgetqconstrdslack(env, lp, qind, nz_p, ind, val, space, surplus_p);
}

// CPXgetqconstrindex
CPXLIBAPI int CPXPUBLIC CPXgetqconstrindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXgetqconstrindex == NULL)
        native_CPXgetqconstrindex = load_symbol("CPXgetqconstrindex");
    return native_CPXgetqconstrindex(env, lp, lname_str, index_p);
}

// CPXgetqconstrinfeas
CPXLIBAPI int CPXPUBLIC CPXgetqconstrinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, int begin, int end)
{
    if (native_CPXgetqconstrinfeas == NULL)
        native_CPXgetqconstrinfeas = load_symbol("CPXgetqconstrinfeas");
    return native_CPXgetqconstrinfeas(env, lp, x, infeasout, begin, end);
}

// CPXgetqconstrname
CPXLIBAPI int CPXPUBLIC CPXgetqconstrname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, int bufspace, int *surplus_p, int which)
{
    if (native_CPXgetqconstrname == NULL)
        native_CPXgetqconstrname = load_symbol("CPXgetqconstrname");
    return native_CPXgetqconstrname(env, lp, buf_str, bufspace, surplus_p, which);
}

// CPXgetqconstrslack
CPXLIBAPI int CPXPUBLIC CPXgetqconstrslack (CPXCENVptr env, CPXCLPptr lp, double *qcslack, int begin, int end)
{
    if (native_CPXgetqconstrslack == NULL)
        native_CPXgetqconstrslack = load_symbol("CPXgetqconstrslack");
    return native_CPXgetqconstrslack(env, lp, qcslack, begin, end);
}

// CPXgetxqxax
CPXLIBAPI int CPXPUBLIC CPXgetxqxax (CPXCENVptr env, CPXCLPptr lp, double *xqxax, int begin, int end)
{
    if (native_CPXgetxqxax == NULL)
        native_CPXgetxqxax = load_symbol("CPXgetxqxax");
    return native_CPXgetxqxax(env, lp, xqxax, begin, end);
}

// CPXqconstrslackfromx
CPXLIBAPI int CPXPUBLIC CPXqconstrslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *qcslack)
{
    if (native_CPXqconstrslackfromx == NULL)
        native_CPXqconstrslackfromx = load_symbol("CPXqconstrslackfromx");
    return native_CPXqconstrslackfromx(env, lp, x, qcslack);
}

// CPXLaddchannel
CPXLIBAPI CPXCHANNELptr CPXPUBLIC CPXLaddchannel (CPXENVptr env)
{
    if (native_CPXLaddchannel == NULL)
        native_CPXLaddchannel = load_symbol("CPXLaddchannel");
    return native_CPXLaddchannel(env);
}

// CPXLaddcols
CPXLIBAPI int CPXPUBLIC CPXLaddcols (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, CPXLONG nzcnt, double const *obj, CPXLONG const *cmatbeg, CPXINT const *cmatind, double const *cmatval, double const *lb, double const *ub, char const *const *colname)
{
    if (native_CPXLaddcols == NULL)
        native_CPXLaddcols = load_symbol("CPXLaddcols");
    return native_CPXLaddcols(env, lp, ccnt, nzcnt, obj, cmatbeg, cmatind, cmatval, lb, ub, colname);
}

// CPXLaddfpdest
CPXLIBAPI int CPXPUBLIC CPXLaddfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr)
{
    if (native_CPXLaddfpdest == NULL)
        native_CPXLaddfpdest = load_symbol("CPXLaddfpdest");
    return native_CPXLaddfpdest(env, channel, fileptr);
}

// CPXLaddfuncdest
CPXLIBAPI int CPXPUBLIC CPXLaddfuncdest (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, char const *))
{
    if (native_CPXLaddfuncdest == NULL)
        native_CPXLaddfuncdest = load_symbol("CPXLaddfuncdest");
    return native_CPXLaddfuncdest(env, channel, handle, msgfunction);
}

// CPXLaddrows
CPXLIBAPI int CPXPUBLIC CPXLaddrows (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *colname, char const *const *rowname)
{
    if (native_CPXLaddrows == NULL)
        native_CPXLaddrows = load_symbol("CPXLaddrows");
    return native_CPXLaddrows(env, lp, ccnt, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, colname, rowname);
}

// CPXLbasicpresolve
CPXLIBAPI int CPXPUBLIC CPXLbasicpresolve (CPXCENVptr env, CPXLPptr lp, double *redlb, double *redub, int *rstat)
{
    if (native_CPXLbasicpresolve == NULL)
        native_CPXLbasicpresolve = load_symbol("CPXLbasicpresolve");
    return native_CPXLbasicpresolve(env, lp, redlb, redub, rstat);
}

// CPXLbinvacol
CPXLIBAPI int CPXPUBLIC CPXLbinvacol (CPXCENVptr env, CPXCLPptr lp, CPXINT j, double *x)
{
    if (native_CPXLbinvacol == NULL)
        native_CPXLbinvacol = load_symbol("CPXLbinvacol");
    return native_CPXLbinvacol(env, lp, j, x);
}

// CPXLbinvarow
CPXLIBAPI int CPXPUBLIC CPXLbinvarow (CPXCENVptr env, CPXCLPptr lp, CPXINT i, double *z)
{
    if (native_CPXLbinvarow == NULL)
        native_CPXLbinvarow = load_symbol("CPXLbinvarow");
    return native_CPXLbinvarow(env, lp, i, z);
}

// CPXLbinvcol
CPXLIBAPI int CPXPUBLIC CPXLbinvcol (CPXCENVptr env, CPXCLPptr lp, CPXINT j, double *x)
{
    if (native_CPXLbinvcol == NULL)
        native_CPXLbinvcol = load_symbol("CPXLbinvcol");
    return native_CPXLbinvcol(env, lp, j, x);
}

// CPXLbinvrow
CPXLIBAPI int CPXPUBLIC CPXLbinvrow (CPXCENVptr env, CPXCLPptr lp, CPXINT i, double *y)
{
    if (native_CPXLbinvrow == NULL)
        native_CPXLbinvrow = load_symbol("CPXLbinvrow");
    return native_CPXLbinvrow(env, lp, i, y);
}

// CPXLboundsa
CPXLIBAPI int CPXPUBLIC CPXLboundsa (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end, double *lblower, double *lbupper, double *ublower, double *ubupper)
{
    if (native_CPXLboundsa == NULL)
        native_CPXLboundsa = load_symbol("CPXLboundsa");
    return native_CPXLboundsa(env, lp, begin, end, lblower, lbupper, ublower, ubupper);
}

// CPXLbtran
CPXLIBAPI int CPXPUBLIC CPXLbtran (CPXCENVptr env, CPXCLPptr lp, double *y)
{
    if (native_CPXLbtran == NULL)
        native_CPXLbtran = load_symbol("CPXLbtran");
    return native_CPXLbtran(env, lp, y);
}

// CPXLcheckdfeas
CPXLIBAPI int CPXPUBLIC CPXLcheckdfeas (CPXCENVptr env, CPXLPptr lp, CPXINT * infeas_p)
{
    if (native_CPXLcheckdfeas == NULL)
        native_CPXLcheckdfeas = load_symbol("CPXLcheckdfeas");
    return native_CPXLcheckdfeas(env, lp, infeas_p);
}

// CPXLcheckpfeas
CPXLIBAPI int CPXPUBLIC CPXLcheckpfeas (CPXCENVptr env, CPXLPptr lp, CPXINT * infeas_p)
{
    if (native_CPXLcheckpfeas == NULL)
        native_CPXLcheckpfeas = load_symbol("CPXLcheckpfeas");
    return native_CPXLcheckpfeas(env, lp, infeas_p);
}

// CPXLchecksoln
CPXLIBAPI int CPXPUBLIC CPXLchecksoln (CPXCENVptr env, CPXLPptr lp, int *lpstatus_p)
{
    if (native_CPXLchecksoln == NULL)
        native_CPXLchecksoln = load_symbol("CPXLchecksoln");
    return native_CPXLchecksoln(env, lp, lpstatus_p);
}

// CPXLchgbds
CPXLIBAPI int CPXPUBLIC CPXLchgbds (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd)
{
    if (native_CPXLchgbds == NULL)
        native_CPXLchgbds = load_symbol("CPXLchgbds");
    return native_CPXLchgbds(env, lp, cnt, indices, lu, bd);
}

// CPXLchgcoef
CPXLIBAPI int CPXPUBLIC CPXLchgcoef (CPXCENVptr env, CPXLPptr lp, CPXINT i, CPXINT j, double newvalue)
{
    if (native_CPXLchgcoef == NULL)
        native_CPXLchgcoef = load_symbol("CPXLchgcoef");
    return native_CPXLchgcoef(env, lp, i, j, newvalue);
}

// CPXLchgcoeflist
CPXLIBAPI int CPXPUBLIC CPXLchgcoeflist (CPXCENVptr env, CPXLPptr lp, CPXLONG numcoefs, CPXINT const *rowlist, CPXINT const *collist, double const *vallist)
{
    if (native_CPXLchgcoeflist == NULL)
        native_CPXLchgcoeflist = load_symbol("CPXLchgcoeflist");
    return native_CPXLchgcoeflist(env, lp, numcoefs, rowlist, collist, vallist);
}

// CPXLchgcolname
CPXLIBAPI int CPXPUBLIC CPXLchgcolname (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *const *newname)
{
    if (native_CPXLchgcolname == NULL)
        native_CPXLchgcolname = load_symbol("CPXLchgcolname");
    return native_CPXLchgcolname(env, lp, cnt, indices, newname);
}

// CPXLchgname
CPXLIBAPI int CPXPUBLIC CPXLchgname (CPXCENVptr env, CPXLPptr lp, int key, CPXINT ij, char const *newname_str)
{
    if (native_CPXLchgname == NULL)
        native_CPXLchgname = load_symbol("CPXLchgname");
    return native_CPXLchgname(env, lp, key, ij, newname_str);
}

// CPXLchgobj
CPXLIBAPI int CPXPUBLIC CPXLchgobj (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values)
{
    if (native_CPXLchgobj == NULL)
        native_CPXLchgobj = load_symbol("CPXLchgobj");
    return native_CPXLchgobj(env, lp, cnt, indices, values);
}

// CPXLchgobjoffset
CPXLIBAPI int CPXPUBLIC CPXLchgobjoffset (CPXCENVptr env, CPXLPptr lp, double offset)
{
    if (native_CPXLchgobjoffset == NULL)
        native_CPXLchgobjoffset = load_symbol("CPXLchgobjoffset");
    return native_CPXLchgobjoffset(env, lp, offset);
}

// CPXLchgobjsen
CPXLIBAPI int CPXPUBLIC CPXLchgobjsen (CPXCENVptr env, CPXLPptr lp, int maxormin)
{
    if (native_CPXLchgobjsen == NULL)
        native_CPXLchgobjsen = load_symbol("CPXLchgobjsen");
    return native_CPXLchgobjsen(env, lp, maxormin);
}

// CPXLchgprobname
CPXLIBAPI int CPXPUBLIC CPXLchgprobname (CPXCENVptr env, CPXLPptr lp, char const *probname)
{
    if (native_CPXLchgprobname == NULL)
        native_CPXLchgprobname = load_symbol("CPXLchgprobname");
    return native_CPXLchgprobname(env, lp, probname);
}

// CPXLchgprobtype
CPXLIBAPI int CPXPUBLIC CPXLchgprobtype (CPXCENVptr env, CPXLPptr lp, int type)
{
    if (native_CPXLchgprobtype == NULL)
        native_CPXLchgprobtype = load_symbol("CPXLchgprobtype");
    return native_CPXLchgprobtype(env, lp, type);
}

// CPXLchgprobtypesolnpool
CPXLIBAPI int CPXPUBLIC CPXLchgprobtypesolnpool (CPXCENVptr env, CPXLPptr lp, int type, int soln)
{
    if (native_CPXLchgprobtypesolnpool == NULL)
        native_CPXLchgprobtypesolnpool = load_symbol("CPXLchgprobtypesolnpool");
    return native_CPXLchgprobtypesolnpool(env, lp, type, soln);
}

// CPXLchgrhs
CPXLIBAPI int CPXPUBLIC CPXLchgrhs (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values)
{
    if (native_CPXLchgrhs == NULL)
        native_CPXLchgrhs = load_symbol("CPXLchgrhs");
    return native_CPXLchgrhs(env, lp, cnt, indices, values);
}

// CPXLchgrngval
CPXLIBAPI int CPXPUBLIC CPXLchgrngval (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values)
{
    if (native_CPXLchgrngval == NULL)
        native_CPXLchgrngval = load_symbol("CPXLchgrngval");
    return native_CPXLchgrngval(env, lp, cnt, indices, values);
}

// CPXLchgrowname
CPXLIBAPI int CPXPUBLIC CPXLchgrowname (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *const *newname)
{
    if (native_CPXLchgrowname == NULL)
        native_CPXLchgrowname = load_symbol("CPXLchgrowname");
    return native_CPXLchgrowname(env, lp, cnt, indices, newname);
}

// CPXLchgsense
CPXLIBAPI int CPXPUBLIC CPXLchgsense (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *sense)
{
    if (native_CPXLchgsense == NULL)
        native_CPXLchgsense = load_symbol("CPXLchgsense");
    return native_CPXLchgsense(env, lp, cnt, indices, sense);
}

// CPXLcleanup
CPXLIBAPI int CPXPUBLIC CPXLcleanup (CPXCENVptr env, CPXLPptr lp, double eps)
{
    if (native_CPXLcleanup == NULL)
        native_CPXLcleanup = load_symbol("CPXLcleanup");
    return native_CPXLcleanup(env, lp, eps);
}

// CPXLcloneprob
CPXLIBAPI CPXLPptr CPXPUBLIC CPXLcloneprob (CPXCENVptr env, CPXCLPptr lp, int *status_p)
{
    if (native_CPXLcloneprob == NULL)
        native_CPXLcloneprob = load_symbol("CPXLcloneprob");
    return native_CPXLcloneprob(env, lp, status_p);
}

// CPXLcloseCPLEX
CPXLIBAPI int CPXPUBLIC CPXLcloseCPLEX (CPXENVptr * env_p)
{
    if (native_CPXLcloseCPLEX == NULL)
        native_CPXLcloseCPLEX = load_symbol("CPXLcloseCPLEX");
    return native_CPXLcloseCPLEX(env_p);
}

// CPXLclpwrite
CPXLIBAPI int CPXPUBLIC CPXLclpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLclpwrite == NULL)
        native_CPXLclpwrite = load_symbol("CPXLclpwrite");
    return native_CPXLclpwrite(env, lp, filename_str);
}

// CPXLcompletelp
CPXLIBAPI int CPXPUBLIC CPXLcompletelp (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLcompletelp == NULL)
        native_CPXLcompletelp = load_symbol("CPXLcompletelp");
    return native_CPXLcompletelp(env, lp);
}

// CPXLcopybase
CPXLIBAPI int CPXPUBLIC CPXLcopybase (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat)
{
    if (native_CPXLcopybase == NULL)
        native_CPXLcopybase = load_symbol("CPXLcopybase");
    return native_CPXLcopybase(env, lp, cstat, rstat);
}

// CPXLcopybasednorms
CPXLIBAPI int CPXPUBLIC CPXLcopybasednorms (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *dnorm)
{
    if (native_CPXLcopybasednorms == NULL)
        native_CPXLcopybasednorms = load_symbol("CPXLcopybasednorms");
    return native_CPXLcopybasednorms(env, lp, cstat, rstat, dnorm);
}

// CPXLcopydnorms
CPXLIBAPI int CPXPUBLIC CPXLcopydnorms (CPXCENVptr env, CPXLPptr lp, double const *norm, CPXINT const *head, CPXINT len)
{
    if (native_CPXLcopydnorms == NULL)
        native_CPXLcopydnorms = load_symbol("CPXLcopydnorms");
    return native_CPXLcopydnorms(env, lp, norm, head, len);
}

// CPXLcopylp
CPXLIBAPI int CPXPUBLIC CPXLcopylp (CPXCENVptr env, CPXLPptr lp, CPXINT numcols, CPXINT numrows, int objsense, double const *objective, double const *rhs, char const *sense, CPXLONG const *matbeg, CPXINT const *matcnt, CPXINT const *matind, double const *matval, double const *lb, double const *ub, double const *rngval)
{
    if (native_CPXLcopylp == NULL)
        native_CPXLcopylp = load_symbol("CPXLcopylp");
    return native_CPXLcopylp(env, lp, numcols, numrows, objsense, objective, rhs, sense, matbeg, matcnt, matind, matval, lb, ub, rngval);
}

// CPXLcopylpwnames
CPXLIBAPI int CPXPUBLIC CPXLcopylpwnames (CPXCENVptr env, CPXLPptr lp, CPXINT numcols, CPXINT numrows, int objsense, double const *objective, double const *rhs, char const *sense, CPXLONG const *matbeg, CPXINT const *matcnt, CPXINT const *matind, double const *matval, double const *lb, double const *ub, double const *rngval, char const *const *colname, char const *const *rowname)
{
    if (native_CPXLcopylpwnames == NULL)
        native_CPXLcopylpwnames = load_symbol("CPXLcopylpwnames");
    return native_CPXLcopylpwnames(env, lp, numcols, numrows, objsense, objective, rhs, sense, matbeg, matcnt, matind, matval, lb, ub, rngval, colname, rowname);
}

// CPXLcopynettolp
CPXLIBAPI int CPXPUBLIC CPXLcopynettolp (CPXCENVptr env, CPXLPptr lp, CPXCNETptr net)
{
    if (native_CPXLcopynettolp == NULL)
        native_CPXLcopynettolp = load_symbol("CPXLcopynettolp");
    return native_CPXLcopynettolp(env, lp, net);
}

// CPXLcopyobjname
CPXLIBAPI int CPXPUBLIC CPXLcopyobjname (CPXCENVptr env, CPXLPptr lp, char const *objname_str)
{
    if (native_CPXLcopyobjname == NULL)
        native_CPXLcopyobjname = load_symbol("CPXLcopyobjname");
    return native_CPXLcopyobjname(env, lp, objname_str);
}

// CPXLcopypartialbase
CPXLIBAPI int CPXPUBLIC CPXLcopypartialbase (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, CPXINT const *cindices, int const *cstat, CPXINT rcnt, CPXINT const *rindices, int const *rstat)
{
    if (native_CPXLcopypartialbase == NULL)
        native_CPXLcopypartialbase = load_symbol("CPXLcopypartialbase");
    return native_CPXLcopypartialbase(env, lp, ccnt, cindices, cstat, rcnt, rindices, rstat);
}

// CPXLcopypnorms
CPXLIBAPI int CPXPUBLIC CPXLcopypnorms (CPXCENVptr env, CPXLPptr lp, double const *cnorm, double const *rnorm, CPXINT len)
{
    if (native_CPXLcopypnorms == NULL)
        native_CPXLcopypnorms = load_symbol("CPXLcopypnorms");
    return native_CPXLcopypnorms(env, lp, cnorm, rnorm, len);
}

// CPXLcopyprotected
CPXLIBAPI int CPXPUBLIC CPXLcopyprotected (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices)
{
    if (native_CPXLcopyprotected == NULL)
        native_CPXLcopyprotected = load_symbol("CPXLcopyprotected");
    return native_CPXLcopyprotected(env, lp, cnt, indices);
}

// CPXLcopystart
CPXLIBAPI int CPXPUBLIC CPXLcopystart (CPXCENVptr env, CPXLPptr lp, int const *cstat, int const *rstat, double const *cprim, double const *rprim, double const *cdual, double const *rdual)
{
    if (native_CPXLcopystart == NULL)
        native_CPXLcopystart = load_symbol("CPXLcopystart");
    return native_CPXLcopystart(env, lp, cstat, rstat, cprim, rprim, cdual, rdual);
}

// CPXLcreateprob
CPXLIBAPI CPXLPptr CPXPUBLIC CPXLcreateprob (CPXCENVptr env, int *status_p, char const *probname_str)
{
    if (native_CPXLcreateprob == NULL)
        native_CPXLcreateprob = load_symbol("CPXLcreateprob");
    return native_CPXLcreateprob(env, status_p, probname_str);
}

// CPXLcrushform
CPXLIBAPI int CPXPUBLIC CPXLcrushform (CPXCENVptr env, CPXCLPptr lp, CPXINT len, CPXINT const *ind, double const *val, CPXINT * plen_p, double *poffset_p, CPXINT * pind, double *pval)
{
    if (native_CPXLcrushform == NULL)
        native_CPXLcrushform = load_symbol("CPXLcrushform");
    return native_CPXLcrushform(env, lp, len, ind, val, plen_p, poffset_p, pind, pval);
}

// CPXLcrushpi
CPXLIBAPI int CPXPUBLIC CPXLcrushpi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *prepi)
{
    if (native_CPXLcrushpi == NULL)
        native_CPXLcrushpi = load_symbol("CPXLcrushpi");
    return native_CPXLcrushpi(env, lp, pi, prepi);
}

// CPXLcrushx
CPXLIBAPI int CPXPUBLIC CPXLcrushx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *prex)
{
    if (native_CPXLcrushx == NULL)
        native_CPXLcrushx = load_symbol("CPXLcrushx");
    return native_CPXLcrushx(env, lp, x, prex);
}

// CPXLdelchannel
CPXLIBAPI int CPXPUBLIC CPXLdelchannel (CPXENVptr env, CPXCHANNELptr * channel_p)
{
    if (native_CPXLdelchannel == NULL)
        native_CPXLdelchannel = load_symbol("CPXLdelchannel");
    return native_CPXLdelchannel(env, channel_p);
}

// CPXLdelcols
CPXLIBAPI int CPXPUBLIC CPXLdelcols (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end)
{
    if (native_CPXLdelcols == NULL)
        native_CPXLdelcols = load_symbol("CPXLdelcols");
    return native_CPXLdelcols(env, lp, begin, end);
}

// CPXLdelfpdest
CPXLIBAPI int CPXPUBLIC CPXLdelfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr)
{
    if (native_CPXLdelfpdest == NULL)
        native_CPXLdelfpdest = load_symbol("CPXLdelfpdest");
    return native_CPXLdelfpdest(env, channel, fileptr);
}

// CPXLdelfuncdest
CPXLIBAPI int CPXPUBLIC CPXLdelfuncdest (CPXCENVptr env, CPXCHANNELptr channel, void *handle, void (CPXPUBLIC * msgfunction) (void *, char const *))
{
    if (native_CPXLdelfuncdest == NULL)
        native_CPXLdelfuncdest = load_symbol("CPXLdelfuncdest");
    return native_CPXLdelfuncdest(env, channel, handle, msgfunction);
}

// CPXLdelnames
CPXLIBAPI int CPXPUBLIC CPXLdelnames (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLdelnames == NULL)
        native_CPXLdelnames = load_symbol("CPXLdelnames");
    return native_CPXLdelnames(env, lp);
}

// CPXLdelrows
CPXLIBAPI int CPXPUBLIC CPXLdelrows (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end)
{
    if (native_CPXLdelrows == NULL)
        native_CPXLdelrows = load_symbol("CPXLdelrows");
    return native_CPXLdelrows(env, lp, begin, end);
}

// CPXLdelsetcols
CPXLIBAPI int CPXPUBLIC CPXLdelsetcols (CPXCENVptr env, CPXLPptr lp, CPXINT * delstat)
{
    if (native_CPXLdelsetcols == NULL)
        native_CPXLdelsetcols = load_symbol("CPXLdelsetcols");
    return native_CPXLdelsetcols(env, lp, delstat);
}

// CPXLdelsetrows
CPXLIBAPI int CPXPUBLIC CPXLdelsetrows (CPXCENVptr env, CPXLPptr lp, CPXINT * delstat)
{
    if (native_CPXLdelsetrows == NULL)
        native_CPXLdelsetrows = load_symbol("CPXLdelsetrows");
    return native_CPXLdelsetrows(env, lp, delstat);
}

// CPXLdeserializercreate
CPXLIBAPI int CPXPUBLIC CPXLdeserializercreate (CPXDESERIALIZERptr * deser_p, CPXLONG size, void const *buffer)
{
    if (native_CPXLdeserializercreate == NULL)
        native_CPXLdeserializercreate = load_symbol("CPXLdeserializercreate");
    return native_CPXLdeserializercreate(deser_p, size, buffer);
}

// CPXLdeserializerdestroy
CPXLIBAPI void CPXPUBLIC CPXLdeserializerdestroy (CPXDESERIALIZERptr deser)
{
    if (native_CPXLdeserializerdestroy == NULL)
        native_CPXLdeserializerdestroy = load_symbol("CPXLdeserializerdestroy");
    return native_CPXLdeserializerdestroy(deser);
}

// CPXLdeserializerleft
CPXLIBAPI CPXLONG CPXPUBLIC CPXLdeserializerleft (CPXCDESERIALIZERptr deser)
{
    if (native_CPXLdeserializerleft == NULL)
        native_CPXLdeserializerleft = load_symbol("CPXLdeserializerleft");
    return native_CPXLdeserializerleft(deser);
}

// CPXLdisconnectchannel
CPXLIBAPI int CPXPUBLIC CPXLdisconnectchannel (CPXCENVptr env, CPXCHANNELptr channel)
{
    if (native_CPXLdisconnectchannel == NULL)
        native_CPXLdisconnectchannel = load_symbol("CPXLdisconnectchannel");
    return native_CPXLdisconnectchannel(env, channel);
}

// CPXLdjfrompi
CPXLIBAPI int CPXPUBLIC CPXLdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *dj)
{
    if (native_CPXLdjfrompi == NULL)
        native_CPXLdjfrompi = load_symbol("CPXLdjfrompi");
    return native_CPXLdjfrompi(env, lp, pi, dj);
}

// CPXLdperwrite
CPXLIBAPI int CPXPUBLIC CPXLdperwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon)
{
    if (native_CPXLdperwrite == NULL)
        native_CPXLdperwrite = load_symbol("CPXLdperwrite");
    return native_CPXLdperwrite(env, lp, filename_str, epsilon);
}

// CPXLdratio
CPXLIBAPI int CPXPUBLIC CPXLdratio (CPXCENVptr env, CPXLPptr lp, CPXINT * indices, CPXINT cnt, double *downratio, double *upratio, CPXINT * downenter, CPXINT * upenter, int *downstatus, int *upstatus)
{
    if (native_CPXLdratio == NULL)
        native_CPXLdratio = load_symbol("CPXLdratio");
    return native_CPXLdratio(env, lp, indices, cnt, downratio, upratio, downenter, upenter, downstatus, upstatus);
}

// CPXLdualfarkas
CPXLIBAPI int CPXPUBLIC CPXLdualfarkas (CPXCENVptr env, CPXCLPptr lp, double *y, double *proof_p)
{
    if (native_CPXLdualfarkas == NULL)
        native_CPXLdualfarkas = load_symbol("CPXLdualfarkas");
    return native_CPXLdualfarkas(env, lp, y, proof_p);
}

// CPXLdualopt
CPXLIBAPI int CPXPUBLIC CPXLdualopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLdualopt == NULL)
        native_CPXLdualopt = load_symbol("CPXLdualopt");
    return native_CPXLdualopt(env, lp);
}

// CPXLdualwrite
CPXLIBAPI int CPXPUBLIC CPXLdualwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, double *objshift_p)
{
    if (native_CPXLdualwrite == NULL)
        native_CPXLdualwrite = load_symbol("CPXLdualwrite");
    return native_CPXLdualwrite(env, lp, filename_str, objshift_p);
}

// CPXLembwrite
CPXLIBAPI int CPXPUBLIC CPXLembwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXLembwrite == NULL)
        native_CPXLembwrite = load_symbol("CPXLembwrite");
    return native_CPXLembwrite(env, lp, filename_str);
}

// CPXLfclose
CPXLIBAPI int CPXPUBLIC CPXLfclose (CPXFILEptr stream)
{
    if (native_CPXLfclose == NULL)
        native_CPXLfclose = load_symbol("CPXLfclose");
    return native_CPXLfclose(stream);
}

// CPXLfeasopt
CPXLIBAPI int CPXPUBLIC CPXLfeasopt (CPXCENVptr env, CPXLPptr lp, double const *rhs, double const *rng, double const *lb, double const *ub)
{
    if (native_CPXLfeasopt == NULL)
        native_CPXLfeasopt = load_symbol("CPXLfeasopt");
    return native_CPXLfeasopt(env, lp, rhs, rng, lb, ub);
}

// CPXLfeasoptext
CPXLIBAPI int CPXPUBLIC CPXLfeasoptext (CPXCENVptr env, CPXLPptr lp, CPXINT grpcnt, CPXLONG concnt, double const *grppref, CPXLONG const *grpbeg, CPXINT const *grpind, char const *grptype)
{
    if (native_CPXLfeasoptext == NULL)
        native_CPXLfeasoptext = load_symbol("CPXLfeasoptext");
    return native_CPXLfeasoptext(env, lp, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}

// CPXLfinalize
CPXLIBAPI void CPXPUBLIC CPXLfinalize (void)
{
    if (native_CPXLfinalize == NULL)
        native_CPXLfinalize = load_symbol("CPXLfinalize");
    return native_CPXLfinalize();
}

// CPXLflushchannel
CPXLIBAPI int CPXPUBLIC CPXLflushchannel (CPXCENVptr env, CPXCHANNELptr channel)
{
    if (native_CPXLflushchannel == NULL)
        native_CPXLflushchannel = load_symbol("CPXLflushchannel");
    return native_CPXLflushchannel(env, channel);
}

// CPXLflushstdchannels
CPXLIBAPI int CPXPUBLIC CPXLflushstdchannels (CPXCENVptr env)
{
    if (native_CPXLflushstdchannels == NULL)
        native_CPXLflushstdchannels = load_symbol("CPXLflushstdchannels");
    return native_CPXLflushstdchannels(env);
}

// CPXLfopen
CPXLIBAPI CPXFILEptr CPXPUBLIC CPXLfopen (char const *filename_str, char const *type_str)
{
    if (native_CPXLfopen == NULL)
        native_CPXLfopen = load_symbol("CPXLfopen");
    return native_CPXLfopen(filename_str, type_str);
}

// CPXLfputs
CPXLIBAPI int CPXPUBLIC CPXLfputs (char const *s_str, CPXFILEptr stream)
{
    if (native_CPXLfputs == NULL)
        native_CPXLfputs = load_symbol("CPXLfputs");
    return native_CPXLfputs(s_str, stream);
}

// CPXLfree
CPXLIBAPI void CPXPUBLIC CPXLfree (void *ptr)
{
    if (native_CPXLfree == NULL)
        native_CPXLfree = load_symbol("CPXLfree");
    return native_CPXLfree(ptr);
}

// CPXLfreeparenv
CPXLIBAPI int CPXPUBLIC CPXLfreeparenv (CPXENVptr env, CPXENVptr * child_p)
{
    if (native_CPXLfreeparenv == NULL)
        native_CPXLfreeparenv = load_symbol("CPXLfreeparenv");
    return native_CPXLfreeparenv(env, child_p);
}

// CPXLfreepresolve
CPXLIBAPI int CPXPUBLIC CPXLfreepresolve (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLfreepresolve == NULL)
        native_CPXLfreepresolve = load_symbol("CPXLfreepresolve");
    return native_CPXLfreepresolve(env, lp);
}

// CPXLfreeprob
CPXLIBAPI int CPXPUBLIC CPXLfreeprob (CPXCENVptr env, CPXLPptr * lp_p)
{
    if (native_CPXLfreeprob == NULL)
        native_CPXLfreeprob = load_symbol("CPXLfreeprob");
    return native_CPXLfreeprob(env, lp_p);
}

// CPXLftran
CPXLIBAPI int CPXPUBLIC CPXLftran (CPXCENVptr env, CPXCLPptr lp, double *x)
{
    if (native_CPXLftran == NULL)
        native_CPXLftran = load_symbol("CPXLftran");
    return native_CPXLftran(env, lp, x);
}

// CPXLgetax
CPXLIBAPI int CPXPUBLIC CPXLgetax (CPXCENVptr env, CPXCLPptr lp, double *x, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetax == NULL)
        native_CPXLgetax = load_symbol("CPXLgetax");
    return native_CPXLgetax(env, lp, x, begin, end);
}

// CPXLgetbaritcnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetbaritcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetbaritcnt == NULL)
        native_CPXLgetbaritcnt = load_symbol("CPXLgetbaritcnt");
    return native_CPXLgetbaritcnt(env, lp);
}

// CPXLgetbase
CPXLIBAPI int CPXPUBLIC CPXLgetbase (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat)
{
    if (native_CPXLgetbase == NULL)
        native_CPXLgetbase = load_symbol("CPXLgetbase");
    return native_CPXLgetbase(env, lp, cstat, rstat);
}

// CPXLgetbasednorms
CPXLIBAPI int CPXPUBLIC CPXLgetbasednorms (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat, double *dnorm)
{
    if (native_CPXLgetbasednorms == NULL)
        native_CPXLgetbasednorms = load_symbol("CPXLgetbasednorms");
    return native_CPXLgetbasednorms(env, lp, cstat, rstat, dnorm);
}

// CPXLgetbhead
CPXLIBAPI int CPXPUBLIC CPXLgetbhead (CPXCENVptr env, CPXCLPptr lp, CPXINT * head, double *x)
{
    if (native_CPXLgetbhead == NULL)
        native_CPXLgetbhead = load_symbol("CPXLgetbhead");
    return native_CPXLgetbhead(env, lp, head, x);
}

// CPXLgetcallbackinfo
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackinfo (CPXCENVptr env, void *cbdata, int wherefrom, int whichinfo, void *result_p)
{
    if (native_CPXLgetcallbackinfo == NULL)
        native_CPXLgetcallbackinfo = load_symbol("CPXLgetcallbackinfo");
    return native_CPXLgetcallbackinfo(env, cbdata, wherefrom, whichinfo, result_p);
}

// CPXLgetchannels
CPXLIBAPI int CPXPUBLIC CPXLgetchannels (CPXCENVptr env, CPXCHANNELptr * cpxresults_p, CPXCHANNELptr * cpxwarning_p, CPXCHANNELptr * cpxerror_p, CPXCHANNELptr * cpxlog_p)
{
    if (native_CPXLgetchannels == NULL)
        native_CPXLgetchannels = load_symbol("CPXLgetchannels");
    return native_CPXLgetchannels(env, cpxresults_p, cpxwarning_p, cpxerror_p, cpxlog_p);
}

// CPXLgetchgparam
CPXLIBAPI int CPXPUBLIC CPXLgetchgparam (CPXCENVptr env, int *cnt_p, int *paramnum, int pspace, int *surplus_p)
{
    if (native_CPXLgetchgparam == NULL)
        native_CPXLgetchgparam = load_symbol("CPXLgetchgparam");
    return native_CPXLgetchgparam(env, cnt_p, paramnum, pspace, surplus_p);
}

// CPXLgetcoef
CPXLIBAPI int CPXPUBLIC CPXLgetcoef (CPXCENVptr env, CPXCLPptr lp, CPXINT i, CPXINT j, double *coef_p)
{
    if (native_CPXLgetcoef == NULL)
        native_CPXLgetcoef = load_symbol("CPXLgetcoef");
    return native_CPXLgetcoef(env, lp, i, j, coef_p);
}

// CPXLgetcolindex
CPXLIBAPI int CPXPUBLIC CPXLgetcolindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p)
{
    if (native_CPXLgetcolindex == NULL)
        native_CPXLgetcolindex = load_symbol("CPXLgetcolindex");
    return native_CPXLgetcolindex(env, lp, lname_str, index_p);
}

// CPXLgetcolinfeas
CPXLIBAPI int CPXPUBLIC CPXLgetcolinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcolinfeas == NULL)
        native_CPXLgetcolinfeas = load_symbol("CPXLgetcolinfeas");
    return native_CPXLgetcolinfeas(env, lp, x, infeasout, begin, end);
}

// CPXLgetcolname
CPXLIBAPI int CPXPUBLIC CPXLgetcolname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, CPXSIZE storespace, CPXSIZE * surplus_p, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcolname == NULL)
        native_CPXLgetcolname = load_symbol("CPXLgetcolname");
    return native_CPXLgetcolname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}

// CPXLgetcols
CPXLIBAPI int CPXPUBLIC CPXLgetcols (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * cmatbeg, CPXINT * cmatind, double *cmatval, CPXLONG cmatspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcols == NULL)
        native_CPXLgetcols = load_symbol("CPXLgetcols");
    return native_CPXLgetcols(env, lp, nzcnt_p, cmatbeg, cmatind, cmatval, cmatspace, surplus_p, begin, end);
}

// CPXLgetconflict
CPXLIBAPI int CPXPUBLIC CPXLgetconflict (CPXCENVptr env, CPXCLPptr lp, int *confstat_p, CPXINT * rowind, int *rowbdstat, CPXINT * confnumrows_p, CPXINT * colind, int *colbdstat, CPXINT * confnumcols_p)
{
    if (native_CPXLgetconflict == NULL)
        native_CPXLgetconflict = load_symbol("CPXLgetconflict");
    return native_CPXLgetconflict(env, lp, confstat_p, rowind, rowbdstat, confnumrows_p, colind, colbdstat, confnumcols_p);
}

// CPXLgetconflictext
CPXLIBAPI int CPXPUBLIC CPXLgetconflictext (CPXCENVptr env, CPXCLPptr lp, int *grpstat, CPXLONG beg, CPXLONG end)
{
    if (native_CPXLgetconflictext == NULL)
        native_CPXLgetconflictext = load_symbol("CPXLgetconflictext");
    return native_CPXLgetconflictext(env, lp, grpstat, beg, end);
}

// CPXLgetcrossdexchcnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetcrossdexchcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetcrossdexchcnt == NULL)
        native_CPXLgetcrossdexchcnt = load_symbol("CPXLgetcrossdexchcnt");
    return native_CPXLgetcrossdexchcnt(env, lp);
}

// CPXLgetcrossdpushcnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetcrossdpushcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetcrossdpushcnt == NULL)
        native_CPXLgetcrossdpushcnt = load_symbol("CPXLgetcrossdpushcnt");
    return native_CPXLgetcrossdpushcnt(env, lp);
}

// CPXLgetcrosspexchcnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetcrosspexchcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetcrosspexchcnt == NULL)
        native_CPXLgetcrosspexchcnt = load_symbol("CPXLgetcrosspexchcnt");
    return native_CPXLgetcrosspexchcnt(env, lp);
}

// CPXLgetcrossppushcnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetcrossppushcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetcrossppushcnt == NULL)
        native_CPXLgetcrossppushcnt = load_symbol("CPXLgetcrossppushcnt");
    return native_CPXLgetcrossppushcnt(env, lp);
}

// CPXLgetdblparam
CPXLIBAPI int CPXPUBLIC CPXLgetdblparam (CPXCENVptr env, int whichparam, double *value_p)
{
    if (native_CPXLgetdblparam == NULL)
        native_CPXLgetdblparam = load_symbol("CPXLgetdblparam");
    return native_CPXLgetdblparam(env, whichparam, value_p);
}

// CPXLgetdblquality
CPXLIBAPI int CPXPUBLIC CPXLgetdblquality (CPXCENVptr env, CPXCLPptr lp, double *quality_p, int what)
{
    if (native_CPXLgetdblquality == NULL)
        native_CPXLgetdblquality = load_symbol("CPXLgetdblquality");
    return native_CPXLgetdblquality(env, lp, quality_p, what);
}

// CPXLgetdettime
CPXLIBAPI int CPXPUBLIC CPXLgetdettime (CPXCENVptr env, double *dettimestamp_p)
{
    if (native_CPXLgetdettime == NULL)
        native_CPXLgetdettime = load_symbol("CPXLgetdettime");
    return native_CPXLgetdettime(env, dettimestamp_p);
}

// CPXLgetdj
CPXLIBAPI int CPXPUBLIC CPXLgetdj (CPXCENVptr env, CPXCLPptr lp, double *dj, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetdj == NULL)
        native_CPXLgetdj = load_symbol("CPXLgetdj");
    return native_CPXLgetdj(env, lp, dj, begin, end);
}

// CPXLgetdnorms
CPXLIBAPI int CPXPUBLIC CPXLgetdnorms (CPXCENVptr env, CPXCLPptr lp, double *norm, CPXINT * head, CPXINT * len_p)
{
    if (native_CPXLgetdnorms == NULL)
        native_CPXLgetdnorms = load_symbol("CPXLgetdnorms");
    return native_CPXLgetdnorms(env, lp, norm, head, len_p);
}

// CPXLgetdsbcnt
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetdsbcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetdsbcnt == NULL)
        native_CPXLgetdsbcnt = load_symbol("CPXLgetdsbcnt");
    return native_CPXLgetdsbcnt(env, lp);
}

// CPXLgeterrorstring
CPXLIBAPI CPXCCHARptr CPXPUBLIC CPXLgeterrorstring (CPXCENVptr env, int errcode, char *buffer_str)
{
    if (native_CPXLgeterrorstring == NULL)
        native_CPXLgeterrorstring = load_symbol("CPXLgeterrorstring");
    return native_CPXLgeterrorstring(env, errcode, buffer_str);
}

// CPXLgetgrad
CPXLIBAPI int CPXPUBLIC CPXLgetgrad (CPXCENVptr env, CPXCLPptr lp, CPXINT j, CPXINT * head, double *y)
{
    if (native_CPXLgetgrad == NULL)
        native_CPXLgetgrad = load_symbol("CPXLgetgrad");
    return native_CPXLgetgrad(env, lp, j, head, y);
}

// CPXLgetijdiv
CPXLIBAPI int CPXPUBLIC CPXLgetijdiv (CPXCENVptr env, CPXCLPptr lp, CPXINT * idiv_p, CPXINT * jdiv_p)
{
    if (native_CPXLgetijdiv == NULL)
        native_CPXLgetijdiv = load_symbol("CPXLgetijdiv");
    return native_CPXLgetijdiv(env, lp, idiv_p, jdiv_p);
}

// CPXLgetijrow
CPXLIBAPI int CPXPUBLIC CPXLgetijrow (CPXCENVptr env, CPXCLPptr lp, CPXINT i, CPXINT j, CPXINT * row_p)
{
    if (native_CPXLgetijrow == NULL)
        native_CPXLgetijrow = load_symbol("CPXLgetijrow");
    return native_CPXLgetijrow(env, lp, i, j, row_p);
}

// CPXLgetintparam
CPXLIBAPI int CPXPUBLIC CPXLgetintparam (CPXCENVptr env, int whichparam, CPXINT * value_p)
{
    if (native_CPXLgetintparam == NULL)
        native_CPXLgetintparam = load_symbol("CPXLgetintparam");
    return native_CPXLgetintparam(env, whichparam, value_p);
}

// CPXLgetintquality
CPXLIBAPI int CPXPUBLIC CPXLgetintquality (CPXCENVptr env, CPXCLPptr lp, CPXINT * quality_p, int what)
{
    if (native_CPXLgetintquality == NULL)
        native_CPXLgetintquality = load_symbol("CPXLgetintquality");
    return native_CPXLgetintquality(env, lp, quality_p, what);
}

// CPXLgetitcnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetitcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetitcnt == NULL)
        native_CPXLgetitcnt = load_symbol("CPXLgetitcnt");
    return native_CPXLgetitcnt(env, lp);
}

// CPXLgetlb
CPXLIBAPI int CPXPUBLIC CPXLgetlb (CPXCENVptr env, CPXCLPptr lp, double *lb, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetlb == NULL)
        native_CPXLgetlb = load_symbol("CPXLgetlb");
    return native_CPXLgetlb(env, lp, lb, begin, end);
}

// CPXLgetlogfile
CPXLIBAPI int CPXPUBLIC CPXLgetlogfile (CPXCENVptr env, CPXFILEptr * logfile_p)
{
    if (native_CPXLgetlogfile == NULL)
        native_CPXLgetlogfile = load_symbol("CPXLgetlogfile");
    return native_CPXLgetlogfile(env, logfile_p);
}

// CPXLgetlongparam
CPXLIBAPI int CPXPUBLIC CPXLgetlongparam (CPXCENVptr env, int whichparam, CPXLONG * value_p)
{
    if (native_CPXLgetlongparam == NULL)
        native_CPXLgetlongparam = load_symbol("CPXLgetlongparam");
    return native_CPXLgetlongparam(env, whichparam, value_p);
}

// CPXLgetlpcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetlpcallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p)
{
    if (native_CPXLgetlpcallbackfunc == NULL)
        native_CPXLgetlpcallbackfunc = load_symbol("CPXLgetlpcallbackfunc");
    return native_CPXLgetlpcallbackfunc(env, callback_p, cbhandle_p);
}

// CPXLgetmethod
CPXLIBAPI int CPXPUBLIC CPXLgetmethod (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetmethod == NULL)
        native_CPXLgetmethod = load_symbol("CPXLgetmethod");
    return native_CPXLgetmethod(env, lp);
}

// CPXLgetnetcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetnetcallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p)
{
    if (native_CPXLgetnetcallbackfunc == NULL)
        native_CPXLgetnetcallbackfunc = load_symbol("CPXLgetnetcallbackfunc");
    return native_CPXLgetnetcallbackfunc(env, callback_p, cbhandle_p);
}

// CPXLgetnumcols
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumcols (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumcols == NULL)
        native_CPXLgetnumcols = load_symbol("CPXLgetnumcols");
    return native_CPXLgetnumcols(env, lp);
}

// CPXLgetnumcores
CPXLIBAPI int CPXPUBLIC CPXLgetnumcores (CPXCENVptr env, int *numcores_p)
{
    if (native_CPXLgetnumcores == NULL)
        native_CPXLgetnumcores = load_symbol("CPXLgetnumcores");
    return native_CPXLgetnumcores(env, numcores_p);
}

// CPXLgetnumnz
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnumnz (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumnz == NULL)
        native_CPXLgetnumnz = load_symbol("CPXLgetnumnz");
    return native_CPXLgetnumnz(env, lp);
}

// CPXLgetnumrows
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumrows (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumrows == NULL)
        native_CPXLgetnumrows = load_symbol("CPXLgetnumrows");
    return native_CPXLgetnumrows(env, lp);
}

// CPXLgetobj
CPXLIBAPI int CPXPUBLIC CPXLgetobj (CPXCENVptr env, CPXCLPptr lp, double *obj, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetobj == NULL)
        native_CPXLgetobj = load_symbol("CPXLgetobj");
    return native_CPXLgetobj(env, lp, obj, begin, end);
}

// CPXLgetobjname
CPXLIBAPI int CPXPUBLIC CPXLgetobjname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p)
{
    if (native_CPXLgetobjname == NULL)
        native_CPXLgetobjname = load_symbol("CPXLgetobjname");
    return native_CPXLgetobjname(env, lp, buf_str, bufspace, surplus_p);
}

// CPXLgetobjoffset
CPXLIBAPI int CPXPUBLIC CPXLgetobjoffset (CPXCENVptr env, CPXCLPptr lp, double *objoffset_p)
{
    if (native_CPXLgetobjoffset == NULL)
        native_CPXLgetobjoffset = load_symbol("CPXLgetobjoffset");
    return native_CPXLgetobjoffset(env, lp, objoffset_p);
}

// CPXLgetobjsen
CPXLIBAPI int CPXPUBLIC CPXLgetobjsen (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetobjsen == NULL)
        native_CPXLgetobjsen = load_symbol("CPXLgetobjsen");
    return native_CPXLgetobjsen(env, lp);
}

// CPXLgetobjval
CPXLIBAPI int CPXPUBLIC CPXLgetobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p)
{
    if (native_CPXLgetobjval == NULL)
        native_CPXLgetobjval = load_symbol("CPXLgetobjval");
    return native_CPXLgetobjval(env, lp, objval_p);
}

// CPXLgetparamname
CPXLIBAPI int CPXPUBLIC CPXLgetparamname (CPXCENVptr env, int whichparam, char *name_str)
{
    if (native_CPXLgetparamname == NULL)
        native_CPXLgetparamname = load_symbol("CPXLgetparamname");
    return native_CPXLgetparamname(env, whichparam, name_str);
}

// CPXLgetparamnum
CPXLIBAPI int CPXPUBLIC CPXLgetparamnum (CPXCENVptr env, char const *name_str, int *whichparam_p)
{
    if (native_CPXLgetparamnum == NULL)
        native_CPXLgetparamnum = load_symbol("CPXLgetparamnum");
    return native_CPXLgetparamnum(env, name_str, whichparam_p);
}

// CPXLgetparamtype
CPXLIBAPI int CPXPUBLIC CPXLgetparamtype (CPXCENVptr env, int whichparam, int *paramtype)
{
    if (native_CPXLgetparamtype == NULL)
        native_CPXLgetparamtype = load_symbol("CPXLgetparamtype");
    return native_CPXLgetparamtype(env, whichparam, paramtype);
}

// CPXLgetphase1cnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetphase1cnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetphase1cnt == NULL)
        native_CPXLgetphase1cnt = load_symbol("CPXLgetphase1cnt");
    return native_CPXLgetphase1cnt(env, lp);
}

// CPXLgetpi
CPXLIBAPI int CPXPUBLIC CPXLgetpi (CPXCENVptr env, CPXCLPptr lp, double *pi, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetpi == NULL)
        native_CPXLgetpi = load_symbol("CPXLgetpi");
    return native_CPXLgetpi(env, lp, pi, begin, end);
}

// CPXLgetpnorms
CPXLIBAPI int CPXPUBLIC CPXLgetpnorms (CPXCENVptr env, CPXCLPptr lp, double *cnorm, double *rnorm, CPXINT * len_p)
{
    if (native_CPXLgetpnorms == NULL)
        native_CPXLgetpnorms = load_symbol("CPXLgetpnorms");
    return native_CPXLgetpnorms(env, lp, cnorm, rnorm, len_p);
}

// CPXLgetprestat
CPXLIBAPI int CPXPUBLIC CPXLgetprestat (CPXCENVptr env, CPXCLPptr lp, int *prestat_p, CPXINT * pcstat, CPXINT * prstat, CPXINT * ocstat, CPXINT * orstat)
{
    if (native_CPXLgetprestat == NULL)
        native_CPXLgetprestat = load_symbol("CPXLgetprestat");
    return native_CPXLgetprestat(env, lp, prestat_p, pcstat, prstat, ocstat, orstat);
}

// CPXLgetprobname
CPXLIBAPI int CPXPUBLIC CPXLgetprobname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p)
{
    if (native_CPXLgetprobname == NULL)
        native_CPXLgetprobname = load_symbol("CPXLgetprobname");
    return native_CPXLgetprobname(env, lp, buf_str, bufspace, surplus_p);
}

// CPXLgetprobtype
CPXLIBAPI int CPXPUBLIC CPXLgetprobtype (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetprobtype == NULL)
        native_CPXLgetprobtype = load_symbol("CPXLgetprobtype");
    return native_CPXLgetprobtype(env, lp);
}

// CPXLgetprotected
CPXLIBAPI int CPXPUBLIC CPXLgetprotected (CPXCENVptr env, CPXCLPptr lp, CPXINT * cnt_p, CPXINT * indices, CPXINT pspace, CPXINT * surplus_p)
{
    if (native_CPXLgetprotected == NULL)
        native_CPXLgetprotected = load_symbol("CPXLgetprotected");
    return native_CPXLgetprotected(env, lp, cnt_p, indices, pspace, surplus_p);
}

// CPXLgetpsbcnt
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetpsbcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetpsbcnt == NULL)
        native_CPXLgetpsbcnt = load_symbol("CPXLgetpsbcnt");
    return native_CPXLgetpsbcnt(env, lp);
}

// CPXLgetray
CPXLIBAPI int CPXPUBLIC CPXLgetray (CPXCENVptr env, CPXCLPptr lp, double *z)
{
    if (native_CPXLgetray == NULL)
        native_CPXLgetray = load_symbol("CPXLgetray");
    return native_CPXLgetray(env, lp, z);
}

// CPXLgetredlp
CPXLIBAPI int CPXPUBLIC CPXLgetredlp (CPXCENVptr env, CPXCLPptr lp, CPXCLPptr * redlp_p)
{
    if (native_CPXLgetredlp == NULL)
        native_CPXLgetredlp = load_symbol("CPXLgetredlp");
    return native_CPXLgetredlp(env, lp, redlp_p);
}

// CPXLgetrhs
CPXLIBAPI int CPXPUBLIC CPXLgetrhs (CPXCENVptr env, CPXCLPptr lp, double *rhs, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetrhs == NULL)
        native_CPXLgetrhs = load_symbol("CPXLgetrhs");
    return native_CPXLgetrhs(env, lp, rhs, begin, end);
}

// CPXLgetrngval
CPXLIBAPI int CPXPUBLIC CPXLgetrngval (CPXCENVptr env, CPXCLPptr lp, double *rngval, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetrngval == NULL)
        native_CPXLgetrngval = load_symbol("CPXLgetrngval");
    return native_CPXLgetrngval(env, lp, rngval, begin, end);
}

// CPXLgetrowindex
CPXLIBAPI int CPXPUBLIC CPXLgetrowindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p)
{
    if (native_CPXLgetrowindex == NULL)
        native_CPXLgetrowindex = load_symbol("CPXLgetrowindex");
    return native_CPXLgetrowindex(env, lp, lname_str, index_p);
}

// CPXLgetrowinfeas
CPXLIBAPI int CPXPUBLIC CPXLgetrowinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetrowinfeas == NULL)
        native_CPXLgetrowinfeas = load_symbol("CPXLgetrowinfeas");
    return native_CPXLgetrowinfeas(env, lp, x, infeasout, begin, end);
}

// CPXLgetrowname
CPXLIBAPI int CPXPUBLIC CPXLgetrowname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, CPXSIZE storespace, CPXSIZE * surplus_p, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetrowname == NULL)
        native_CPXLgetrowname = load_symbol("CPXLgetrowname");
    return native_CPXLgetrowname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}

// CPXLgetrows
CPXLIBAPI int CPXPUBLIC CPXLgetrows (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * rmatbeg, CPXINT * rmatind, double *rmatval, CPXLONG rmatspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetrows == NULL)
        native_CPXLgetrows = load_symbol("CPXLgetrows");
    return native_CPXLgetrows(env, lp, nzcnt_p, rmatbeg, rmatind, rmatval, rmatspace, surplus_p, begin, end);
}

// CPXLgetsense
CPXLIBAPI int CPXPUBLIC CPXLgetsense (CPXCENVptr env, CPXCLPptr lp, char *sense, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetsense == NULL)
        native_CPXLgetsense = load_symbol("CPXLgetsense");
    return native_CPXLgetsense(env, lp, sense, begin, end);
}

// CPXLgetsiftitcnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetsiftitcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetsiftitcnt == NULL)
        native_CPXLgetsiftitcnt = load_symbol("CPXLgetsiftitcnt");
    return native_CPXLgetsiftitcnt(env, lp);
}

// CPXLgetsiftphase1cnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetsiftphase1cnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetsiftphase1cnt == NULL)
        native_CPXLgetsiftphase1cnt = load_symbol("CPXLgetsiftphase1cnt");
    return native_CPXLgetsiftphase1cnt(env, lp);
}

// CPXLgetslack
CPXLIBAPI int CPXPUBLIC CPXLgetslack (CPXCENVptr env, CPXCLPptr lp, double *slack, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetslack == NULL)
        native_CPXLgetslack = load_symbol("CPXLgetslack");
    return native_CPXLgetslack(env, lp, slack, begin, end);
}

// CPXLgetsolnpooldblquality
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpooldblquality (CPXCENVptr env, CPXCLPptr lp, int soln, double *quality_p, int what)
{
    if (native_CPXLgetsolnpooldblquality == NULL)
        native_CPXLgetsolnpooldblquality = load_symbol("CPXLgetsolnpooldblquality");
    return native_CPXLgetsolnpooldblquality(env, lp, soln, quality_p, what);
}

// CPXLgetsolnpoolintquality
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolintquality (CPXCENVptr env, CPXCLPptr lp, int soln, CPXINT * quality_p, int what)
{
    if (native_CPXLgetsolnpoolintquality == NULL)
        native_CPXLgetsolnpoolintquality = load_symbol("CPXLgetsolnpoolintquality");
    return native_CPXLgetsolnpoolintquality(env, lp, soln, quality_p, what);
}

// CPXLgetstat
CPXLIBAPI int CPXPUBLIC CPXLgetstat (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetstat == NULL)
        native_CPXLgetstat = load_symbol("CPXLgetstat");
    return native_CPXLgetstat(env, lp);
}

// CPXLgetstatstring
CPXLIBAPI CPXCHARptr CPXPUBLIC CPXLgetstatstring (CPXCENVptr env, int statind, char *buffer_str)
{
    if (native_CPXLgetstatstring == NULL)
        native_CPXLgetstatstring = load_symbol("CPXLgetstatstring");
    return native_CPXLgetstatstring(env, statind, buffer_str);
}

// CPXLgetstrparam
CPXLIBAPI int CPXPUBLIC CPXLgetstrparam (CPXCENVptr env, int whichparam, char *value_str)
{
    if (native_CPXLgetstrparam == NULL)
        native_CPXLgetstrparam = load_symbol("CPXLgetstrparam");
    return native_CPXLgetstrparam(env, whichparam, value_str);
}

// CPXLgettime
CPXLIBAPI int CPXPUBLIC CPXLgettime (CPXCENVptr env, double *timestamp_p)
{
    if (native_CPXLgettime == NULL)
        native_CPXLgettime = load_symbol("CPXLgettime");
    return native_CPXLgettime(env, timestamp_p);
}

// CPXLgettuningcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgettuningcallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p)
{
    if (native_CPXLgettuningcallbackfunc == NULL)
        native_CPXLgettuningcallbackfunc = load_symbol("CPXLgettuningcallbackfunc");
    return native_CPXLgettuningcallbackfunc(env, callback_p, cbhandle_p);
}

// CPXLgetub
CPXLIBAPI int CPXPUBLIC CPXLgetub (CPXCENVptr env, CPXCLPptr lp, double *ub, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetub == NULL)
        native_CPXLgetub = load_symbol("CPXLgetub");
    return native_CPXLgetub(env, lp, ub, begin, end);
}

// CPXLgetweight
CPXLIBAPI int CPXPUBLIC CPXLgetweight (CPXCENVptr env, CPXCLPptr lp, CPXINT rcnt, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, double *weight, int dpriind)
{
    if (native_CPXLgetweight == NULL)
        native_CPXLgetweight = load_symbol("CPXLgetweight");
    return native_CPXLgetweight(env, lp, rcnt, rmatbeg, rmatind, rmatval, weight, dpriind);
}

// CPXLgetx
CPXLIBAPI int CPXPUBLIC CPXLgetx (CPXCENVptr env, CPXCLPptr lp, double *x, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetx == NULL)
        native_CPXLgetx = load_symbol("CPXLgetx");
    return native_CPXLgetx(env, lp, x, begin, end);
}

// CPXLhybnetopt
CPXLIBAPI int CPXPUBLIC CPXLhybnetopt (CPXCENVptr env, CPXLPptr lp, int method)
{
    if (native_CPXLhybnetopt == NULL)
        native_CPXLhybnetopt = load_symbol("CPXLhybnetopt");
    return native_CPXLhybnetopt(env, lp, method);
}

// CPXLinfodblparam
CPXLIBAPI int CPXPUBLIC CPXLinfodblparam (CPXCENVptr env, int whichparam, double *defvalue_p, double *minvalue_p, double *maxvalue_p)
{
    if (native_CPXLinfodblparam == NULL)
        native_CPXLinfodblparam = load_symbol("CPXLinfodblparam");
    return native_CPXLinfodblparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}

// CPXLinfointparam
CPXLIBAPI int CPXPUBLIC CPXLinfointparam (CPXCENVptr env, int whichparam, CPXINT * defvalue_p, CPXINT * minvalue_p, CPXINT * maxvalue_p)
{
    if (native_CPXLinfointparam == NULL)
        native_CPXLinfointparam = load_symbol("CPXLinfointparam");
    return native_CPXLinfointparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}

// CPXLinfolongparam
CPXLIBAPI int CPXPUBLIC CPXLinfolongparam (CPXCENVptr env, int whichparam, CPXLONG * defvalue_p, CPXLONG * minvalue_p, CPXLONG * maxvalue_p)
{
    if (native_CPXLinfolongparam == NULL)
        native_CPXLinfolongparam = load_symbol("CPXLinfolongparam");
    return native_CPXLinfolongparam(env, whichparam, defvalue_p, minvalue_p, maxvalue_p);
}

// CPXLinfostrparam
CPXLIBAPI int CPXPUBLIC CPXLinfostrparam (CPXCENVptr env, int whichparam, char *defvalue_str)
{
    if (native_CPXLinfostrparam == NULL)
        native_CPXLinfostrparam = load_symbol("CPXLinfostrparam");
    return native_CPXLinfostrparam(env, whichparam, defvalue_str);
}

// CPXLinitialize
CPXLIBAPI void CPXPUBLIC CPXLinitialize (void)
{
    if (native_CPXLinitialize == NULL)
        native_CPXLinitialize = load_symbol("CPXLinitialize");
    return native_CPXLinitialize();
}

// CPXLkilldnorms
CPXLIBAPI int CPXPUBLIC CPXLkilldnorms (CPXLPptr lp)
{
    if (native_CPXLkilldnorms == NULL)
        native_CPXLkilldnorms = load_symbol("CPXLkilldnorms");
    return native_CPXLkilldnorms(lp);
}

// CPXLkillpnorms
CPXLIBAPI int CPXPUBLIC CPXLkillpnorms (CPXLPptr lp)
{
    if (native_CPXLkillpnorms == NULL)
        native_CPXLkillpnorms = load_symbol("CPXLkillpnorms");
    return native_CPXLkillpnorms(lp);
}

// CPXLlpopt
CPXLIBAPI int CPXPUBLIC CPXLlpopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLlpopt == NULL)
        native_CPXLlpopt = load_symbol("CPXLlpopt");
    return native_CPXLlpopt(env, lp);
}

// CPXLlprewrite
CPXLIBAPI int CPXPUBLIC CPXLlprewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLlprewrite == NULL)
        native_CPXLlprewrite = load_symbol("CPXLlprewrite");
    return native_CPXLlprewrite(env, lp, filename_str);
}

// CPXLlpwrite
CPXLIBAPI int CPXPUBLIC CPXLlpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLlpwrite == NULL)
        native_CPXLlpwrite = load_symbol("CPXLlpwrite");
    return native_CPXLlpwrite(env, lp, filename_str);
}

// CPXLmalloc
CPXLIBAPI CPXVOIDptr CPXPUBLIC CPXLmalloc (size_t size)
{
    if (native_CPXLmalloc == NULL)
        native_CPXLmalloc = load_symbol("CPXLmalloc");
    return native_CPXLmalloc(size);
}

// CPXLmbasewrite
CPXLIBAPI int CPXPUBLIC CPXLmbasewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLmbasewrite == NULL)
        native_CPXLmbasewrite = load_symbol("CPXLmbasewrite");
    return native_CPXLmbasewrite(env, lp, filename_str);
}

// CPXLmdleave
CPXLIBAPI int CPXPUBLIC CPXLmdleave (CPXCENVptr env, CPXLPptr lp, CPXINT const *indices, CPXINT cnt, double *downratio, double *upratio)
{
    if (native_CPXLmdleave == NULL)
        native_CPXLmdleave = load_symbol("CPXLmdleave");
    return native_CPXLmdleave(env, lp, indices, cnt, downratio, upratio);
}

// CPXLmemcpy
CPXLIBAPI CPXVOIDptr CPXPUBLIC CPXLmemcpy (void *s1, void *s2, size_t n)
{
    if (native_CPXLmemcpy == NULL)
        native_CPXLmemcpy = load_symbol("CPXLmemcpy");
    return native_CPXLmemcpy(s1, s2, n);
}

// CPXLmpsrewrite
CPXLIBAPI int CPXPUBLIC CPXLmpsrewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLmpsrewrite == NULL)
        native_CPXLmpsrewrite = load_symbol("CPXLmpsrewrite");
    return native_CPXLmpsrewrite(env, lp, filename_str);
}

// CPXLmpswrite
CPXLIBAPI int CPXPUBLIC CPXLmpswrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLmpswrite == NULL)
        native_CPXLmpswrite = load_symbol("CPXLmpswrite");
    return native_CPXLmpswrite(env, lp, filename_str);
}

// CPXLmsg
CPXLIBAPI int CPXPUBVARARGS CPXLmsg (CPXCHANNELptr channel, char const *format, ...)
{
    if (native_CPXLmsg == NULL)
        native_CPXLmsg = load_symbol("CPXLmsg");
    return native_CPXLmsg(channel, format);
}

// CPXLmsgstr
CPXLIBAPI int CPXPUBLIC CPXLmsgstr (CPXCHANNELptr channel, char const *msg_str)
{
    if (native_CPXLmsgstr == NULL)
        native_CPXLmsgstr = load_symbol("CPXLmsgstr");
    return native_CPXLmsgstr(channel, msg_str);
}

// CPXLNETextract
CPXLIBAPI int CPXPUBLIC CPXLNETextract (CPXCENVptr env, CPXNETptr net, CPXCLPptr lp, CPXINT * colmap, CPXINT * rowmap)
{
    if (native_CPXLNETextract == NULL)
        native_CPXLNETextract = load_symbol("CPXLNETextract");
    return native_CPXLNETextract(env, net, lp, colmap, rowmap);
}

// CPXLnewcols
CPXLIBAPI int CPXPUBLIC CPXLnewcols (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, double const *obj, double const *lb, double const *ub, char const *xctype, char const *const *colname)
{
    if (native_CPXLnewcols == NULL)
        native_CPXLnewcols = load_symbol("CPXLnewcols");
    return native_CPXLnewcols(env, lp, ccnt, obj, lb, ub, xctype, colname);
}

// CPXLnewrows
CPXLIBAPI int CPXPUBLIC CPXLnewrows (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, double const *rhs, char const *sense, double const *rngval, char const *const *rowname)
{
    if (native_CPXLnewrows == NULL)
        native_CPXLnewrows = load_symbol("CPXLnewrows");
    return native_CPXLnewrows(env, lp, rcnt, rhs, sense, rngval, rowname);
}

// CPXLobjsa
CPXLIBAPI int CPXPUBLIC CPXLobjsa (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end, double *lower, double *upper)
{
    if (native_CPXLobjsa == NULL)
        native_CPXLobjsa = load_symbol("CPXLobjsa");
    return native_CPXLobjsa(env, lp, begin, end, lower, upper);
}

// CPXLopenCPLEX
CPXLIBAPI CPXENVptr CPXPUBLIC CPXLopenCPLEX (int *status_p)
{
    if (native_CPXLopenCPLEX == NULL)
        native_CPXLopenCPLEX = load_symbol("CPXLopenCPLEX");
    return native_CPXLopenCPLEX(status_p);
}

// CPXLopenCPLEXruntime
CPXLIBAPI CPXENVptr CPXPUBLIC CPXLopenCPLEXruntime (int *status_p, int serialnum, char const *licenvstring_str)
{
    if (native_CPXLopenCPLEXruntime == NULL)
        native_CPXLopenCPLEXruntime = load_symbol("CPXLopenCPLEXruntime");
    return native_CPXLopenCPLEXruntime(status_p, serialnum, licenvstring_str);
}

// CPXLparenv
CPXLIBAPI CPXENVptr CPXPUBLIC CPXLparenv (CPXENVptr env, int *status_p)
{
    if (native_CPXLparenv == NULL)
        native_CPXLparenv = load_symbol("CPXLparenv");
    return native_CPXLparenv(env, status_p);
}

// CPXLpivot
CPXLIBAPI int CPXPUBLIC CPXLpivot (CPXCENVptr env, CPXLPptr lp, CPXINT jenter, CPXINT jleave, int leavestat)
{
    if (native_CPXLpivot == NULL)
        native_CPXLpivot = load_symbol("CPXLpivot");
    return native_CPXLpivot(env, lp, jenter, jleave, leavestat);
}

// CPXLpivotin
CPXLIBAPI int CPXPUBLIC CPXLpivotin (CPXCENVptr env, CPXLPptr lp, CPXINT const *rlist, CPXINT rlen)
{
    if (native_CPXLpivotin == NULL)
        native_CPXLpivotin = load_symbol("CPXLpivotin");
    return native_CPXLpivotin(env, lp, rlist, rlen);
}

// CPXLpivotout
CPXLIBAPI int CPXPUBLIC CPXLpivotout (CPXCENVptr env, CPXLPptr lp, CPXINT const *clist, CPXINT clen)
{
    if (native_CPXLpivotout == NULL)
        native_CPXLpivotout = load_symbol("CPXLpivotout");
    return native_CPXLpivotout(env, lp, clist, clen);
}

// CPXLpperwrite
CPXLIBAPI int CPXPUBLIC CPXLpperwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double epsilon)
{
    if (native_CPXLpperwrite == NULL)
        native_CPXLpperwrite = load_symbol("CPXLpperwrite");
    return native_CPXLpperwrite(env, lp, filename_str, epsilon);
}

// CPXLpratio
CPXLIBAPI int CPXPUBLIC CPXLpratio (CPXCENVptr env, CPXLPptr lp, CPXINT * indices, CPXINT cnt, double *downratio, double *upratio, CPXINT * downleave, CPXINT * upleave, int *downleavestatus, int *upleavestatus, int *downstatus, int *upstatus)
{
    if (native_CPXLpratio == NULL)
        native_CPXLpratio = load_symbol("CPXLpratio");
    return native_CPXLpratio(env, lp, indices, cnt, downratio, upratio, downleave, upleave, downleavestatus, upleavestatus, downstatus, upstatus);
}

// CPXLpreaddrows
CPXLIBAPI int CPXPUBLIC CPXLpreaddrows (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *rowname)
{
    if (native_CPXLpreaddrows == NULL)
        native_CPXLpreaddrows = load_symbol("CPXLpreaddrows");
    return native_CPXLpreaddrows(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}

// CPXLprechgobj
CPXLIBAPI int CPXPUBLIC CPXLprechgobj (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, double const *values)
{
    if (native_CPXLprechgobj == NULL)
        native_CPXLprechgobj = load_symbol("CPXLprechgobj");
    return native_CPXLprechgobj(env, lp, cnt, indices, values);
}

// CPXLpreslvwrite
CPXLIBAPI int CPXPUBLIC CPXLpreslvwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str, double *objoff_p)
{
    if (native_CPXLpreslvwrite == NULL)
        native_CPXLpreslvwrite = load_symbol("CPXLpreslvwrite");
    return native_CPXLpreslvwrite(env, lp, filename_str, objoff_p);
}

// CPXLpresolve
CPXLIBAPI int CPXPUBLIC CPXLpresolve (CPXCENVptr env, CPXLPptr lp, int method)
{
    if (native_CPXLpresolve == NULL)
        native_CPXLpresolve = load_symbol("CPXLpresolve");
    return native_CPXLpresolve(env, lp, method);
}

// CPXLprimopt
CPXLIBAPI int CPXPUBLIC CPXLprimopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLprimopt == NULL)
        native_CPXLprimopt = load_symbol("CPXLprimopt");
    return native_CPXLprimopt(env, lp);
}

// CPXLputenv
CPXLIBAPI int CPXPUBLIC CPXLputenv (char *envsetting_str)
{
    if (native_CPXLputenv == NULL)
        native_CPXLputenv = load_symbol("CPXLputenv");
    return native_CPXLputenv(envsetting_str);
}

// CPXLqpdjfrompi
CPXLIBAPI int CPXPUBLIC CPXLqpdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double const *x, double *dj)
{
    if (native_CPXLqpdjfrompi == NULL)
        native_CPXLqpdjfrompi = load_symbol("CPXLqpdjfrompi");
    return native_CPXLqpdjfrompi(env, lp, pi, x, dj);
}

// CPXLqpuncrushpi
CPXLIBAPI int CPXPUBLIC CPXLqpuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi, double const *x)
{
    if (native_CPXLqpuncrushpi == NULL)
        native_CPXLqpuncrushpi = load_symbol("CPXLqpuncrushpi");
    return native_CPXLqpuncrushpi(env, lp, pi, prepi, x);
}

// CPXLreadcopybase
CPXLIBAPI int CPXPUBLIC CPXLreadcopybase (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXLreadcopybase == NULL)
        native_CPXLreadcopybase = load_symbol("CPXLreadcopybase");
    return native_CPXLreadcopybase(env, lp, filename_str);
}

// CPXLreadcopyparam
CPXLIBAPI int CPXPUBLIC CPXLreadcopyparam (CPXENVptr env, char const *filename_str)
{
    if (native_CPXLreadcopyparam == NULL)
        native_CPXLreadcopyparam = load_symbol("CPXLreadcopyparam");
    return native_CPXLreadcopyparam(env, filename_str);
}

// CPXLreadcopyprob
CPXLIBAPI int CPXPUBLIC CPXLreadcopyprob (CPXCENVptr env, CPXLPptr lp, char const *filename_str, char const *filetype_str)
{
    if (native_CPXLreadcopyprob == NULL)
        native_CPXLreadcopyprob = load_symbol("CPXLreadcopyprob");
    return native_CPXLreadcopyprob(env, lp, filename_str, filetype_str);
}

// CPXLreadcopysol
CPXLIBAPI int CPXPUBLIC CPXLreadcopysol (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXLreadcopysol == NULL)
        native_CPXLreadcopysol = load_symbol("CPXLreadcopysol");
    return native_CPXLreadcopysol(env, lp, filename_str);
}

// CPXLrealloc
CPXLIBAPI CPXVOIDptr CPXPUBLIC CPXLrealloc (void *ptr, size_t size)
{
    if (native_CPXLrealloc == NULL)
        native_CPXLrealloc = load_symbol("CPXLrealloc");
    return native_CPXLrealloc(ptr, size);
}

// CPXLrefineconflict
CPXLIBAPI int CPXPUBLIC CPXLrefineconflict (CPXCENVptr env, CPXLPptr lp, CPXINT * confnumrows_p, CPXINT * confnumcols_p)
{
    if (native_CPXLrefineconflict == NULL)
        native_CPXLrefineconflict = load_symbol("CPXLrefineconflict");
    return native_CPXLrefineconflict(env, lp, confnumrows_p, confnumcols_p);
}

// CPXLrefineconflictext
CPXLIBAPI int CPXPUBLIC CPXLrefineconflictext (CPXCENVptr env, CPXLPptr lp, CPXLONG grpcnt, CPXLONG concnt, double const *grppref, CPXLONG const *grpbeg, CPXINT const *grpind, char const *grptype)
{
    if (native_CPXLrefineconflictext == NULL)
        native_CPXLrefineconflictext = load_symbol("CPXLrefineconflictext");
    return native_CPXLrefineconflictext(env, lp, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}

// CPXLrhssa
CPXLIBAPI int CPXPUBLIC CPXLrhssa (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end, double *lower, double *upper)
{
    if (native_CPXLrhssa == NULL)
        native_CPXLrhssa = load_symbol("CPXLrhssa");
    return native_CPXLrhssa(env, lp, begin, end, lower, upper);
}

// CPXLrobustopt
CPXLIBAPI int CPXPUBLIC CPXLrobustopt (CPXCENVptr env, CPXLPptr lp, CPXLPptr lblp, CPXLPptr ublp, double objchg, double const *maxchg)
{
    if (native_CPXLrobustopt == NULL)
        native_CPXLrobustopt = load_symbol("CPXLrobustopt");
    return native_CPXLrobustopt(env, lp, lblp, ublp, objchg, maxchg);
}

// CPXLsavwrite
CPXLIBAPI int CPXPUBLIC CPXLsavwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLsavwrite == NULL)
        native_CPXLsavwrite = load_symbol("CPXLsavwrite");
    return native_CPXLsavwrite(env, lp, filename_str);
}

// CPXLserializercreate
CPXLIBAPI int CPXPUBLIC CPXLserializercreate (CPXSERIALIZERptr * ser_p)
{
    if (native_CPXLserializercreate == NULL)
        native_CPXLserializercreate = load_symbol("CPXLserializercreate");
    return native_CPXLserializercreate(ser_p);
}

// CPXLserializerdestroy
CPXLIBAPI void CPXPUBLIC CPXLserializerdestroy (CPXSERIALIZERptr ser)
{
    if (native_CPXLserializerdestroy == NULL)
        native_CPXLserializerdestroy = load_symbol("CPXLserializerdestroy");
    return native_CPXLserializerdestroy(ser);
}

// CPXLserializerlength
CPXLIBAPI CPXLONG CPXPUBLIC CPXLserializerlength (CPXCSERIALIZERptr ser)
{
    if (native_CPXLserializerlength == NULL)
        native_CPXLserializerlength = load_symbol("CPXLserializerlength");
    return native_CPXLserializerlength(ser);
}

// CPXLserializerpayload
CPXLIBAPI void const *CPXPUBLIC CPXLserializerpayload (CPXCSERIALIZERptr ser)
{
    if (native_CPXLserializerpayload == NULL)
        native_CPXLserializerpayload = load_symbol("CPXLserializerpayload");
    return native_CPXLserializerpayload(ser);
}

// CPXLsetdblparam
CPXLIBAPI int CPXPUBLIC CPXLsetdblparam (CPXENVptr env, int whichparam, double newvalue)
{
    if (native_CPXLsetdblparam == NULL)
        native_CPXLsetdblparam = load_symbol("CPXLsetdblparam");
    return native_CPXLsetdblparam(env, whichparam, newvalue);
}

// CPXLsetdefaults
CPXLIBAPI int CPXPUBLIC CPXLsetdefaults (CPXENVptr env)
{
    if (native_CPXLsetdefaults == NULL)
        native_CPXLsetdefaults = load_symbol("CPXLsetdefaults");
    return native_CPXLsetdefaults(env);
}

// CPXLsetintparam
CPXLIBAPI int CPXPUBLIC CPXLsetintparam (CPXENVptr env, int whichparam, CPXINT newvalue)
{
    if (native_CPXLsetintparam == NULL)
        native_CPXLsetintparam = load_symbol("CPXLsetintparam");
    return native_CPXLsetintparam(env, whichparam, newvalue);
}

// CPXLsetlogfile
CPXLIBAPI int CPXPUBLIC CPXLsetlogfile (CPXENVptr env, CPXFILEptr lfile)
{
    if (native_CPXLsetlogfile == NULL)
        native_CPXLsetlogfile = load_symbol("CPXLsetlogfile");
    return native_CPXLsetlogfile(env, lfile);
}

// CPXLsetlongparam
CPXLIBAPI int CPXPUBLIC CPXLsetlongparam (CPXENVptr env, int whichparam, CPXLONG newvalue)
{
    if (native_CPXLsetlongparam == NULL)
        native_CPXLsetlongparam = load_symbol("CPXLsetlongparam");
    return native_CPXLsetlongparam(env, whichparam, newvalue);
}

// CPXLsetlpcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetlpcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle)
{
    if (native_CPXLsetlpcallbackfunc == NULL)
        native_CPXLsetlpcallbackfunc = load_symbol("CPXLsetlpcallbackfunc");
    return native_CPXLsetlpcallbackfunc(env, callback, cbhandle);
}

// CPXLsetnetcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetnetcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle)
{
    if (native_CPXLsetnetcallbackfunc == NULL)
        native_CPXLsetnetcallbackfunc = load_symbol("CPXLsetnetcallbackfunc");
    return native_CPXLsetnetcallbackfunc(env, callback, cbhandle);
}

// CPXLsetphase2
CPXLIBAPI int CPXPUBLIC CPXLsetphase2 (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLsetphase2 == NULL)
        native_CPXLsetphase2 = load_symbol("CPXLsetphase2");
    return native_CPXLsetphase2(env, lp);
}

// CPXLsetprofcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetprofcallbackfunc (CPXENVptr env, CPXL_CALLBACK_PROF * callback, void *cbhandle)
{
    if (native_CPXLsetprofcallbackfunc == NULL)
        native_CPXLsetprofcallbackfunc = load_symbol("CPXLsetprofcallbackfunc");
    return native_CPXLsetprofcallbackfunc(env, callback, cbhandle);
}

// CPXLsetstrparam
CPXLIBAPI int CPXPUBLIC CPXLsetstrparam (CPXENVptr env, int whichparam, char const *newvalue_str)
{
    if (native_CPXLsetstrparam == NULL)
        native_CPXLsetstrparam = load_symbol("CPXLsetstrparam");
    return native_CPXLsetstrparam(env, whichparam, newvalue_str);
}

// CPXLsetterminate
CPXLIBAPI int CPXPUBLIC CPXLsetterminate (CPXENVptr env, volatile int *terminate_p)
{
    if (native_CPXLsetterminate == NULL)
        native_CPXLsetterminate = load_symbol("CPXLsetterminate");
    return native_CPXLsetterminate(env, terminate_p);
}

// CPXLsettuningcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsettuningcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle)
{
    if (native_CPXLsettuningcallbackfunc == NULL)
        native_CPXLsettuningcallbackfunc = load_symbol("CPXLsettuningcallbackfunc");
    return native_CPXLsettuningcallbackfunc(env, callback, cbhandle);
}

// CPXLsiftopt
CPXLIBAPI int CPXPUBLIC CPXLsiftopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLsiftopt == NULL)
        native_CPXLsiftopt = load_symbol("CPXLsiftopt");
    return native_CPXLsiftopt(env, lp);
}

// CPXLslackfromx
CPXLIBAPI int CPXPUBLIC CPXLslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *slack)
{
    if (native_CPXLslackfromx == NULL)
        native_CPXLslackfromx = load_symbol("CPXLslackfromx");
    return native_CPXLslackfromx(env, lp, x, slack);
}

// CPXLsolninfo
CPXLIBAPI int CPXPUBLIC CPXLsolninfo (CPXCENVptr env, CPXCLPptr lp, int *solnmethod_p, int *solntype_p, int *pfeasind_p, int *dfeasind_p)
{
    if (native_CPXLsolninfo == NULL)
        native_CPXLsolninfo = load_symbol("CPXLsolninfo");
    return native_CPXLsolninfo(env, lp, solnmethod_p, solntype_p, pfeasind_p, dfeasind_p);
}

// CPXLsolution
CPXLIBAPI int CPXPUBLIC CPXLsolution (CPXCENVptr env, CPXCLPptr lp, int *lpstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj)
{
    if (native_CPXLsolution == NULL)
        native_CPXLsolution = load_symbol("CPXLsolution");
    return native_CPXLsolution(env, lp, lpstat_p, objval_p, x, pi, slack, dj);
}

// CPXLsolwrite
CPXLIBAPI int CPXPUBLIC CPXLsolwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLsolwrite == NULL)
        native_CPXLsolwrite = load_symbol("CPXLsolwrite");
    return native_CPXLsolwrite(env, lp, filename_str);
}

// CPXLsolwritesolnpool
CPXLIBAPI int CPXPUBLIC CPXLsolwritesolnpool (CPXCENVptr env, CPXCLPptr lp, int soln, char const *filename_str)
{
    if (native_CPXLsolwritesolnpool == NULL)
        native_CPXLsolwritesolnpool = load_symbol("CPXLsolwritesolnpool");
    return native_CPXLsolwritesolnpool(env, lp, soln, filename_str);
}

// CPXLsolwritesolnpoolall
CPXLIBAPI int CPXPUBLIC CPXLsolwritesolnpoolall (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLsolwritesolnpoolall == NULL)
        native_CPXLsolwritesolnpoolall = load_symbol("CPXLsolwritesolnpoolall");
    return native_CPXLsolwritesolnpoolall(env, lp, filename_str);
}

// CPXLstrcpy
CPXLIBAPI CPXCHARptr CPXPUBLIC CPXLstrcpy (char *dest_str, char const *src_str)
{
    if (native_CPXLstrcpy == NULL)
        native_CPXLstrcpy = load_symbol("CPXLstrcpy");
    return native_CPXLstrcpy(dest_str, src_str);
}

// CPXLstrlen
CPXLIBAPI size_t CPXPUBLIC CPXLstrlen (char const *s_str)
{
    if (native_CPXLstrlen == NULL)
        native_CPXLstrlen = load_symbol("CPXLstrlen");
    return native_CPXLstrlen(s_str);
}

// CPXLstrongbranch
CPXLIBAPI int CPXPUBLIC CPXLstrongbranch (CPXCENVptr env, CPXLPptr lp, CPXINT const *indices, CPXINT cnt, double *downobj, double *upobj, CPXLONG itlim)
{
    if (native_CPXLstrongbranch == NULL)
        native_CPXLstrongbranch = load_symbol("CPXLstrongbranch");
    return native_CPXLstrongbranch(env, lp, indices, cnt, downobj, upobj, itlim);
}

// CPXLtightenbds
CPXLIBAPI int CPXPUBLIC CPXLtightenbds (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd)
{
    if (native_CPXLtightenbds == NULL)
        native_CPXLtightenbds = load_symbol("CPXLtightenbds");
    return native_CPXLtightenbds(env, lp, cnt, indices, lu, bd);
}

// CPXLtuneparam
CPXLIBAPI int CPXPUBLIC CPXLtuneparam (CPXENVptr env, CPXLPptr lp, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char const *const *strval, int *tunestat_p)
{
    if (native_CPXLtuneparam == NULL)
        native_CPXLtuneparam = load_symbol("CPXLtuneparam");
    return native_CPXLtuneparam(env, lp, intcnt, intnum, intval, dblcnt, dblnum, dblval, strcnt, strnum, strval, tunestat_p);
}

// CPXLtuneparamprobset
CPXLIBAPI int CPXPUBLIC CPXLtuneparamprobset (CPXENVptr env, int filecnt, char const *const *filename, char const *const *filetype, int intcnt, int const *intnum, int const *intval, int dblcnt, int const *dblnum, double const *dblval, int strcnt, int const *strnum, char const *const *strval, int *tunestat_p)
{
    if (native_CPXLtuneparamprobset == NULL)
        native_CPXLtuneparamprobset = load_symbol("CPXLtuneparamprobset");
    return native_CPXLtuneparamprobset(env, filecnt, filename, filetype, intcnt, intnum, intval, dblcnt, dblnum, dblval, strcnt, strnum, strval, tunestat_p);
}

// CPXLuncrushform
CPXLIBAPI int CPXPUBLIC CPXLuncrushform (CPXCENVptr env, CPXCLPptr lp, CPXINT plen, CPXINT const *pind, double const *pval, CPXINT * len_p, double *offset_p, CPXINT * ind, double *val)
{
    if (native_CPXLuncrushform == NULL)
        native_CPXLuncrushform = load_symbol("CPXLuncrushform");
    return native_CPXLuncrushform(env, lp, plen, pind, pval, len_p, offset_p, ind, val);
}

// CPXLuncrushpi
CPXLIBAPI int CPXPUBLIC CPXLuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi, double const *prepi)
{
    if (native_CPXLuncrushpi == NULL)
        native_CPXLuncrushpi = load_symbol("CPXLuncrushpi");
    return native_CPXLuncrushpi(env, lp, pi, prepi);
}

// CPXLuncrushx
CPXLIBAPI int CPXPUBLIC CPXLuncrushx (CPXCENVptr env, CPXCLPptr lp, double *x, double const *prex)
{
    if (native_CPXLuncrushx == NULL)
        native_CPXLuncrushx = load_symbol("CPXLuncrushx");
    return native_CPXLuncrushx(env, lp, x, prex);
}

// CPXLunscaleprob
CPXLIBAPI int CPXPUBLIC CPXLunscaleprob (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLunscaleprob == NULL)
        native_CPXLunscaleprob = load_symbol("CPXLunscaleprob");
    return native_CPXLunscaleprob(env, lp);
}

// CPXLversion
CPXLIBAPI CPXCCHARptr CPXPUBLIC CPXLversion (CPXCENVptr env)
{
    if (native_CPXLversion == NULL)
        native_CPXLversion = load_symbol("CPXLversion");
    return native_CPXLversion(env);
}

// CPXLversionnumber
CPXLIBAPI int CPXPUBLIC CPXLversionnumber (CPXCENVptr env, int *version_p)
{
    if (native_CPXLversionnumber == NULL)
        native_CPXLversionnumber = load_symbol("CPXLversionnumber");
    return native_CPXLversionnumber(env, version_p);
}

// CPXLwriteparam
CPXLIBAPI int CPXPUBLIC CPXLwriteparam (CPXCENVptr env, char const *filename_str)
{
    if (native_CPXLwriteparam == NULL)
        native_CPXLwriteparam = load_symbol("CPXLwriteparam");
    return native_CPXLwriteparam(env, filename_str);
}

// CPXLwriteprob
CPXLIBAPI int CPXPUBLIC CPXLwriteprob (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, char const *filetype_str)
{
    if (native_CPXLwriteprob == NULL)
        native_CPXLwriteprob = load_symbol("CPXLwriteprob");
    return native_CPXLwriteprob(env, lp, filename_str, filetype_str);
}

// CPXLbaropt
CPXLIBAPI int CPXPUBLIC CPXLbaropt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLbaropt == NULL)
        native_CPXLbaropt = load_symbol("CPXLbaropt");
    return native_CPXLbaropt(env, lp);
}

// CPXLhybbaropt
CPXLIBAPI int CPXPUBLIC CPXLhybbaropt (CPXCENVptr env, CPXLPptr lp, int method)
{
    if (native_CPXLhybbaropt == NULL)
        native_CPXLhybbaropt = load_symbol("CPXLhybbaropt");
    return native_CPXLhybbaropt(env, lp, method);
}

// CPXLaddindconstr
CPXLIBAPI int CPXPUBLIC CPXLaddindconstr (CPXCENVptr env, CPXLPptr lp, CPXINT indvar, int complemented, CPXINT nzcnt, double rhs, int sense, CPXINT const *linind, double const *linval, char const *indname_str)
{
    if (native_CPXLaddindconstr == NULL)
        native_CPXLaddindconstr = load_symbol("CPXLaddindconstr");
    return native_CPXLaddindconstr(env, lp, indvar, complemented, nzcnt, rhs, sense, linind, linval, indname_str);
}

// CPXLchgqpcoef
CPXLIBAPI int CPXPUBLIC CPXLchgqpcoef (CPXCENVptr env, CPXLPptr lp, CPXINT i, CPXINT j, double newvalue)
{
    if (native_CPXLchgqpcoef == NULL)
        native_CPXLchgqpcoef = load_symbol("CPXLchgqpcoef");
    return native_CPXLchgqpcoef(env, lp, i, j, newvalue);
}

// CPXLcopyqpsep
CPXLIBAPI int CPXPUBLIC CPXLcopyqpsep (CPXCENVptr env, CPXLPptr lp, double const *qsepvec)
{
    if (native_CPXLcopyqpsep == NULL)
        native_CPXLcopyqpsep = load_symbol("CPXLcopyqpsep");
    return native_CPXLcopyqpsep(env, lp, qsepvec);
}

// CPXLcopyquad
CPXLIBAPI int CPXPUBLIC CPXLcopyquad (CPXCENVptr env, CPXLPptr lp, CPXLONG const *qmatbeg, CPXINT const *qmatcnt, CPXINT const *qmatind, double const *qmatval)
{
    if (native_CPXLcopyquad == NULL)
        native_CPXLcopyquad = load_symbol("CPXLcopyquad");
    return native_CPXLcopyquad(env, lp, qmatbeg, qmatcnt, qmatind, qmatval);
}

// CPXLgetnumqpnz
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnumqpnz (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumqpnz == NULL)
        native_CPXLgetnumqpnz = load_symbol("CPXLgetnumqpnz");
    return native_CPXLgetnumqpnz(env, lp);
}

// CPXLgetnumquad
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumquad (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumquad == NULL)
        native_CPXLgetnumquad = load_symbol("CPXLgetnumquad");
    return native_CPXLgetnumquad(env, lp);
}

// CPXLgetqpcoef
CPXLIBAPI int CPXPUBLIC CPXLgetqpcoef (CPXCENVptr env, CPXCLPptr lp, CPXINT rownum, CPXINT colnum, double *coef_p)
{
    if (native_CPXLgetqpcoef == NULL)
        native_CPXLgetqpcoef = load_symbol("CPXLgetqpcoef");
    return native_CPXLgetqpcoef(env, lp, rownum, colnum, coef_p);
}

// CPXLgetquad
CPXLIBAPI int CPXPUBLIC CPXLgetquad (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * qmatbeg, CPXINT * qmatind, double *qmatval, CPXLONG qmatspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetquad == NULL)
        native_CPXLgetquad = load_symbol("CPXLgetquad");
    return native_CPXLgetquad(env, lp, nzcnt_p, qmatbeg, qmatind, qmatval, qmatspace, surplus_p, begin, end);
}

// CPXLqpindefcertificate
CPXLIBAPI int CPXPUBLIC CPXLqpindefcertificate (CPXCENVptr env, CPXCLPptr lp, double *x)
{
    if (native_CPXLqpindefcertificate == NULL)
        native_CPXLqpindefcertificate = load_symbol("CPXLqpindefcertificate");
    return native_CPXLqpindefcertificate(env, lp, x);
}

// CPXLqpopt
CPXLIBAPI int CPXPUBLIC CPXLqpopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLqpopt == NULL)
        native_CPXLqpopt = load_symbol("CPXLqpopt");
    return native_CPXLqpopt(env, lp);
}

// CPXLaddqconstr
CPXLIBAPI int CPXPUBLIC CPXLaddqconstr (CPXCENVptr env, CPXLPptr lp, CPXINT linnzcnt, CPXLONG quadnzcnt, double rhs, int sense, CPXINT const *linind, double const *linval, CPXINT const *quadrow, CPXINT const *quadcol, double const *quadval, char const *lname_str)
{
    if (native_CPXLaddqconstr == NULL)
        native_CPXLaddqconstr = load_symbol("CPXLaddqconstr");
    return native_CPXLaddqconstr(env, lp, linnzcnt, quadnzcnt, rhs, sense, linind, linval, quadrow, quadcol, quadval, lname_str);
}

// CPXLdelqconstrs
CPXLIBAPI int CPXPUBLIC CPXLdelqconstrs (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end)
{
    if (native_CPXLdelqconstrs == NULL)
        native_CPXLdelqconstrs = load_symbol("CPXLdelqconstrs");
    return native_CPXLdelqconstrs(env, lp, begin, end);
}

// CPXLgetnumqconstrs
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumqconstrs (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumqconstrs == NULL)
        native_CPXLgetnumqconstrs = load_symbol("CPXLgetnumqconstrs");
    return native_CPXLgetnumqconstrs(env, lp);
}

// CPXLgetqconstr
CPXLIBAPI int CPXPUBLIC CPXLgetqconstr (CPXCENVptr env, CPXCLPptr lp, CPXINT * linnzcnt_p, CPXLONG * quadnzcnt_p, double *rhs_p, char *sense_p, CPXINT * linind, double *linval, CPXINT linspace, CPXINT * linsurplus_p, CPXINT * quadrow, CPXINT * quadcol, double *quadval, CPXLONG quadspace, CPXLONG * quadsurplus_p, CPXINT which)
{
    if (native_CPXLgetqconstr == NULL)
        native_CPXLgetqconstr = load_symbol("CPXLgetqconstr");
    return native_CPXLgetqconstr(env, lp, linnzcnt_p, quadnzcnt_p, rhs_p, sense_p, linind, linval, linspace, linsurplus_p, quadrow, quadcol, quadval, quadspace, quadsurplus_p, which);
}

// CPXLgetqconstrdslack
CPXLIBAPI int CPXPUBLIC CPXLgetqconstrdslack (CPXCENVptr env, CPXCLPptr lp, CPXINT qind, CPXINT * nz_p, CPXINT * ind, double *val, CPXINT space, CPXINT * surplus_p)
{
    if (native_CPXLgetqconstrdslack == NULL)
        native_CPXLgetqconstrdslack = load_symbol("CPXLgetqconstrdslack");
    return native_CPXLgetqconstrdslack(env, lp, qind, nz_p, ind, val, space, surplus_p);
}

// CPXLgetqconstrindex
CPXLIBAPI int CPXPUBLIC CPXLgetqconstrindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p)
{
    if (native_CPXLgetqconstrindex == NULL)
        native_CPXLgetqconstrindex = load_symbol("CPXLgetqconstrindex");
    return native_CPXLgetqconstrindex(env, lp, lname_str, index_p);
}

// CPXLgetqconstrinfeas
CPXLIBAPI int CPXPUBLIC CPXLgetqconstrinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetqconstrinfeas == NULL)
        native_CPXLgetqconstrinfeas = load_symbol("CPXLgetqconstrinfeas");
    return native_CPXLgetqconstrinfeas(env, lp, x, infeasout, begin, end);
}

// CPXLgetqconstrname
CPXLIBAPI int CPXPUBLIC CPXLgetqconstrname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p, CPXINT which)
{
    if (native_CPXLgetqconstrname == NULL)
        native_CPXLgetqconstrname = load_symbol("CPXLgetqconstrname");
    return native_CPXLgetqconstrname(env, lp, buf_str, bufspace, surplus_p, which);
}

// CPXLgetqconstrslack
CPXLIBAPI int CPXPUBLIC CPXLgetqconstrslack (CPXCENVptr env, CPXCLPptr lp, double *qcslack, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetqconstrslack == NULL)
        native_CPXLgetqconstrslack = load_symbol("CPXLgetqconstrslack");
    return native_CPXLgetqconstrslack(env, lp, qcslack, begin, end);
}

// CPXLgetxqxax
CPXLIBAPI int CPXPUBLIC CPXLgetxqxax (CPXCENVptr env, CPXCLPptr lp, double *xqxax, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetxqxax == NULL)
        native_CPXLgetxqxax = load_symbol("CPXLgetxqxax");
    return native_CPXLgetxqxax(env, lp, xqxax, begin, end);
}

// CPXLqconstrslackfromx
CPXLIBAPI int CPXPUBLIC CPXLqconstrslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *qcslack)
{
    if (native_CPXLqconstrslackfromx == NULL)
        native_CPXLqconstrslackfromx = load_symbol("CPXLqconstrslackfromx");
    return native_CPXLqconstrslackfromx(env, lp, x, qcslack);
}

// CPXLNETaddarcs
CPXLIBAPI int CPXPUBLIC CPXLNETaddarcs (CPXCENVptr env, CPXNETptr net, CPXINT narcs, CPXINT const *fromnode, CPXINT const *tonode, double const *low, double const *up, double const *obj, char const *const *anames)
{
    if (native_CPXLNETaddarcs == NULL)
        native_CPXLNETaddarcs = load_symbol("CPXLNETaddarcs");
    return native_CPXLNETaddarcs(env, net, narcs, fromnode, tonode, low, up, obj, anames);
}

// CPXLNETaddnodes
CPXLIBAPI int CPXPUBLIC CPXLNETaddnodes (CPXCENVptr env, CPXNETptr net, CPXINT nnodes, double const *supply, char const *const *name)
{
    if (native_CPXLNETaddnodes == NULL)
        native_CPXLNETaddnodes = load_symbol("CPXLNETaddnodes");
    return native_CPXLNETaddnodes(env, net, nnodes, supply, name);
}

// CPXLNETbasewrite
CPXLIBAPI int CPXPUBLIC CPXLNETbasewrite (CPXCENVptr env, CPXCNETptr net, char const *filename_str)
{
    if (native_CPXLNETbasewrite == NULL)
        native_CPXLNETbasewrite = load_symbol("CPXLNETbasewrite");
    return native_CPXLNETbasewrite(env, net, filename_str);
}

// CPXLNETchgarcname
CPXLIBAPI int CPXPUBLIC CPXLNETchgarcname (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, char const *const *newname)
{
    if (native_CPXLNETchgarcname == NULL)
        native_CPXLNETchgarcname = load_symbol("CPXLNETchgarcname");
    return native_CPXLNETchgarcname(env, net, cnt, indices, newname);
}

// CPXLNETchgarcnodes
CPXLIBAPI int CPXPUBLIC CPXLNETchgarcnodes (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, CPXINT const *fromnode, CPXINT const *tonode)
{
    if (native_CPXLNETchgarcnodes == NULL)
        native_CPXLNETchgarcnodes = load_symbol("CPXLNETchgarcnodes");
    return native_CPXLNETchgarcnodes(env, net, cnt, indices, fromnode, tonode);
}

// CPXLNETchgbds
CPXLIBAPI int CPXPUBLIC CPXLNETchgbds (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd)
{
    if (native_CPXLNETchgbds == NULL)
        native_CPXLNETchgbds = load_symbol("CPXLNETchgbds");
    return native_CPXLNETchgbds(env, net, cnt, indices, lu, bd);
}

// CPXLNETchgname
CPXLIBAPI int CPXPUBLIC CPXLNETchgname (CPXCENVptr env, CPXNETptr net, int key, CPXINT vindex, char const *name_str)
{
    if (native_CPXLNETchgname == NULL)
        native_CPXLNETchgname = load_symbol("CPXLNETchgname");
    return native_CPXLNETchgname(env, net, key, vindex, name_str);
}

// CPXLNETchgnodename
CPXLIBAPI int CPXPUBLIC CPXLNETchgnodename (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, char const *const *newname)
{
    if (native_CPXLNETchgnodename == NULL)
        native_CPXLNETchgnodename = load_symbol("CPXLNETchgnodename");
    return native_CPXLNETchgnodename(env, net, cnt, indices, newname);
}

// CPXLNETchgobj
CPXLIBAPI int CPXPUBLIC CPXLNETchgobj (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, double const *obj)
{
    if (native_CPXLNETchgobj == NULL)
        native_CPXLNETchgobj = load_symbol("CPXLNETchgobj");
    return native_CPXLNETchgobj(env, net, cnt, indices, obj);
}

// CPXLNETchgobjsen
CPXLIBAPI int CPXPUBLIC CPXLNETchgobjsen (CPXCENVptr env, CPXNETptr net, int maxormin)
{
    if (native_CPXLNETchgobjsen == NULL)
        native_CPXLNETchgobjsen = load_symbol("CPXLNETchgobjsen");
    return native_CPXLNETchgobjsen(env, net, maxormin);
}

// CPXLNETchgsupply
CPXLIBAPI int CPXPUBLIC CPXLNETchgsupply (CPXCENVptr env, CPXNETptr net, CPXINT cnt, CPXINT const *indices, double const *supply)
{
    if (native_CPXLNETchgsupply == NULL)
        native_CPXLNETchgsupply = load_symbol("CPXLNETchgsupply");
    return native_CPXLNETchgsupply(env, net, cnt, indices, supply);
}

// CPXLNETcopybase
CPXLIBAPI int CPXPUBLIC CPXLNETcopybase (CPXCENVptr env, CPXNETptr net, int const *astat, int const *nstat)
{
    if (native_CPXLNETcopybase == NULL)
        native_CPXLNETcopybase = load_symbol("CPXLNETcopybase");
    return native_CPXLNETcopybase(env, net, astat, nstat);
}

// CPXLNETcopynet
CPXLIBAPI int CPXPUBLIC CPXLNETcopynet (CPXCENVptr env, CPXNETptr net, int objsen, CPXINT nnodes, double const *supply, char const *const *nnames, CPXINT narcs, CPXINT const *fromnode, CPXINT const *tonode, double const *low, double const *up, double const *obj, char const *const *anames)
{
    if (native_CPXLNETcopynet == NULL)
        native_CPXLNETcopynet = load_symbol("CPXLNETcopynet");
    return native_CPXLNETcopynet(env, net, objsen, nnodes, supply, nnames, narcs, fromnode, tonode, low, up, obj, anames);
}

// CPXLNETcreateprob
CPXLIBAPI CPXNETptr CPXPUBLIC CPXLNETcreateprob (CPXENVptr env, int *status_p, char const *name_str)
{
    if (native_CPXLNETcreateprob == NULL)
        native_CPXLNETcreateprob = load_symbol("CPXLNETcreateprob");
    return native_CPXLNETcreateprob(env, status_p, name_str);
}

// CPXLNETdelarcs
CPXLIBAPI int CPXPUBLIC CPXLNETdelarcs (CPXCENVptr env, CPXNETptr net, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETdelarcs == NULL)
        native_CPXLNETdelarcs = load_symbol("CPXLNETdelarcs");
    return native_CPXLNETdelarcs(env, net, begin, end);
}

// CPXLNETdelnodes
CPXLIBAPI int CPXPUBLIC CPXLNETdelnodes (CPXCENVptr env, CPXNETptr net, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETdelnodes == NULL)
        native_CPXLNETdelnodes = load_symbol("CPXLNETdelnodes");
    return native_CPXLNETdelnodes(env, net, begin, end);
}

// CPXLNETdelset
CPXLIBAPI int CPXPUBLIC CPXLNETdelset (CPXCENVptr env, CPXNETptr net, CPXINT * whichnodes, CPXINT * whicharcs)
{
    if (native_CPXLNETdelset == NULL)
        native_CPXLNETdelset = load_symbol("CPXLNETdelset");
    return native_CPXLNETdelset(env, net, whichnodes, whicharcs);
}

// CPXLNETfreeprob
CPXLIBAPI int CPXPUBLIC CPXLNETfreeprob (CPXENVptr env, CPXNETptr * net_p)
{
    if (native_CPXLNETfreeprob == NULL)
        native_CPXLNETfreeprob = load_symbol("CPXLNETfreeprob");
    return native_CPXLNETfreeprob(env, net_p);
}

// CPXLNETgetarcindex
CPXLIBAPI int CPXPUBLIC CPXLNETgetarcindex (CPXCENVptr env, CPXCNETptr net, char const *lname_str, CPXINT * index_p)
{
    if (native_CPXLNETgetarcindex == NULL)
        native_CPXLNETgetarcindex = load_symbol("CPXLNETgetarcindex");
    return native_CPXLNETgetarcindex(env, net, lname_str, index_p);
}

// CPXLNETgetarcname
CPXLIBAPI int CPXPUBLIC CPXLNETgetarcname (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, CPXSIZE namespc, CPXSIZE * surplus_p, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetarcname == NULL)
        native_CPXLNETgetarcname = load_symbol("CPXLNETgetarcname");
    return native_CPXLNETgetarcname(env, net, nnames, namestore, namespc, surplus_p, begin, end);
}

// CPXLNETgetarcnodes
CPXLIBAPI int CPXPUBLIC CPXLNETgetarcnodes (CPXCENVptr env, CPXCNETptr net, CPXINT * fromnode, CPXINT * tonode, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetarcnodes == NULL)
        native_CPXLNETgetarcnodes = load_symbol("CPXLNETgetarcnodes");
    return native_CPXLNETgetarcnodes(env, net, fromnode, tonode, begin, end);
}

// CPXLNETgetbase
CPXLIBAPI int CPXPUBLIC CPXLNETgetbase (CPXCENVptr env, CPXCNETptr net, int *astat, int *nstat)
{
    if (native_CPXLNETgetbase == NULL)
        native_CPXLNETgetbase = load_symbol("CPXLNETgetbase");
    return native_CPXLNETgetbase(env, net, astat, nstat);
}

// CPXLNETgetdj
CPXLIBAPI int CPXPUBLIC CPXLNETgetdj (CPXCENVptr env, CPXCNETptr net, double *dj, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetdj == NULL)
        native_CPXLNETgetdj = load_symbol("CPXLNETgetdj");
    return native_CPXLNETgetdj(env, net, dj, begin, end);
}

// CPXLNETgetitcnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLNETgetitcnt (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXLNETgetitcnt == NULL)
        native_CPXLNETgetitcnt = load_symbol("CPXLNETgetitcnt");
    return native_CPXLNETgetitcnt(env, net);
}

// CPXLNETgetlb
CPXLIBAPI int CPXPUBLIC CPXLNETgetlb (CPXCENVptr env, CPXCNETptr net, double *low, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetlb == NULL)
        native_CPXLNETgetlb = load_symbol("CPXLNETgetlb");
    return native_CPXLNETgetlb(env, net, low, begin, end);
}

// CPXLNETgetnodearcs
CPXLIBAPI int CPXPUBLIC CPXLNETgetnodearcs (CPXCENVptr env, CPXCNETptr net, CPXINT * arccnt_p, CPXINT * arcbeg, CPXINT * arc, CPXINT arcspace, CPXINT * surplus_p, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetnodearcs == NULL)
        native_CPXLNETgetnodearcs = load_symbol("CPXLNETgetnodearcs");
    return native_CPXLNETgetnodearcs(env, net, arccnt_p, arcbeg, arc, arcspace, surplus_p, begin, end);
}

// CPXLNETgetnodeindex
CPXLIBAPI int CPXPUBLIC CPXLNETgetnodeindex (CPXCENVptr env, CPXCNETptr net, char const *lname_str, CPXINT * index_p)
{
    if (native_CPXLNETgetnodeindex == NULL)
        native_CPXLNETgetnodeindex = load_symbol("CPXLNETgetnodeindex");
    return native_CPXLNETgetnodeindex(env, net, lname_str, index_p);
}

// CPXLNETgetnodename
CPXLIBAPI int CPXPUBLIC CPXLNETgetnodename (CPXCENVptr env, CPXCNETptr net, char **nnames, char *namestore, CPXSIZE namespc, CPXSIZE * surplus_p, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetnodename == NULL)
        native_CPXLNETgetnodename = load_symbol("CPXLNETgetnodename");
    return native_CPXLNETgetnodename(env, net, nnames, namestore, namespc, surplus_p, begin, end);
}

// CPXLNETgetnumarcs
CPXLIBAPI CPXINT CPXPUBLIC CPXLNETgetnumarcs (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXLNETgetnumarcs == NULL)
        native_CPXLNETgetnumarcs = load_symbol("CPXLNETgetnumarcs");
    return native_CPXLNETgetnumarcs(env, net);
}

// CPXLNETgetnumnodes
CPXLIBAPI CPXINT CPXPUBLIC CPXLNETgetnumnodes (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXLNETgetnumnodes == NULL)
        native_CPXLNETgetnumnodes = load_symbol("CPXLNETgetnumnodes");
    return native_CPXLNETgetnumnodes(env, net);
}

// CPXLNETgetobj
CPXLIBAPI int CPXPUBLIC CPXLNETgetobj (CPXCENVptr env, CPXCNETptr net, double *obj, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetobj == NULL)
        native_CPXLNETgetobj = load_symbol("CPXLNETgetobj");
    return native_CPXLNETgetobj(env, net, obj, begin, end);
}

// CPXLNETgetobjsen
CPXLIBAPI int CPXPUBLIC CPXLNETgetobjsen (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXLNETgetobjsen == NULL)
        native_CPXLNETgetobjsen = load_symbol("CPXLNETgetobjsen");
    return native_CPXLNETgetobjsen(env, net);
}

// CPXLNETgetobjval
CPXLIBAPI int CPXPUBLIC CPXLNETgetobjval (CPXCENVptr env, CPXCNETptr net, double *objval_p)
{
    if (native_CPXLNETgetobjval == NULL)
        native_CPXLNETgetobjval = load_symbol("CPXLNETgetobjval");
    return native_CPXLNETgetobjval(env, net, objval_p);
}

// CPXLNETgetphase1cnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLNETgetphase1cnt (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXLNETgetphase1cnt == NULL)
        native_CPXLNETgetphase1cnt = load_symbol("CPXLNETgetphase1cnt");
    return native_CPXLNETgetphase1cnt(env, net);
}

// CPXLNETgetpi
CPXLIBAPI int CPXPUBLIC CPXLNETgetpi (CPXCENVptr env, CPXCNETptr net, double *pi, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetpi == NULL)
        native_CPXLNETgetpi = load_symbol("CPXLNETgetpi");
    return native_CPXLNETgetpi(env, net, pi, begin, end);
}

// CPXLNETgetprobname
CPXLIBAPI int CPXPUBLIC CPXLNETgetprobname (CPXCENVptr env, CPXCNETptr net, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p)
{
    if (native_CPXLNETgetprobname == NULL)
        native_CPXLNETgetprobname = load_symbol("CPXLNETgetprobname");
    return native_CPXLNETgetprobname(env, net, buf_str, bufspace, surplus_p);
}

// CPXLNETgetslack
CPXLIBAPI int CPXPUBLIC CPXLNETgetslack (CPXCENVptr env, CPXCNETptr net, double *slack, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetslack == NULL)
        native_CPXLNETgetslack = load_symbol("CPXLNETgetslack");
    return native_CPXLNETgetslack(env, net, slack, begin, end);
}

// CPXLNETgetstat
CPXLIBAPI int CPXPUBLIC CPXLNETgetstat (CPXCENVptr env, CPXCNETptr net)
{
    if (native_CPXLNETgetstat == NULL)
        native_CPXLNETgetstat = load_symbol("CPXLNETgetstat");
    return native_CPXLNETgetstat(env, net);
}

// CPXLNETgetsupply
CPXLIBAPI int CPXPUBLIC CPXLNETgetsupply (CPXCENVptr env, CPXCNETptr net, double *supply, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetsupply == NULL)
        native_CPXLNETgetsupply = load_symbol("CPXLNETgetsupply");
    return native_CPXLNETgetsupply(env, net, supply, begin, end);
}

// CPXLNETgetub
CPXLIBAPI int CPXPUBLIC CPXLNETgetub (CPXCENVptr env, CPXCNETptr net, double *up, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetub == NULL)
        native_CPXLNETgetub = load_symbol("CPXLNETgetub");
    return native_CPXLNETgetub(env, net, up, begin, end);
}

// CPXLNETgetx
CPXLIBAPI int CPXPUBLIC CPXLNETgetx (CPXCENVptr env, CPXCNETptr net, double *x, CPXINT begin, CPXINT end)
{
    if (native_CPXLNETgetx == NULL)
        native_CPXLNETgetx = load_symbol("CPXLNETgetx");
    return native_CPXLNETgetx(env, net, x, begin, end);
}

// CPXLNETprimopt
CPXLIBAPI int CPXPUBLIC CPXLNETprimopt (CPXCENVptr env, CPXNETptr net)
{
    if (native_CPXLNETprimopt == NULL)
        native_CPXLNETprimopt = load_symbol("CPXLNETprimopt");
    return native_CPXLNETprimopt(env, net);
}

// CPXLNETreadcopybase
CPXLIBAPI int CPXPUBLIC CPXLNETreadcopybase (CPXCENVptr env, CPXNETptr net, char const *filename_str)
{
    if (native_CPXLNETreadcopybase == NULL)
        native_CPXLNETreadcopybase = load_symbol("CPXLNETreadcopybase");
    return native_CPXLNETreadcopybase(env, net, filename_str);
}

// CPXLNETreadcopyprob
CPXLIBAPI int CPXPUBLIC CPXLNETreadcopyprob (CPXCENVptr env, CPXNETptr net, char const *filename_str)
{
    if (native_CPXLNETreadcopyprob == NULL)
        native_CPXLNETreadcopyprob = load_symbol("CPXLNETreadcopyprob");
    return native_CPXLNETreadcopyprob(env, net, filename_str);
}

// CPXLNETsolninfo
CPXLIBAPI int CPXPUBLIC CPXLNETsolninfo (CPXCENVptr env, CPXCNETptr net, int *pfeasind_p, int *dfeasind_p)
{
    if (native_CPXLNETsolninfo == NULL)
        native_CPXLNETsolninfo = load_symbol("CPXLNETsolninfo");
    return native_CPXLNETsolninfo(env, net, pfeasind_p, dfeasind_p);
}

// CPXLNETsolution
CPXLIBAPI int CPXPUBLIC CPXLNETsolution (CPXCENVptr env, CPXCNETptr net, int *netstat_p, double *objval_p, double *x, double *pi, double *slack, double *dj)
{
    if (native_CPXLNETsolution == NULL)
        native_CPXLNETsolution = load_symbol("CPXLNETsolution");
    return native_CPXLNETsolution(env, net, netstat_p, objval_p, x, pi, slack, dj);
}

// CPXLNETwriteprob
CPXLIBAPI int CPXPUBLIC CPXLNETwriteprob (CPXCENVptr env, CPXCNETptr net, char const *filename_str, char const *format_str)
{
    if (native_CPXLNETwriteprob == NULL)
        native_CPXLNETwriteprob = load_symbol("CPXLNETwriteprob");
    return native_CPXLNETwriteprob(env, net, filename_str, format_str);
}

// CPXLaddlazyconstraints
CPXLIBAPI int CPXPUBLIC CPXLaddlazyconstraints (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *rowname)
{
    if (native_CPXLaddlazyconstraints == NULL)
        native_CPXLaddlazyconstraints = load_symbol("CPXLaddlazyconstraints");
    return native_CPXLaddlazyconstraints(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}

// CPXLaddmipstarts
CPXLIBAPI int CPXPUBLIC CPXLaddmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt, CPXLONG nzcnt, CPXLONG const *beg, CPXINT const *varindices, double const *values, int const *effortlevel, char const *const *mipstartname)
{
    if (native_CPXLaddmipstarts == NULL)
        native_CPXLaddmipstarts = load_symbol("CPXLaddmipstarts");
    return native_CPXLaddmipstarts(env, lp, mcnt, nzcnt, beg, varindices, values, effortlevel, mipstartname);
}

// CPXLaddsolnpooldivfilter
CPXLIBAPI int CPXPUBLIC CPXLaddsolnpooldivfilter (CPXCENVptr env, CPXLPptr lp, double lower_bound, double upper_bound, CPXINT nzcnt, CPXINT const *ind, double const *weight, double const *refval, char const *lname_str)
{
    if (native_CPXLaddsolnpooldivfilter == NULL)
        native_CPXLaddsolnpooldivfilter = load_symbol("CPXLaddsolnpooldivfilter");
    return native_CPXLaddsolnpooldivfilter(env, lp, lower_bound, upper_bound, nzcnt, ind, weight, refval, lname_str);
}

// CPXLaddsolnpoolrngfilter
CPXLIBAPI int CPXPUBLIC CPXLaddsolnpoolrngfilter (CPXCENVptr env, CPXLPptr lp, double lb, double ub, CPXINT nzcnt, CPXINT const *ind, double const *val, char const *lname_str)
{
    if (native_CPXLaddsolnpoolrngfilter == NULL)
        native_CPXLaddsolnpoolrngfilter = load_symbol("CPXLaddsolnpoolrngfilter");
    return native_CPXLaddsolnpoolrngfilter(env, lp, lb, ub, nzcnt, ind, val, lname_str);
}

// CPXLaddsos
CPXLIBAPI int CPXPUBLIC CPXLaddsos (CPXCENVptr env, CPXLPptr lp, CPXINT numsos, CPXLONG numsosnz, char const *sostype, CPXLONG const *sosbeg, CPXINT const *sosind, double const *soswt, char const *const *sosname)
{
    if (native_CPXLaddsos == NULL)
        native_CPXLaddsos = load_symbol("CPXLaddsos");
    return native_CPXLaddsos(env, lp, numsos, numsosnz, sostype, sosbeg, sosind, soswt, sosname);
}

// CPXLaddusercuts
CPXLIBAPI int CPXPUBLIC CPXLaddusercuts (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, char const *const *rowname)
{
    if (native_CPXLaddusercuts == NULL)
        native_CPXLaddusercuts = load_symbol("CPXLaddusercuts");
    return native_CPXLaddusercuts(env, lp, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, rowname);
}

// CPXLbranchcallbackbranchasCPLEX
CPXLIBAPI int CPXPUBLIC CPXLbranchcallbackbranchasCPLEX (CPXCENVptr env, void *cbdata, int wherefrom, int num, void *userhandle, CPXLONG * seqnum_p)
{
    if (native_CPXLbranchcallbackbranchasCPLEX == NULL)
        native_CPXLbranchcallbackbranchasCPLEX = load_symbol("CPXLbranchcallbackbranchasCPLEX");
    return native_CPXLbranchcallbackbranchasCPLEX(env, cbdata, wherefrom, num, userhandle, seqnum_p);
}

// CPXLbranchcallbackbranchbds
CPXLIBAPI int CPXPUBLIC CPXLbranchcallbackbranchbds (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT cnt, CPXINT const *indices, char const *lu, double const *bd, double nodeest, void *userhandle, CPXLONG * seqnum_p)
{
    if (native_CPXLbranchcallbackbranchbds == NULL)
        native_CPXLbranchcallbackbranchbds = load_symbol("CPXLbranchcallbackbranchbds");
    return native_CPXLbranchcallbackbranchbds(env, cbdata, wherefrom, cnt, indices, lu, bd, nodeest, userhandle, seqnum_p);
}

// CPXLbranchcallbackbranchconstraints
CPXLIBAPI int CPXPUBLIC CPXLbranchcallbackbranchconstraints (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, double nodeest, void *userhandle, CPXLONG * seqnum_p)
{
    if (native_CPXLbranchcallbackbranchconstraints == NULL)
        native_CPXLbranchcallbackbranchconstraints = load_symbol("CPXLbranchcallbackbranchconstraints");
    return native_CPXLbranchcallbackbranchconstraints(env, cbdata, wherefrom, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, nodeest, userhandle, seqnum_p);
}

// CPXLbranchcallbackbranchgeneral
CPXLIBAPI int CPXPUBLIC CPXLbranchcallbackbranchgeneral (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT varcnt, CPXINT const *varind, char const *varlu, double const *varbd, CPXINT rcnt, CPXLONG nzcnt, double const *rhs, char const *sense, CPXLONG const *rmatbeg, CPXINT const *rmatind, double const *rmatval, double nodeest, void *userhandle, CPXLONG * seqnum_p)
{
    if (native_CPXLbranchcallbackbranchgeneral == NULL)
        native_CPXLbranchcallbackbranchgeneral = load_symbol("CPXLbranchcallbackbranchgeneral");
    return native_CPXLbranchcallbackbranchgeneral(env, cbdata, wherefrom, varcnt, varind, varlu, varbd, rcnt, nzcnt, rhs, sense, rmatbeg, rmatind, rmatval, nodeest, userhandle, seqnum_p);
}

// CPXLcallbacksetnodeuserhandle
CPXLIBAPI int CPXPUBLIC CPXLcallbacksetnodeuserhandle (CPXCENVptr env, void *cbdata, int wherefrom, CPXLONG nodeindex, void *userhandle, void **olduserhandle_p)
{
    if (native_CPXLcallbacksetnodeuserhandle == NULL)
        native_CPXLcallbacksetnodeuserhandle = load_symbol("CPXLcallbacksetnodeuserhandle");
    return native_CPXLcallbacksetnodeuserhandle(env, cbdata, wherefrom, nodeindex, userhandle, olduserhandle_p);
}

// CPXLcallbacksetuserhandle
CPXLIBAPI int CPXPUBLIC CPXLcallbacksetuserhandle (CPXCENVptr env, void *cbdata, int wherefrom, void *userhandle, void **olduserhandle_p)
{
    if (native_CPXLcallbacksetuserhandle == NULL)
        native_CPXLcallbacksetuserhandle = load_symbol("CPXLcallbacksetuserhandle");
    return native_CPXLcallbacksetuserhandle(env, cbdata, wherefrom, userhandle, olduserhandle_p);
}

// CPXLchgctype
CPXLIBAPI int CPXPUBLIC CPXLchgctype (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, char const *xctype)
{
    if (native_CPXLchgctype == NULL)
        native_CPXLchgctype = load_symbol("CPXLchgctype");
    return native_CPXLchgctype(env, lp, cnt, indices, xctype);
}

// CPXLchgmipstarts
CPXLIBAPI int CPXPUBLIC CPXLchgmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt, int const *mipstartindices, CPXLONG nzcnt, CPXLONG const *beg, CPXINT const *varindices, double const *values, int const *effortlevel)
{
    if (native_CPXLchgmipstarts == NULL)
        native_CPXLchgmipstarts = load_symbol("CPXLchgmipstarts");
    return native_CPXLchgmipstarts(env, lp, mcnt, mipstartindices, nzcnt, beg, varindices, values, effortlevel);
}

// CPXLcopyctype
CPXLIBAPI int CPXPUBLIC CPXLcopyctype (CPXCENVptr env, CPXLPptr lp, char const *xctype)
{
    if (native_CPXLcopyctype == NULL)
        native_CPXLcopyctype = load_symbol("CPXLcopyctype");
    return native_CPXLcopyctype(env, lp, xctype);
}

// CPXLcopyorder
CPXLIBAPI int CPXPUBLIC CPXLcopyorder (CPXCENVptr env, CPXLPptr lp, CPXINT cnt, CPXINT const *indices, CPXINT const *priority, int const *direction)
{
    if (native_CPXLcopyorder == NULL)
        native_CPXLcopyorder = load_symbol("CPXLcopyorder");
    return native_CPXLcopyorder(env, lp, cnt, indices, priority, direction);
}

// CPXLcopysos
CPXLIBAPI int CPXPUBLIC CPXLcopysos (CPXCENVptr env, CPXLPptr lp, CPXINT numsos, CPXLONG numsosnz, char const *sostype, CPXLONG const *sosbeg, CPXINT const *sosind, double const *soswt, char const *const *sosname)
{
    if (native_CPXLcopysos == NULL)
        native_CPXLcopysos = load_symbol("CPXLcopysos");
    return native_CPXLcopysos(env, lp, numsos, numsosnz, sostype, sosbeg, sosind, soswt, sosname);
}

// CPXLcutcallbackadd
CPXLIBAPI int CPXPUBLIC CPXLcutcallbackadd (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT nzcnt, double rhs, int sense, CPXINT const *cutind, double const *cutval, int purgeable)
{
    if (native_CPXLcutcallbackadd == NULL)
        native_CPXLcutcallbackadd = load_symbol("CPXLcutcallbackadd");
    return native_CPXLcutcallbackadd(env, cbdata, wherefrom, nzcnt, rhs, sense, cutind, cutval, purgeable);
}

// CPXLcutcallbackaddlocal
CPXLIBAPI int CPXPUBLIC CPXLcutcallbackaddlocal (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT nzcnt, double rhs, int sense, CPXINT const *cutind, double const *cutval)
{
    if (native_CPXLcutcallbackaddlocal == NULL)
        native_CPXLcutcallbackaddlocal = load_symbol("CPXLcutcallbackaddlocal");
    return native_CPXLcutcallbackaddlocal(env, cbdata, wherefrom, nzcnt, rhs, sense, cutind, cutval);
}

// CPXLdelindconstrs
CPXLIBAPI int CPXPUBLIC CPXLdelindconstrs (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end)
{
    if (native_CPXLdelindconstrs == NULL)
        native_CPXLdelindconstrs = load_symbol("CPXLdelindconstrs");
    return native_CPXLdelindconstrs(env, lp, begin, end);
}

// CPXLdelmipstarts
CPXLIBAPI int CPXPUBLIC CPXLdelmipstarts (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXLdelmipstarts == NULL)
        native_CPXLdelmipstarts = load_symbol("CPXLdelmipstarts");
    return native_CPXLdelmipstarts(env, lp, begin, end);
}

// CPXLdelsetmipstarts
CPXLIBAPI int CPXPUBLIC CPXLdelsetmipstarts (CPXCENVptr env, CPXLPptr lp, int *delstat)
{
    if (native_CPXLdelsetmipstarts == NULL)
        native_CPXLdelsetmipstarts = load_symbol("CPXLdelsetmipstarts");
    return native_CPXLdelsetmipstarts(env, lp, delstat);
}

// CPXLdelsetsolnpoolfilters
CPXLIBAPI int CPXPUBLIC CPXLdelsetsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int *delstat)
{
    if (native_CPXLdelsetsolnpoolfilters == NULL)
        native_CPXLdelsetsolnpoolfilters = load_symbol("CPXLdelsetsolnpoolfilters");
    return native_CPXLdelsetsolnpoolfilters(env, lp, delstat);
}

// CPXLdelsetsolnpoolsolns
CPXLIBAPI int CPXPUBLIC CPXLdelsetsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int *delstat)
{
    if (native_CPXLdelsetsolnpoolsolns == NULL)
        native_CPXLdelsetsolnpoolsolns = load_symbol("CPXLdelsetsolnpoolsolns");
    return native_CPXLdelsetsolnpoolsolns(env, lp, delstat);
}

// CPXLdelsetsos
CPXLIBAPI int CPXPUBLIC CPXLdelsetsos (CPXCENVptr env, CPXLPptr lp, CPXINT * delset)
{
    if (native_CPXLdelsetsos == NULL)
        native_CPXLdelsetsos = load_symbol("CPXLdelsetsos");
    return native_CPXLdelsetsos(env, lp, delset);
}

// CPXLdelsolnpoolfilters
CPXLIBAPI int CPXPUBLIC CPXLdelsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXLdelsolnpoolfilters == NULL)
        native_CPXLdelsolnpoolfilters = load_symbol("CPXLdelsolnpoolfilters");
    return native_CPXLdelsolnpoolfilters(env, lp, begin, end);
}

// CPXLdelsolnpoolsolns
CPXLIBAPI int CPXPUBLIC CPXLdelsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int begin, int end)
{
    if (native_CPXLdelsolnpoolsolns == NULL)
        native_CPXLdelsolnpoolsolns = load_symbol("CPXLdelsolnpoolsolns");
    return native_CPXLdelsolnpoolsolns(env, lp, begin, end);
}

// CPXLdelsos
CPXLIBAPI int CPXPUBLIC CPXLdelsos (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end)
{
    if (native_CPXLdelsos == NULL)
        native_CPXLdelsos = load_symbol("CPXLdelsos");
    return native_CPXLdelsos(env, lp, begin, end);
}

// CPXLfltwrite
CPXLIBAPI int CPXPUBLIC CPXLfltwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLfltwrite == NULL)
        native_CPXLfltwrite = load_symbol("CPXLfltwrite");
    return native_CPXLfltwrite(env, lp, filename_str);
}

// CPXLfreelazyconstraints
CPXLIBAPI int CPXPUBLIC CPXLfreelazyconstraints (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLfreelazyconstraints == NULL)
        native_CPXLfreelazyconstraints = load_symbol("CPXLfreelazyconstraints");
    return native_CPXLfreelazyconstraints(env, lp);
}

// CPXLfreeusercuts
CPXLIBAPI int CPXPUBLIC CPXLfreeusercuts (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLfreeusercuts == NULL)
        native_CPXLfreeusercuts = load_symbol("CPXLfreeusercuts");
    return native_CPXLfreeusercuts(env, lp);
}

// CPXLgetbestobjval
CPXLIBAPI int CPXPUBLIC CPXLgetbestobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p)
{
    if (native_CPXLgetbestobjval == NULL)
        native_CPXLgetbestobjval = load_symbol("CPXLgetbestobjval");
    return native_CPXLgetbestobjval(env, lp, objval_p);
}

// CPXLgetbranchcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetbranchcallbackfunc (CPXCENVptr env, CPXL_CALLBACK_BRANCH ** branchcallback_p, void **cbhandle_p)
{
    if (native_CPXLgetbranchcallbackfunc == NULL)
        native_CPXLgetbranchcallbackfunc = load_symbol("CPXLgetbranchcallbackfunc");
    return native_CPXLgetbranchcallbackfunc(env, branchcallback_p, cbhandle_p);
}

// CPXLgetbranchnosolncallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetbranchnosolncallbackfunc (CPXCENVptr env, CPXL_CALLBACK_BRANCH ** branchnosolncallback_p, void **cbhandle_p)
{
    if (native_CPXLgetbranchnosolncallbackfunc == NULL)
        native_CPXLgetbranchnosolncallbackfunc = load_symbol("CPXLgetbranchnosolncallbackfunc");
    return native_CPXLgetbranchnosolncallbackfunc(env, branchnosolncallback_p, cbhandle_p);
}

// CPXLgetcallbackbranchconstraints
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackbranchconstraints (CPXCENVptr env, void *cbdata, int wherefrom, int which, CPXINT * cuts_p, CPXLONG * nzcnt_p, double *rhs, char *sense, CPXLONG * rmatbeg, CPXINT * rmatind, double *rmatval, CPXLONG rmatsz, CPXLONG * surplus_p)
{
    if (native_CPXLgetcallbackbranchconstraints == NULL)
        native_CPXLgetcallbackbranchconstraints = load_symbol("CPXLgetcallbackbranchconstraints");
    return native_CPXLgetcallbackbranchconstraints(env, cbdata, wherefrom, which, cuts_p, nzcnt_p, rhs, sense, rmatbeg, rmatind, rmatval, rmatsz, surplus_p);
}

// CPXLgetcallbackctype
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackctype (CPXCENVptr env, void *cbdata, int wherefrom, char *xctype, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcallbackctype == NULL)
        native_CPXLgetcallbackctype = load_symbol("CPXLgetcallbackctype");
    return native_CPXLgetcallbackctype(env, cbdata, wherefrom, xctype, begin, end);
}

// CPXLgetcallbackgloballb
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackgloballb (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcallbackgloballb == NULL)
        native_CPXLgetcallbackgloballb = load_symbol("CPXLgetcallbackgloballb");
    return native_CPXLgetcallbackgloballb(env, cbdata, wherefrom, lb, begin, end);
}

// CPXLgetcallbackglobalub
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackglobalub (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcallbackglobalub == NULL)
        native_CPXLgetcallbackglobalub = load_symbol("CPXLgetcallbackglobalub");
    return native_CPXLgetcallbackglobalub(env, cbdata, wherefrom, ub, begin, end);
}

// CPXLgetcallbackincumbent
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackincumbent (CPXCENVptr env, void *cbdata, int wherefrom, double *x, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcallbackincumbent == NULL)
        native_CPXLgetcallbackincumbent = load_symbol("CPXLgetcallbackincumbent");
    return native_CPXLgetcallbackincumbent(env, cbdata, wherefrom, x, begin, end);
}

// CPXLgetcallbackindicatorinfo
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackindicatorinfo (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT iindex, int whichinfo, void *result_p)
{
    if (native_CPXLgetcallbackindicatorinfo == NULL)
        native_CPXLgetcallbackindicatorinfo = load_symbol("CPXLgetcallbackindicatorinfo");
    return native_CPXLgetcallbackindicatorinfo(env, cbdata, wherefrom, iindex, whichinfo, result_p);
}

// CPXLgetcallbacklp
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacklp (CPXCENVptr env, void *cbdata, int wherefrom, CPXCLPptr * lp_p)
{
    if (native_CPXLgetcallbacklp == NULL)
        native_CPXLgetcallbacklp = load_symbol("CPXLgetcallbacklp");
    return native_CPXLgetcallbacklp(env, cbdata, wherefrom, lp_p);
}

// CPXLgetcallbacknodeinfo
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeinfo (CPXCENVptr env, void *cbdata, int wherefrom, CPXLONG nodeindex, int whichinfo, void *result_p)
{
    if (native_CPXLgetcallbacknodeinfo == NULL)
        native_CPXLgetcallbacknodeinfo = load_symbol("CPXLgetcallbacknodeinfo");
    return native_CPXLgetcallbacknodeinfo(env, cbdata, wherefrom, nodeindex, whichinfo, result_p);
}

// CPXLgetcallbacknodeintfeas
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeintfeas (CPXCENVptr env, void *cbdata, int wherefrom, int *feas, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcallbacknodeintfeas == NULL)
        native_CPXLgetcallbacknodeintfeas = load_symbol("CPXLgetcallbacknodeintfeas");
    return native_CPXLgetcallbacknodeintfeas(env, cbdata, wherefrom, feas, begin, end);
}

// CPXLgetcallbacknodelb
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodelb (CPXCENVptr env, void *cbdata, int wherefrom, double *lb, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcallbacknodelb == NULL)
        native_CPXLgetcallbacknodelb = load_symbol("CPXLgetcallbacknodelb");
    return native_CPXLgetcallbacknodelb(env, cbdata, wherefrom, lb, begin, end);
}

// CPXLgetcallbacknodelp
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodelp (CPXCENVptr env, void *cbdata, int wherefrom, CPXLPptr * nodelp_p)
{
    if (native_CPXLgetcallbacknodelp == NULL)
        native_CPXLgetcallbacknodelp = load_symbol("CPXLgetcallbacknodelp");
    return native_CPXLgetcallbacknodelp(env, cbdata, wherefrom, nodelp_p);
}

// CPXLgetcallbacknodeobjval
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeobjval (CPXCENVptr env, void *cbdata, int wherefrom, double *objval_p)
{
    if (native_CPXLgetcallbacknodeobjval == NULL)
        native_CPXLgetcallbacknodeobjval = load_symbol("CPXLgetcallbacknodeobjval");
    return native_CPXLgetcallbacknodeobjval(env, cbdata, wherefrom, objval_p);
}

// CPXLgetcallbacknodestat
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodestat (CPXCENVptr env, void *cbdata, int wherefrom, int *nodestat_p)
{
    if (native_CPXLgetcallbacknodestat == NULL)
        native_CPXLgetcallbacknodestat = load_symbol("CPXLgetcallbacknodestat");
    return native_CPXLgetcallbacknodestat(env, cbdata, wherefrom, nodestat_p);
}

// CPXLgetcallbacknodeub
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeub (CPXCENVptr env, void *cbdata, int wherefrom, double *ub, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcallbacknodeub == NULL)
        native_CPXLgetcallbacknodeub = load_symbol("CPXLgetcallbacknodeub");
    return native_CPXLgetcallbacknodeub(env, cbdata, wherefrom, ub, begin, end);
}

// CPXLgetcallbacknodex
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodex (CPXCENVptr env, void *cbdata, int wherefrom, double *x, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcallbacknodex == NULL)
        native_CPXLgetcallbacknodex = load_symbol("CPXLgetcallbacknodex");
    return native_CPXLgetcallbacknodex(env, cbdata, wherefrom, x, begin, end);
}

// CPXLgetcallbackorder
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackorder (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT * priority, int *direction, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcallbackorder == NULL)
        native_CPXLgetcallbackorder = load_symbol("CPXLgetcallbackorder");
    return native_CPXLgetcallbackorder(env, cbdata, wherefrom, priority, direction, begin, end);
}

// CPXLgetcallbackpseudocosts
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackpseudocosts (CPXCENVptr env, void *cbdata, int wherefrom, double *uppc, double *downpc, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetcallbackpseudocosts == NULL)
        native_CPXLgetcallbackpseudocosts = load_symbol("CPXLgetcallbackpseudocosts");
    return native_CPXLgetcallbackpseudocosts(env, cbdata, wherefrom, uppc, downpc, begin, end);
}

// CPXLgetcallbackseqinfo
CPXLIBAPI int CPXPUBLIC CPXLgetcallbackseqinfo (CPXCENVptr env, void *cbdata, int wherefrom, CPXLONG seqid, int whichinfo, void *result_p)
{
    if (native_CPXLgetcallbackseqinfo == NULL)
        native_CPXLgetcallbackseqinfo = load_symbol("CPXLgetcallbackseqinfo");
    return native_CPXLgetcallbackseqinfo(env, cbdata, wherefrom, seqid, whichinfo, result_p);
}

// CPXLgetcallbacksosinfo
CPXLIBAPI int CPXPUBLIC CPXLgetcallbacksosinfo (CPXCENVptr env, void *cbdata, int wherefrom, CPXINT sosindex, CPXINT member, int whichinfo, void *result_p)
{
    if (native_CPXLgetcallbacksosinfo == NULL)
        native_CPXLgetcallbacksosinfo = load_symbol("CPXLgetcallbacksosinfo");
    return native_CPXLgetcallbacksosinfo(env, cbdata, wherefrom, sosindex, member, whichinfo, result_p);
}

// CPXLgetctype
CPXLIBAPI int CPXPUBLIC CPXLgetctype (CPXCENVptr env, CPXCLPptr lp, char *xctype, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetctype == NULL)
        native_CPXLgetctype = load_symbol("CPXLgetctype");
    return native_CPXLgetctype(env, lp, xctype, begin, end);
}

// CPXLgetcutoff
CPXLIBAPI int CPXPUBLIC CPXLgetcutoff (CPXCENVptr env, CPXCLPptr lp, double *cutoff_p)
{
    if (native_CPXLgetcutoff == NULL)
        native_CPXLgetcutoff = load_symbol("CPXLgetcutoff");
    return native_CPXLgetcutoff(env, lp, cutoff_p);
}

// CPXLgetdeletenodecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetdeletenodecallbackfunc (CPXCENVptr env, CPXL_CALLBACK_DELETENODE ** deletecallback_p, void **cbhandle_p)
{
    if (native_CPXLgetdeletenodecallbackfunc == NULL)
        native_CPXLgetdeletenodecallbackfunc = load_symbol("CPXLgetdeletenodecallbackfunc");
    return native_CPXLgetdeletenodecallbackfunc(env, deletecallback_p, cbhandle_p);
}

// CPXLgetheuristiccallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetheuristiccallbackfunc (CPXCENVptr env, CPXL_CALLBACK_HEURISTIC ** heuristiccallback_p, void **cbhandle_p)
{
    if (native_CPXLgetheuristiccallbackfunc == NULL)
        native_CPXLgetheuristiccallbackfunc = load_symbol("CPXLgetheuristiccallbackfunc");
    return native_CPXLgetheuristiccallbackfunc(env, heuristiccallback_p, cbhandle_p);
}

// CPXLgetincumbentcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetincumbentcallbackfunc (CPXCENVptr env, CPXL_CALLBACK_INCUMBENT ** incumbentcallback_p, void **cbhandle_p)
{
    if (native_CPXLgetincumbentcallbackfunc == NULL)
        native_CPXLgetincumbentcallbackfunc = load_symbol("CPXLgetincumbentcallbackfunc");
    return native_CPXLgetincumbentcallbackfunc(env, incumbentcallback_p, cbhandle_p);
}

// CPXLgetindconstr
CPXLIBAPI int CPXPUBLIC CPXLgetindconstr (CPXCENVptr env, CPXCLPptr lp, CPXINT * indvar_p, int *complemented_p, CPXINT * nzcnt_p, double *rhs_p, char *sense_p, CPXINT * linind, double *linval, CPXINT space, CPXINT * surplus_p, CPXINT which)
{
    if (native_CPXLgetindconstr == NULL)
        native_CPXLgetindconstr = load_symbol("CPXLgetindconstr");
    return native_CPXLgetindconstr(env, lp, indvar_p, complemented_p, nzcnt_p, rhs_p, sense_p, linind, linval, space, surplus_p, which);
}

// CPXLgetindconstrindex
CPXLIBAPI int CPXPUBLIC CPXLgetindconstrindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p)
{
    if (native_CPXLgetindconstrindex == NULL)
        native_CPXLgetindconstrindex = load_symbol("CPXLgetindconstrindex");
    return native_CPXLgetindconstrindex(env, lp, lname_str, index_p);
}

// CPXLgetindconstrinfeas
CPXLIBAPI int CPXPUBLIC CPXLgetindconstrinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetindconstrinfeas == NULL)
        native_CPXLgetindconstrinfeas = load_symbol("CPXLgetindconstrinfeas");
    return native_CPXLgetindconstrinfeas(env, lp, x, infeasout, begin, end);
}

// CPXLgetindconstrname
CPXLIBAPI int CPXPUBLIC CPXLgetindconstrname (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p, CPXINT which)
{
    if (native_CPXLgetindconstrname == NULL)
        native_CPXLgetindconstrname = load_symbol("CPXLgetindconstrname");
    return native_CPXLgetindconstrname(env, lp, buf_str, bufspace, surplus_p, which);
}

// CPXLgetindconstrslack
CPXLIBAPI int CPXPUBLIC CPXLgetindconstrslack (CPXCENVptr env, CPXCLPptr lp, double *indslack, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetindconstrslack == NULL)
        native_CPXLgetindconstrslack = load_symbol("CPXLgetindconstrslack");
    return native_CPXLgetindconstrslack(env, lp, indslack, begin, end);
}

// CPXLgetinfocallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetinfocallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p)
{
    if (native_CPXLgetinfocallbackfunc == NULL)
        native_CPXLgetinfocallbackfunc = load_symbol("CPXLgetinfocallbackfunc");
    return native_CPXLgetinfocallbackfunc(env, callback_p, cbhandle_p);
}

// CPXLgetlazyconstraintcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetlazyconstraintcallbackfunc (CPXCENVptr env, CPXL_CALLBACK_CUT ** cutcallback_p, void **cbhandle_p)
{
    if (native_CPXLgetlazyconstraintcallbackfunc == NULL)
        native_CPXLgetlazyconstraintcallbackfunc = load_symbol("CPXLgetlazyconstraintcallbackfunc");
    return native_CPXLgetlazyconstraintcallbackfunc(env, cutcallback_p, cbhandle_p);
}

// CPXLgetmipcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetmipcallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p, void **cbhandle_p)
{
    if (native_CPXLgetmipcallbackfunc == NULL)
        native_CPXLgetmipcallbackfunc = load_symbol("CPXLgetmipcallbackfunc");
    return native_CPXLgetmipcallbackfunc(env, callback_p, cbhandle_p);
}

// CPXLgetmipitcnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetmipitcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetmipitcnt == NULL)
        native_CPXLgetmipitcnt = load_symbol("CPXLgetmipitcnt");
    return native_CPXLgetmipitcnt(env, lp);
}

// CPXLgetmiprelgap
CPXLIBAPI int CPXPUBLIC CPXLgetmiprelgap (CPXCENVptr env, CPXCLPptr lp, double *gap_p)
{
    if (native_CPXLgetmiprelgap == NULL)
        native_CPXLgetmiprelgap = load_symbol("CPXLgetmiprelgap");
    return native_CPXLgetmiprelgap(env, lp, gap_p);
}

// CPXLgetmipstartindex
CPXLIBAPI int CPXPUBLIC CPXLgetmipstartindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXLgetmipstartindex == NULL)
        native_CPXLgetmipstartindex = load_symbol("CPXLgetmipstartindex");
    return native_CPXLgetmipstartindex(env, lp, lname_str, index_p);
}

// CPXLgetmipstartname
CPXLIBAPI int CPXPUBLIC CPXLgetmipstartname (CPXCENVptr env, CPXCLPptr lp, char **name, char *store, CPXSIZE storesz, CPXSIZE * surplus_p, int begin, int end)
{
    if (native_CPXLgetmipstartname == NULL)
        native_CPXLgetmipstartname = load_symbol("CPXLgetmipstartname");
    return native_CPXLgetmipstartname(env, lp, name, store, storesz, surplus_p, begin, end);
}

// CPXLgetmipstarts
CPXLIBAPI int CPXPUBLIC CPXLgetmipstarts (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p, CPXLONG * beg, CPXINT * varindices, double *values, int *effortlevel, CPXLONG startspace, CPXLONG * surplus_p, int begin, int end)
{
    if (native_CPXLgetmipstarts == NULL)
        native_CPXLgetmipstarts = load_symbol("CPXLgetmipstarts");
    return native_CPXLgetmipstarts(env, lp, nzcnt_p, beg, varindices, values, effortlevel, startspace, surplus_p, begin, end);
}

// CPXLgetnodecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetnodecallbackfunc (CPXCENVptr env, CPXL_CALLBACK_NODE ** nodecallback_p, void **cbhandle_p)
{
    if (native_CPXLgetnodecallbackfunc == NULL)
        native_CPXLgetnodecallbackfunc = load_symbol("CPXLgetnodecallbackfunc");
    return native_CPXLgetnodecallbackfunc(env, nodecallback_p, cbhandle_p);
}

// CPXLgetnodecnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnodecnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnodecnt == NULL)
        native_CPXLgetnodecnt = load_symbol("CPXLgetnodecnt");
    return native_CPXLgetnodecnt(env, lp);
}

// CPXLgetnodeint
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnodeint (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnodeint == NULL)
        native_CPXLgetnodeint = load_symbol("CPXLgetnodeint");
    return native_CPXLgetnodeint(env, lp);
}

// CPXLgetnodeleftcnt
CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnodeleftcnt (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnodeleftcnt == NULL)
        native_CPXLgetnodeleftcnt = load_symbol("CPXLgetnodeleftcnt");
    return native_CPXLgetnodeleftcnt(env, lp);
}

// CPXLgetnumbin
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumbin (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumbin == NULL)
        native_CPXLgetnumbin = load_symbol("CPXLgetnumbin");
    return native_CPXLgetnumbin(env, lp);
}

// CPXLgetnumcuts
CPXLIBAPI int CPXPUBLIC CPXLgetnumcuts (CPXCENVptr env, CPXCLPptr lp, int cuttype, CPXINT * num_p)
{
    if (native_CPXLgetnumcuts == NULL)
        native_CPXLgetnumcuts = load_symbol("CPXLgetnumcuts");
    return native_CPXLgetnumcuts(env, lp, cuttype, num_p);
}

// CPXLgetnumindconstrs
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumindconstrs (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumindconstrs == NULL)
        native_CPXLgetnumindconstrs = load_symbol("CPXLgetnumindconstrs");
    return native_CPXLgetnumindconstrs(env, lp);
}

// CPXLgetnumint
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumint (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumint == NULL)
        native_CPXLgetnumint = load_symbol("CPXLgetnumint");
    return native_CPXLgetnumint(env, lp);
}

// CPXLgetnumlazyconstraints
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumlazyconstraints (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumlazyconstraints == NULL)
        native_CPXLgetnumlazyconstraints = load_symbol("CPXLgetnumlazyconstraints");
    return native_CPXLgetnumlazyconstraints(env, lp);
}

// CPXLgetnummipstarts
CPXLIBAPI int CPXPUBLIC CPXLgetnummipstarts (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnummipstarts == NULL)
        native_CPXLgetnummipstarts = load_symbol("CPXLgetnummipstarts");
    return native_CPXLgetnummipstarts(env, lp);
}

// CPXLgetnumsemicont
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumsemicont (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumsemicont == NULL)
        native_CPXLgetnumsemicont = load_symbol("CPXLgetnumsemicont");
    return native_CPXLgetnumsemicont(env, lp);
}

// CPXLgetnumsemiint
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumsemiint (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumsemiint == NULL)
        native_CPXLgetnumsemiint = load_symbol("CPXLgetnumsemiint");
    return native_CPXLgetnumsemiint(env, lp);
}

// CPXLgetnumsos
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumsos (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumsos == NULL)
        native_CPXLgetnumsos = load_symbol("CPXLgetnumsos");
    return native_CPXLgetnumsos(env, lp);
}

// CPXLgetnumusercuts
CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumusercuts (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetnumusercuts == NULL)
        native_CPXLgetnumusercuts = load_symbol("CPXLgetnumusercuts");
    return native_CPXLgetnumusercuts(env, lp);
}

// CPXLgetorder
CPXLIBAPI int CPXPUBLIC CPXLgetorder (CPXCENVptr env, CPXCLPptr lp, CPXINT * cnt_p, CPXINT * indices, CPXINT * priority, int *direction, CPXINT ordspace, CPXINT * surplus_p)
{
    if (native_CPXLgetorder == NULL)
        native_CPXLgetorder = load_symbol("CPXLgetorder");
    return native_CPXLgetorder(env, lp, cnt_p, indices, priority, direction, ordspace, surplus_p);
}

// CPXLgetsolnpooldivfilter
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpooldivfilter (CPXCENVptr env, CPXCLPptr lp, double *lower_cutoff_p, double *upper_cutoff_p, CPXINT * nzcnt_p, CPXINT * ind, double *val, double *refval, CPXINT space, CPXINT * surplus_p, int which)
{
    if (native_CPXLgetsolnpooldivfilter == NULL)
        native_CPXLgetsolnpooldivfilter = load_symbol("CPXLgetsolnpooldivfilter");
    return native_CPXLgetsolnpooldivfilter(env, lp, lower_cutoff_p, upper_cutoff_p, nzcnt_p, ind, val, refval, space, surplus_p, which);
}

// CPXLgetsolnpoolfilterindex
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolfilterindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXLgetsolnpoolfilterindex == NULL)
        native_CPXLgetsolnpoolfilterindex = load_symbol("CPXLgetsolnpoolfilterindex");
    return native_CPXLgetsolnpoolfilterindex(env, lp, lname_str, index_p);
}

// CPXLgetsolnpoolfiltername
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolfiltername (CPXCENVptr env, CPXCLPptr lp, char *buf_str, CPXSIZE bufspace, CPXSIZE * surplus_p, int which)
{
    if (native_CPXLgetsolnpoolfiltername == NULL)
        native_CPXLgetsolnpoolfiltername = load_symbol("CPXLgetsolnpoolfiltername");
    return native_CPXLgetsolnpoolfiltername(env, lp, buf_str, bufspace, surplus_p, which);
}

// CPXLgetsolnpoolfiltertype
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolfiltertype (CPXCENVptr env, CPXCLPptr lp, int *ftype_p, int which)
{
    if (native_CPXLgetsolnpoolfiltertype == NULL)
        native_CPXLgetsolnpoolfiltertype = load_symbol("CPXLgetsolnpoolfiltertype");
    return native_CPXLgetsolnpoolfiltertype(env, lp, ftype_p, which);
}

// CPXLgetsolnpoolmeanobjval
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolmeanobjval (CPXCENVptr env, CPXCLPptr lp, double *meanobjval_p)
{
    if (native_CPXLgetsolnpoolmeanobjval == NULL)
        native_CPXLgetsolnpoolmeanobjval = load_symbol("CPXLgetsolnpoolmeanobjval");
    return native_CPXLgetsolnpoolmeanobjval(env, lp, meanobjval_p);
}

// CPXLgetsolnpoolnumfilters
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolnumfilters (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetsolnpoolnumfilters == NULL)
        native_CPXLgetsolnpoolnumfilters = load_symbol("CPXLgetsolnpoolnumfilters");
    return native_CPXLgetsolnpoolnumfilters(env, lp);
}

// CPXLgetsolnpoolnumreplaced
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolnumreplaced (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetsolnpoolnumreplaced == NULL)
        native_CPXLgetsolnpoolnumreplaced = load_symbol("CPXLgetsolnpoolnumreplaced");
    return native_CPXLgetsolnpoolnumreplaced(env, lp);
}

// CPXLgetsolnpoolnumsolns
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolnumsolns (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetsolnpoolnumsolns == NULL)
        native_CPXLgetsolnpoolnumsolns = load_symbol("CPXLgetsolnpoolnumsolns");
    return native_CPXLgetsolnpoolnumsolns(env, lp);
}

// CPXLgetsolnpoolobjval
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolobjval (CPXCENVptr env, CPXCLPptr lp, int soln, double *objval_p)
{
    if (native_CPXLgetsolnpoolobjval == NULL)
        native_CPXLgetsolnpoolobjval = load_symbol("CPXLgetsolnpoolobjval");
    return native_CPXLgetsolnpoolobjval(env, lp, soln, objval_p);
}

// CPXLgetsolnpoolqconstrslack
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolqconstrslack (CPXCENVptr env, CPXCLPptr lp, int soln, double *qcslack, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetsolnpoolqconstrslack == NULL)
        native_CPXLgetsolnpoolqconstrslack = load_symbol("CPXLgetsolnpoolqconstrslack");
    return native_CPXLgetsolnpoolqconstrslack(env, lp, soln, qcslack, begin, end);
}

// CPXLgetsolnpoolrngfilter
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolrngfilter (CPXCENVptr env, CPXCLPptr lp, double *lb_p, double *ub_p, CPXINT * nzcnt_p, CPXINT * ind, double *val, CPXINT space, CPXINT * surplus_p, int which)
{
    if (native_CPXLgetsolnpoolrngfilter == NULL)
        native_CPXLgetsolnpoolrngfilter = load_symbol("CPXLgetsolnpoolrngfilter");
    return native_CPXLgetsolnpoolrngfilter(env, lp, lb_p, ub_p, nzcnt_p, ind, val, space, surplus_p, which);
}

// CPXLgetsolnpoolslack
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolslack (CPXCENVptr env, CPXCLPptr lp, int soln, double *slack, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetsolnpoolslack == NULL)
        native_CPXLgetsolnpoolslack = load_symbol("CPXLgetsolnpoolslack");
    return native_CPXLgetsolnpoolslack(env, lp, soln, slack, begin, end);
}

// CPXLgetsolnpoolsolnindex
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolsolnindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, int *index_p)
{
    if (native_CPXLgetsolnpoolsolnindex == NULL)
        native_CPXLgetsolnpoolsolnindex = load_symbol("CPXLgetsolnpoolsolnindex");
    return native_CPXLgetsolnpoolsolnindex(env, lp, lname_str, index_p);
}

// CPXLgetsolnpoolsolnname
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolsolnname (CPXCENVptr env, CPXCLPptr lp, char *store, CPXSIZE storesz, CPXSIZE * surplus_p, int which)
{
    if (native_CPXLgetsolnpoolsolnname == NULL)
        native_CPXLgetsolnpoolsolnname = load_symbol("CPXLgetsolnpoolsolnname");
    return native_CPXLgetsolnpoolsolnname(env, lp, store, storesz, surplus_p, which);
}

// CPXLgetsolnpoolx
CPXLIBAPI int CPXPUBLIC CPXLgetsolnpoolx (CPXCENVptr env, CPXCLPptr lp, int soln, double *x, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetsolnpoolx == NULL)
        native_CPXLgetsolnpoolx = load_symbol("CPXLgetsolnpoolx");
    return native_CPXLgetsolnpoolx(env, lp, soln, x, begin, end);
}

// CPXLgetsolvecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetsolvecallbackfunc (CPXCENVptr env, CPXL_CALLBACK_SOLVE ** solvecallback_p, void **cbhandle_p)
{
    if (native_CPXLgetsolvecallbackfunc == NULL)
        native_CPXLgetsolvecallbackfunc = load_symbol("CPXLgetsolvecallbackfunc");
    return native_CPXLgetsolvecallbackfunc(env, solvecallback_p, cbhandle_p);
}

// CPXLgetsos
CPXLIBAPI int CPXPUBLIC CPXLgetsos (CPXCENVptr env, CPXCLPptr lp, CPXLONG * numsosnz_p, char *sostype, CPXLONG * sosbeg, CPXINT * sosind, double *soswt, CPXLONG sosspace, CPXLONG * surplus_p, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetsos == NULL)
        native_CPXLgetsos = load_symbol("CPXLgetsos");
    return native_CPXLgetsos(env, lp, numsosnz_p, sostype, sosbeg, sosind, soswt, sosspace, surplus_p, begin, end);
}

// CPXLgetsosindex
CPXLIBAPI int CPXPUBLIC CPXLgetsosindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str, CPXINT * index_p)
{
    if (native_CPXLgetsosindex == NULL)
        native_CPXLgetsosindex = load_symbol("CPXLgetsosindex");
    return native_CPXLgetsosindex(env, lp, lname_str, index_p);
}

// CPXLgetsosinfeas
CPXLIBAPI int CPXPUBLIC CPXLgetsosinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x, double *infeasout, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetsosinfeas == NULL)
        native_CPXLgetsosinfeas = load_symbol("CPXLgetsosinfeas");
    return native_CPXLgetsosinfeas(env, lp, x, infeasout, begin, end);
}

// CPXLgetsosname
CPXLIBAPI int CPXPUBLIC CPXLgetsosname (CPXCENVptr env, CPXCLPptr lp, char **name, char *namestore, CPXSIZE storespace, CPXSIZE * surplus_p, CPXINT begin, CPXINT end)
{
    if (native_CPXLgetsosname == NULL)
        native_CPXLgetsosname = load_symbol("CPXLgetsosname");
    return native_CPXLgetsosname(env, lp, name, namestore, storespace, surplus_p, begin, end);
}

// CPXLgetsubmethod
CPXLIBAPI int CPXPUBLIC CPXLgetsubmethod (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetsubmethod == NULL)
        native_CPXLgetsubmethod = load_symbol("CPXLgetsubmethod");
    return native_CPXLgetsubmethod(env, lp);
}

// CPXLgetsubstat
CPXLIBAPI int CPXPUBLIC CPXLgetsubstat (CPXCENVptr env, CPXCLPptr lp)
{
    if (native_CPXLgetsubstat == NULL)
        native_CPXLgetsubstat = load_symbol("CPXLgetsubstat");
    return native_CPXLgetsubstat(env, lp);
}

// CPXLgetusercutcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLgetusercutcallbackfunc (CPXCENVptr env, CPXL_CALLBACK_CUT ** cutcallback_p, void **cbhandle_p)
{
    if (native_CPXLgetusercutcallbackfunc == NULL)
        native_CPXLgetusercutcallbackfunc = load_symbol("CPXLgetusercutcallbackfunc");
    return native_CPXLgetusercutcallbackfunc(env, cutcallback_p, cbhandle_p);
}

// CPXLindconstrslackfromx
CPXLIBAPI int CPXPUBLIC CPXLindconstrslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *indslack)
{
    if (native_CPXLindconstrslackfromx == NULL)
        native_CPXLindconstrslackfromx = load_symbol("CPXLindconstrslackfromx");
    return native_CPXLindconstrslackfromx(env, lp, x, indslack);
}

// CPXLmipopt
CPXLIBAPI int CPXPUBLIC CPXLmipopt (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLmipopt == NULL)
        native_CPXLmipopt = load_symbol("CPXLmipopt");
    return native_CPXLmipopt(env, lp);
}

// CPXLordread
CPXLIBAPI int CPXPUBLIC CPXLordread (CPXCENVptr env, char const *filename_str, CPXINT numcols, char const *const *colname, CPXINT * cnt_p, CPXINT * indices, CPXINT * priority, int *direction)
{
    if (native_CPXLordread == NULL)
        native_CPXLordread = load_symbol("CPXLordread");
    return native_CPXLordread(env, filename_str, numcols, colname, cnt_p, indices, priority, direction);
}

// CPXLordwrite
CPXLIBAPI int CPXPUBLIC CPXLordwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str)
{
    if (native_CPXLordwrite == NULL)
        native_CPXLordwrite = load_symbol("CPXLordwrite");
    return native_CPXLordwrite(env, lp, filename_str);
}

// CPXLpopulate
CPXLIBAPI int CPXPUBLIC CPXLpopulate (CPXCENVptr env, CPXLPptr lp)
{
    if (native_CPXLpopulate == NULL)
        native_CPXLpopulate = load_symbol("CPXLpopulate");
    return native_CPXLpopulate(env, lp);
}

// CPXLreadcopymipstarts
CPXLIBAPI int CPXPUBLIC CPXLreadcopymipstarts (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXLreadcopymipstarts == NULL)
        native_CPXLreadcopymipstarts = load_symbol("CPXLreadcopymipstarts");
    return native_CPXLreadcopymipstarts(env, lp, filename_str);
}

// CPXLreadcopyorder
CPXLIBAPI int CPXPUBLIC CPXLreadcopyorder (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXLreadcopyorder == NULL)
        native_CPXLreadcopyorder = load_symbol("CPXLreadcopyorder");
    return native_CPXLreadcopyorder(env, lp, filename_str);
}

// CPXLreadcopysolnpoolfilters
CPXLIBAPI int CPXPUBLIC CPXLreadcopysolnpoolfilters (CPXCENVptr env, CPXLPptr lp, char const *filename_str)
{
    if (native_CPXLreadcopysolnpoolfilters == NULL)
        native_CPXLreadcopysolnpoolfilters = load_symbol("CPXLreadcopysolnpoolfilters");
    return native_CPXLreadcopysolnpoolfilters(env, lp, filename_str);
}

// CPXLrefinemipstartconflict
CPXLIBAPI int CPXPUBLIC CPXLrefinemipstartconflict (CPXCENVptr env, CPXLPptr lp, int mipstartindex, CPXINT * confnumrows_p, CPXINT * confnumcols_p)
{
    if (native_CPXLrefinemipstartconflict == NULL)
        native_CPXLrefinemipstartconflict = load_symbol("CPXLrefinemipstartconflict");
    return native_CPXLrefinemipstartconflict(env, lp, mipstartindex, confnumrows_p, confnumcols_p);
}

// CPXLrefinemipstartconflictext
CPXLIBAPI int CPXPUBLIC CPXLrefinemipstartconflictext (CPXCENVptr env, CPXLPptr lp, int mipstartindex, CPXLONG grpcnt, CPXLONG concnt, double const *grppref, CPXLONG const *grpbeg, CPXINT const *grpind, char const *grptype)
{
    if (native_CPXLrefinemipstartconflictext == NULL)
        native_CPXLrefinemipstartconflictext = load_symbol("CPXLrefinemipstartconflictext");
    return native_CPXLrefinemipstartconflictext(env, lp, mipstartindex, grpcnt, concnt, grppref, grpbeg, grpind, grptype);
}

// CPXLsetbranchcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetbranchcallbackfunc (CPXENVptr env, CPXL_CALLBACK_BRANCH * branchcallback, void *cbhandle)
{
    if (native_CPXLsetbranchcallbackfunc == NULL)
        native_CPXLsetbranchcallbackfunc = load_symbol("CPXLsetbranchcallbackfunc");
    return native_CPXLsetbranchcallbackfunc(env, branchcallback, cbhandle);
}

// CPXLsetbranchnosolncallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetbranchnosolncallbackfunc (CPXENVptr env, CPXL_CALLBACK_BRANCH * branchnosolncallback, void *cbhandle)
{
    if (native_CPXLsetbranchnosolncallbackfunc == NULL)
        native_CPXLsetbranchnosolncallbackfunc = load_symbol("CPXLsetbranchnosolncallbackfunc");
    return native_CPXLsetbranchnosolncallbackfunc(env, branchnosolncallback, cbhandle);
}

// CPXLsetdeletenodecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetdeletenodecallbackfunc (CPXENVptr env, CPXL_CALLBACK_DELETENODE * deletecallback, void *cbhandle)
{
    if (native_CPXLsetdeletenodecallbackfunc == NULL)
        native_CPXLsetdeletenodecallbackfunc = load_symbol("CPXLsetdeletenodecallbackfunc");
    return native_CPXLsetdeletenodecallbackfunc(env, deletecallback, cbhandle);
}

// CPXLsetheuristiccallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetheuristiccallbackfunc (CPXENVptr env, CPXL_CALLBACK_HEURISTIC * heuristiccallback, void *cbhandle)
{
    if (native_CPXLsetheuristiccallbackfunc == NULL)
        native_CPXLsetheuristiccallbackfunc = load_symbol("CPXLsetheuristiccallbackfunc");
    return native_CPXLsetheuristiccallbackfunc(env, heuristiccallback, cbhandle);
}

// CPXLsetincumbentcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetincumbentcallbackfunc (CPXENVptr env, CPXL_CALLBACK_INCUMBENT * incumbentcallback, void *cbhandle)
{
    if (native_CPXLsetincumbentcallbackfunc == NULL)
        native_CPXLsetincumbentcallbackfunc = load_symbol("CPXLsetincumbentcallbackfunc");
    return native_CPXLsetincumbentcallbackfunc(env, incumbentcallback, cbhandle);
}

// CPXLsetinfocallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetinfocallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle)
{
    if (native_CPXLsetinfocallbackfunc == NULL)
        native_CPXLsetinfocallbackfunc = load_symbol("CPXLsetinfocallbackfunc");
    return native_CPXLsetinfocallbackfunc(env, callback, cbhandle);
}

// CPXLsetlazyconstraintcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetlazyconstraintcallbackfunc (CPXENVptr env, CPXL_CALLBACK_CUT * lazyconcallback, void *cbhandle)
{
    if (native_CPXLsetlazyconstraintcallbackfunc == NULL)
        native_CPXLsetlazyconstraintcallbackfunc = load_symbol("CPXLsetlazyconstraintcallbackfunc");
    return native_CPXLsetlazyconstraintcallbackfunc(env, lazyconcallback, cbhandle);
}

// CPXLsetmipcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetmipcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback, void *cbhandle)
{
    if (native_CPXLsetmipcallbackfunc == NULL)
        native_CPXLsetmipcallbackfunc = load_symbol("CPXLsetmipcallbackfunc");
    return native_CPXLsetmipcallbackfunc(env, callback, cbhandle);
}

// CPXLsetnodecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetnodecallbackfunc (CPXENVptr env, CPXL_CALLBACK_NODE * nodecallback, void *cbhandle)
{
    if (native_CPXLsetnodecallbackfunc == NULL)
        native_CPXLsetnodecallbackfunc = load_symbol("CPXLsetnodecallbackfunc");
    return native_CPXLsetnodecallbackfunc(env, nodecallback, cbhandle);
}

// CPXLsetsolvecallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetsolvecallbackfunc (CPXENVptr env, CPXL_CALLBACK_SOLVE * solvecallback, void *cbhandle)
{
    if (native_CPXLsetsolvecallbackfunc == NULL)
        native_CPXLsetsolvecallbackfunc = load_symbol("CPXLsetsolvecallbackfunc");
    return native_CPXLsetsolvecallbackfunc(env, solvecallback, cbhandle);
}

// CPXLsetusercutcallbackfunc
CPXLIBAPI int CPXPUBLIC CPXLsetusercutcallbackfunc (CPXENVptr env, CPXL_CALLBACK_CUT * cutcallback, void *cbhandle)
{
    if (native_CPXLsetusercutcallbackfunc == NULL)
        native_CPXLsetusercutcallbackfunc = load_symbol("CPXLsetusercutcallbackfunc");
    return native_CPXLsetusercutcallbackfunc(env, cutcallback, cbhandle);
}

// CPXLwritemipstarts
CPXLIBAPI int CPXPUBLIC CPXLwritemipstarts (CPXCENVptr env, CPXCLPptr lp, char const *filename_str, int begin, int end)
{
    if (native_CPXLwritemipstarts == NULL)
        native_CPXLwritemipstarts = load_symbol("CPXLwritemipstarts");
    return native_CPXLwritemipstarts(env, lp, filename_str, begin, end);
}

