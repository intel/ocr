/***************************************************************************
=============================================================================
SCF-NWCHEM for OCR

Jaime Arteaga, jaime@udel.edu, August 2013

Based on the code found on https://www.xstackwiki.com/index.php/DynAX.

=============================================================================

Copyright (c) 2013, University of Delaware
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Intel Corporation
     nor the names of its contributors may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#include "scf.h"


double * g_dens, * g_fock, * g_tfock, * g_schwarz, * g_work, * g_ident, * g_orbs, * offsets;
double * g_schwarz_max_j, * g_fock_const;
double * g_partial_fock;
precalc * g_precalc;


double* g_cache = NULL;
size_t g_cache_index = 0;
size_t g_cache_size = 0;
size_t g_cached_values = 0;

//double enrep, q[maxatom], ax[maxatom], ay[maxatom], az[maxatom], eigv[maxnbfn];
double *q, *ax, *ay, *az, *eigv;
//double x[maxnbfn], y[maxnbfn], z[maxnbfn], expnt[maxnbfn], rnorm[maxnbfn];
double *x, *y, *z, *expnt, *rnorm;
//long long int iky[maxnbfn], nocc, nbfn, nnbfn;
long long int *iky, nocc, nbfn, nnbfn;
long long int icut1,icut2,icut3,icut4;
int natom;
int iter;
double schwmax;

double eone, etwo, energy, enrep;
double tinit, tonel, ttwoel, tdiag, tdens, tprint, i_timer;
ocrGuid_t *symm_finished;
ocrGuid_t *reduction_finished;
ocrGuid_t *complement_finished;
ocrGuid_t *redu_guid;
ocrGuid_t *comp_guid;
ocrGuid_t *symm_guid;
twoel_param_t * my_twoel_params;
ocrGuid_t my_twoel_params_db;

//double fm[2001][5];
double *fm;
double rdelta, delta, delo2;

double tdgemm;

ocrGuid_t g_dens_db;
ocrGuid_t g_schwarz_db;
ocrGuid_t g_fock_db;
ocrGuid_t g_tfock_db;
ocrGuid_t g_work_db;
ocrGuid_t g_ident_db;
ocrGuid_t g_orbs_db;
ocrGuid_t g_partial_fock_db;
ocrGuid_t g_fock_const_db;
ocrGuid_t g_precalc_db;
ocrGuid_t g_schwarz_max_j_db;
ocrGuid_t g_cache_db;

//======================================================================================================//
ocrGuid_t mainEdt(u32 paramc, u64 params[], void* paramv[], u32 depc, ocrEdtDep_t depv[])
{

	PRINTF("========================================= 	\n");
	PRINTF("SCF-NWCHEM on OCR 				\n");
	PRINTF("========================================= 	\n");

	//     CAUTION: int precision requirements;
	//     nints, maxint, etc. are proportional to the number of basis functions to the
	//     fourth power! 216^4 is greater than the largest number that can be represented
	//     as a 32-bit signed interger, so 64-bit arithmetic is needed to count integrals
	//     when calculating more than 14 Be atoms with 15 basis functions each. Since
	//     integrals are counted over all iterations, 18 iterations with 7 atoms can result
	//     in precision problems. Note that the wave function could be calculated correctly;
	//     for much larger basis sets without 64-bit ints because the required indexing is
	//     usually proportional to nbfn^2, which is good to 46,340 basis functions.

	//double schwmax;
	tdgemm 	= 0;
	icut1 	= 0;
	icut2 	= 0;
	icut3 	= 0;
	icut4 	= 0;
	tinit	= 0.0;
	tonel 	= 0.0;
	ttwoel 	= 0.0;
	tdiag 	= 0.0;
	tdens 	= 0.0;
	tprint 	= 0.0;
	i_timer = 0.0;
	eone 	= 0.0;
	etwo 	= 0.0;
	enrep 	= 0.0;
	energy 	= 0.0;


	// init the timer
	timer();

	//======================================================================================================//
	//TODO: Add argc,argv inputs.
	int atoms;
	/*if(argc==1)
	{
		printf("Please provide ./scf.x THREADS ATOMS\n");
		return 0;
	}
	else
	{
		threads = atoi(argv[1]);
		atoms = atoi(argv[2]);
	}*/
	//allocation of memory is limited for up to 48 threads and 23 atoms.
	threads = 2;
	atoms 	= 2;
	//======================================================================================================//

	//setup vectors to avoid using stack
	setvectors();

	// get input from file be.inpt;
	input(atoms);

	// create and allocate global arrays;
	setarrays();
	ininrm();

	mkpre();

	// create initial guess for density matrix by using single atom densities;
	denges();

	// make initial orthogonal orbital set for solution method using similarity transform;
	makeob();

	// make info for sparsity test;
	schwmax = makesz();

	oneel(schwmax);
	memcpy(g_fock_const, g_fock, nbfn * nbfn * sizeof(double));

	// get the actual time spent initializing
	tinit = timer();

	//perform twoel using EDTs
	twoel_ocr();

	return NULL_GUID;
}
//======================================================================================================//


//======================================================================================================//
void mkpre(void)
{
	int i, j, j_off=0;

	for (j = 0; j < nbfn; j++)
	{
		for (i = 0; i < nbfn; i++)
		{
			precalc* pre = &g_precalc[j_off + i];

			double dx = x[i] - x[j];
			double dy = y[i] - y[j];
			double dz = z[i] - z[j];

			double expntIJ = expnt[i] + expnt[j];

			double r2 = dx * dx + dy * dy + dz * dz;
			double fac  = expnt[i] * expnt[j] / expntIJ;

			pre->facr2 = exp(-fac * r2);
			pre->expnt = expntIJ;

			pre->x = (x[i] * expnt[i] + x[j] * expnt[j]) / expntIJ;
			pre->y = (y[i] * expnt[i] + y[j] * expnt[j]) / expntIJ;
			pre->z = (z[i] * expnt[i] + z[j] * expnt[j]) / expntIJ;

			pre->rnorm = rnorm[i] * rnorm[j];
		}
		j_off += nbfn;
	}
}
//======================================================================================================//


//======================================================================================================//
void setvectors(void) {

	ocrGuid_t affinity=NULL_GUID;

	PTR_T q_ptr;
	ocrGuid_t q_db;
	DBCREATE( &q_db, &q_ptr, maxatom * sizeof(double), FLAGS, affinity, NO_ALLOC);
	q = (double*)q_ptr;

	PTR_T ax_ptr;
	ocrGuid_t ax_db;
	DBCREATE( &ax_db, &ax_ptr, maxatom * sizeof(double), FLAGS, affinity, NO_ALLOC);
	ax = (double*)ax_ptr;

	PTR_T ay_ptr;
	ocrGuid_t ay_db;
	DBCREATE( &ay_db, &ay_ptr, maxatom * sizeof(double), FLAGS, affinity, NO_ALLOC);
	ay = (double*)ay_ptr;

	PTR_T az_ptr;
	ocrGuid_t az_db;
	DBCREATE( &az_db, &az_ptr, maxatom * sizeof(double), FLAGS, affinity, NO_ALLOC);
	az = (double*)az_ptr;

	PTR_T eigv_ptr;
	ocrGuid_t eigv_db;
	DBCREATE( &eigv_db, &eigv_ptr, maxnbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	eigv = (double*)eigv_ptr;

	PTR_T x_ptr;
	ocrGuid_t x_db;
	DBCREATE( &x_db, &x_ptr, maxnbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	x = (double*)x_ptr;

	PTR_T y_ptr;
	ocrGuid_t y_db;
	DBCREATE( &y_db, &y_ptr, maxnbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	y = (double*)y_ptr;

	PTR_T z_ptr;
	ocrGuid_t z_db;
	DBCREATE( &z_db, &z_ptr, maxnbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	z = (double*)z_ptr;

	PTR_T expnt_ptr;
	ocrGuid_t expnt_db;
	DBCREATE( &expnt_db, &expnt_ptr, maxnbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	expnt = (double*)expnt_ptr;

	PTR_T rnorm_ptr;
	ocrGuid_t rnorm_db;
	DBCREATE( &rnorm_db, &rnorm_ptr, maxnbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	rnorm = (double*)rnorm_ptr;

	PTR_T iky_ptr;
	ocrGuid_t iky_db;
	DBCREATE( &iky_db, &iky_ptr, maxnbfn * sizeof(long long int), FLAGS, affinity, NO_ALLOC);
	iky = (long long int*)iky_ptr;

	PTR_T fm_ptr;
	ocrGuid_t fm_db;
	DBCREATE( &fm_db, &fm_ptr, 2001 * 5 * sizeof(double), FLAGS, affinity, NO_ALLOC);
	fm = (double*)fm_ptr;

	//outuput events from symm edts
	//ocrGuid_t symm_finished[mxiter*threads];
	PTR_T symm_finished_ptr;
	ocrGuid_t symm_finished_db;
	DBCREATE( &symm_finished_db, &symm_finished_ptr, mxiter*threads * sizeof(ocrGuid_t), FLAGS, affinity, NO_ALLOC);
	symm_finished = (ocrGuid_t*)symm_finished_ptr;

	//outuput events from reduction edts
	//ocrGuid_t reduction_finished[mxiter];
	PTR_T reduction_finished_ptr;
	ocrGuid_t reduction_finished_db;
	DBCREATE( &reduction_finished_db, &reduction_finished_ptr, mxiter * sizeof(ocrGuid_t), FLAGS, affinity, NO_ALLOC);
	reduction_finished = (ocrGuid_t*)reduction_finished_ptr;

	//outuput events from complement edts
	//ocrGuid_t complement_finished[mxiter];
	PTR_T complement_finished_ptr;
	ocrGuid_t complement_finished_db;
	DBCREATE( &complement_finished_db, &complement_finished_ptr, mxiter * sizeof(ocrGuid_t), FLAGS, affinity, NO_ALLOC);
	complement_finished = (ocrGuid_t*)complement_finished_ptr;

	//ocrGuid_t *symm_guid;
	//symm_guid = (ocrGuid_t *) malloc(sizeof(ocrGuid_t)*mxiter*threads);
	PTR_T symm_guid_ptr;
	ocrGuid_t symm_guid_db;
	DBCREATE( &symm_guid_db, &symm_guid_ptr, mxiter * threads * sizeof(ocrGuid_t), FLAGS, affinity, NO_ALLOC);
	symm_guid = (ocrGuid_t*)symm_guid_ptr;

	//ocrGuid_t *redu_guid;
	//redu_guid = (ocrGuid_t *) malloc(sizeof(ocrGuid_t)*mxiter);
	PTR_T redu_guid_ptr;
	ocrGuid_t redu_guid_db;
	DBCREATE( &redu_guid_db, &redu_guid_ptr, mxiter * sizeof(ocrGuid_t), FLAGS, affinity, NO_ALLOC);
	redu_guid = (ocrGuid_t*)redu_guid_ptr;

	//ocrGuid_t *comp_guid;
	//comp_guid = (ocrGuid_t *) malloc(sizeof(ocrGuid_t)*mxiter);
	PTR_T comp_guid_ptr;
	ocrGuid_t comp_guid_db;
	DBCREATE( &comp_guid_db, &comp_guid_ptr, mxiter * sizeof(ocrGuid_t), FLAGS, affinity, NO_ALLOC);
	comp_guid = (ocrGuid_t*)comp_guid_ptr;

	PTR_T my_twoel_params_ptr;
 	//ocrGuid_t my_twoel_params_db;
	ocrGuid_t my_twoel_params_affinity=NULL_GUID;
  	DBCREATE( &my_twoel_params_db, &my_twoel_params_ptr, sizeof(twoel_param_t), FLAGS, my_twoel_params_affinity, NO_ALLOC);
	my_twoel_params = (twoel_param_t*)my_twoel_params_ptr;

}

//======================================================================================================//



//======================================================================================================//
void setarrays(void) {

	int i, j, j_off=0;

	ocrGuid_t affinity=NULL_GUID;

	//g_dens    = (double *) malloc(nbfn * nbfn * sizeof(double));
	PTR_T g_dens_ptr;
	DBCREATE( &g_dens_db, &g_dens_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	if(g_dens_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_dens, using malloc instead !!!!!!\n\n");
		g_dens = (double*)malloc(nbfn * nbfn * sizeof(double));
	}
	else
	{
		g_dens = (double*)g_dens_ptr;
	}


	//g_schwarz = (double *) malloc(nbfn * nbfn * sizeof(double));
	PTR_T g_schwarz_ptr;
	DBCREATE( &g_schwarz_db, &g_schwarz_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	g_schwarz = (double*)g_schwarz_ptr;
	if(g_schwarz_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_schwarz, using malloc instead !!!!!!\n\n");
		g_schwarz = (double*)malloc(nbfn * nbfn * sizeof(double));
	}
	else
	{
		g_schwarz = (double*)g_schwarz_ptr;
	}

	//g_fock    = (double *) malloc(nbfn * nbfn * sizeof(double));
	PTR_T g_fock_ptr;
	DBCREATE( &g_fock_db, &g_fock_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	g_fock = (double*)g_fock_ptr;
	if(g_fock_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_fock, using malloc instead !!!!!!\n\n");
		g_fock = (double*)malloc(nbfn * nbfn * sizeof(double));
	}
	else
	{
		g_fock = (double*)g_fock_ptr;
	}

	//g_tfock   = (double *) malloc(nbfn * nbfn * sizeof(double));
	PTR_T g_tfock_ptr;
	DBCREATE( &g_tfock_db, &g_tfock_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	g_tfock = (double*)g_tfock_ptr;
	if(g_tfock_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_tfock, using malloc instead !!!!!!\n\n");
		g_tfock = (double*)malloc(nbfn * nbfn * sizeof(double));
	}
	else
	{
		g_tfock = (double*)g_tfock_ptr;
	}

	//g_work    = (double *) malloc(nbfn * nbfn * sizeof(double));
	PTR_T g_work_ptr;
	DBCREATE( &g_work_db, &g_work_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	g_work = (double*)g_work_ptr;
	if(g_work_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_work, using malloc instead !!!!!!\n\n");
		g_work = (double*)malloc(nbfn * nbfn * sizeof(double));
	}
	else
	{
		g_work = (double*)g_work_ptr;
	}

	//g_ident   = (double *) malloc(nbfn * nbfn * sizeof(double));
	PTR_T g_ident_ptr;
	DBCREATE( &g_ident_db, &g_ident_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	g_ident = (double*)g_ident_ptr;
	if(g_ident_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_ident, using malloc instead !!!!!!\n\n");
		g_ident = (double*)malloc(nbfn * nbfn * sizeof(double));
	}
	else
	{
		g_ident = (double*)g_ident_ptr;
	}

	//g_orbs    = (double *) malloc(nbfn * nbfn * sizeof(double));
	PTR_T g_orbs_ptr;
	DBCREATE( &g_orbs_db, &g_orbs_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	g_orbs = (double*)g_orbs_ptr;
	if(g_orbs_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_orbs, using malloc instead !!!!!!\n\n");
		g_orbs = (double*)malloc(nbfn * nbfn * sizeof(double));
	}
	else
	{
		g_orbs = (double*)g_orbs_ptr;
	}

	//g_schwarz_max_j = (double*) malloc(nbfn * sizeof(double));
	PTR_T g_schwarz_max_j_ptr;
	DBCREATE( &g_schwarz_max_j_db, &g_schwarz_max_j_ptr, nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	g_schwarz_max_j = (double*)g_schwarz_max_j_ptr;
	if(g_schwarz_max_j_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_schwarz_max_j, using malloc instead !!!!!!\n\n");
		g_schwarz_max_j = (double*)malloc(nbfn * sizeof(double));
	}
	else
	{
		g_schwarz_max_j = (double*)g_schwarz_max_j_ptr;
	}

	//g_precalc = (precalc*) malloc(nbfn * nbfn * sizeof(precalc));
	PTR_T g_precalc_ptr;
	DBCREATE( &g_precalc_db, &g_precalc_ptr, nbfn * nbfn * sizeof(precalc), FLAGS, affinity, NO_ALLOC);
	g_precalc = (precalc*)g_precalc_ptr;
	if(g_precalc_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_precalc, using malloc instead !!!!!!\n\n");
		g_precalc = (precalc*)malloc(nbfn * nbfn * sizeof(precalc));
	}
	else
	{
		g_precalc = (precalc*)g_precalc_ptr;
	}

	//g_fock_const = (double *) malloc(nbfn * nbfn * sizeof(double));
	PTR_T g_fock_const_ptr;
	DBCREATE( &g_fock_const_db, &g_fock_const_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	g_fock_const = (double*)g_fock_const_ptr;
	if(g_fock_const_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_fock_const, using malloc instead !!!!!!\n\n");
		g_fock_const = (double*)malloc(nbfn * nbfn * sizeof(double));
	}
	else
	{
		g_fock_const = (double*)g_fock_const_ptr;
	}

	//g_partial_fock = (double *) calloc(nbfn * nbfn * sizeof(double), 1);
	PTR_T g_partial_fock_ptr;
	DBCREATE( &g_partial_fock_db, &g_partial_fock_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
	g_partial_fock = (double*)g_partial_fock_ptr;
	if(g_partial_fock_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for g_partial_fock, using malloc instead !!!!!!\n\n");
		g_partial_fock = (double*)malloc(nbfn * nbfn * sizeof(double));
	}
	else
	{
		g_partial_fock = (double*)g_partial_fock_ptr;
	}

	for (j = 0; j < nbfn; j ++)
	{
		g_schwarz_max_j[j] = 0.0;

		for (i = 0; i < nbfn; i ++)
		{
			g_dens[j_off + i]    		= 0.0;
			g_schwarz[j_off + i] 		= 0.0;
			g_fock[j_off + i]    		= 0.0;
			g_tfock[j_off + i]   		= 0.0;
			g_work[j_off + i]    		= 0.0;
			g_ident[j_off + i]   		= 0.0;
			g_orbs [j_off + i]   		= 0.0;
			g_partial_fock [j_off + i]  	= 0.0;
		}
		j_off += nbfn;
	}

  return;
}
//======================================================================================================//

//======================================================================================================//
void ininrm(void)
{
	int i;
	long long int bf4 = pow((long long int) nbfn, 4);

	// write a little welcome message;
	printf(" Example Direct Self Consistent Field Program \n");
	printf(" -------------------------------------------- \n\n");
	printf(" no. of atoms .............. %5d\n", natom);
	printf(" no. of occupied orbitals .. %5lld\n", nocc);
	printf(" no. of basis functions .... %5lld\n", nbfn);
	printf(" basis functions^4 ......... %5lld\n", bf4);
	printf(" convergence threshold ..... %9.4f\n", tol);
	printf(" threads ................... %d\n", threads);

	// generate normalisation coefficients for the basis functions and the index array iky;
	for (i = 0; i < nbfn; i++)
	{
		iky[i] = (i + 1) * i / 2;
	//for (i = 0; i < nbfn; i++)
		rnorm[i] = pow((expnt[i] * 2.00 / pi), 0.750);
	}

	// initialize common for computing f0;
	setfm();
}
//======================================================================================================//


//======================================================================================================//
void setfm(void)
{
  int i, ii;
  double t[2001];
  double et[2001], rr, tt;

  delta  = 0.014;
  delo2  = delta * 0.5;
  rdelta = 1.0 / delta;

  for (i = 0; i < 2001; i++) {
    tt       = delta * (double)i;
    et[i]    = exprjh(-tt);
    t[i]     = tt * 2.0;
    //fm[i][4] = 0.0;
    fm[i*5+4] = 0.0;
  }

  for (i = 199; i > 3; i--) {
    rr = 1.0 / (double)(2 * i + 1);
    //for (ii = 0; ii < 2001; ii++) fm[ii][4] = (et[ii] + t[ii] * fm[ii][4]) * rr;
	for (ii = 0; ii < 2001; ii++) fm[ii*5+4] = (et[ii] + t[ii] * fm[ii*5+4]) * rr;
  }

  for (i = 3; i >= 0; i--) {
    rr = 1.0 / (double) (2 * i + 1);
    //for (ii = 0; ii < 2001; ii++) fm[ii][i] = (et[ii] + t[ii] * fm[ii][i+1]) * rr;
	for (ii = 0; ii < 2001; ii++) fm[ii*5+i] = (et[ii] + t[ii] * fm[ii*5+i+1]) * rr;
  }

  return;
}
//======================================================================================================//

//======================================================================================================//
// form guess density from superposition of atomic densities in the AO basis set ...
// instead of doing the atomic SCF hardwire for this small basis set for the Be atom;
void denges(void) {
  int i, j, k, l;

  double atdens[15][15] = {
    {0.000002,0.000027,0.000129,0.000428,0.000950,0.001180,
     0.000457,-0.000270,-0.000271,0.000004,0.000004,0.000004,
     0.000004,0.000004,0.000004},
    {0.000027,0.000102,0.000987,
     0.003269,0.007254,0.009007,0.003492,-0.002099,-0.002108,
     0.000035,0.000035,0.000035,0.000035,0.000035,0.000035},
    {0.000129,0.000987,0.002381,0.015766,0.034988,0.043433,
     0.016835,-0.010038,-0.010082,0.000166,0.000166,0.000166,
     0.000166,0.000166,0.000166},
    {0.000428,0.003269,0.015766,
     0.026100,0.115858,0.144064,0.055967,-0.035878,-0.035990,
     0.000584,0.000584,0.000584,0.000584,0.000584,0.000584},
    {0.000950,0.007254,0.034988,0.115858,0.128586,0.320120,
     0.124539,-0.083334,-0.083536,0.001346,0.001346,0.001346,
     0.001346,0.001346,0.001346},
    {0.001180,0.009007,0.043433,
     0.144064,0.320120,0.201952,0.159935,-0.162762,-0.162267,
     0.002471,0.002471,0.002471,0.002471,0.002471,0.002471},
    {0.000457,0.003492,0.016835,0.055967,0.124539,0.159935,
     0.032378,-0.093780,-0.093202,0.001372,0.001372,0.001372,
     0.001372,0.001372,0.001372},
    {-0.000270,-0.002099,-0.010038,
     -0.035878,-0.083334,-0.162762,-0.093780,0.334488,0.660918,
     -0.009090,-0.009090,-0.009090,-0.009090,-0.009090,-0.009090},
    {-0.000271,-0.002108,-0.010082,-0.035990,-0.083536,-0.162267,
     -0.093202,0.660918,0.326482,-0.008982,-0.008982,-0.008981,
     -0.008981,-0.008981,-0.008982},
    {0.000004,0.000035,0.000166,
     0.000584,0.001346,0.002471,0.001372,-0.009090,-0.008982,
     0.000062,0.000124,0.000124,0.000124,0.000124,0.000124},
    {0.000004,0.000035,0.000166,0.000584,0.001346,0.002471,
     0.001372,-0.009090,-0.008982,0.000124,0.000062,0.000124,
     0.000124,0.000124,0.000124},
    {0.000004,0.000035,0.000166,
     0.000584,0.001346,0.002471,0.001372,-0.009090,-0.008981,
     0.000124,0.000124,0.000062,0.000124,0.000124,0.000124},
    {0.000004,0.000035,0.000166,0.000584,0.001346,0.002471,
     0.001372,-0.009090,-0.008981,0.000124,0.000124,0.000124,
     0.000062,0.000124,0.000124},
    {0.000004,0.000035,0.000166,
     0.000584,0.001346,0.002471,0.001372,-0.009090,-0.008981,
     0.000124,0.000124,0.000124,0.000124,0.000062,0.000124},
    {0.000004,0.000035,0.000166,0.000584,0.001346,0.002471,
     0.001372,-0.009090,-0.008982,0.000124,0.000124,0.000124,
     0.000124,0.000124,0.000062}};

// correct for a factor of two along the diagonal;
  for (i = 0; i < 15; i++) atdens[i][i] = 2.00 * atdens[i][i];

// fill in each block of 15 rows and 15 columns
  for (i = 0; i < nbfn; i++)
  for (j = 0; j < nbfn; j++) g_dens[OFFSET(i,j)] = 0.0;

  for (i = 0; i < nbfn; i += 15) {
      for (k = 0; k < 15; k++) {
      for (l = 0; l < 15; l++) {
          g_dens[OFFSET(i+k,i+l)] = atdens[k][l];
  } } }

  return;
}
//======================================================================================================//


//======================================================================================================//
// generate the overlap matrix element between the normalized primitve gaussian 1s functions i and j;
double s(int i, int j) {
  double rab2, facij, temp;

  rab2  = (x[i] - x[j]) * (x[i] - x[j]) + (y[i] - y[j]) * (y[i] - y[j]) + (z[i] - z[j]) * (z[i] - z[j]);
  facij = expnt[i] * expnt[j] / (expnt[i] + expnt[j]);
  temp  = pow((pi / (expnt[i] + expnt[j])), 1.50) * exprjh(-facij * rab2) * rnorm[i] * rnorm[j];

  return temp;
}
//======================================================================================================//


//======================================================================================================//
// generate set of orthonormal vectors by creating a random symmetric matrix
// and solving associated generalized eigenvalue problem using the correct overlap matrix.
void makeob(void) {
  int i, j;
  //double eval[maxnbfn];
  ocrGuid_t affinity=NULL_GUID;
  PTR_T eval_ptr;
  ocrGuid_t eval_db;
  DBCREATE( &eval_db, &eval_ptr, maxnbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *eval = (double*)eval_ptr;

  for (i = 0; i < nbfn; i ++) {
  for (j = 0; j < nbfn; j ++) {
      g_ident[OFFSET(i,j)] = s(i, j);
      g_fock [OFFSET(i,j)] = 0.5;
  } }

  Eigen_gen(g_fock, g_ident, g_orbs, eval);

  ocrDbRelease(eval_db);

  return;
}
//======================================================================================================//

//======================================================================================================//
double makesz()
{
	int i, j;
	double smax = 0.0;

	for (j = 0; j < nbfn; j++)
	{
		double jmax = 0.0;
		for (i = 0; i < nbfn; i++)
		{
			// g_fast can be called instead here, but NEVER use a caching variant
			// of g unless it actually looks up the result based on the arguments
			double gg = sqrt( g(i, j, i, j) );
			if (gg > smax)
				smax = gg;

			g_schwarz[OFFSET(i,j)] = gg;

			if (gg > jmax)
				jmax = gg;
		}
		g_schwarz_max_j[j] = jmax;
	}

	return smax;
}
//======================================================================================================//

//======================================================================================================//
// fill in the one-electron part of the fock matrix and compute the one-electron energy contribution;
double oneel(double schwmax)
{
	int i, j, i_off=0;
	double tol2e_over_schwmax = tol2e / schwmax;
	for (i = 0; i < nbfn; i++)
	{
		for (j = 0; j < nbfn; j++)
		{
			g_fock[i_off + j] = (g_schwarz[i_off + j]  > tol2e_over_schwmax) ? h(i, j) : 0.0;
		}
		i_off += nbfn;
	}

	return (0.50 * contract_matrices(g_fock, g_dens));
}
//======================================================================================================//

//======================================================================================================//
// evalute sum_ij a_ij * b_ij;
double contract_matrices(double * a, double * b) {
#ifdef BLAS
  return cblas_ddot(nbfn*nbfn, a, 1, b, 1);
#else
  int i;
  double value = 0.0;

  for (i = 0; i < nbfn * nbfn; i++) {
    value += *(a++) * *(b++);
  }

  return value;

#endif
}
//======================================================================================================//

//======================================================================================================//
// generate the one particle hamiltonian matrix element over
// the normalized primitive 1s functions i and j;
double h(int i,int j) {
  double f0val = 0.00, sum = 0.00;
  double facij,expij,repij, xp,yp,zp,rpc2, rab2;
  int iat;

  rab2  = (x[i]-x[j])*(x[i]-x[j]) + (y[i]-y[j])*(y[i]-y[j]) + (z[i]-z[j])*(z[i]-z[j]);
  facij = expnt[i]*expnt[j]/(expnt[i]+expnt[j]);
  expij = exprjh(-facij * rab2);
  repij = (2.00 * pi / (expnt[i] + expnt[j])) * expij;

// first do the nuclear attraction integrals;
  for (iat = 0;iat < natom;iat++) {
      xp = (x[i]*expnt[i] + x[j]*expnt[j])/(expnt[i]+expnt[j]);
      yp = (y[i]*expnt[i] + y[j]*expnt[j])/(expnt[i]+expnt[j]);
      zp = (z[i]*expnt[i] + z[j]*expnt[j])/(expnt[i]+expnt[j]);
      rpc2 = (xp-ax[iat])*(xp-ax[iat]) + (yp-ay[iat])*(yp-ay[iat]) + (zp-az[iat])*(zp-az[iat]);
      f0val = f0((expnt[i] + expnt[j]) * rpc2);
      sum = sum - repij * q[iat] * f0val;
   }

// add on the kinetic energy term;
   sum = sum + facij*(3.00-2.00*facij*rab2) * pow((pi/(expnt[i]+expnt[j])),1.50) * expij;

// finally multiply by the normalization constants;
   return sum * rnorm[i] * rnorm[j];
}
//======================================================================================================//

//======================================================================================================//
void closearrays(void) {
  ocrDbRelease(g_dens_db);  ocrDbRelease(g_fock_db); ocrDbRelease(g_schwarz_db);
  ocrDbRelease(g_tfock_db); ocrDbRelease(g_work_db); ocrDbRelease(g_ident_db); ocrDbRelease(g_orbs_db);
  ocrDbRelease(g_schwarz_max_j_db); ocrDbRelease(g_fock_const_db); ocrDbRelease(g_partial_fock_db);

  if (g_cache) ocrDbRelease(g_cache_db);

  return;
}
//======================================================================================================//

//======================================================================================================//
// generate density matrix from orbitals in g_orbs. the first nocc orbitals are doubly occupied.
void makden(void) {
#ifdef BLAS
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasTrans, \
              nbfn, nbfn, nocc, 2.0, g_orbs, nbfn, g_orbs, nbfn, 0.0, g_work, nbfn);
#else
  int i, j, k;

  for (i = 0; i < nbfn; i++) {
  for (j = 0; j < nbfn; j++) {
    double p = 0.0;
    for (k = 0; k < nocc; k++) p = p + g_orbs[OFFSET(i,k)] * g_orbs[OFFSET(j,k)];
    g_work[OFFSET(i,j)] = 2.0 * p;
  } }
#endif
}
//======================================================================================================//

//======================================================================================================//
// create damped density matrix as a linear combination of old density matrix
// and density matrix formed from new orbitals;
void damp(double fac) {
#ifdef BLAS_L1
  cblas_dscal(nbfn*nbfn, fac, g_dens, 1);
  cblas_daxpy(nbfn*nbfn, 1.0 - fac, g_work, 1, g_dens, 1);
#else
  int i;
  double ofac = 1.00 - fac;
  double* dens = g_dens;
  double* work = g_work;

  for (i = 0; i < nbfn * nbfn; i++) {
    *dens = (fac * *dens) + (ofac * *(work++));
    dens++;
  }
#endif
}
//======================================================================================================//

