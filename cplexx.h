#ifndef _CPLEX_H
#define _CPLEX_H

/*
 * This header was partly generated from a solver header of the same name,
 * to make lazylpsolverlibs callable. It contains only constants,
 * structures, and macros generated from the original header, and thus,
 * contains no copyrightable information.
 *
 * Additionnal hand editing was also probably performed.
 */

#include "cpxconst.h"
#ifdef _WIN32
#pragma pack(push, 8)
#endif
#ifdef __cplusplus
extern "C"
{
#endif
#ifndef CPX_MODERN
#define CPXgetsbcnt CPXgetpsbcnt
#endif
#ifndef CPX_MODERN
#define CPXoptimize CPXlpopt
#endif
  CPXCHANNELptr CPXPUBLIC CPXaddchannel (CPXENVptr env);
    CPXLIBAPI
    int CPXPUBLIC
    CPXaddcols (CPXCENVptr env, CPXLPptr lp, int ccnt, int nzcnt,
                double const *obj, int const *cmatbeg,
                int const *cmatind, double const *cmatval,
                double const *lb, double const *ub, char **colname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXaddfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr);
    CPXLIBAPI
    int CPXPUBLIC
    CPXaddfuncdest (CPXCENVptr env, CPXCHANNELptr channel, void *handle,
                    void (CPXPUBLIC * msgfunction) (void *, const char *));
    CPXLIBAPI
    int CPXPUBLIC
    CPXaddrows (CPXCENVptr env, CPXLPptr lp, int ccnt, int rcnt,
                int nzcnt, double const *rhs, char const *sense,
                int const *rmatbeg, int const *rmatind,
                double const *rmatval, char **colname, char **rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXbasicpresolve (CPXCENVptr env, CPXLPptr lp, double *redlb,
                      double *redub, int *rstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXbinvacol (CPXCENVptr env, CPXCLPptr lp, int j, double *x);
    CPXLIBAPI
    int CPXPUBLIC
    CPXbinvarow (CPXCENVptr env, CPXCLPptr lp, int i, double *z);
    CPXLIBAPI
    int CPXPUBLIC CPXbinvcol (CPXCENVptr env, CPXCLPptr lp, int j, double *x);
    CPXLIBAPI
    int CPXPUBLIC CPXbinvrow (CPXCENVptr env, CPXCLPptr lp, int i, double *y);
    CPXLIBAPI
    int CPXPUBLIC
    CPXboundsa (CPXCENVptr env, CPXCLPptr lp, int begin, int end,
                double *lblower, double *lbupper, double *ublower,
                double *ubupper);
    CPXLIBAPI
    int CPXPUBLIC CPXbtran (CPXCENVptr env, CPXCLPptr lp, double *y);
    CPXLIBAPI
    int CPXPUBLIC CPXcheckdfeas (CPXCENVptr env, CPXLPptr lp, int *infeas_p);
    CPXLIBAPI
    int CPXPUBLIC CPXcheckpfeas (CPXCENVptr env, CPXLPptr lp, int *infeas_p);
    CPXLIBAPI
    int CPXPUBLIC CPXchecksoln (CPXCENVptr env, CPXLPptr lp, int *lpstatus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgbds (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices,
               char const *lu, double const *bd);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgcoef (CPXCENVptr env, CPXLPptr lp, int i, int j, double newvalue);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgcoeflist (CPXCENVptr env, CPXLPptr lp, int numcoefs,
                    int const *rowlist, int const *collist,
                    double const *vallist);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgcolname (CPXCENVptr env, CPXLPptr lp, int cnt,
                   int const *indices, char **newname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgname (CPXCENVptr env, CPXLPptr lp, int key, int ij,
                char const *newname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgobj (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices,
               double const *values);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgobjoffset (CPXCENVptr env, CPXLPptr lp, double offset);
    CPXLIBAPI
    int CPXPUBLIC CPXchgobjsen (CPXCENVptr env, CPXLPptr lp, int maxormin);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgprobname (CPXCENVptr env, CPXLPptr lp, char const *probname);
    CPXLIBAPI
    int CPXPUBLIC CPXchgprobtype (CPXCENVptr env, CPXLPptr lp, int type);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgprobtypesolnpool (CPXCENVptr env, CPXLPptr lp, int type, int soln);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgrhs (CPXCENVptr env, CPXLPptr lp, int cnt, int const *indices,
               double const *values);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgrngval (CPXCENVptr env, CPXLPptr lp, int cnt,
                  int const *indices, double const *values);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgrowname (CPXCENVptr env, CPXLPptr lp, int cnt,
                   int const *indices, char **newname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgsense (CPXCENVptr env, CPXLPptr lp, int cnt,
                 int const *indices, char const *sense);
    CPXLIBAPI
    int CPXPUBLIC CPXcleanup (CPXCENVptr env, CPXLPptr lp, double eps);
    CPXLIBAPI
    CPXLPptr CPXPUBLIC
    CPXcloneprob (CPXCENVptr env, CPXCLPptr lp, int *status_p);
    CPXLIBAPI int CPXPUBLIC CPXcloseCPLEX (CPXENVptr * env_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXclpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI int CPXPUBLIC CPXcompletelp (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopybase (CPXCENVptr env, CPXLPptr lp, int const *cstat,
                 int const *rstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopybasednorms (CPXCENVptr env, CPXLPptr lp, int const *cstat,
                       int const *rstat, double const *dnorm);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopydnorms (CPXCENVptr env, CPXLPptr lp, double const *norm,
                   int const *head, int len);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopylp (CPXCENVptr env, CPXLPptr lp, int numcols, int numrows,
               int objsense, double const *objective, double const *rhs,
               char const *sense, int const *matbeg, int const *matcnt,
               int const *matind, double const *matval,
               double const *lb, double const *ub, double const *rngval);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopylpwnames (CPXCENVptr env, CPXLPptr lp, int numcols,
                     int numrows, int objsense, double const *objective,
                     double const *rhs, char const *sense,
                     int const *matbeg, int const *matcnt,
                     int const *matind, double const *matval,
                     double const *lb, double const *ub,
                     double const *rngval, char **colname, char **rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopynettolp (CPXCENVptr env, CPXLPptr lp, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopyobjname (CPXCENVptr env, CPXLPptr lp, char const *objname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopypartialbase (CPXCENVptr env, CPXLPptr lp, int ccnt,
                        int const *cindices, int const *cstat, int rcnt,
                        int const *rindices, int const *rstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopypnorms (CPXCENVptr env, CPXLPptr lp, double const *cnorm,
                   double const *rnorm, int len);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopyprotected (CPXCENVptr env, CPXLPptr lp, int cnt,
                      int const *indices);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopystart (CPXCENVptr env, CPXLPptr lp, int const *cstat,
                  int const *rstat, double const *cprim,
                  double const *rprim, double const *cdual,
                  double const *rdual);
    CPXLIBAPI
    CPXLPptr CPXPUBLIC
    CPXcreateprob (CPXCENVptr env, int *status_p, char const *probname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcrushform (CPXCENVptr env, CPXCLPptr lp, int len, int const *ind,
                  double const *val, int *plen_p, double *poffset_p,
                  int *pind, double *pval);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcrushpi (CPXCENVptr env, CPXCLPptr lp, double const *pi,
                double *prepi);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcrushx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *prex);
  int CPXPUBLIC CPXdelchannel (CPXENVptr env, CPXCHANNELptr * channel_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelcols (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelfuncdest (CPXCENVptr env, CPXCHANNELptr channel, void *handle,
                    void (CPXPUBLIC * msgfunction) (void *, const char *));
    CPXLIBAPI int CPXPUBLIC CPXdelnames (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelrows (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC CPXdelsetcols (CPXCENVptr env, CPXLPptr lp, int *delstat);
    CPXLIBAPI
    int CPXPUBLIC CPXdelsetrows (CPXCENVptr env, CPXLPptr lp, int *delstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdeserializercreate (CPXDESERIALIZERptr * deser_p, CPXLONG size,
                           void const *buffer);
    CPXLIBAPI
    void CPXPUBLIC CPXdeserializerdestroy (CPXDESERIALIZERptr deser);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXdeserializerleft (CPXCDESERIALIZERptr deser);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdisconnectchannel (CPXCENVptr env, CPXCHANNELptr channel);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *dj);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdperwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str,
                  double epsilon);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdratio (CPXCENVptr env, CPXLPptr lp, int *indices, int cnt,
               double *downratio, double *upratio, int *downenter,
               int *upenter, int *downstatus, int *upstatus);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdualfarkas (CPXCENVptr env, CPXCLPptr lp, double *y, double *proof_p);
    CPXLIBAPI int CPXPUBLIC CPXdualopt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdualwrite (CPXCENVptr env, CPXCLPptr lp,
                  char const *filename_str, double *objshift_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXembwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str);
    CPXLIBAPI int CPXPUBLIC CPXfclose (CPXFILEptr stream);
    CPXLIBAPI
    int CPXPUBLIC
    CPXfeasopt (CPXCENVptr env, CPXLPptr lp, double const *rhs,
                double const *rng, double const *lb, double const *ub);
    CPXLIBAPI
    int CPXPUBLIC
    CPXfeasoptext (CPXCENVptr env, CPXLPptr lp, int grpcnt, int concnt,
                   double const *grppref, int const *grpbeg,
                   int const *grpind, char const *grptype);
    CPXLIBAPI void CPXPUBLIC CPXfinalize (void);
  int CPXPUBLIC
    CPXfindiis (CPXCENVptr env, CPXLPptr lp, int *iisnumrows_p,
                int *iisnumcols_p);
    CPXLIBAPI
    int CPXPUBLIC CPXflushchannel (CPXCENVptr env, CPXCHANNELptr channel);
    CPXLIBAPI int CPXPUBLIC CPXflushstdchannels (CPXCENVptr env);
    CPXLIBAPI
    CPXFILEptr CPXPUBLIC
    CPXfopen (char const *filename_str, char const *type_str);
    CPXLIBAPI int CPXPUBLIC CPXfputs (char const *s_str, CPXFILEptr stream);
  void CPXPUBLIC CPXfree (void *ptr);
    CPXLIBAPI
    int CPXPUBLIC CPXfreeparenv (CPXENVptr env, CPXENVptr * child_p);
    CPXLIBAPI int CPXPUBLIC CPXfreepresolve (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXfreeprob (CPXCENVptr env, CPXLPptr * lp_p);
    CPXLIBAPI
    int CPXPUBLIC CPXftran (CPXCENVptr env, CPXCLPptr lp, double *x);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetax (CPXCENVptr env, CPXCLPptr lp, double *x, int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetbaritcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetbase (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetbasednorms (CPXCENVptr env, CPXCLPptr lp, int *cstat,
                      int *rstat, double *dnorm);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetbhead (CPXCENVptr env, CPXCLPptr lp, int *head, double *x);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetcallbackinfo (CPXCENVptr env, void *cbdata, int wherefrom,
                        int whichinfo, void *result_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetchannels (CPXCENVptr env, CPXCHANNELptr * cpxresults_p,
                    CPXCHANNELptr * cpxwarning_p,
                    CPXCHANNELptr * cpxerror_p, CPXCHANNELptr * cpxlog_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetchgparam (CPXCENVptr env, int *cnt_p, int *paramnum,
                    int pspace, int *surplus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetcoef (CPXCENVptr env, CPXCLPptr lp, int i, int j, double *coef_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetcolindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str,
                    int *index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetcolinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x,
                     double *infeasout, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetcolname (CPXCENVptr env, CPXCLPptr lp, char **name,
                   char *namestore, int storespace, int *surplus_p,
                   int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetcols (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p,
                int *cmatbeg, int *cmatind, double *cmatval,
                int cmatspace, int *surplus_p, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetconflict (CPXCENVptr env, CPXCLPptr lp, int *confstat_p,
                    int *rowind, int *rowbdstat, int *confnumrows_p,
                    int *colind, int *colbdstat, int *confnumcols_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetconflictext (CPXCENVptr env, CPXCLPptr lp, int *grpstat,
                       int beg, int end);
    CPXLIBAPI
    int CPXPUBLIC CPXgetcrossdexchcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXgetcrossdpushcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXgetcrosspexchcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXgetcrossppushcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetdblparam (CPXCENVptr env, int whichparam, double *value_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetdblquality (CPXCENVptr env, CPXCLPptr lp, double *quality_p,
                      int what);
    CPXLIBAPI
    int CPXPUBLIC CPXgetdettime (CPXCENVptr env, double *dettimestamp_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetdj (CPXCENVptr env, CPXCLPptr lp, double *dj, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetdnorms (CPXCENVptr env, CPXCLPptr lp, double *norm, int *head,
                  int *len_p);
    CPXLIBAPI int CPXPUBLIC CPXgetdsbcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXCCHARptr CPXPUBLIC
    CPXgeterrorstring (CPXCENVptr env, int errcode, char *buffer_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetgrad (CPXCENVptr env, CPXCLPptr lp, int j, int *head, double *y);
  int CPXPUBLIC
    CPXgetiis (CPXCENVptr env, CPXCLPptr lp, int *iisstat_p,
               int *rowind, int *rowbdstat, int *iisnumrows_p,
               int *colind, int *colbdstat, int *iisnumcols_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetijdiv (CPXCENVptr env, CPXCLPptr lp, int *idiv_p, int *jdiv_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetijrow (CPXCENVptr env, CPXCLPptr lp, int i, int j, int *row_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetintparam (CPXCENVptr env, int whichparam, CPXINT * value_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetintquality (CPXCENVptr env, CPXCLPptr lp, int *quality_p, int what);
    CPXLIBAPI int CPXPUBLIC CPXgetitcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetlb (CPXCENVptr env, CPXCLPptr lp, double *lb, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC CPXgetlogfile (CPXCENVptr env, CPXFILEptr * logfile_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetlongparam (CPXCENVptr env, int whichparam, CPXLONG * value_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetlpcallbackfunc (CPXCENVptr env,
                          int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *,
                                                         int, void *),
                          void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetmethod (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetnetcallbackfunc (CPXCENVptr env,
                           int (CPXPUBLIC ** callback_p) (CPXCENVptr, void *,
                                                          int, void *),
                           void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetnumcols (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnumcores (CPXCENVptr env, int *numcores_p);
    CPXLIBAPI int CPXPUBLIC CPXgetnumnz (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnumrows (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetobj (CPXCENVptr env, CPXCLPptr lp, double *obj, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetobjname (CPXCENVptr env, CPXCLPptr lp, char *buf_str,
                   int bufspace, int *surplus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetobjoffset (CPXCENVptr env, CPXCLPptr lp, double *objoffset_p);
    CPXLIBAPI int CPXPUBLIC CPXgetobjsen (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetparamname (CPXCENVptr env, int whichparam, char *name_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetparamnum (CPXCENVptr env, char const *name_str, int *whichparam_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetparamtype (CPXCENVptr env, int whichparam, int *paramtype);
    CPXLIBAPI int CPXPUBLIC CPXgetphase1cnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetpi (CPXCENVptr env, CPXCLPptr lp, double *pi, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetpnorms (CPXCENVptr env, CPXCLPptr lp, double *cnorm,
                  double *rnorm, int *len_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetprestat (CPXCENVptr env, CPXCLPptr lp, int *prestat_p,
                   int *pcstat, int *prstat, int *ocstat, int *orstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetprobname (CPXCENVptr env, CPXCLPptr lp, char *buf_str,
                    int bufspace, int *surplus_p);
    CPXLIBAPI int CPXPUBLIC CPXgetprobtype (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetprotected (CPXCENVptr env, CPXCLPptr lp, int *cnt_p,
                     int *indices, int pspace, int *surplus_p);
    CPXLIBAPI int CPXPUBLIC CPXgetpsbcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXgetray (CPXCENVptr env, CPXCLPptr lp, double *z);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetredlp (CPXCENVptr env, CPXCLPptr lp, CPXCLPptr * redlp_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetrhs (CPXCENVptr env, CPXCLPptr lp, double *rhs, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetrngval (CPXCENVptr env, CPXCLPptr lp, double *rngval,
                  int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetrowindex (CPXCENVptr env, CPXCLPptr lp, char const *lname_str,
                    int *index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetrowinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x,
                     double *infeasout, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetrowname (CPXCENVptr env, CPXCLPptr lp, char **name,
                   char *namestore, int storespace, int *surplus_p,
                   int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetrows (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p,
                int *rmatbeg, int *rmatind, double *rmatval,
                int rmatspace, int *surplus_p, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsense (CPXCENVptr env, CPXCLPptr lp, char *sense, int begin,
                 int end);
    CPXLIBAPI int CPXPUBLIC CPXgetsiftitcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXgetsiftphase1cnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetslack (CPXCENVptr env, CPXCLPptr lp, double *slack, int begin,
                 int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpooldblquality (CPXCENVptr env, CPXCLPptr lp, int soln,
                              double *quality_p, int what);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolintquality (CPXCENVptr env, CPXCLPptr lp, int soln,
                              int *quality_p, int what);
    CPXLIBAPI int CPXPUBLIC CPXgetstat (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXCHARptr CPXPUBLIC
    CPXgetstatstring (CPXCENVptr env, int statind, char *buffer_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetstrparam (CPXCENVptr env, int whichparam, char *value_str);
    CPXLIBAPI int CPXPUBLIC CPXgettime (CPXCENVptr env, double *timestamp_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgettuningcallbackfunc (CPXCENVptr env,
                              int (CPXPUBLIC ** callback_p) (CPXCENVptr,
                                                             void *, int,
                                                             void *),
                              void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetub (CPXCENVptr env, CPXCLPptr lp,
                                      double *ub, int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetweight (CPXCENVptr env, CPXCLPptr lp,
                                          int rcnt, int const *rmatbeg,
                                          int const *rmatind,
                                          double const *rmatval,
                                          double *weight, int dpriind);
    CPXLIBAPI int CPXPUBLIC CPXgetx (CPXCENVptr env, CPXCLPptr lp, double *x,
                                     int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXhybnetopt (CPXCENVptr env, CPXLPptr lp,
                                          int method);
    CPXLIBAPI int CPXPUBLIC CPXinfodblparam (CPXCENVptr env, int whichparam,
                                             double *defvalue_p,
                                             double *minvalue_p,
                                             double *maxvalue_p);
    CPXLIBAPI int CPXPUBLIC CPXinfointparam (CPXCENVptr env, int whichparam,
                                             CPXINT * defvalue_p,
                                             CPXINT * minvalue_p,
                                             CPXINT * maxvalue_p);
    CPXLIBAPI int CPXPUBLIC CPXinfolongparam (CPXCENVptr env, int whichparam,
                                              CPXLONG * defvalue_p,
                                              CPXLONG * minvalue_p,
                                              CPXLONG * maxvalue_p);
    CPXLIBAPI int CPXPUBLIC CPXinfostrparam (CPXCENVptr env, int whichparam,
                                             char *defvalue_str);
    CPXLIBAPI void CPXPUBLIC CPXinitialize (void);
    CPXLIBAPI int CPXPUBLIC CPXkilldnorms (CPXLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXkillpnorms (CPXLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXlpopt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXlprewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXlpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
  CPXVOIDptr CPXPUBLIC CPXmalloc (size_t size);
    CPXLIBAPI
    int CPXPUBLIC
    CPXmbasewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXmdleave (CPXCENVptr env, CPXLPptr lp, int const *indices,
                int cnt, double *downratio, double *upratio);
  CPXVOIDptr CPXPUBLIC CPXmemcpy (void *s1, void *s2, size_t n);
    CPXLIBAPI
    int CPXPUBLIC
    CPXmpsrewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXmpswrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBVARARGS CPXmsg (CPXCHANNELptr channel, char const *format, ...);
    CPXLIBAPI
    int CPXPUBLIC CPXmsgstr (CPXCHANNELptr channel, char const *msg_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETextract (CPXCENVptr env, CPXNETptr net, CPXCLPptr lp,
                   int *colmap, int *rowmap);
    CPXLIBAPI
    int CPXPUBLIC
    CPXnewcols (CPXCENVptr env, CPXLPptr lp, int ccnt,
                double const *obj, double const *lb, double const *ub,
                char const *xctype, char **colname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXnewrows (CPXCENVptr env, CPXLPptr lp, int rcnt,
                double const *rhs, char const *sense,
                double const *rngval, char **rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXobjsa (CPXCENVptr env, CPXCLPptr lp, int begin, int end,
              double *lower, double *upper);
    CPXLIBAPI CPXENVptr CPXPUBLIC CPXopenCPLEX (int *status_p);
    CPXLIBAPI
    CPXENVptr CPXPUBLIC
    CPXopenCPLEXruntime (int *status_p, int serialnum,
                         char const *licenvstring_str);
    CPXLIBAPI CPXENVptr CPXPUBLIC CPXparenv (CPXENVptr env, int *status_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXpivot (CPXCENVptr env, CPXLPptr lp, int jenter, int jleave,
              int leavestat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXpivotin (CPXCENVptr env, CPXLPptr lp, int const *rlist, int rlen);
    CPXLIBAPI
    int CPXPUBLIC
    CPXpivotout (CPXCENVptr env, CPXLPptr lp, int const *clist, int clen);
    CPXLIBAPI
    int CPXPUBLIC
    CPXpperwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str,
                  double epsilon);
    CPXLIBAPI
    int CPXPUBLIC
    CPXpratio (CPXCENVptr env, CPXLPptr lp, int *indices, int cnt,
               double *downratio, double *upratio, int *downleave,
               int *upleave, int *downleavestatus, int *upleavestatus,
               int *downstatus, int *upstatus);
    CPXLIBAPI
    int CPXPUBLIC
    CPXpreaddrows (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt,
                   double const *rhs, char const *sense,
                   int const *rmatbeg, int const *rmatind,
                   double const *rmatval, char **rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXprechgobj (CPXCENVptr env, CPXLPptr lp, int cnt,
                  int const *indices, double const *values);
    CPXLIBAPI
    int CPXPUBLIC
    CPXpreslvwrite (CPXCENVptr env, CPXLPptr lp,
                    char const *filename_str, double *objoff_p);
    CPXLIBAPI
    int CPXPUBLIC CPXpresolve (CPXCENVptr env, CPXLPptr lp, int method);
    CPXLIBAPI int CPXPUBLIC CPXprimopt (CPXCENVptr env, CPXLPptr lp);
  int CPXPUBLIC CPXputenv (char *envsetting_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXqpdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi,
                   double const *x, double *dj);
    CPXLIBAPI
    int CPXPUBLIC
    CPXqpuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi,
                    double const *prepi, double const *x);
    CPXLIBAPI
    int CPXPUBLIC
    CPXreadcopybase (CPXCENVptr env, CPXLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC CPXreadcopyparam (CPXENVptr env, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXreadcopyprob (CPXCENVptr env, CPXLPptr lp,
                     char const *filename_str, char const *filetype_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXreadcopysol (CPXCENVptr env, CPXLPptr lp, char const *filename_str);
  int CPXPUBLIC
    CPXreadcopyvec (CPXCENVptr env, CPXLPptr lp, char const *filename_str);
  CPXVOIDptr CPXPUBLIC CPXrealloc (void *ptr, size_t size);
    CPXLIBAPI
    int CPXPUBLIC
    CPXrefineconflict (CPXCENVptr env, CPXLPptr lp, int *confnumrows_p,
                       int *confnumcols_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXrefineconflictext (CPXCENVptr env, CPXLPptr lp, int grpcnt,
                          int concnt, double const *grppref,
                          int const *grpbeg, int const *grpind,
                          char const *grptype);
    CPXLIBAPI
    int CPXPUBLIC
    CPXrhssa (CPXCENVptr env, CPXCLPptr lp, int begin, int end,
              double *lower, double *upper);
    CPXLIBAPI
    int CPXPUBLIC
    CPXrobustopt (CPXCENVptr env, CPXLPptr lp, CPXLPptr lblp,
                  CPXLPptr ublp, double objchg, double const *maxchg);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsavwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI int CPXPUBLIC CPXserializercreate (CPXSERIALIZERptr * ser_p);
    CPXLIBAPI void CPXPUBLIC CPXserializerdestroy (CPXSERIALIZERptr ser);
    CPXLIBAPI CPXLONG CPXPUBLIC CPXserializerlength (CPXCSERIALIZERptr ser);
    CPXLIBAPI
    void const *CPXPUBLIC CPXserializerpayload (CPXCSERIALIZERptr ser);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsetdblparam (CPXENVptr env, int whichparam, double newvalue);
    CPXLIBAPI int CPXPUBLIC CPXsetdefaults (CPXENVptr env);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsetintparam (CPXENVptr env, int whichparam, CPXINT newvalue);
    CPXLIBAPI int CPXPUBLIC CPXsetlogfile (CPXENVptr env, CPXFILEptr lfile);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsetlongparam (CPXENVptr env, int whichparam, CPXLONG newvalue);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsetlpcallbackfunc (CPXENVptr env,
                          int (CPXPUBLIC * callback) (CPXCENVptr, void *, int,
                                                      void *),
                          void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetnetcallbackfunc (CPXENVptr env,
                                                   int (CPXPUBLIC *
                                                        callback) (CPXCENVptr,
                                                                   void *,
                                                                   int,
                                                                   void *),
                                                   void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetphase2 (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsetprofcallbackfunc (CPXENVptr env,
                            int (CPXPUBLIC * callback) (CPXCENVptr, int,
                                                        void *),
                            void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetstrparam (CPXENVptr env, int whichparam,
                                            char const *newvalue_str);
    CPXLIBAPI int CPXPUBLIC CPXsetterminate (CPXENVptr env,
                                             volatile int *terminate_p);
    CPXLIBAPI int CPXPUBLIC CPXsettuningcallbackfunc (CPXENVptr env,
                                                      int (CPXPUBLIC *
                                                           callback)
                                                      (CPXCENVptr, void *,
                                                       int, void *),
                                                      void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsiftopt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x,
                   double *slack);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsolninfo (CPXCENVptr env, CPXCLPptr lp, int *solnmethod_p,
                 int *solntype_p, int *pfeasind_p, int *dfeasind_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsolution (CPXCENVptr env, CPXCLPptr lp, int *lpstat_p,
                 double *objval_p, double *x, double *pi, double *slack,
                 double *dj);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsolwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsolwritesolnpool (CPXCENVptr env, CPXCLPptr lp, int soln,
                         char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsolwritesolnpoolall (CPXCENVptr env, CPXCLPptr lp,
                            char const *filename_str);
  CPXCHARptr CPXPUBLIC CPXstrcpy (char *dest_str, char const *src_str);
  size_t CPXPUBLIC CPXstrlen (char const *s_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXstrongbranch (CPXCENVptr env, CPXLPptr lp, int const *indices,
                     int cnt, double *downobj, double *upobj, int itlim);
    CPXLIBAPI
    int CPXPUBLIC
    CPXtightenbds (CPXCENVptr env, CPXLPptr lp, int cnt,
                   int const *indices, char const *lu, double const *bd);
    CPXLIBAPI
    int CPXPUBLIC
    CPXtuneparam (CPXENVptr env, CPXLPptr lp, int intcnt,
                  int const *intnum, int const *intval, int dblcnt,
                  int const *dblnum, double const *dblval, int strcnt,
                  int const *strnum, char **strval, int *tunestat_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXtuneparamprobset (CPXENVptr env, int filecnt, char **filename,
                         char **filetype, int intcnt, int const *intnum,
                         int const *intval, int dblcnt,
                         int const *dblnum, double const *dblval,
                         int strcnt, int const *strnum, char **strval,
                         int *tunestat_p);
  int CPXPUBLIC
    CPXtxtsolwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXuncrushform (CPXCENVptr env, CPXCLPptr lp, int plen,
                    int const *pind, double const *pval, int *len_p,
                    double *offset_p, int *ind, double *val);
    CPXLIBAPI
    int CPXPUBLIC
    CPXuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi,
                  double const *prepi);
    CPXLIBAPI
    int CPXPUBLIC
    CPXuncrushx (CPXCENVptr env, CPXCLPptr lp, double *x, double const *prex);
    CPXLIBAPI int CPXPUBLIC CPXunscaleprob (CPXCENVptr env, CPXLPptr lp);
  int CPXPUBLIC
    CPXvecwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI CPXCCHARptr CPXPUBLIC CPXversion (CPXCENVptr env);
    CPXLIBAPI int CPXPUBLIC CPXversionnumber (CPXCENVptr env, int *version_p);
    CPXLIBAPI
    int CPXPUBLIC CPXwriteparam (CPXCENVptr env, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXwriteprob (CPXCENVptr env, CPXCLPptr lp,
                  char const *filename_str, char const *filetype_str);
#ifdef __cplusplus
}
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif
#endif
#ifndef CPXBAR_H
#define CPXBAR_H 1
#include "cpxconst.h"
#ifdef _WIN32
#pragma pack(push, 8)
#endif
#ifdef __cplusplus
extern "C"
{
#endif
  CPXLIBAPI int CPXPUBLIC CPXbaropt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXhybbaropt (CPXCENVptr env, CPXLPptr lp, int method);
#ifdef __cplusplus
}
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif
#endif
#ifndef CPXMIP_H
#define CPXMIP_H 1
#ifdef _WIN32
#pragma pack(push, 8)
#endif
#ifdef __cplusplus
extern "C"
{
#endif
#define CPXgetmipobjval(env,lp,objval_p) CPXgetobjval(env, lp, objval_p)
#define CPXgetmipqconstrslack(env,lp,qcslack,begin,end) CPXgetqconstrslack(env, lp, qcslack, begin, end)
#define CPXgetmipslack(env,lp,slack,begin,end) CPXgetslack(env, lp, slack, begin, end)
#define CPXgetmipx(env,lp,x,begin,end) CPXgetx(env, lp, x, begin, end)
  CPXLIBAPI
    int CPXPUBLIC
    CPXaddlazyconstraints (CPXCENVptr env, CPXLPptr lp, int rcnt,
                           int nzcnt, double const *rhs,
                           char const *sense, int const *rmatbeg,
                           int const *rmatind, double const *rmatval,
                           char **rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXaddmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt, int nzcnt,
                     int const *beg, int const *varindices,
                     double const *values, int const *effortlevel,
                     char **mipstartname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXaddsolnpooldivfilter (CPXCENVptr env, CPXLPptr lp,
                             double lower_bound, double upper_bound,
                             int nzcnt, int const *ind,
                             double const *weight, double const *refval,
                             char const *lname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXaddsolnpoolrngfilter (CPXCENVptr env, CPXLPptr lp, double lb,
                             double ub, int nzcnt, int const *ind,
                             double const *val, char const *lname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXaddsos (CPXCENVptr env, CPXLPptr lp, int numsos, int numsosnz,
               char const *sostype, int const *sosbeg,
               int const *sosind, double const *soswt, char **sosname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXaddusercuts (CPXCENVptr env, CPXLPptr lp, int rcnt, int nzcnt,
                    double const *rhs, char const *sense,
                    int const *rmatbeg, int const *rmatind,
                    double const *rmatval, char **rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXbranchcallbackbranchasCPLEX (CPXCENVptr env, void *cbdata,
                                    int wherefrom, int num,
                                    void *userhandle, int *seqnum_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXbranchcallbackbranchbds (CPXCENVptr env, void *cbdata,
                                int wherefrom, int cnt,
                                int const *indices, char const *lu,
                                double const *bd, double nodeest,
                                void *userhandle, int *seqnum_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXbranchcallbackbranchconstraints (CPXCENVptr env, void *cbdata,
                                        int wherefrom, int rcnt,
                                        int nzcnt, double const *rhs,
                                        char const *sense,
                                        int const *rmatbeg,
                                        int const *rmatind,
                                        double const *rmatval,
                                        double nodeest,
                                        void *userhandle, int *seqnum_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXbranchcallbackbranchgeneral (CPXCENVptr env, void *cbdata,
                                    int wherefrom, int varcnt,
                                    int const *varind,
                                    char const *varlu,
                                    double const *varbd, int rcnt,
                                    int nzcnt, double const *rhs,
                                    char const *sense,
                                    int const *rmatbeg,
                                    int const *rmatind,
                                    double const *rmatval,
                                    double nodeest, void *userhandle,
                                    int *seqnum_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcallbacksetnodeuserhandle (CPXCENVptr env, void *cbdata,
                                  int wherefrom, int nodeindex,
                                  void *userhandle, void **olduserhandle_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcallbacksetuserhandle (CPXCENVptr env, void *cbdata,
                              int wherefrom, void *userhandle,
                              void **olduserhandle_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgctype (CPXCENVptr env, CPXLPptr lp, int cnt,
                 int const *indices, char const *xctype);
    CPXLIBAPI
    int CPXPUBLIC
    CPXchgmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt,
                     int const *mipstartindices, int nzcnt,
                     int const *beg, int const *varindices,
                     double const *values, int const *effortlevel);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopyctype (CPXCENVptr env, CPXLPptr lp, char const *xctype);
  int CPXPUBLIC
    CPXcopymipstart (CPXCENVptr env, CPXLPptr lp, int cnt,
                     int const *indices, double const *values);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopyorder (CPXCENVptr env, CPXLPptr lp, int cnt,
                  int const *indices, int const *priority,
                  int const *direction);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopysos (CPXCENVptr env, CPXLPptr lp, int numsos, int numsosnz,
                char const *sostype, int const *sosbeg,
                int const *sosind, double const *soswt, char **sosname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcutcallbackadd (CPXCENVptr env, void *cbdata, int wherefrom,
                       int nzcnt, double rhs, int sense,
                       int const *cutind, double const *cutval,
                       int purgeable);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcutcallbackaddlocal (CPXCENVptr env, void *cbdata, int wherefrom,
                            int nzcnt, double rhs, int sense,
                            int const *cutind, double const *cutval);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelindconstrs (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelmipstarts (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelsetmipstarts (CPXCENVptr env, CPXLPptr lp, int *delstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelsetsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int *delstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelsetsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int *delstat);
    CPXLIBAPI
    int CPXPUBLIC CPXdelsetsos (CPXCENVptr env, CPXLPptr lp, int *delset);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC CPXdelsos (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXfltwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC CPXfreelazyconstraints (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXfreeusercuts (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetbestobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetbranchcallbackfunc (CPXCENVptr env,
                              int (CPXPUBLIC **
                                   branchcallback_p) (CALLBACK_BRANCH_ARGS),
                              void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetbranchnosolncallbackfunc (CPXCENVptr env,
                                                            int (CPXPUBLIC **
                                                                 branchnosolncallback_p)
                                                            (CALLBACK_BRANCH_ARGS),
                                                            void
                                                            **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbackbranchconstraints (CPXCENVptr env,
                                                             void *cbdata,
                                                             int wherefrom,
                                                             int which,
                                                             int *cuts_p,
                                                             int *nzcnt_p,
                                                             double *rhs,
                                                             char *sense,
                                                             int *rmatbeg,
                                                             int *rmatind,
                                                             double *rmatval,
                                                             int rmatsz,
                                                             int *surplus_p);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbackctype (CPXCENVptr env, void *cbdata,
                                                 int wherefrom, char *xctype,
                                                 int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbackgloballb (CPXCENVptr env,
                                                    void *cbdata,
                                                    int wherefrom, double *lb,
                                                    int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbackglobalub (CPXCENVptr env,
                                                    void *cbdata,
                                                    int wherefrom, double *ub,
                                                    int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbackincumbent (CPXCENVptr env,
                                                     void *cbdata,
                                                     int wherefrom, double *x,
                                                     int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbackindicatorinfo (CPXCENVptr env,
                                                         void *cbdata,
                                                         int wherefrom,
                                                         int iindex,
                                                         int whichinfo,
                                                         void *result_p);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbacklp (CPXCENVptr env, void *cbdata,
                                              int wherefrom,
                                              CPXCLPptr * lp_p);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeinfo (CPXCENVptr env,
                                                    void *cbdata,
                                                    int wherefrom,
                                                    int nodeindex,
                                                    int whichinfo,
                                                    void *result_p);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeintfeas (CPXCENVptr env,
                                                       void *cbdata,
                                                       int wherefrom,
                                                       int *feas, int begin,
                                                       int end);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodelb (CPXCENVptr env,
                                                  void *cbdata, int wherefrom,
                                                  double *lb, int begin,
                                                  int end);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodelp (CPXCENVptr env,
                                                  void *cbdata, int wherefrom,
                                                  CPXLPptr * nodelp_p);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeobjval (CPXCENVptr env,
                                                      void *cbdata,
                                                      int wherefrom,
                                                      double *objval_p);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodestat (CPXCENVptr env,
                                                    void *cbdata,
                                                    int wherefrom,
                                                    int *nodestat_p);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodeub (CPXCENVptr env,
                                                  void *cbdata, int wherefrom,
                                                  double *ub, int begin,
                                                  int end);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbacknodex (CPXCENVptr env, void *cbdata,
                                                 int wherefrom, double *x,
                                                 int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbackorder (CPXCENVptr env, void *cbdata,
                                                 int wherefrom, int *priority,
                                                 int *direction, int begin,
                                                 int end);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbackpseudocosts (CPXCENVptr env,
                                                       void *cbdata,
                                                       int wherefrom,
                                                       double *uppc,
                                                       double *downpc,
                                                       int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbackseqinfo (CPXCENVptr env,
                                                   void *cbdata,
                                                   int wherefrom, int seqid,
                                                   int whichinfo,
                                                   void *result_p);
    CPXLIBAPI int CPXPUBLIC CPXgetcallbacksosinfo (CPXCENVptr env,
                                                   void *cbdata,
                                                   int wherefrom,
                                                   int sosindex, int member,
                                                   int whichinfo,
                                                   void *result_p);
    CPXLIBAPI int CPXPUBLIC CPXgetctype (CPXCENVptr env, CPXCLPptr lp,
                                         char *xctype, int begin, int end);
  int CPXPUBLIC CPXgetcutcallbackfunc (CPXCENVptr env,
                                       int (CPXPUBLIC **
                                            cutcallback_p)
                                       (CALLBACK_CUT_ARGS),
                                       void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetcutoff (CPXCENVptr env, CPXCLPptr lp,
                                          double *cutoff_p);
    CPXLIBAPI int CPXPUBLIC CPXgetdeletenodecallbackfunc (CPXCENVptr env,
                                                          void (CPXPUBLIC **
                                                                deletecallback_p)
                                                          (CALLBACK_DELETENODE_ARGS),
                                                          void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetheuristiccallbackfunc (CPXCENVptr env,
                                                         int (CPXPUBLIC **
                                                              heuristiccallback_p)
                                                         (CALLBACK_HEURISTIC_ARGS),
                                                         void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetincumbentcallbackfunc (CPXCENVptr env,
                                                         int (CPXPUBLIC **
                                                              incumbentcallback_p)
                                                         (CALLBACK_INCUMBENT_ARGS),
                                                         void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetindconstr (CPXCENVptr env, CPXCLPptr lp,
                                             int *indvar_p,
                                             int *complemented_p,
                                             int *nzcnt_p, double *rhs_p,
                                             char *sense_p, int *linind,
                                             double *linval, int space,
                                             int *surplus_p, int which);
    CPXLIBAPI int CPXPUBLIC CPXgetindconstrindex (CPXCENVptr env,
                                                  CPXCLPptr lp,
                                                  char const *lname_str,
                                                  int *index_p);
    CPXLIBAPI int CPXPUBLIC CPXgetindconstrinfeas (CPXCENVptr env,
                                                   CPXCLPptr lp,
                                                   double const *x,
                                                   double *infeasout,
                                                   int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetindconstrname (CPXCENVptr env, CPXCLPptr lp,
                                                 char *buf_str, int bufspace,
                                                 int *surplus_p, int which);
    CPXLIBAPI int CPXPUBLIC CPXgetindconstrslack (CPXCENVptr env,
                                                  CPXCLPptr lp,
                                                  double *indslack, int begin,
                                                  int end);
    CPXLIBAPI int CPXPUBLIC CPXgetinfocallbackfunc (CPXCENVptr env,
                                                    int (CPXPUBLIC **
                                                         callback_p)
                                                    (CPXCENVptr, void *, int,
                                                     void *),
                                                    void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetlazyconstraintcallbackfunc (CPXCENVptr env,
                                                              int (CPXPUBLIC
                                                                   **
                                                                   cutcallback_p)
                                                              (CALLBACK_CUT_ARGS),
                                                              void
                                                              **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetmipcallbackfunc (CPXCENVptr env,
                                                   int (CPXPUBLIC **
                                                        callback_p)
                                                   (CPXCENVptr, void *, int,
                                                    void *),
                                                   void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetmipitcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetmiprelgap (CPXCENVptr env, CPXCLPptr lp, double *gap_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetmipstartindex (CPXCENVptr env, CPXCLPptr lp,
                         char const *lname_str, int *index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetmipstartname (CPXCENVptr env, CPXCLPptr lp, char **name,
                        char *store, int storesz, int *surplus_p,
                        int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetmipstarts (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p,
                     int *beg, int *varindices, double *values,
                     int *effortlevel, int startspace, int *surplus_p,
                     int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetnodecallbackfunc (CPXCENVptr env,
                            int (CPXPUBLIC **
                                 nodecallback_p) (CALLBACK_NODE_ARGS),
                            void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetnodecnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnodeint (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnodeleftcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnumbin (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetnumcuts (CPXCENVptr env, CPXCLPptr lp, int cuttype, int *num_p);
    CPXLIBAPI
    int CPXPUBLIC CPXgetnumindconstrs (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnumint (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXgetnumlazyconstraints (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnummipstarts (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnumsemicont (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnumsemiint (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnumsos (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnumusercuts (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetorder (CPXCENVptr env, CPXCLPptr lp, int *cnt_p, int *indices,
                 int *priority, int *direction, int ordspace, int *surplus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpooldivfilter (CPXCENVptr env, CPXCLPptr lp,
                             double *lower_cutoff_p,
                             double *upper_cutoff_p, int *nzcnt_p,
                             int *ind, double *val, double *refval,
                             int space, int *surplus_p, int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolfilterindex (CPXCENVptr env, CPXCLPptr lp,
                               char const *lname_str, int *index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolfiltername (CPXCENVptr env, CPXCLPptr lp,
                              char *buf_str, int bufspace,
                              int *surplus_p, int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolfiltertype (CPXCENVptr env, CPXCLPptr lp,
                              int *ftype_p, int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolmeanobjval (CPXCENVptr env, CPXCLPptr lp,
                              double *meanobjval_p);
    CPXLIBAPI
    int CPXPUBLIC CPXgetsolnpoolnumfilters (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXgetsolnpoolnumreplaced (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXgetsolnpoolnumsolns (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolobjval (CPXCENVptr env, CPXCLPptr lp, int soln,
                          double *objval_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolqconstrslack (CPXCENVptr env, CPXCLPptr lp, int soln,
                                double *qcslack, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolrngfilter (CPXCENVptr env, CPXCLPptr lp, double *lb_p,
                             double *ub_p, int *nzcnt_p, int *ind,
                             double *val, int space, int *surplus_p,
                             int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolslack (CPXCENVptr env, CPXCLPptr lp, int soln,
                         double *slack, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolsolnindex (CPXCENVptr env, CPXCLPptr lp,
                             char const *lname_str, int *index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolsolnname (CPXCENVptr env, CPXCLPptr lp, char *store,
                            int storesz, int *surplus_p, int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolnpoolx (CPXCENVptr env, CPXCLPptr lp, int soln, double *x,
                     int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetsolvecallbackfunc (CPXCENVptr env,
                             int (CPXPUBLIC **
                                  solvecallback_p) (CALLBACK_SOLVE_ARGS),
                             void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXgetsos (CPXCENVptr env, CPXCLPptr lp,
                                       int *numsosnz_p, char *sostype,
                                       int *sosbeg, int *sosind,
                                       double *soswt, int sosspace,
                                       int *surplus_p, int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetsosindex (CPXCENVptr env, CPXCLPptr lp,
                                            char const *lname_str,
                                            int *index_p);
    CPXLIBAPI int CPXPUBLIC CPXgetsosinfeas (CPXCENVptr env, CPXCLPptr lp,
                                             double const *x,
                                             double *infeasout, int begin,
                                             int end);
    CPXLIBAPI int CPXPUBLIC CPXgetsosname (CPXCENVptr env, CPXCLPptr lp,
                                           char **name, char *namestore,
                                           int storespace, int *surplus_p,
                                           int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetsubmethod (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetsubstat (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetusercutcallbackfunc (CPXCENVptr env,
                               int (CPXPUBLIC **
                                    cutcallback_p) (CALLBACK_CUT_ARGS),
                               void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXindconstrslackfromx (CPXCENVptr env,
                                                    CPXCLPptr lp,
                                                    double const *x,
                                                    double *indslack);
    CPXLIBAPI int CPXPUBLIC CPXmipopt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXordread (CPXCENVptr env, char const *filename_str, int numcols,
                char **colname, int *cnt_p, int *indices, int *priority,
                int *direction);
    CPXLIBAPI
    int CPXPUBLIC
    CPXordwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI int CPXPUBLIC CPXpopulate (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXreadcopymipstarts (CPXCENVptr env, CPXLPptr lp,
                          char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXreadcopyorder (CPXCENVptr env, CPXLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXreadcopysolnpoolfilters (CPXCENVptr env, CPXLPptr lp,
                                char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXrefinemipstartconflict (CPXCENVptr env, CPXLPptr lp,
                               int mipstartindex, int *confnumrows_p,
                               int *confnumcols_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXrefinemipstartconflictext (CPXCENVptr env, CPXLPptr lp,
                                  int mipstartindex, int grpcnt,
                                  int concnt, double const *grppref,
                                  int const *grpbeg, int const *grpind,
                                  char const *grptype);
    CPXLIBAPI
    int CPXPUBLIC
    CPXsetbranchcallbackfunc (CPXENVptr env,
                              int (CPXPUBLIC *
                                   branchcallback) (CALLBACK_BRANCH_ARGS),
                              void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetbranchnosolncallbackfunc (CPXENVptr env,
                                                            int (CPXPUBLIC *
                                                                 branchnosolncallback)
                                                            (CALLBACK_BRANCH_ARGS),
                                                            void *cbhandle);
  int CPXPUBLIC CPXsetcutcallbackfunc (CPXENVptr env,
                                       int (CPXPUBLIC *
                                            cutcallback) (CALLBACK_CUT_ARGS),
                                       void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetdeletenodecallbackfunc (CPXENVptr env,
                                                          void (CPXPUBLIC *
                                                                deletecallback)
                                                          (CALLBACK_DELETENODE_ARGS),
                                                          void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetheuristiccallbackfunc (CPXENVptr env,
                                                         int (CPXPUBLIC *
                                                              heuristiccallback)
                                                         (CALLBACK_HEURISTIC_ARGS),
                                                         void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetincumbentcallbackfunc (CPXENVptr env,
                                                         int (CPXPUBLIC *
                                                              incumbentcallback)
                                                         (CALLBACK_INCUMBENT_ARGS),
                                                         void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetinfocallbackfunc (CPXENVptr env,
                                                    int (CPXPUBLIC *
                                                         callback)
                                                    (CPXCENVptr, void *, int,
                                                     void *), void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetlazyconstraintcallbackfunc (CPXENVptr env,
                                                              int (CPXPUBLIC *
                                                                   lazyconcallback)
                                                              (CALLBACK_CUT_ARGS),
                                                              void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetmipcallbackfunc (CPXENVptr env,
                                                   int (CPXPUBLIC *
                                                        callback) (CPXCENVptr,
                                                                   void *,
                                                                   int,
                                                                   void *),
                                                   void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetnodecallbackfunc (CPXENVptr env,
                                                    int (CPXPUBLIC *
                                                         nodecallback)
                                                    (CALLBACK_NODE_ARGS),
                                                    void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetsolvecallbackfunc (CPXENVptr env,
                                                     int (CPXPUBLIC *
                                                          solvecallback)
                                                     (CALLBACK_SOLVE_ARGS),
                                                     void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXsetusercutcallbackfunc (CPXENVptr env,
                                                       int (CPXPUBLIC *
                                                            cutcallback)
                                                       (CALLBACK_CUT_ARGS),
                                                       void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXwritemipstarts (CPXCENVptr env, CPXCLPptr lp,
                                               char const *filename_str,
                                               int begin, int end);
#ifdef __cplusplus
}
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif
#endif
#ifndef CPXGC_H
#define CPXGC_H 1
#ifdef _WIN32
#pragma pack(push, 8)
#endif
#ifdef __cplusplus
extern "C"
{
#endif
  CPXLIBAPI
    int CPXPUBLIC
    CPXaddindconstr (CPXCENVptr env, CPXLPptr lp, int indvar,
                     int complemented, int nzcnt, double rhs, int sense,
                     int const *linind, double const *linval,
                     char const *indname_str);
#ifdef __cplusplus
}
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif
#endif
#ifndef CPXNET_H
#define CPXNET_H 1
#ifdef _WIN32
#pragma pack(push, 8)
#endif
#ifdef __cplusplus
extern "C"
{
#endif
  CPXLIBAPI
    int CPXPUBLIC
    CPXNETaddarcs (CPXCENVptr env, CPXNETptr net, int narcs,
                   int const *fromnode, int const *tonode,
                   double const *low, double const *up,
                   double const *obj, char **anames);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETaddnodes (CPXCENVptr env, CPXNETptr net, int nnodes,
                    double const *supply, char **name);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETbasewrite (CPXCENVptr env, CPXCNETptr net,
                     char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETchgarcname (CPXCENVptr env, CPXNETptr net, int cnt,
                      int const *indices, char **newname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETchgarcnodes (CPXCENVptr env, CPXNETptr net, int cnt,
                       int const *indices, int const *fromnode,
                       int const *tonode);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETchgbds (CPXCENVptr env, CPXNETptr net, int cnt,
                  int const *indices, char const *lu, double const *bd);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETchgname (CPXCENVptr env, CPXNETptr net, int key, int vindex,
                   char const *name_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETchgnodename (CPXCENVptr env, CPXNETptr net, int cnt,
                       int const *indices, char **newname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETchgobj (CPXCENVptr env, CPXNETptr net, int cnt,
                  int const *indices, double const *obj);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETchgobjsen (CPXCENVptr env, CPXNETptr net, int maxormin);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETchgsupply (CPXCENVptr env, CPXNETptr net, int cnt,
                     int const *indices, double const *supply);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETcopybase (CPXCENVptr env, CPXNETptr net, int const *astat,
                    int const *nstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETcopynet (CPXCENVptr env, CPXNETptr net, int objsen,
                   int nnodes, double const *supply, char **nnames,
                   int narcs, int const *fromnode, int const *tonode,
                   double const *low, double const *up,
                   double const *obj, char **anames);
    CPXLIBAPI
    CPXNETptr CPXPUBLIC
    CPXNETcreateprob (CPXENVptr env, int *status_p, char const *name_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETdelarcs (CPXCENVptr env, CPXNETptr net, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETdelnodes (CPXCENVptr env, CPXNETptr net, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETdelset (CPXCENVptr env, CPXNETptr net, int *whichnodes,
                  int *whicharcs);
    CPXLIBAPI int CPXPUBLIC CPXNETfreeprob (CPXENVptr env, CPXNETptr * net_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetarcindex (CPXCENVptr env, CPXCNETptr net,
                       char const *lname_str, int *index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetarcname (CPXCENVptr env, CPXCNETptr net, char **nnames,
                      char *namestore, int namespc, int *surplus_p,
                      int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetarcnodes (CPXCENVptr env, CPXCNETptr net, int *fromnode,
                       int *tonode, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetbase (CPXCENVptr env, CPXCNETptr net, int *astat, int *nstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetdj (CPXCENVptr env, CPXCNETptr net, double *dj, int begin,
                 int end);
    CPXLIBAPI int CPXPUBLIC CPXNETgetitcnt (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetlb (CPXCENVptr env, CPXCNETptr net, double *low, int begin,
                 int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetnodearcs (CPXCENVptr env, CPXCNETptr net, int *arccnt_p,
                       int *arcbeg, int *arc, int arcspace,
                       int *surplus_p, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetnodeindex (CPXCENVptr env, CPXCNETptr net,
                        char const *lname_str, int *index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetnodename (CPXCENVptr env, CPXCNETptr net, char **nnames,
                       char *namestore, int namespc, int *surplus_p,
                       int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXNETgetnumarcs (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC CPXNETgetnumnodes (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetobj (CPXCENVptr env, CPXCNETptr net, double *obj,
                  int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXNETgetobjsen (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetobjval (CPXCENVptr env, CPXCNETptr net, double *objval_p);
    CPXLIBAPI
    int CPXPUBLIC CPXNETgetphase1cnt (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetpi (CPXCENVptr env, CPXCNETptr net, double *pi, int begin,
                 int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetprobname (CPXCENVptr env, CPXCNETptr net, char *buf_str,
                       int bufspace, int *surplus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetslack (CPXCENVptr env, CPXCNETptr net, double *slack,
                    int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXNETgetstat (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetsupply (CPXCENVptr env, CPXCNETptr net, double *supply,
                     int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetub (CPXCENVptr env, CPXCNETptr net, double *up, int begin,
                 int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETgetx (CPXCENVptr env, CPXCNETptr net, double *x, int begin,
                int end);
    CPXLIBAPI int CPXPUBLIC CPXNETprimopt (CPXCENVptr env, CPXNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETreadcopybase (CPXCENVptr env, CPXNETptr net,
                        char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETreadcopyprob (CPXCENVptr env, CPXNETptr net,
                        char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETsolninfo (CPXCENVptr env, CPXCNETptr net, int *pfeasind_p,
                    int *dfeasind_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETsolution (CPXCENVptr env, CPXCNETptr net, int *netstat_p,
                    double *objval_p, double *x, double *pi,
                    double *slack, double *dj);
    CPXLIBAPI
    int CPXPUBLIC
    CPXNETwriteprob (CPXCENVptr env, CPXCNETptr net,
                     char const *filename_str, char const *format_str);
#ifdef __cplusplus
}
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif
#endif
#ifndef CPXQP_H
#define CPXQP_H 1
#ifdef _WIN32
#pragma pack(push, 8)
#endif
#ifdef __cplusplus
extern "C"
{
#endif
  CPXLIBAPI
    int CPXPUBLIC
    CPXchgqpcoef (CPXCENVptr env, CPXLPptr lp, int i, int j, double newvalue);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopyqpsep (CPXCENVptr env, CPXLPptr lp, double const *qsepvec);
    CPXLIBAPI
    int CPXPUBLIC
    CPXcopyquad (CPXCENVptr env, CPXLPptr lp, int const *qmatbeg,
                 int const *qmatcnt, int const *qmatind,
                 double const *qmatval);
    CPXLIBAPI int CPXPUBLIC CPXgetnumqpnz (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXgetnumquad (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetqpcoef (CPXCENVptr env, CPXCLPptr lp, int rownum, int colnum,
                  double *coef_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetquad (CPXCENVptr env, CPXCLPptr lp, int *nzcnt_p,
                int *qmatbeg, int *qmatind, double *qmatval,
                int qmatspace, int *surplus_p, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXqpindefcertificate (CPXCENVptr env, CPXCLPptr lp, double *x);
    CPXLIBAPI int CPXPUBLIC CPXqpopt (CPXCENVptr env, CPXLPptr lp);
#ifdef __cplusplus
}
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif
#endif
#ifndef CPXSOCP_H
#define CPXSOCP_H 1
#ifdef _WIN32
#pragma pack(push, 8)
#endif
#ifdef __cplusplus
extern "C"
{
#endif
  CPXLIBAPI
    int CPXPUBLIC
    CPXaddqconstr (CPXCENVptr env, CPXLPptr lp, int linnzcnt,
                   int quadnzcnt, double rhs, int sense,
                   int const *linind, double const *linval,
                   int const *quadrow, int const *quadcol,
                   double const *quadval, char const *lname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXdelqconstrs (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXgetnumqconstrs (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetqconstr (CPXCENVptr env, CPXCLPptr lp, int *linnzcnt_p,
                   int *quadnzcnt_p, double *rhs_p, char *sense_p,
                   int *linind, double *linval, int linspace,
                   int *linsurplus_p, int *quadrow, int *quadcol,
                   double *quadval, int quadspace, int *quadsurplus_p,
                   int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetqconstrdslack (CPXCENVptr env, CPXCLPptr lp, int qind,
                         int *nz_p, int *ind, double *val, int space,
                         int *surplus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetqconstrindex (CPXCENVptr env, CPXCLPptr lp,
                        char const *lname_str, int *index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetqconstrinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x,
                         double *infeasout, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetqconstrname (CPXCENVptr env, CPXCLPptr lp, char *buf_str,
                       int bufspace, int *surplus_p, int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetqconstrslack (CPXCENVptr env, CPXCLPptr lp, double *qcslack,
                        int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXgetxqxax (CPXCENVptr env, CPXCLPptr lp, double *xqxax, int begin,
                 int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXqconstrslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x,
                          double *qcslack);
#ifdef __cplusplus
}
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#endif                          /* _CPLEX_H */
#ifndef _CPLEXL_H
#define _CPLEXL_H

/*
 * This header was partly generated from a solver header of the same name,
 * to make lazylpsolverlibs callable. It contains only constants,
 * structures, and macros generated from the original header, and thus,
 * contains no copyrightable information.
 *
 * Additionnal hand editing was also probably performed.
 */

#include "cpxconst.h"
#ifdef _WIN32
#pragma pack(push, 8)
#endif
#ifdef __cplusplus
extern "C"
{
#endif
#if defined(__x86_64__) || defined(__ia64) || defined(_WIN64) || defined(__powerpc64__) || defined(__64BIT__) || defined(__sparcv9) || defined(__LP64__)
#define CPX_CPLEX_L_API_DEFINED 1
#define CPXL_CALLBACK_ARGS CPXCENVptr env, void *cbdata, int wherefrom, \
      void *cbhandle
#define CPXL_CALLBACK_PROF_ARGS CPXCENVptr env, int wherefrom, void *cbhandle
#define CPXL_CALLBACK_BRANCH_ARGS CPXCENVptr xenv, void *cbdata, \
      int wherefrom, void *cbhandle, int brtype, CPXINT brset, \
      int nodecnt, CPXINT bdcnt, \
      const CPXINT *nodebeg, const CPXINT *xindex, const char *lu, \
      const double *bd, const double *nodeest,int *useraction_p
#define CPXL_CALLBACK_NODE_ARGS CPXCENVptr xenv, void *cbdata, \
      int wherefrom, void *cbhandle, CPXLONG *nodeindex, int *useraction
#define CPXL_CALLBACK_HEURISTIC_ARGS CPXCENVptr xenv, void *cbdata, \
      int wherefrom, void *cbhandle, double *objval_p, double *x, \
      int *checkfeas_p, int *useraction_p
#define CPXL_CALLBACK_SOLVE_ARGS CPXCENVptr xenv, void *cbdata, \
      int wherefrom, void *cbhandle, int *useraction
#define CPXL_CALLBACK_CUT_ARGS CPXCENVptr xenv, void *cbdata, \
      int wherefrom, void *cbhandle, int *useraction_p
#define CPXL_CALLBACK_INCUMBENT_ARGS CPXCENVptr xenv, void *cbdata, \
      int wherefrom, void *cbhandle, double objval, double *x, \
      int *isfeas_p, int *useraction_p
#define CPXL_CALLBACK_DELETENODE_ARGS CPXCENVptr xenv, \
   int wherefrom, void *cbhandle, CPXLONG seqnum, void *handle
  typedef int (CPXPUBLIC CPXL_CALLBACK) (CPXL_CALLBACK_ARGS);
  typedef int (CPXPUBLIC CPXL_CALLBACK_PROF) (CPXL_CALLBACK_PROF_ARGS);
  typedef int (CPXPUBLIC CPXL_CALLBACK_BRANCH) (CPXL_CALLBACK_BRANCH_ARGS);
  typedef int (CPXPUBLIC CPXL_CALLBACK_NODE) (CPXL_CALLBACK_NODE_ARGS);
  typedef int (CPXPUBLIC
               CPXL_CALLBACK_HEURISTIC) (CPXL_CALLBACK_HEURISTIC_ARGS);
  typedef int (CPXPUBLIC CPXL_CALLBACK_SOLVE) (CPXL_CALLBACK_SOLVE_ARGS);
  typedef int (CPXPUBLIC CPXL_CALLBACK_CUT) (CPXL_CALLBACK_CUT_ARGS);
  typedef int (CPXPUBLIC
               CPXL_CALLBACK_INCUMBENT) (CPXL_CALLBACK_INCUMBENT_ARGS);
  typedef void (CPXPUBLIC
                CPXL_CALLBACK_DELETENODE) (CPXL_CALLBACK_DELETENODE_ARGS);
    CPXLIBAPI CPXCHANNELptr CPXPUBLIC CPXLaddchannel (CPXENVptr env);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddcols (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt,
                 CPXLONG nzcnt, double const *obj,
                 CPXLONG const *cmatbeg, CPXINT const *cmatind,
                 double const *cmatval, double const *lb,
                 double const *ub, char const *const *colname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddfuncdest (CPXCENVptr env, CPXCHANNELptr channel,
                     void *handle,
                     void (CPXPUBLIC * msgfunction) (void *, char const *));
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddrows (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt, CPXINT rcnt,
                 CPXLONG nzcnt, double const *rhs, char const *sense,
                 CPXLONG const *rmatbeg, CPXINT const *rmatind,
                 double const *rmatval, char const *const *colname,
                 char const *const *rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLbasicpresolve (CPXCENVptr env, CPXLPptr lp, double *redlb,
                       double *redub, int *rstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLbinvacol (CPXCENVptr env, CPXCLPptr lp, CPXINT j, double *x);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLbinvarow (CPXCENVptr env, CPXCLPptr lp, CPXINT i, double *z);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLbinvcol (CPXCENVptr env, CPXCLPptr lp, CPXINT j, double *x);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLbinvrow (CPXCENVptr env, CPXCLPptr lp, CPXINT i, double *y);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLboundsa (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end,
                 double *lblower, double *lbupper, double *ublower,
                 double *ubupper);
    CPXLIBAPI
    int CPXPUBLIC CPXLbtran (CPXCENVptr env, CPXCLPptr lp, double *y);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcheckdfeas (CPXCENVptr env, CPXLPptr lp, CPXINT * infeas_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcheckpfeas (CPXCENVptr env, CPXLPptr lp, CPXINT * infeas_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchecksoln (CPXCENVptr env, CPXLPptr lp, int *lpstatus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgbds (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                CPXINT const *indices, char const *lu, double const *bd);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgcoef (CPXCENVptr env, CPXLPptr lp, CPXINT i, CPXINT j,
                 double newvalue);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgcoeflist (CPXCENVptr env, CPXLPptr lp, CPXLONG numcoefs,
                     CPXINT const *rowlist, CPXINT const *collist,
                     double const *vallist);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgcolname (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                    CPXINT const *indices, char const *const *newname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgname (CPXCENVptr env, CPXLPptr lp, int key, CPXINT ij,
                 char const *newname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgobj (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                CPXINT const *indices, double const *values);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgobjoffset (CPXCENVptr env, CPXLPptr lp, double offset);
    CPXLIBAPI
    int CPXPUBLIC CPXLchgobjsen (CPXCENVptr env, CPXLPptr lp, int maxormin);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgprobname (CPXCENVptr env, CPXLPptr lp, char const *probname);
    CPXLIBAPI
    int CPXPUBLIC CPXLchgprobtype (CPXCENVptr env, CPXLPptr lp, int type);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgprobtypesolnpool (CPXCENVptr env, CPXLPptr lp, int type, int soln);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgrhs (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                CPXINT const *indices, double const *values);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgrngval (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                   CPXINT const *indices, double const *values);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgrowname (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                    CPXINT const *indices, char const *const *newname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgsense (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                  CPXINT const *indices, char const *sense);
    CPXLIBAPI
    int CPXPUBLIC CPXLcleanup (CPXCENVptr env, CPXLPptr lp, double eps);
    CPXLIBAPI
    CPXLPptr CPXPUBLIC
    CPXLcloneprob (CPXCENVptr env, CPXCLPptr lp, int *status_p);
    CPXLIBAPI int CPXPUBLIC CPXLcloseCPLEX (CPXENVptr * env_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLclpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI int CPXPUBLIC CPXLcompletelp (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopybase (CPXCENVptr env, CPXLPptr lp, int const *cstat,
                  int const *rstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopybasednorms (CPXCENVptr env, CPXLPptr lp, int const *cstat,
                        int const *rstat, double const *dnorm);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopydnorms (CPXCENVptr env, CPXLPptr lp, double const *norm,
                    CPXINT const *head, CPXINT len);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopylp (CPXCENVptr env, CPXLPptr lp, CPXINT numcols,
                CPXINT numrows, int objsense, double const *objective,
                double const *rhs, char const *sense,
                CPXLONG const *matbeg, CPXINT const *matcnt,
                CPXINT const *matind, double const *matval,
                double const *lb, double const *ub, double const *rngval);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopylpwnames (CPXCENVptr env, CPXLPptr lp, CPXINT numcols,
                      CPXINT numrows, int objsense,
                      double const *objective, double const *rhs,
                      char const *sense, CPXLONG const *matbeg,
                      CPXINT const *matcnt, CPXINT const *matind,
                      double const *matval, double const *lb,
                      double const *ub, double const *rngval,
                      char const *const *colname, char const *const *rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopynettolp (CPXCENVptr env, CPXLPptr lp, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopyobjname (CPXCENVptr env, CPXLPptr lp, char const *objname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopypartialbase (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt,
                         CPXINT const *cindices, int const *cstat,
                         CPXINT rcnt, CPXINT const *rindices,
                         int const *rstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopypnorms (CPXCENVptr env, CPXLPptr lp, double const *cnorm,
                    double const *rnorm, CPXINT len);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopyprotected (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                       CPXINT const *indices);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopystart (CPXCENVptr env, CPXLPptr lp, int const *cstat,
                   int const *rstat, double const *cprim,
                   double const *rprim, double const *cdual,
                   double const *rdual);
    CPXLIBAPI
    CPXLPptr CPXPUBLIC
    CPXLcreateprob (CPXCENVptr env, int *status_p, char const *probname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcrushform (CPXCENVptr env, CPXCLPptr lp, CPXINT len,
                   CPXINT const *ind, double const *val, CPXINT * plen_p,
                   double *poffset_p, CPXINT * pind, double *pval);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcrushpi (CPXCENVptr env, CPXCLPptr lp, double const *pi,
                 double *prepi);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcrushx (CPXCENVptr env, CPXCLPptr lp, double const *x, double *prex);
    CPXLIBAPI
    int CPXPUBLIC CPXLdelchannel (CPXENVptr env, CPXCHANNELptr * channel_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelcols (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelfpdest (CPXCENVptr env, CPXCHANNELptr channel, CPXFILEptr fileptr);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelfuncdest (CPXCENVptr env, CPXCHANNELptr channel,
                     void *handle,
                     void (CPXPUBLIC * msgfunction) (void *, char const *));
    CPXLIBAPI int CPXPUBLIC CPXLdelnames (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelrows (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelsetcols (CPXCENVptr env, CPXLPptr lp, CPXINT * delstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelsetrows (CPXCENVptr env, CPXLPptr lp, CPXINT * delstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdeserializercreate (CPXDESERIALIZERptr * deser_p, CPXLONG size,
                            void const *buffer);
    CPXLIBAPI
    void CPXPUBLIC CPXLdeserializerdestroy (CPXDESERIALIZERptr deser);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLdeserializerleft (CPXCDESERIALIZERptr deser);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdisconnectchannel (CPXCENVptr env, CPXCHANNELptr channel);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi, double *dj);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdperwrite (CPXCENVptr env, CPXLPptr lp,
                   char const *filename_str, double epsilon);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdratio (CPXCENVptr env, CPXLPptr lp, CPXINT * indices,
                CPXINT cnt, double *downratio, double *upratio,
                CPXINT * downenter, CPXINT * upenter, int *downstatus,
                int *upstatus);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdualfarkas (CPXCENVptr env, CPXCLPptr lp, double *y, double *proof_p);
    CPXLIBAPI int CPXPUBLIC CPXLdualopt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdualwrite (CPXCENVptr env, CPXCLPptr lp,
                   char const *filename_str, double *objshift_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLembwrite (CPXCENVptr env, CPXLPptr lp, char const *filename_str);
    CPXLIBAPI int CPXPUBLIC CPXLfclose (CPXFILEptr stream);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLfeasopt (CPXCENVptr env, CPXLPptr lp, double const *rhs,
                 double const *rng, double const *lb, double const *ub);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLfeasoptext (CPXCENVptr env, CPXLPptr lp, CPXINT grpcnt,
                    CPXLONG concnt, double const *grppref,
                    CPXLONG const *grpbeg, CPXINT const *grpind,
                    char const *grptype);
    CPXLIBAPI void CPXPUBLIC CPXLfinalize (void);
    CPXLIBAPI
    int CPXPUBLIC CPXLflushchannel (CPXCENVptr env, CPXCHANNELptr channel);
    CPXLIBAPI int CPXPUBLIC CPXLflushstdchannels (CPXCENVptr env);
    CPXLIBAPI
    CPXFILEptr CPXPUBLIC
    CPXLfopen (char const *filename_str, char const *type_str);
    CPXLIBAPI int CPXPUBLIC CPXLfputs (char const *s_str, CPXFILEptr stream);
    CPXLIBAPI void CPXPUBLIC CPXLfree (void *ptr);
    CPXLIBAPI
    int CPXPUBLIC CPXLfreeparenv (CPXENVptr env, CPXENVptr * child_p);
    CPXLIBAPI int CPXPUBLIC CPXLfreepresolve (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXLfreeprob (CPXCENVptr env, CPXLPptr * lp_p);
    CPXLIBAPI
    int CPXPUBLIC CPXLftran (CPXCENVptr env, CPXCLPptr lp, double *x);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetax (CPXCENVptr env, CPXCLPptr lp, double *x, CPXINT begin,
               CPXINT end);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLgetbaritcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetbase (CPXCENVptr env, CPXCLPptr lp, int *cstat, int *rstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetbasednorms (CPXCENVptr env, CPXCLPptr lp, int *cstat,
                       int *rstat, double *dnorm);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetbhead (CPXCENVptr env, CPXCLPptr lp, CPXINT * head, double *x);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetcallbackinfo (CPXCENVptr env, void *cbdata, int wherefrom,
                         int whichinfo, void *result_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetchannels (CPXCENVptr env, CPXCHANNELptr * cpxresults_p,
                     CPXCHANNELptr * cpxwarning_p,
                     CPXCHANNELptr * cpxerror_p, CPXCHANNELptr * cpxlog_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetchgparam (CPXCENVptr env, int *cnt_p, int *paramnum,
                     int pspace, int *surplus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetcoef (CPXCENVptr env, CPXCLPptr lp, CPXINT i, CPXINT j,
                 double *coef_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetcolindex (CPXCENVptr env, CPXCLPptr lp,
                     char const *lname_str, CPXINT * index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetcolinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x,
                      double *infeasout, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetcolname (CPXCENVptr env, CPXCLPptr lp, char **name,
                    char *namestore, CPXSIZE storespace,
                    CPXSIZE * surplus_p, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetcols (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p,
                 CPXLONG * cmatbeg, CPXINT * cmatind, double *cmatval,
                 CPXLONG cmatspace, CPXLONG * surplus_p, CPXINT begin,
                 CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetconflict (CPXCENVptr env, CPXCLPptr lp, int *confstat_p,
                     CPXINT * rowind, int *rowbdstat,
                     CPXINT * confnumrows_p, CPXINT * colind,
                     int *colbdstat, CPXINT * confnumcols_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetconflictext (CPXCENVptr env, CPXCLPptr lp, int *grpstat,
                        CPXLONG beg, CPXLONG end);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLgetcrossdexchcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLgetcrossdpushcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLgetcrosspexchcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLgetcrossppushcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetdblparam (CPXCENVptr env, int whichparam, double *value_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetdblquality (CPXCENVptr env, CPXCLPptr lp, double *quality_p,
                       int what);
    CPXLIBAPI
    int CPXPUBLIC CPXLgetdettime (CPXCENVptr env, double *dettimestamp_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetdj (CPXCENVptr env, CPXCLPptr lp, double *dj, CPXINT begin,
               CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetdnorms (CPXCENVptr env, CPXCLPptr lp, double *norm,
                   CPXINT * head, CPXINT * len_p);
    CPXLIBAPI CPXINT CPXPUBLIC CPXLgetdsbcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXCCHARptr CPXPUBLIC
    CPXLgeterrorstring (CPXCENVptr env, int errcode, char *buffer_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetgrad (CPXCENVptr env, CPXCLPptr lp, CPXINT j, CPXINT * head,
                 double *y);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetijdiv (CPXCENVptr env, CPXCLPptr lp, CPXINT * idiv_p,
                  CPXINT * jdiv_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetijrow (CPXCENVptr env, CPXCLPptr lp, CPXINT i, CPXINT j,
                  CPXINT * row_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetintparam (CPXCENVptr env, int whichparam, CPXINT * value_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetintquality (CPXCENVptr env, CPXCLPptr lp, CPXINT * quality_p,
                       int what);
    CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetitcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetlb (CPXCENVptr env, CPXCLPptr lp, double *lb, CPXINT begin,
               CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC CPXLgetlogfile (CPXCENVptr env, CPXFILEptr * logfile_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetlongparam (CPXCENVptr env, int whichparam, CPXLONG * value_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetlpcallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p,
                           void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetmethod (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetnetcallbackfunc (CPXCENVptr env, CPXL_CALLBACK ** callback_p,
                            void **cbhandle_p);
    CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumcols (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXLgetnumcores (CPXCENVptr env, int *numcores_p);
    CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnumnz (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumrows (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetobj (CPXCENVptr env, CPXCLPptr lp, double *obj, CPXINT begin,
                CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetobjname (CPXCENVptr env, CPXCLPptr lp, char *buf_str,
                    CPXSIZE bufspace, CPXSIZE * surplus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetobjoffset (CPXCENVptr env, CPXCLPptr lp, double *objoffset_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetobjsen (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetparamname (CPXCENVptr env, int whichparam, char *name_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetparamnum (CPXCENVptr env, char const *name_str, int *whichparam_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetparamtype (CPXCENVptr env, int whichparam, int *paramtype);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLgetphase1cnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetpi (CPXCENVptr env, CPXCLPptr lp, double *pi, CPXINT begin,
               CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetpnorms (CPXCENVptr env, CPXCLPptr lp, double *cnorm,
                   double *rnorm, CPXINT * len_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetprestat (CPXCENVptr env, CPXCLPptr lp, int *prestat_p,
                    CPXINT * pcstat, CPXINT * prstat, CPXINT * ocstat,
                    CPXINT * orstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetprobname (CPXCENVptr env, CPXCLPptr lp, char *buf_str,
                     CPXSIZE bufspace, CPXSIZE * surplus_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetprobtype (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetprotected (CPXCENVptr env, CPXCLPptr lp, CPXINT * cnt_p,
                      CPXINT * indices, CPXINT pspace, CPXINT * surplus_p);
    CPXLIBAPI CPXINT CPXPUBLIC CPXLgetpsbcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXLgetray (CPXCENVptr env, CPXCLPptr lp, double *z);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetredlp (CPXCENVptr env, CPXCLPptr lp, CPXCLPptr * redlp_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetrhs (CPXCENVptr env, CPXCLPptr lp, double *rhs, CPXINT begin,
                CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetrngval (CPXCENVptr env, CPXCLPptr lp, double *rngval,
                   CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetrowindex (CPXCENVptr env, CPXCLPptr lp,
                     char const *lname_str, CPXINT * index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetrowinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x,
                      double *infeasout, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetrowname (CPXCENVptr env, CPXCLPptr lp, char **name,
                    char *namestore, CPXSIZE storespace,
                    CPXSIZE * surplus_p, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetrows (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p,
                 CPXLONG * rmatbeg, CPXINT * rmatind, double *rmatval,
                 CPXLONG rmatspace, CPXLONG * surplus_p, CPXINT begin,
                 CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsense (CPXCENVptr env, CPXCLPptr lp, char *sense,
                  CPXINT begin, CPXINT end);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLgetsiftitcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLgetsiftphase1cnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetslack (CPXCENVptr env, CPXCLPptr lp, double *slack,
                  CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpooldblquality (CPXCENVptr env, CPXCLPptr lp, int soln,
                               double *quality_p, int what);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolintquality (CPXCENVptr env, CPXCLPptr lp, int soln,
                               CPXINT * quality_p, int what);
    CPXLIBAPI int CPXPUBLIC CPXLgetstat (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXCHARptr CPXPUBLIC
    CPXLgetstatstring (CPXCENVptr env, int statind, char *buffer_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetstrparam (CPXCENVptr env, int whichparam, char *value_str);
    CPXLIBAPI int CPXPUBLIC CPXLgettime (CPXCENVptr env, double *timestamp_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgettuningcallbackfunc (CPXCENVptr env,
                               CPXL_CALLBACK ** callback_p,
                               void **cbhandle_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetub (CPXCENVptr env, CPXCLPptr lp, double *ub, CPXINT begin,
               CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetweight (CPXCENVptr env, CPXCLPptr lp, CPXINT rcnt,
                   CPXLONG const *rmatbeg, CPXINT const *rmatind,
                   double const *rmatval, double *weight, int dpriind);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetx (CPXCENVptr env, CPXCLPptr lp, double *x, CPXINT begin,
              CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC CPXLhybnetopt (CPXCENVptr env, CPXLPptr lp, int method);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLinfodblparam (CPXCENVptr env, int whichparam,
                      double *defvalue_p, double *minvalue_p,
                      double *maxvalue_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLinfointparam (CPXCENVptr env, int whichparam,
                      CPXINT * defvalue_p, CPXINT * minvalue_p,
                      CPXINT * maxvalue_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLinfolongparam (CPXCENVptr env, int whichparam,
                       CPXLONG * defvalue_p, CPXLONG * minvalue_p,
                       CPXLONG * maxvalue_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLinfostrparam (CPXCENVptr env, int whichparam, char *defvalue_str);
    CPXLIBAPI void CPXPUBLIC CPXLinitialize (void);
    CPXLIBAPI int CPXPUBLIC CPXLkilldnorms (CPXLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXLkillpnorms (CPXLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXLlpopt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLlprewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLlpwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI CPXVOIDptr CPXPUBLIC CPXLmalloc (size_t size);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLmbasewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLmdleave (CPXCENVptr env, CPXLPptr lp, CPXINT const *indices,
                 CPXINT cnt, double *downratio, double *upratio);
    CPXLIBAPI CPXVOIDptr CPXPUBLIC CPXLmemcpy (void *s1, void *s2, size_t n);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLmpsrewrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLmpswrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBVARARGS
    CPXLmsg (CPXCHANNELptr channel, char const *format, ...);
    CPXLIBAPI
    int CPXPUBLIC CPXLmsgstr (CPXCHANNELptr channel, char const *msg_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETextract (CPXCENVptr env, CPXNETptr net, CPXCLPptr lp,
                    CPXINT * colmap, CPXINT * rowmap);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLnewcols (CPXCENVptr env, CPXLPptr lp, CPXINT ccnt,
                 double const *obj, double const *lb, double const *ub,
                 char const *xctype, char const *const *colname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLnewrows (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt,
                 double const *rhs, char const *sense,
                 double const *rngval, char const *const *rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLobjsa (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end,
               double *lower, double *upper);
    CPXLIBAPI CPXENVptr CPXPUBLIC CPXLopenCPLEX (int *status_p);
    CPXLIBAPI
    CPXENVptr CPXPUBLIC
    CPXLopenCPLEXruntime (int *status_p, int serialnum,
                          char const *licenvstring_str);
    CPXLIBAPI CPXENVptr CPXPUBLIC CPXLparenv (CPXENVptr env, int *status_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLpivot (CPXCENVptr env, CPXLPptr lp, CPXINT jenter,
               CPXINT jleave, int leavestat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLpivotin (CPXCENVptr env, CPXLPptr lp, CPXINT const *rlist,
                 CPXINT rlen);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLpivotout (CPXCENVptr env, CPXLPptr lp, CPXINT const *clist,
                  CPXINT clen);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLpperwrite (CPXCENVptr env, CPXLPptr lp,
                   char const *filename_str, double epsilon);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLpratio (CPXCENVptr env, CPXLPptr lp, CPXINT * indices,
                CPXINT cnt, double *downratio, double *upratio,
                CPXINT * downleave, CPXINT * upleave,
                int *downleavestatus, int *upleavestatus,
                int *downstatus, int *upstatus);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLpreaddrows (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt,
                    CPXLONG nzcnt, double const *rhs, char const *sense,
                    CPXLONG const *rmatbeg, CPXINT const *rmatind,
                    double const *rmatval, char const *const *rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLprechgobj (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                   CPXINT const *indices, double const *values);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLpreslvwrite (CPXCENVptr env, CPXLPptr lp,
                     char const *filename_str, double *objoff_p);
    CPXLIBAPI
    int CPXPUBLIC CPXLpresolve (CPXCENVptr env, CPXLPptr lp, int method);
    CPXLIBAPI int CPXPUBLIC CPXLprimopt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXLputenv (char *envsetting_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLqpdjfrompi (CPXCENVptr env, CPXCLPptr lp, double const *pi,
                    double const *x, double *dj);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLqpuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi,
                     double const *prepi, double const *x);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLreadcopybase (CPXCENVptr env, CPXLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC CPXLreadcopyparam (CPXENVptr env, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLreadcopyprob (CPXCENVptr env, CPXLPptr lp,
                      char const *filename_str, char const *filetype_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLreadcopysol (CPXCENVptr env, CPXLPptr lp, char const *filename_str);
    CPXLIBAPI CPXVOIDptr CPXPUBLIC CPXLrealloc (void *ptr, size_t size);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLrefineconflict (CPXCENVptr env, CPXLPptr lp,
                        CPXINT * confnumrows_p, CPXINT * confnumcols_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLrefineconflictext (CPXCENVptr env, CPXLPptr lp, CPXLONG grpcnt,
                           CPXLONG concnt, double const *grppref,
                           CPXLONG const *grpbeg, CPXINT const *grpind,
                           char const *grptype);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLrhssa (CPXCENVptr env, CPXCLPptr lp, CPXINT begin, CPXINT end,
               double *lower, double *upper);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLrobustopt (CPXCENVptr env, CPXLPptr lp, CPXLPptr lblp,
                   CPXLPptr ublp, double objchg, double const *maxchg);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsavwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI int CPXPUBLIC CPXLserializercreate (CPXSERIALIZERptr * ser_p);
    CPXLIBAPI void CPXPUBLIC CPXLserializerdestroy (CPXSERIALIZERptr ser);
    CPXLIBAPI CPXLONG CPXPUBLIC CPXLserializerlength (CPXCSERIALIZERptr ser);
    CPXLIBAPI
    void const *CPXPUBLIC CPXLserializerpayload (CPXCSERIALIZERptr ser);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsetdblparam (CPXENVptr env, int whichparam, double newvalue);
    CPXLIBAPI int CPXPUBLIC CPXLsetdefaults (CPXENVptr env);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsetintparam (CPXENVptr env, int whichparam, CPXINT newvalue);
    CPXLIBAPI int CPXPUBLIC CPXLsetlogfile (CPXENVptr env, CPXFILEptr lfile);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsetlongparam (CPXENVptr env, int whichparam, CPXLONG newvalue);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsetlpcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback,
                           void *cbhandle);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsetnetcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback,
                            void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsetphase2 (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsetprofcallbackfunc (CPXENVptr env,
                             CPXL_CALLBACK_PROF * callback, void *cbhandle);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsetstrparam (CPXENVptr env, int whichparam, char const *newvalue_str);
    CPXLIBAPI
    int CPXPUBLIC CPXLsetterminate (CPXENVptr env, volatile int *terminate_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsettuningcallbackfunc (CPXENVptr env, CPXL_CALLBACK * callback,
                               void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsiftopt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLslackfromx (CPXCENVptr env, CPXCLPptr lp, double const *x,
                    double *slack);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsolninfo (CPXCENVptr env, CPXCLPptr lp, int *solnmethod_p,
                  int *solntype_p, int *pfeasind_p, int *dfeasind_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsolution (CPXCENVptr env, CPXCLPptr lp, int *lpstat_p,
                  double *objval_p, double *x, double *pi,
                  double *slack, double *dj);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsolwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsolwritesolnpool (CPXCENVptr env, CPXCLPptr lp, int soln,
                          char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsolwritesolnpoolall (CPXCENVptr env, CPXCLPptr lp,
                             char const *filename_str);
    CPXLIBAPI
    CPXCHARptr CPXPUBLIC CPXLstrcpy (char *dest_str, char const *src_str);
    CPXLIBAPI size_t CPXPUBLIC CPXLstrlen (char const *s_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLstrongbranch (CPXCENVptr env, CPXLPptr lp,
                      CPXINT const *indices, CPXINT cnt,
                      double *downobj, double *upobj, CPXLONG itlim);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLtightenbds (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                    CPXINT const *indices, char const *lu, double const *bd);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLtuneparam (CPXENVptr env, CPXLPptr lp, int intcnt,
                   int const *intnum, int const *intval, int dblcnt,
                   int const *dblnum, double const *dblval, int strcnt,
                   int const *strnum, char const *const *strval,
                   int *tunestat_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLtuneparamprobset (CPXENVptr env, int filecnt,
                          char const *const *filename,
                          char const *const *filetype, int intcnt,
                          int const *intnum, int const *intval,
                          int dblcnt, int const *dblnum,
                          double const *dblval, int strcnt,
                          int const *strnum, char const *const *strval,
                          int *tunestat_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLuncrushform (CPXCENVptr env, CPXCLPptr lp, CPXINT plen,
                     CPXINT const *pind, double const *pval,
                     CPXINT * len_p, double *offset_p, CPXINT * ind,
                     double *val);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLuncrushpi (CPXCENVptr env, CPXCLPptr lp, double *pi,
                   double const *prepi);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLuncrushx (CPXCENVptr env, CPXCLPptr lp, double *x,
                  double const *prex);
    CPXLIBAPI int CPXPUBLIC CPXLunscaleprob (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI CPXCCHARptr CPXPUBLIC CPXLversion (CPXCENVptr env);
    CPXLIBAPI
    int CPXPUBLIC CPXLversionnumber (CPXCENVptr env, int *version_p);
    CPXLIBAPI
    int CPXPUBLIC CPXLwriteparam (CPXCENVptr env, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLwriteprob (CPXCENVptr env, CPXCLPptr lp,
                   char const *filename_str, char const *filetype_str);
    CPXLIBAPI int CPXPUBLIC CPXLbaropt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXLhybbaropt (CPXCENVptr env, CPXLPptr lp, int method);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddindconstr (CPXCENVptr env, CPXLPptr lp, CPXINT indvar,
                      int complemented, CPXINT nzcnt, double rhs,
                      int sense, CPXINT const *linind,
                      double const *linval, char const *indname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgqpcoef (CPXCENVptr env, CPXLPptr lp, CPXINT i, CPXINT j,
                   double newvalue);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopyqpsep (CPXCENVptr env, CPXLPptr lp, double const *qsepvec);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopyquad (CPXCENVptr env, CPXLPptr lp, CPXLONG const *qmatbeg,
                  CPXINT const *qmatcnt, CPXINT const *qmatind,
                  double const *qmatval);
    CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnumqpnz (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumquad (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetqpcoef (CPXCENVptr env, CPXCLPptr lp, CPXINT rownum,
                   CPXINT colnum, double *coef_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetquad (CPXCENVptr env, CPXCLPptr lp, CPXLONG * nzcnt_p,
                 CPXLONG * qmatbeg, CPXINT * qmatind, double *qmatval,
                 CPXLONG qmatspace, CPXLONG * surplus_p, CPXINT begin,
                 CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLqpindefcertificate (CPXCENVptr env, CPXCLPptr lp, double *x);
    CPXLIBAPI int CPXPUBLIC CPXLqpopt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddqconstr (CPXCENVptr env, CPXLPptr lp, CPXINT linnzcnt,
                    CPXLONG quadnzcnt, double rhs, int sense,
                    CPXINT const *linind, double const *linval,
                    CPXINT const *quadrow, CPXINT const *quadcol,
                    double const *quadval, char const *lname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelqconstrs (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end);
    CPXLIBAPI
    CPXINT CPXPUBLIC CPXLgetnumqconstrs (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetqconstr (CPXCENVptr env, CPXCLPptr lp, CPXINT * linnzcnt_p,
                    CPXLONG * quadnzcnt_p, double *rhs_p, char *sense_p,
                    CPXINT * linind, double *linval, CPXINT linspace,
                    CPXINT * linsurplus_p, CPXINT * quadrow,
                    CPXINT * quadcol, double *quadval, CPXLONG quadspace,
                    CPXLONG * quadsurplus_p, CPXINT which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetqconstrdslack (CPXCENVptr env, CPXCLPptr lp, CPXINT qind,
                          CPXINT * nz_p, CPXINT * ind, double *val,
                          CPXINT space, CPXINT * surplus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetqconstrindex (CPXCENVptr env, CPXCLPptr lp,
                         char const *lname_str, CPXINT * index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetqconstrinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x,
                          double *infeasout, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetqconstrname (CPXCENVptr env, CPXCLPptr lp, char *buf_str,
                        CPXSIZE bufspace, CPXSIZE * surplus_p, CPXINT which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetqconstrslack (CPXCENVptr env, CPXCLPptr lp, double *qcslack,
                         CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetxqxax (CPXCENVptr env, CPXCLPptr lp, double *xqxax,
                  CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLqconstrslackfromx (CPXCENVptr env, CPXCLPptr lp,
                           double const *x, double *qcslack);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETaddarcs (CPXCENVptr env, CPXNETptr net, CPXINT narcs,
                    CPXINT const *fromnode, CPXINT const *tonode,
                    double const *low, double const *up,
                    double const *obj, char const *const *anames);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETaddnodes (CPXCENVptr env, CPXNETptr net, CPXINT nnodes,
                     double const *supply, char const *const *name);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETbasewrite (CPXCENVptr env, CPXCNETptr net,
                      char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETchgarcname (CPXCENVptr env, CPXNETptr net, CPXINT cnt,
                       CPXINT const *indices, char const *const *newname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETchgarcnodes (CPXCENVptr env, CPXNETptr net, CPXINT cnt,
                        CPXINT const *indices, CPXINT const *fromnode,
                        CPXINT const *tonode);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETchgbds (CPXCENVptr env, CPXNETptr net, CPXINT cnt,
                   CPXINT const *indices, char const *lu, double const *bd);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETchgname (CPXCENVptr env, CPXNETptr net, int key,
                    CPXINT vindex, char const *name_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETchgnodename (CPXCENVptr env, CPXNETptr net, CPXINT cnt,
                        CPXINT const *indices, char const *const *newname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETchgobj (CPXCENVptr env, CPXNETptr net, CPXINT cnt,
                   CPXINT const *indices, double const *obj);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETchgobjsen (CPXCENVptr env, CPXNETptr net, int maxormin);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETchgsupply (CPXCENVptr env, CPXNETptr net, CPXINT cnt,
                      CPXINT const *indices, double const *supply);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETcopybase (CPXCENVptr env, CPXNETptr net, int const *astat,
                     int const *nstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETcopynet (CPXCENVptr env, CPXNETptr net, int objsen,
                    CPXINT nnodes, double const *supply,
                    char const *const *nnames, CPXINT narcs,
                    CPXINT const *fromnode, CPXINT const *tonode,
                    double const *low, double const *up,
                    double const *obj, char const *const *anames);
    CPXLIBAPI
    CPXNETptr CPXPUBLIC
    CPXLNETcreateprob (CPXENVptr env, int *status_p, char const *name_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETdelarcs (CPXCENVptr env, CPXNETptr net, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETdelnodes (CPXCENVptr env, CPXNETptr net, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETdelset (CPXCENVptr env, CPXNETptr net, CPXINT * whichnodes,
                   CPXINT * whicharcs);
    CPXLIBAPI
    int CPXPUBLIC CPXLNETfreeprob (CPXENVptr env, CPXNETptr * net_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetarcindex (CPXCENVptr env, CPXCNETptr net,
                        char const *lname_str, CPXINT * index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetarcname (CPXCENVptr env, CPXCNETptr net, char **nnames,
                       char *namestore, CPXSIZE namespc,
                       CPXSIZE * surplus_p, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetarcnodes (CPXCENVptr env, CPXCNETptr net,
                        CPXINT * fromnode, CPXINT * tonode, CPXINT begin,
                        CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetbase (CPXCENVptr env, CPXCNETptr net, int *astat, int *nstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetdj (CPXCENVptr env, CPXCNETptr net, double *dj,
                  CPXINT begin, CPXINT end);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLNETgetitcnt (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetlb (CPXCENVptr env, CPXCNETptr net, double *low,
                  CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetnodearcs (CPXCENVptr env, CPXCNETptr net,
                        CPXINT * arccnt_p, CPXINT * arcbeg, CPXINT * arc,
                        CPXINT arcspace, CPXINT * surplus_p,
                        CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetnodeindex (CPXCENVptr env, CPXCNETptr net,
                         char const *lname_str, CPXINT * index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetnodename (CPXCENVptr env, CPXCNETptr net, char **nnames,
                        char *namestore, CPXSIZE namespc,
                        CPXSIZE * surplus_p, CPXINT begin, CPXINT end);
    CPXLIBAPI
    CPXINT CPXPUBLIC CPXLNETgetnumarcs (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    CPXINT CPXPUBLIC CPXLNETgetnumnodes (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetobj (CPXCENVptr env, CPXCNETptr net, double *obj,
                   CPXINT begin, CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLNETgetobjsen (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetobjval (CPXCENVptr env, CPXCNETptr net, double *objval_p);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLNETgetphase1cnt (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetpi (CPXCENVptr env, CPXCNETptr net, double *pi,
                  CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetprobname (CPXCENVptr env, CPXCNETptr net, char *buf_str,
                        CPXSIZE bufspace, CPXSIZE * surplus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetslack (CPXCENVptr env, CPXCNETptr net, double *slack,
                     CPXINT begin, CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLNETgetstat (CPXCENVptr env, CPXCNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetsupply (CPXCENVptr env, CPXCNETptr net, double *supply,
                      CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetub (CPXCENVptr env, CPXCNETptr net, double *up,
                  CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETgetx (CPXCENVptr env, CPXCNETptr net, double *x,
                 CPXINT begin, CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLNETprimopt (CPXCENVptr env, CPXNETptr net);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETreadcopybase (CPXCENVptr env, CPXNETptr net,
                         char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETreadcopyprob (CPXCENVptr env, CPXNETptr net,
                         char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETsolninfo (CPXCENVptr env, CPXCNETptr net, int *pfeasind_p,
                     int *dfeasind_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETsolution (CPXCENVptr env, CPXCNETptr net, int *netstat_p,
                     double *objval_p, double *x, double *pi,
                     double *slack, double *dj);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLNETwriteprob (CPXCENVptr env, CPXCNETptr net,
                      char const *filename_str, char const *format_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddlazyconstraints (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt,
                            CPXLONG nzcnt, double const *rhs,
                            char const *sense, CPXLONG const *rmatbeg,
                            CPXINT const *rmatind,
                            double const *rmatval,
                            char const *const *rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt,
                      CPXLONG nzcnt, CPXLONG const *beg,
                      CPXINT const *varindices, double const *values,
                      int const *effortlevel,
                      char const *const *mipstartname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddsolnpooldivfilter (CPXCENVptr env, CPXLPptr lp,
                              double lower_bound, double upper_bound,
                              CPXINT nzcnt, CPXINT const *ind,
                              double const *weight,
                              double const *refval, char const *lname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddsolnpoolrngfilter (CPXCENVptr env, CPXLPptr lp, double lb,
                              double ub, CPXINT nzcnt,
                              CPXINT const *ind, double const *val,
                              char const *lname_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddsos (CPXCENVptr env, CPXLPptr lp, CPXINT numsos,
                CPXLONG numsosnz, char const *sostype,
                CPXLONG const *sosbeg, CPXINT const *sosind,
                double const *soswt, char const *const *sosname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLaddusercuts (CPXCENVptr env, CPXLPptr lp, CPXINT rcnt,
                     CPXLONG nzcnt, double const *rhs,
                     char const *sense, CPXLONG const *rmatbeg,
                     CPXINT const *rmatind, double const *rmatval,
                     char const *const *rowname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLbranchcallbackbranchasCPLEX (CPXCENVptr env, void *cbdata,
                                     int wherefrom, int num,
                                     void *userhandle, CPXLONG * seqnum_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLbranchcallbackbranchbds (CPXCENVptr env, void *cbdata,
                                 int wherefrom, CPXINT cnt,
                                 CPXINT const *indices, char const *lu,
                                 double const *bd, double nodeest,
                                 void *userhandle, CPXLONG * seqnum_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLbranchcallbackbranchconstraints (CPXCENVptr env, void *cbdata,
                                         int wherefrom, CPXINT rcnt,
                                         CPXLONG nzcnt,
                                         double const *rhs,
                                         char const *sense,
                                         CPXLONG const *rmatbeg,
                                         CPXINT const *rmatind,
                                         double const *rmatval,
                                         double nodeest,
                                         void *userhandle,
                                         CPXLONG * seqnum_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLbranchcallbackbranchgeneral (CPXCENVptr env, void *cbdata,
                                     int wherefrom, CPXINT varcnt,
                                     CPXINT const *varind,
                                     char const *varlu,
                                     double const *varbd, CPXINT rcnt,
                                     CPXLONG nzcnt, double const *rhs,
                                     char const *sense,
                                     CPXLONG const *rmatbeg,
                                     CPXINT const *rmatind,
                                     double const *rmatval,
                                     double nodeest, void *userhandle,
                                     CPXLONG * seqnum_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcallbacksetnodeuserhandle (CPXCENVptr env, void *cbdata,
                                   int wherefrom, CPXLONG nodeindex,
                                   void *userhandle, void **olduserhandle_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcallbacksetuserhandle (CPXCENVptr env, void *cbdata,
                               int wherefrom, void *userhandle,
                               void **olduserhandle_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgctype (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                  CPXINT const *indices, char const *xctype);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLchgmipstarts (CPXCENVptr env, CPXLPptr lp, int mcnt,
                      int const *mipstartindices, CPXLONG nzcnt,
                      CPXLONG const *beg, CPXINT const *varindices,
                      double const *values, int const *effortlevel);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopyctype (CPXCENVptr env, CPXLPptr lp, char const *xctype);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopyorder (CPXCENVptr env, CPXLPptr lp, CPXINT cnt,
                   CPXINT const *indices, CPXINT const *priority,
                   int const *direction);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcopysos (CPXCENVptr env, CPXLPptr lp, CPXINT numsos,
                 CPXLONG numsosnz, char const *sostype,
                 CPXLONG const *sosbeg, CPXINT const *sosind,
                 double const *soswt, char const *const *sosname);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcutcallbackadd (CPXCENVptr env, void *cbdata, int wherefrom,
                        CPXINT nzcnt, double rhs, int sense,
                        CPXINT const *cutind, double const *cutval,
                        int purgeable);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLcutcallbackaddlocal (CPXCENVptr env, void *cbdata,
                             int wherefrom, CPXINT nzcnt, double rhs,
                             int sense, CPXINT const *cutind,
                             double const *cutval);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelindconstrs (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelmipstarts (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelsetmipstarts (CPXCENVptr env, CPXLPptr lp, int *delstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelsetsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int *delstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelsetsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int *delstat);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelsetsos (CPXCENVptr env, CPXLPptr lp, CPXINT * delset);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelsolnpoolfilters (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelsolnpoolsolns (CPXCENVptr env, CPXLPptr lp, int begin, int end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLdelsos (CPXCENVptr env, CPXLPptr lp, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLfltwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC CPXLfreelazyconstraints (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXLfreeusercuts (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetbestobjval (CPXCENVptr env, CPXCLPptr lp, double *objval_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetbranchcallbackfunc (CPXCENVptr env,
                               CPXL_CALLBACK_BRANCH ** branchcallback_p,
                               void **cbhandle_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetbranchnosolncallbackfunc (CPXCENVptr env,
                                     CPXL_CALLBACK_BRANCH **
                                     branchnosolncallback_p,
                                     void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbackbranchconstraints (CPXCENVptr env,
                                                              void *cbdata,
                                                              int wherefrom,
                                                              int which,
                                                              CPXINT * cuts_p,
                                                              CPXLONG *
                                                              nzcnt_p,
                                                              double *rhs,
                                                              char *sense,
                                                              CPXLONG *
                                                              rmatbeg,
                                                              CPXINT *
                                                              rmatind,
                                                              double *rmatval,
                                                              CPXLONG rmatsz,
                                                              CPXLONG *
                                                              surplus_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbackctype (CPXCENVptr env,
                                                  void *cbdata, int wherefrom,
                                                  char *xctype, CPXINT begin,
                                                  CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbackgloballb (CPXCENVptr env,
                                                     void *cbdata,
                                                     int wherefrom,
                                                     double *lb, CPXINT begin,
                                                     CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbackglobalub (CPXCENVptr env,
                                                     void *cbdata,
                                                     int wherefrom,
                                                     double *ub, CPXINT begin,
                                                     CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbackincumbent (CPXCENVptr env,
                                                      void *cbdata,
                                                      int wherefrom,
                                                      double *x, CPXINT begin,
                                                      CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbackindicatorinfo (CPXCENVptr env,
                                                          void *cbdata,
                                                          int wherefrom,
                                                          CPXINT iindex,
                                                          int whichinfo,
                                                          void *result_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbacklp (CPXCENVptr env, void *cbdata,
                                               int wherefrom,
                                               CPXCLPptr * lp_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeinfo (CPXCENVptr env,
                                                     void *cbdata,
                                                     int wherefrom,
                                                     CPXLONG nodeindex,
                                                     int whichinfo,
                                                     void *result_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeintfeas (CPXCENVptr env,
                                                        void *cbdata,
                                                        int wherefrom,
                                                        int *feas,
                                                        CPXINT begin,
                                                        CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodelb (CPXCENVptr env,
                                                   void *cbdata,
                                                   int wherefrom, double *lb,
                                                   CPXINT begin, CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodelp (CPXCENVptr env,
                                                   void *cbdata,
                                                   int wherefrom,
                                                   CPXLPptr * nodelp_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeobjval (CPXCENVptr env,
                                                       void *cbdata,
                                                       int wherefrom,
                                                       double *objval_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodestat (CPXCENVptr env,
                                                     void *cbdata,
                                                     int wherefrom,
                                                     int *nodestat_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodeub (CPXCENVptr env,
                                                   void *cbdata,
                                                   int wherefrom, double *ub,
                                                   CPXINT begin, CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbacknodex (CPXCENVptr env,
                                                  void *cbdata, int wherefrom,
                                                  double *x, CPXINT begin,
                                                  CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbackorder (CPXCENVptr env,
                                                  void *cbdata, int wherefrom,
                                                  CPXINT * priority,
                                                  int *direction,
                                                  CPXINT begin, CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbackpseudocosts (CPXCENVptr env,
                                                        void *cbdata,
                                                        int wherefrom,
                                                        double *uppc,
                                                        double *downpc,
                                                        CPXINT begin,
                                                        CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbackseqinfo (CPXCENVptr env,
                                                    void *cbdata,
                                                    int wherefrom,
                                                    CPXLONG seqid,
                                                    int whichinfo,
                                                    void *result_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetcallbacksosinfo (CPXCENVptr env,
                                                    void *cbdata,
                                                    int wherefrom,
                                                    CPXINT sosindex,
                                                    CPXINT member,
                                                    int whichinfo,
                                                    void *result_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetctype (CPXCENVptr env, CPXCLPptr lp,
                                          char *xctype, CPXINT begin,
                                          CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetcutoff (CPXCENVptr env, CPXCLPptr lp,
                                           double *cutoff_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetdeletenodecallbackfunc (CPXCENVptr env,
                                                           CPXL_CALLBACK_DELETENODE
                                                           **
                                                           deletecallback_p,
                                                           void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetheuristiccallbackfunc (CPXCENVptr env,
                                                          CPXL_CALLBACK_HEURISTIC
                                                          **
                                                          heuristiccallback_p,
                                                          void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetincumbentcallbackfunc (CPXCENVptr env,
                                                          CPXL_CALLBACK_INCUMBENT
                                                          **
                                                          incumbentcallback_p,
                                                          void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetindconstr (CPXCENVptr env, CPXCLPptr lp,
                                              CPXINT * indvar_p,
                                              int *complemented_p,
                                              CPXINT * nzcnt_p, double *rhs_p,
                                              char *sense_p, CPXINT * linind,
                                              double *linval, CPXINT space,
                                              CPXINT * surplus_p,
                                              CPXINT which);
    CPXLIBAPI int CPXPUBLIC CPXLgetindconstrindex (CPXCENVptr env,
                                                   CPXCLPptr lp,
                                                   char const *lname_str,
                                                   CPXINT * index_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetindconstrinfeas (CPXCENVptr env,
                                                    CPXCLPptr lp,
                                                    double const *x,
                                                    double *infeasout,
                                                    CPXINT begin, CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetindconstrname (CPXCENVptr env,
                                                  CPXCLPptr lp, char *buf_str,
                                                  CPXSIZE bufspace,
                                                  CPXSIZE * surplus_p,
                                                  CPXINT which);
    CPXLIBAPI int CPXPUBLIC CPXLgetindconstrslack (CPXCENVptr env,
                                                   CPXCLPptr lp,
                                                   double *indslack,
                                                   CPXINT begin, CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetinfocallbackfunc (CPXCENVptr env,
                                                     CPXL_CALLBACK **
                                                     callback_p,
                                                     void **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetlazyconstraintcallbackfunc (CPXCENVptr env,
                                                               CPXL_CALLBACK_CUT
                                                               **
                                                               cutcallback_p,
                                                               void
                                                               **cbhandle_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetmipcallbackfunc (CPXCENVptr env,
                                                    CPXL_CALLBACK **
                                                    callback_p,
                                                    void **cbhandle_p);
    CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetmipitcnt (CPXCENVptr env,
                                                 CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXLgetmiprelgap (CPXCENVptr env, CPXCLPptr lp,
                                              double *gap_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetmipstartindex (CPXCENVptr env,
                                                  CPXCLPptr lp,
                                                  char const *lname_str,
                                                  int *index_p);
    CPXLIBAPI int CPXPUBLIC CPXLgetmipstartname (CPXCENVptr env, CPXCLPptr lp,
                                                 char **name, char *store,
                                                 CPXSIZE storesz,
                                                 CPXSIZE * surplus_p,
                                                 int begin, int end);
    CPXLIBAPI int CPXPUBLIC CPXLgetmipstarts (CPXCENVptr env, CPXCLPptr lp,
                                              CPXLONG * nzcnt_p,
                                              CPXLONG * beg,
                                              CPXINT * varindices,
                                              double *values,
                                              int *effortlevel,
                                              CPXLONG startspace,
                                              CPXLONG * surplus_p, int begin,
                                              int end);
    CPXLIBAPI int CPXPUBLIC CPXLgetnodecallbackfunc (CPXCENVptr env,
                                                     CPXL_CALLBACK_NODE **
                                                     nodecallback_p,
                                                     void **cbhandle_p);
    CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnodecnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI CPXLONG CPXPUBLIC CPXLgetnodeint (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXLONG CPXPUBLIC CPXLgetnodeleftcnt (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumbin (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetnumcuts (CPXCENVptr env, CPXCLPptr lp, int cuttype,
                    CPXINT * num_p);
    CPXLIBAPI
    CPXINT CPXPUBLIC CPXLgetnumindconstrs (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumint (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXINT CPXPUBLIC CPXLgetnumlazyconstraints (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXLgetnummipstarts (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXINT CPXPUBLIC CPXLgetnumsemicont (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXINT CPXPUBLIC CPXLgetnumsemiint (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI CPXINT CPXPUBLIC CPXLgetnumsos (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    CPXINT CPXPUBLIC CPXLgetnumusercuts (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetorder (CPXCENVptr env, CPXCLPptr lp, CPXINT * cnt_p,
                  CPXINT * indices, CPXINT * priority, int *direction,
                  CPXINT ordspace, CPXINT * surplus_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpooldivfilter (CPXCENVptr env, CPXCLPptr lp,
                              double *lower_cutoff_p,
                              double *upper_cutoff_p, CPXINT * nzcnt_p,
                              CPXINT * ind, double *val, double *refval,
                              CPXINT space, CPXINT * surplus_p, int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolfilterindex (CPXCENVptr env, CPXCLPptr lp,
                                char const *lname_str, int *index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolfiltername (CPXCENVptr env, CPXCLPptr lp,
                               char *buf_str, CPXSIZE bufspace,
                               CPXSIZE * surplus_p, int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolfiltertype (CPXCENVptr env, CPXCLPptr lp,
                               int *ftype_p, int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolmeanobjval (CPXCENVptr env, CPXCLPptr lp,
                               double *meanobjval_p);
    CPXLIBAPI
    int CPXPUBLIC CPXLgetsolnpoolnumfilters (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXLgetsolnpoolnumreplaced (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC CPXLgetsolnpoolnumsolns (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolobjval (CPXCENVptr env, CPXCLPptr lp, int soln,
                           double *objval_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolqconstrslack (CPXCENVptr env, CPXCLPptr lp, int soln,
                                 double *qcslack, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolrngfilter (CPXCENVptr env, CPXCLPptr lp,
                              double *lb_p, double *ub_p,
                              CPXINT * nzcnt_p, CPXINT * ind, double *val,
                              CPXINT space, CPXINT * surplus_p, int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolslack (CPXCENVptr env, CPXCLPptr lp, int soln,
                          double *slack, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolsolnindex (CPXCENVptr env, CPXCLPptr lp,
                              char const *lname_str, int *index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolsolnname (CPXCENVptr env, CPXCLPptr lp, char *store,
                             CPXSIZE storesz, CPXSIZE * surplus_p, int which);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolnpoolx (CPXCENVptr env, CPXCLPptr lp, int soln, double *x,
                      CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsolvecallbackfunc (CPXCENVptr env,
                              CPXL_CALLBACK_SOLVE ** solvecallback_p,
                              void **cbhandle_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsos (CPXCENVptr env, CPXCLPptr lp, CPXLONG * numsosnz_p,
                char *sostype, CPXLONG * sosbeg, CPXINT * sosind,
                double *soswt, CPXLONG sosspace, CPXLONG * surplus_p,
                CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsosindex (CPXCENVptr env, CPXCLPptr lp,
                     char const *lname_str, CPXINT * index_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsosinfeas (CPXCENVptr env, CPXCLPptr lp, double const *x,
                      double *infeasout, CPXINT begin, CPXINT end);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetsosname (CPXCENVptr env, CPXCLPptr lp, char **name,
                    char *namestore, CPXSIZE storespace,
                    CPXSIZE * surplus_p, CPXINT begin, CPXINT end);
    CPXLIBAPI int CPXPUBLIC CPXLgetsubmethod (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI int CPXPUBLIC CPXLgetsubstat (CPXCENVptr env, CPXCLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLgetusercutcallbackfunc (CPXCENVptr env,
                                CPXL_CALLBACK_CUT ** cutcallback_p,
                                void **cbhandle_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLindconstrslackfromx (CPXCENVptr env, CPXCLPptr lp,
                             double const *x, double *indslack);
    CPXLIBAPI int CPXPUBLIC CPXLmipopt (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLordread (CPXCENVptr env, char const *filename_str,
                 CPXINT numcols, char const *const *colname,
                 CPXINT * cnt_p, CPXINT * indices, CPXINT * priority,
                 int *direction);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLordwrite (CPXCENVptr env, CPXCLPptr lp, char const *filename_str);
    CPXLIBAPI int CPXPUBLIC CPXLpopulate (CPXCENVptr env, CPXLPptr lp);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLreadcopymipstarts (CPXCENVptr env, CPXLPptr lp,
                           char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLreadcopyorder (CPXCENVptr env, CPXLPptr lp, char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLreadcopysolnpoolfilters (CPXCENVptr env, CPXLPptr lp,
                                 char const *filename_str);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLrefinemipstartconflict (CPXCENVptr env, CPXLPptr lp,
                                int mipstartindex,
                                CPXINT * confnumrows_p,
                                CPXINT * confnumcols_p);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLrefinemipstartconflictext (CPXCENVptr env, CPXLPptr lp,
                                   int mipstartindex, CPXLONG grpcnt,
                                   CPXLONG concnt,
                                   double const *grppref,
                                   CPXLONG const *grpbeg,
                                   CPXINT const *grpind, char const *grptype);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsetbranchcallbackfunc (CPXENVptr env,
                               CPXL_CALLBACK_BRANCH * branchcallback,
                               void *cbhandle);
    CPXLIBAPI
    int CPXPUBLIC
    CPXLsetbranchnosolncallbackfunc (CPXENVptr env,
                                     CPXL_CALLBACK_BRANCH *
                                     branchnosolncallback, void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsetdeletenodecallbackfunc (CPXENVptr env,
                                                           CPXL_CALLBACK_DELETENODE
                                                           * deletecallback,
                                                           void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsetheuristiccallbackfunc (CPXENVptr env,
                                                          CPXL_CALLBACK_HEURISTIC
                                                          * heuristiccallback,
                                                          void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsetincumbentcallbackfunc (CPXENVptr env,
                                                          CPXL_CALLBACK_INCUMBENT
                                                          * incumbentcallback,
                                                          void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsetinfocallbackfunc (CPXENVptr env,
                                                     CPXL_CALLBACK * callback,
                                                     void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsetlazyconstraintcallbackfunc (CPXENVptr env,
                                                               CPXL_CALLBACK_CUT
                                                               *
                                                               lazyconcallback,
                                                               void
                                                               *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsetmipcallbackfunc (CPXENVptr env,
                                                    CPXL_CALLBACK * callback,
                                                    void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsetnodecallbackfunc (CPXENVptr env,
                                                     CPXL_CALLBACK_NODE *
                                                     nodecallback,
                                                     void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsetsolvecallbackfunc (CPXENVptr env,
                                                      CPXL_CALLBACK_SOLVE *
                                                      solvecallback,
                                                      void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLsetusercutcallbackfunc (CPXENVptr env,
                                                        CPXL_CALLBACK_CUT *
                                                        cutcallback,
                                                        void *cbhandle);
    CPXLIBAPI int CPXPUBLIC CPXLwritemipstarts (CPXCENVptr env, CPXCLPptr lp,
                                                char const *filename_str,
                                                int begin, int end);
#endif
#ifdef __cplusplus
}
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#endif                          /* _CPLEXL_H */
