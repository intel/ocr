#ifndef CSCC_H
#define CSCC_H

#define __OCR__
#include "compat.h"

#define FLAGS DB_PROP_NONE
#define PROPERTIES EDT_PROP_NONE

//#define maxatom 384
#define maxatom 38
#define maxnbfn 15 * maxatom
#define mxiter 30
#define maxnnbfn maxnbfn * (maxnbfn + 1) / 2
#define pi 3.141592653589793
#define two_pi_pow_2_5 34.98683665524972496996269910596311092376708984375
#define tol 0.006
#define tol2e 0.000001

#define MAX(a,b) (((a) >= (b)) ? (a) : (b))
#define MIN(a,b) (((a) <= (b)) ? (a) : (b))

extern double * g_dens, * g_fock, * g_tfock, * g_schwarz, * g_work, * g_ident, * g_orbs, *g_schwarz_max_j, * g_fock_const;
extern double * g_partial_fock;
//extern double enrep, q[maxatom], ax[maxatom], ay[maxatom], az[maxatom], eigv[maxnbfn];
extern double *q, *ax, *ay, *az, *eigv;
//extern double x[maxnbfn], y[maxnbfn], z[maxnbfn], expnt[maxnbfn], rnorm[maxnbfn];
extern double *x, *y, *z, *expnt, *rnorm;
//extern long long int iky[maxnbfn], nocc, nbfn, nnbfn;
extern long long int *iky, nocc, nbfn, nnbfn;
extern long long int icut1,icut2,icut3,icut4;
extern int natom;
extern double* t_fock;
extern int iter;
extern ocrGuid_t *symm_finished;
extern ocrGuid_t *reduction_finished;
extern ocrGuid_t *complement_finished;
extern ocrGuid_t *redu_guid;
extern ocrGuid_t *comp_guid;
extern ocrGuid_t *symm_guid;
extern double schwmax;
extern ocrGuid_t my_twoel_params_db;

extern double eone, etwo, energy, enrep;
extern double tinit, tonel, ttwoel, tdiag, tdens, tprint, i_timer;

extern double tdgemm;

typedef struct
{
  double* fock;
  long long int icut1;
  long long int icut2;
  long long int icut3;
  long long int icut4;
} twoel_context_t ;

extern twoel_context_t* twoel_context;

//======================================================================================================//
//num of threads
int threads;


typedef struct {
	double *g_schwarz_max_j;
	double * g_schwarz;
	double * g_dens;
	double * g_work;
	double * g_orbs;
	double * g_fock;
	double * g_tfock;
	double * t_fock;
	double * eigv;
	double * expnt;
	double * q;
	double * ax;
	double * ay;
	double * az;
	double * x;
	double * y;
	double * z;
	double * fm;
	double * rnorm;
	long long int * iky;
	long long int * icut1;
	long long int * icut2;
	long long int * icut3;
	long long int * icut4;
	twoel_context_t * twoel_context;

	double tol2e_over_schwmax;
	double *energy;
	double *eone;
	double *etwo;
	double *enrep;
	long long int nbfn;
	long long int nocc;


	double *tinit;
	double *tonel;
	double *ttwoel;
	double *tdiag;
	double *tdens;
	double *tprint;
	double *tdgemm;
	double *i_timer;
	int threads;

	long id;
	ocrGuid_t *finished;
	int start;
	int step;

}twoel_param_t;
extern twoel_param_t * my_twoel_params;
//======================================================================================================//

#define OFFSET(a,b) (((a) * nbfn) + (b))

#endif /* CSCC_H */
