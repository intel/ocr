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

#include "twoel.h"

double contract_matrices(double *, double *);

twoel_context_t* twoel_context;
double* t_fock = NULL;

volatile int mymutex[16];

volatile int g_i, g_ij, g_kl;

//======================================================================================================//
double timer(void)
{

//     return the time since the last call to timer.

//     must be initialized by calling once and throwing away the
//     value
//     ... use cpu time on multi-user machines
//     ... use elapsed time on dedicated or single user machines.
//
//mdc*if unix
//      real*4 dtime, tt(2)
//      timer = dble(dtime(tt))
//mdc*elseif tcgmsg
//Id: timer.F,v 1.1 2005-03-08 23:58:03 d3g293 Exp $

  const double million = 1.0e6;

  static struct timeval tvlast;
  static int initted = 0;

  struct timeval tv;
  struct timezone tz;

  double ret, t0, t1;

  gettimeofday(&tv, &tz);

  if (!initted) {
    tvlast = tv;
    initted = 1;
  }

  t0 = tvlast.tv_sec * million + tvlast.tv_usec;
  t1 = tv.tv_sec * million + tv.tv_usec;

  ret = (t1 - t0) / million;
  tvlast = tv;

  return ret;
}
//======================================================================================================//

//======================================================================================================//
double timer2(void)
{

//     return the time since the last call to timer.

//     must be initialized by calling once and throwing away the
//     value
//     ... use cpu time on multi-user machines
//     ... use elapsed time on dedicated or single user machines.
//
//mdc*if unix
//      real*4 dtime, tt(2)
//      timer = dble(dtime(tt))
//mdc*elseif tcgmsg
//Id: timer.F,v 1.1 2005-03-08 23:58:03 d3g293 Exp $

  const double million = 1.0e6;

  struct timeval tv;
  struct timezone tz;

  double ret, t1;

  gettimeofday(&tv, &tz);

  t1 = tv.tv_sec * million + tv.tv_usec;

  ret = t1 / million;

  return ret;
}
//======================================================================================================//


//======================================================================================================//
// compute largest change in {density - work} matrix elements;
double dendif(void) {
  double denmax = 0.00;
#ifdef BLAS_L1
  cblas_dcopy(nbfn*nbfn, g_dens, 1, g_tfock, 1);
  cblas_daxpy(nbfn*nbfn, -1.0, g_work, 1, g_tfock, 1);
  denmax = fabs(g_tfock[cblas_idamax(nbfn*nbfn, g_tfock, 1)]);
#else
  int i;
  double* dens = g_dens;
  double* work = g_work;

  for (i = 0; i < nbfn * nbfn; i++) {
    double xdiff = fabs(*(dens++) - *(work++));
    if (xdiff > denmax) denmax = xdiff;
  }
#endif
  return denmax;
}
//======================================================================================================//

//======================================================================================================//
double diagon(int iter) {
  int i, j, i_off=0;
  double shift, tester = 0.00;

// use similarity transform to solve standard eigenvalue problem
// (overlap matrix has been transformed out of the problem);

#ifdef BLAS
  cblas_dsymm(CblasRowMajor, CblasLeft, CblasUpper, \
      nbfn, nbfn, 1.0, g_fock, nbfn, g_orbs, nbfn, 0.0, g_tfock, nbfn);
  cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, \
      nbfn, nbfn, nbfn, 1.0, g_orbs, nbfn, g_tfock, nbfn, 0.0, g_fock, nbfn);
#else
  Dgemm('n', 'n', nbfn, nbfn, nbfn, g_fock, g_orbs,  g_tfock);
  Dgemm('t', 'n', nbfn, nbfn, nbfn, g_orbs, g_tfock, g_fock );
#endif

// compute largest change in off-diagonal fock matrix elements;
  for (i = 0; i < nbfn; i++) {
  for (j = 0; j < nbfn; j++) {
      if (i == j) continue;
      double xtmp = fabs(g_fock[i_off + j]);
      if (xtmp > tester) tester = xtmp;
  }
  i_off += nbfn;
  }

  shift = 0.00;
  if      (tester > 0.30) shift = 0.30;
  else if (nbfn   > 60  ) shift = 0.10;
  else                    shift = 0.00;

  if (iter >= 1 && shift != 0.00)
     for (i = nocc; i < nbfn; i++) g_fock[OFFSET(i,i)] += shift;


  memcpy(g_tfock, g_orbs, nbfn*nbfn * sizeof(double));

  double t0 = timer2();
  Eigen_std(g_fock, g_work, eigv);
  tdgemm += (timer2() - t0);

// back transform eigenvectors;
#ifdef BLAS
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, \
      nbfn, nbfn, nbfn, 1.0, g_tfock, nbfn, g_work, nbfn, 0.0, g_orbs, nbfn);
#else
  Dgemm('n', 'n', nbfn, nbfn, nbfn, g_tfock, g_work, g_orbs);
#endif

  if (iter >= 1 && shift != 0.00)
    for (i = nocc; i < nbfn; i++) eigv[i] = eigv[i] - shift;

  return tester;
}
//======================================================================================================//

//======================================================================================================//
void twoel_i_j_k_l_all_different(double tol2e_over_schwmax, int start, int step, int id)
{
	CUT_DECLS;
	int i, j, k, l;
	int i_off, j_off, k_off, l_off;
	double* fock = twoel_context[id].fock;

// 8-way symmetry
#ifdef DYNAMIC_SCHED

	//atomic assign of the next i to be computed
	i = __sync_fetch_and_add (&g_i, CHUNK_SIZE);

	while ( i  < nbfn)
	{
	int end = i + CHUNK_SIZE;

#else

	//static assignment of work for this thread
	i = start;
	int end = i + step;
	{

#endif

		i_off = (i-1) * nbfn;

		for (; i < nbfn && i < end; i++)
		{
			i_off += nbfn;
			j_off = i_off;


		for (j = i + 1; j < nbfn; j++)
		{
			j_off += nbfn;
			DENS_DECL_SYMM(j,i);
			TOL_DECL(i,j);

			if (g_schwarz[OFF(i,j)] < tol2e_over_schwmax)
			{
				CUT1(8 * (-2*j + i*(3-2*nbfn) + i*i + nbfn*(nbfn - 1))/2);
				continue;
			}
			k_off = i_off - nbfn;

			for (k = i; k < nbfn; k++)
			{
				k_off += nbfn;
				DENS_DECL_SYMM(k,i);
				DENS_DECL_SYMM(k,j);
				if (g_schwarz_max_j[k] < tol2e_over_g_schwarz_i_j)
				{
					CUT4(8 * ((k==i) ? nbfn - j - 1: nbfn - k - 1));
					continue;
				}

				l_off = ((k == i) ? j_off : k_off);
				for (l = 1 + ((k == i) ? j : k); l < nbfn; l++)
				{
					l_off += nbfn;

					if (g_schwarz[OFF(k,l)] < tol2e_over_g_schwarz_i_j)
					{
						CUT2(8);
						continue;
					}

					DENS_DECL_SYMM(l,i);
					DENS_DECL_SYMM(l,j);
					DENS_DECL_SYMM(l,k);

					CUT3(8);
					double gg = G(i, j, k, l);


					//-----------------------------------------------//
#if defined OCR_ATOMIC
#ifdef BAD_CACHE_UTILIZATION
					while (__sync_lock_test_and_set(mymutex+0, 1));
					UPDATE1(i,j,k,l);
					__sync_lock_release(mymutex+0);
					while (__sync_lock_test_and_set(mymutex+1, 1));
					UPDATE1(i,j,l,k);
					__sync_lock_release(mymutex+1);
					while (__sync_lock_test_and_set(mymutex+2, 1));
					UPDATE1(j,i,k,l);
					__sync_lock_release(mymutex+2);
					while (__sync_lock_test_and_set(mymutex+3, 1));
					UPDATE1(j,i,l,k);
					__sync_lock_release(mymutex+3);
					while (__sync_lock_test_and_set(mymutex+4, 1));
					UPDATE1(k,l,i,j);
					__sync_lock_release(mymutex+4);
					while (__sync_lock_test_and_set(mymutex+5, 1));
					UPDATE1(k,l,j,i);
					__sync_lock_release(mymutex+5);
					while (__sync_lock_test_and_set(mymutex+6, 1));
					UPDATE1(l,k,i,j);
					__sync_lock_release(mymutex+6);
					while (__sync_lock_test_and_set(mymutex+7, 1));
					UPDATE1(l,k,j,i);
					__sync_lock_release(mymutex+7);

					while (__sync_lock_test_and_set(mymutex+8, 1));
					UPDATE2(i,j,k,l);
					__sync_lock_release(mymutex+8);
					while (__sync_lock_test_and_set(mymutex+9, 1));
					UPDATE2(i,j,l,k);
					__sync_lock_release(mymutex+9);
					while (__sync_lock_test_and_set(mymutex+10, 1));
					UPDATE2(j,i,k,l);
					__sync_lock_release(mymutex+10);
					while (__sync_lock_test_and_set(mymutex+11, 1));
					UPDATE2(j,i,l,k);
					__sync_lock_release(mymutex+11);
					while (__sync_lock_test_and_set(mymutex+12, 1));
					UPDATE2(k,l,i,j);
					__sync_lock_release(mymutex+12);
					while (__sync_lock_test_and_set(mymutex+13, 1));
					UPDATE2(k,l,j,i);
					__sync_lock_release(mymutex+13);
					while (__sync_lock_test_and_set(mymutex+14, 1));
					UPDATE2(l,k,i,j);
					__sync_lock_release(mymutex+14);
					while (__sync_lock_test_and_set(mymutex+15, 1));
					UPDATE2(l,k,j,i);
					__sync_lock_release(mymutex+15);


#else
				    /*
				     * Exploiting yet another symmetry to make the faster and harder to read:
				     * - g_dens is a symmetric matrix
				     * - g_fock is a symmetric matrix
				     *
				     * The normal update sequence pulls identical values from both triangles
				     * of g_dens and writes identical values to both triangles of g_fock.
				     *
				     * We can improve cache utilization by limiting or avoiding memory
				     * access to one of the triangles.
				     *
				     * The following code separates the Coulomb and Exchange force update
				     * so that we can use the symmetry of each to reduce memory operations
				     * and limit access to the lower triangle. There are still some lower
				     * triangle accesses, but they are significantly reduced.
				     *
				     * Once computed, the partial fock matrix will need some more processing
				     * to make it symmetric again, but this can be done in an N^2 loop. To
				     * get the final partial matrix, values from the upper and lower triangles
				     * must be added together and written back to both triangles.
				     *
				     */

					while (__sync_lock_test_and_set(mymutex+0, 1));
					UPDATE_COULOMB2(i,j,k,l);
					__sync_lock_release(mymutex+0);
					while (__sync_lock_test_and_set(mymutex+1, 1));
					UPDATE_COULOMB2(k,l,i,j);
					__sync_lock_release(mymutex+1);
					while (__sync_lock_test_and_set(mymutex+2, 1));
					UPDATE_EXCHANGE(i,j,k,l);
					__sync_lock_release(mymutex+2);
					while (__sync_lock_test_and_set(mymutex+3, 1));
					UPDATE_EXCHANGE(i,j,l,k);
					__sync_lock_release(mymutex+3);
					while (__sync_lock_test_and_set(mymutex+4, 1));
					UPDATE_EXCHANGE(j,i,k,l);
					__sync_lock_release(mymutex+4);
					while (__sync_lock_test_and_set(mymutex+5, 1));
					UPDATE_EXCHANGE(j,i,l,k);
					__sync_lock_release(mymutex+5);

#endif

					//-----------------------------------------------//
#elif defined OCR_CRITICAL
					//enter critical section
					while (__sync_lock_test_and_set(mymutex, 1));

#ifdef BAD_CACHE_UTILIZATION
					UPDATE(i,j,k,l);
					UPDATE(i,j,l,k);
					UPDATE(j,i,k,l);
					UPDATE(j,i,l,k);
					UPDATE(k,l,i,j);
					UPDATE(k,l,j,i);
					UPDATE(l,k,i,j);
					UPDATE(l,k,j,i);
#else
	/*
	* Exploiting yet another symmetry to make the faster and harder to read:
	* - g_dens is a symmetric matrix
	* - g_fock is a symmetric matrix
	*
	* The normal update sequence pulls identical values from both triangles
	* of g_dens and writes identical values to both triangles of g_fock.
	*
	* We can improve cache utilization by limiting or avoiding memory
	* access to one of the triangles.
	*
	* The following code separates the Coulomb and Exchange force update
	* so that we can use the symmetry of each to reduce memory operations
	* and limit access to the lower triangle. There are still some lower
	* triangle accesses, but they are significantly reduced.
	*
	* Once computed, the partial fock matrix will need some more processing
	* to make it symmetric again, but this can be done in an N^2 loop. To
	* get the final partial matrix, values from the upper and lower triangles
	* must be added together and written back to both triangles.
	*
	*/

					UPDATE_COULOMB2(i,j,k,l);
					UPDATE_COULOMB2(k,l,i,j);

					UPDATE_EXCHANGE(i,j,k,l);
					UPDATE_EXCHANGE(i,j,l,k);
					UPDATE_EXCHANGE(j,i,k,l);
					UPDATE_EXCHANGE(j,i,l,k);
#endif
					//leave critical section
					__sync_lock_release(mymutex);


#else

#ifdef BAD_CACHE_UTILIZATION
					UPDATE(i,j,k,l);
					UPDATE(i,j,l,k);
					UPDATE(j,i,k,l);
					UPDATE(j,i,l,k);
					UPDATE(k,l,i,j);
					UPDATE(k,l,j,i);
					UPDATE(l,k,i,j);
					UPDATE(l,k,j,i);
#else
	/*
	* Exploiting yet another symmetry to make the faster and harder to read:
	* - g_dens is a symmetric matrix
	* - g_fock is a symmetric matrix
	*
	* The normal update sequence pulls identical values from both triangles
	* of g_dens and writes identical values to both triangles of g_fock.
	*
	* We can improve cache utilization by limiting or avoiding memory
	* access to one of the triangles.
	*
	* The following code separates the Coulomb and Exchange force update
	* so that we can use the symmetry of each to reduce memory operations
	* and limit access to the lower triangle. There are still some lower
	* triangle accesses, but they are significantly reduced.
	*
	* Once computed, the partial fock matrix will need some more processing
	* to make it symmetric again, but this can be done in an N^2 loop. To
	* get the final partial matrix, values from the upper and lower triangles
	* must be added together and written back to both triangles.
	*
	*/

					UPDATE_COULOMB2(i,j,k,l);
					UPDATE_COULOMB2(k,l,i,j);

					UPDATE_EXCHANGE(i,j,k,l);
					UPDATE_EXCHANGE(i,j,l,k);
					UPDATE_EXCHANGE(j,i,k,l);
					UPDATE_EXCHANGE(j,i,l,k);
#endif


#endif//OCR_VERSION
					//-----------------------------------------------//
				} // l
			} // k
		} // j
	} // i
#ifdef DYNAMIC_SCHED
	//atomic assign of the next i to be computed
	i = __sync_fetch_and_add (&g_i, CHUNK_SIZE);
#endif
	}

#ifndef BAD_CACHE_UTILIZATION
	/*
	* Fix the partial fock matrix.
	* To improve cache utilization, the temp fock matricies had most, but not
	* all of their updates limited to the upper triangle. To get the correct
	* result, we need to add values from both the upper and lower triangles:
	*
	* The asymmetric partial fock matrix has the following form:
	*
	*  |   V(1,1)   V(1,2)   V(1,3)   V(1,4)  |
	*  |   s(2,1)   V(2,2)   V(2,3)   V(2,4)  |
	*  |   s(3,1)   s(3,2)   V(3,3)   V(3,4)  |
	*  |   s(4,1)   s(4,2)   s(4,3)   V(4,4)  |
	*
	*  Where V(i,j) is much larger than s(j,i)
	*
	*  To re-symmetrize:
	*
	* PARTIAL_FOCK(i,j) = ASYM_PARTIAL(i,j) + ASYM_PARTIAL(j,i)
	*
	* Which results in the correct partial fock matrix which has the form:
	*
	*  |     2 * V(1,1)      V(1,2) + s(2,1)   V(1,3) + s(3,1)   V(1,4) + s(4,1)  |
	*  |   s(2,1) + V(1,2)     2 * V(2,2)      V(2,3) + s(3,2)   V(2,4) + s(4,2)  |
	*  |   s(3,1) + V(1,3)   s(3,2) + V(2,3)     2 * V(3,3)      V(3,4) + s(4,3)  |
	*  |   s(4,1) + V(1,4)   s(4,2) + V(2,4)   s(4,3) + V(3,4)     2 * V(4,4)     |
	*
	*/
	k_off = 0;
	for (k = 0; k < nbfn; k++)
	{
		l_off = k_off;
		for (l = k; l < nbfn; l++)
		{
			double value = fock[OFF(k,l)] + fock[OFF(l,k)];
			fock[OFF(k,l)] = value;
			fock[OFF(l,k)] = value;
			l_off += nbfn;
		}
		k_off += nbfn;
	}

#endif

	CUT_WRITEBACK;
}
//======================================================================================================//


//======================================================================================================//
void twoel_i_eq_j(double tol2e_over_schwmax, int start, int step, int id)
{
	CUT_DECLS;
	int i, k, l;
	int i_off, k_off, l_off;
	double* fock = twoel_context[id].fock;

	// 4-way symmetry
#ifdef DYNAMIC_SCHED

	//atomic assign of the next i to be computed
	i = __sync_fetch_and_add (&g_ij, N3_CHUNK_SIZE);
	while ( i  < nbfn)
	{
	int end = i + N3_CHUNK_SIZE;

#else

	//static assignment of work for this thread
	i = start;
	int end = i + step;
	{

#endif

	i_off = (i-1) * nbfn;

	for (; i < nbfn && i < end; i++)
	{
		i_off += nbfn;

		DENS_DECL(i,i);
		TOL_DECL(i,i);

		if (g_schwarz[OFF(i,i)] < tol2e_over_schwmax)
		{
			CUT1(4 * (1 + i - nbfn) * (i - nbfn) / 2);
			continue;
		}
		k_off = i_off - nbfn;

		for (k = i; k < nbfn; k++)
		{
			k_off += nbfn;
			DENS_DECL_SYMM(k,i);
			if (g_schwarz_max_j[k] < tol2e_over_g_schwarz_i_i)
			{
				CUT4(4 * (nbfn - k - 1));
				continue;
			}

			l_off = k_off;
			for (l = 1 + k; l < nbfn; l++)
			{
				l_off += nbfn;

				if (g_schwarz[OFF(k,l)] < tol2e_over_g_schwarz_i_i)
				{
	  				CUT2(4);
					  continue;
				}

				DENS_DECL_SYMM(l,i);
				DENS_DECL_SYMM(l,k);

				CUT3(4);
				double gg = G(i, i, k, l);

				//-----------------------------------------------//
#if defined OCR_ATOMIC

#ifdef BAD_CACHE_UTILIZATION
				while (__sync_lock_test_and_set(mymutex+0, 1));
				UPDATE1(i,i,k,l);
				__sync_lock_release(mymutex+0);
				while (__sync_lock_test_and_set(mymutex+1, 1));
				UPDATE1(i,i,l,k);
				__sync_lock_release(mymutex+1);
				while (__sync_lock_test_and_set(mymutex+2, 1));
				UPDATE1(k,l,i,i);
				__sync_lock_release(mymutex+2);
				while (__sync_lock_test_and_set(mymutex+3, 1));
				UPDATE1(l,k,i,i);
				__sync_lock_release(mymutex+3);

				while (__sync_lock_test_and_set(mymutex+4, 1));
				UPDATE2(i,i,k,l);
				__sync_lock_release(mymutex+4);
				while (__sync_lock_test_and_set(mymutex+5, 1));
				UPDATE2(i,i,l,k);
				__sync_lock_release(mymutex+5);
				while (__sync_lock_test_and_set(mymutex+6, 1));
				UPDATE2(k,l,i,i);
				__sync_lock_release(mymutex+6);
				while (__sync_lock_test_and_set(mymutex+7, 1));
				UPDATE2(l,k,i,i);
				__sync_lock_release(mymutex+7);

#else
				while (__sync_lock_test_and_set(mymutex+0, 1));
				UPDATE_COULOMB1(i,i,k,l);
				__sync_lock_release(mymutex+0);
				while (__sync_lock_test_and_set(mymutex+1, 1));
				UPDATE_COULOMB1(k,l,i,i);
				__sync_lock_release(mymutex+1);

				while (__sync_lock_test_and_set(mymutex+2, 1));
				UPDATE_EXCHANGE(i,i,k,l);
				__sync_lock_release(mymutex+2);
				while (__sync_lock_test_and_set(mymutex+3, 1));
				UPDATE_EXCHANGE(i,i,l,k);
				__sync_lock_release(mymutex+3);
#endif


#elif defined OCR_CRITICAL
				//enter critical section
				while (__sync_lock_test_and_set(mymutex, 1));

#ifdef BAD_CACHE_UTILIZATION
				UPDATE(i,i,k,l);
				UPDATE(i,i,l,k);
				UPDATE(k,l,i,i);
				UPDATE(l,k,i,i);
#else
				UPDATE_COULOMB1(i,i,k,l);
				UPDATE_COULOMB1(k,l,i,i);

				UPDATE_EXCHANGE(i,i,k,l);
				UPDATE_EXCHANGE(i,i,l,k);
#endif

				//leave critical section
				__sync_lock_release(mymutex);

#else

#ifdef BAD_CACHE_UTILIZATION
				UPDATE(i,i,k,l);
				UPDATE(i,i,l,k);
				UPDATE(k,l,i,i);
				UPDATE(l,k,i,i);
#else
				UPDATE_COULOMB1(i,i,k,l);
				UPDATE_COULOMB1(k,l,i,i);

				UPDATE_EXCHANGE(i,i,k,l);
				UPDATE_EXCHANGE(i,i,l,k);
#endif


#endif//OCR_VERSION
				//-----------------------------------------------//


			} // l
		} // k
	} // i
#ifdef DYNAMIC_SCHED
	//atomic assign of the next i to be computed
	i = __sync_fetch_and_add (&g_ij, N3_CHUNK_SIZE);
#endif
	}

	CUT_WRITEBACK;
}
//======================================================================================================//


//======================================================================================================//
void twoel_k_eq_l(double tol2e_over_schwmax, int start, int step, int id)
{
	CUT_DECLS;
	int i, j, k;
	int i_off, j_off, k_off;
	double* fock = twoel_context[id].fock;

	// 4-way symmetry
#ifdef DYNAMIC_SCHED

	//atomic assign of the next i to be computed
	i = __sync_fetch_and_add (&g_kl, N3_CHUNK_SIZE);
	while ( i  < nbfn)
	{
	int end = i + N3_CHUNK_SIZE;

#else

	//static assignment of work for this thread
	i =start;
	int end = i + step;
	{

#endif

	i_off = (i-1) * nbfn;

	for (; i < nbfn && i < end; i++)
	{
		i_off += nbfn;
		j_off = i_off;

		for (j = i + 1; j < nbfn; j++)
		{
			j_off += nbfn;
			DENS_DECL_SYMM(j,i);
			TOL_DECL(i,j);

			if (g_schwarz[OFF(i,j)] < tol2e_over_schwmax)
			{
				CUT1(4 * (nbfn - i - 1));
				continue;
			}
			k_off = i_off;

			for (k = i + 1; k < nbfn; k++)
			{
				k_off += nbfn;

				DENS_DECL_SYMM(k,i);
				DENS_DECL_SYMM(k,j);
				DENS_DECL(k,k);

				if (g_schwarz[OFF(k,k)] < tol2e_over_g_schwarz_i_j)
				{
					CUT2(4);
					continue;
				}

					CUT3(4);
					double gg = G(i, j, k, k);

				//-----------------------------------------------//
#if defined OCR_ATOMIC
#ifdef BAD_CACHE_UTILIZATION

				while (__sync_lock_test_and_set(mymutex, 1));
				UPDATE1(i,j,k,k);
				__sync_lock_release(mymutex);

				while (__sync_lock_test_and_set(mymutex+1, 1));
				UPDATE1(j,i,k,k);
				__sync_lock_release(mymutex+1);

				while (__sync_lock_test_and_set(mymutex+2, 1));
				UPDATE1(k,k,i,j);
				__sync_lock_release(mymutex+2);

				while (__sync_lock_test_and_set(mymutex+3, 1));
				UPDATE1(k,k,j,i);
				__sync_lock_release(mymutex+3);

				while (__sync_lock_test_and_set(mymutex+4, 1));
				UPDATE2(i,j,k,k);
				__sync_lock_release(mymutex+4);

				while (__sync_lock_test_and_set(mymutex+5, 1));
				UPDATE2(j,i,k,k);
				__sync_lock_release(mymutex+5);

				while (__sync_lock_test_and_set(mymutex+6, 1));
				UPDATE2(k,k,i,j);
				__sync_lock_release(mymutex+6);

				while (__sync_lock_test_and_set(mymutex+7, 1));
				UPDATE2(k,k,j,i);
				__sync_lock_release(mymutex+7);


#else
				while (__sync_lock_test_and_set(mymutex, 1));
				UPDATE_COULOMB1(i,j,k,k);
				__sync_lock_release(mymutex);
				while (__sync_lock_test_and_set(mymutex+1, 1));
				UPDATE_COULOMB1(k,k,i,j);
				__sync_lock_release(mymutex+1);

				while (__sync_lock_test_and_set(mymutex+2, 1));
				UPDATE_EXCHANGE(i,j,k,k);
				__sync_lock_release(mymutex+2);
				while (__sync_lock_test_and_set(mymutex+3, 1));
				UPDATE_EXCHANGE(j,i,k,k);
				__sync_lock_release(mymutex+3);
#endif

#elif defined OCR_CRITICAL
				//enter critical section
				while (__sync_lock_test_and_set(mymutex, 1));

#ifdef BAD_CACHE_UTILIZATION
				UPDATE(i,j,k,k);
				UPDATE(j,i,k,k);
				UPDATE(k,k,i,j);
				UPDATE(k,k,j,i);
#else
				UPDATE_COULOMB1(i,j,k,k);
				UPDATE_COULOMB1(k,k,i,j);

				UPDATE_EXCHANGE(i,j,k,k);
				UPDATE_EXCHANGE(j,i,k,k);
#endif

				//leave critical section
				__sync_lock_release(mymutex);

#else

#ifdef BAD_CACHE_UTILIZATION
				UPDATE(i,j,k,k);
				UPDATE(j,i,k,k);
				UPDATE(k,k,i,j);
				UPDATE(k,k,j,i);
#else
				UPDATE_COULOMB1(i,j,k,k);
				UPDATE_COULOMB1(k,k,i,j);

				UPDATE_EXCHANGE(i,j,k,k);
				UPDATE_EXCHANGE(j,i,k,k);
#endif

#endif//OCR_VERSION
				//-----------------------------------------------//

			} // k
		} // j
	} // i
#ifdef DYNAMIC_SCHED
	//atomic assign of the next i to be computed
     	i = __sync_fetch_and_add (&g_kl, N3_CHUNK_SIZE);
#endif
	}

	CUT_WRITEBACK;
}
//======================================================================================================//

//======================================================================================================//
void twoel_ij_eq_kl(double tol2e_over_schwmax, int start, int step, int id)
{
	CUT_DECLS;
	int i, j;
	int i_off, j_off;
	double* fock = twoel_context[id].fock;

	// 4-way symmetry
	i_off = -nbfn;
	for (i = 0; i < nbfn; i++)
	{
		i_off += nbfn;
		j_off = i_off;

		DENS_DECL(i,i);

		for (j = i + 1; j < nbfn; j++)
		{
			j_off += nbfn;
			DENS_DECL_SYMM(j,i);
			DENS_DECL(j,j);
			TOL_DECL(i,j);

			if (g_schwarz[OFF(i,j)] < tol2e_over_g_schwarz_i_j)
			{
				CUT1(4);
				continue;
			}

			CUT3(4);
			double gg = G(i, j, i, j);

			UPDATE(i,j,i,j);
			UPDATE(i,j,j,i);
			UPDATE(j,i,i,j);
			UPDATE(j,i,j,i);
		} // j
	} // i

	CUT_WRITEBACK;
}
//======================================================================================================//


//======================================================================================================//
void twoel_i_eq_j_and_k_eq_l(double tol2e_over_schwmax, int start, int step, int id)
{
	CUT_DECLS;
	int i, k;
	int i_off, k_off;
	double* fock = twoel_context[id].fock;

	// 2-way symmetry
	i_off = -nbfn;
	for (i = 0; i < nbfn; i++)
	{
		i_off += nbfn;

		DENS_DECL(i,i);
		TOL_DECL(i,i);

		if (g_schwarz[OFF(i,i)] < tol2e_over_schwmax)
		{
			CUT1(2 * (nbfn - i - 1));
			continue;
		}
		k_off = i_off;

		for (k = i + 1; k < nbfn; k++)
		{
			k_off += nbfn;
			DENS_DECL_SYMM(k,i);
			DENS_DECL(k,k);

			if (g_schwarz[OFF(k,k)] < tol2e_over_g_schwarz_i_i)
			{
				CUT2(2);
				continue;
			}

			CUT3(2);
			double gg = G(i, i, k, k);

			UPDATE(i,i,k,k);
			UPDATE(k,k,i,i);
		} // k
	} // i

	CUT_WRITEBACK;
}
//======================================================================================================//


//======================================================================================================//
void twoel_i_eq_j_eq_k_eq_l(double tol2e_over_schwmax, int start, int step, int id)
{
	CUT_DECLS;
	int i;
	int i_off;
	double* fock = twoel_context[id].fock;

	// 1-way symmetry
	i_off = -nbfn;
	for (i = 0; i < nbfn; i++)
	{
		i_off += nbfn;

		DENS_DECL(i,i);
		TOL_DECL(i,i);

		if (g_schwarz[OFF(i,i)] < tol2e_over_g_schwarz_i_i)
		{
			CUT2(1);
			continue;
		}

		CUT3(1);
		double gg = G(i, i, i, i);

		UPDATE(i,i,i,i);
	} // i

	CUT_WRITEBACK;
}
//======================================================================================================//



//======================================================================================================//
//twoel_symm:
//	Executes twoel_i_eq_j, twoel_k_eq_l, and twoel_i_j_k_l_all_different for each thread.
//	Satisfy one dependency for twoel_local_reduction
//======================================================================================================//
ocrGuid_t twoel_symm ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[])
{
	//======================================================================================================//
	//gather global data
	twoel_param_t *my_twoel_params 	= (twoel_param_t*) (depv[0].ptr);

	ax					= my_twoel_params->ax;
	ay					= my_twoel_params->ay;
	az					= my_twoel_params->az;
	eigv					= my_twoel_params->eigv;
	expnt					= my_twoel_params->expnt;
	fm					= my_twoel_params->fm;
	g_dens					= my_twoel_params->g_dens;
	g_fock					= my_twoel_params->g_fock;
	g_schwarz				= my_twoel_params->g_schwarz;
	g_schwarz_max_j				= my_twoel_params->g_schwarz_max_j;
	g_orbs					= my_twoel_params->g_orbs;
	g_tfock					= my_twoel_params->g_tfock;
	g_work					= my_twoel_params->g_work;
	iky					= my_twoel_params->iky;
	nbfn					= my_twoel_params->nbfn;
	nocc					= my_twoel_params->nocc;
	q					= my_twoel_params->q;
	rnorm					= my_twoel_params->rnorm;
	threads					= my_twoel_params->threads;
	double tol2e_over_schwmax		= my_twoel_params->tol2e_over_schwmax;
	twoel_context				= my_twoel_params->twoel_context;
	t_fock					= my_twoel_params->t_fock;
	x					= my_twoel_params->x;
	y					= my_twoel_params->y;
	z					= my_twoel_params->z;
	//======================================================================================================//

	//======================================================================================================//
	//gather parameters
	int id 				= (int)paramv[0];
	int myStart 			= (int)paramv[1];
	int myStep 			= (int)paramv[2];
	ocrGuid_t finished_event 	= (ocrGuid_t)paramv[3];

	//PRINTF("twoel_symm %d.%d\n", iter, id);
	//======================================================================================================//

	//
	twoel_i_eq_j(tol2e_over_schwmax, myStart, myStep, id);
	twoel_k_eq_l(tol2e_over_schwmax, myStart, myStep, id);
	twoel_i_j_k_l_all_different(tol2e_over_schwmax, myStart, myStep, id);

	//satisfy dependency
	ocrEventSatisfy(finished_event, NULL_GUID);

	return NULL_GUID;
}

//======================================================================================================//



//======================================================================================================//
//twoel_local_reduction:
//	Reduction of the calculations performed by the twoel_symm instances.
//	Only executed when the dependencies from twoel_symm have been satisfied
//======================================================================================================//
ocrGuid_t twoel_reduction ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[])
{
	//======================================================================================================//
	//gather global data
	twoel_param_t *my_twoel_params 	= (twoel_param_t*) (depv[0].ptr);

	ax					= my_twoel_params->ax;
	ay					= my_twoel_params->ay;
	az					= my_twoel_params->az;
	eigv					= my_twoel_params->eigv;
	double *etwo				= my_twoel_params->etwo;
	expnt					= my_twoel_params->expnt;
	fm					= my_twoel_params->fm;
	g_dens					= my_twoel_params->g_dens;
	g_fock					= my_twoel_params->g_fock;
	g_schwarz				= my_twoel_params->g_schwarz;
	g_schwarz_max_j				= my_twoel_params->g_schwarz_max_j;
	g_orbs					= my_twoel_params->g_orbs;
	g_tfock					= my_twoel_params->g_tfock;
	g_work					= my_twoel_params->g_work;
	long long int *icut1			= my_twoel_params->icut1;
	long long int *icut2			= my_twoel_params->icut2;
	long long int *icut3			= my_twoel_params->icut3;
	long long int *icut4			= my_twoel_params->icut4;
	iky					= my_twoel_params->iky;
	nbfn					= my_twoel_params->nbfn;
	nocc					= my_twoel_params->nocc;
	q					= my_twoel_params->q;
	rnorm					= my_twoel_params->rnorm;
	threads					= my_twoel_params->threads;
	double tol2e_over_schwmax		= my_twoel_params->tol2e_over_schwmax;
	twoel_context				= my_twoel_params->twoel_context;
	t_fock					= my_twoel_params->t_fock;
	x					= my_twoel_params->x;
	y					= my_twoel_params->y;
	z					= my_twoel_params->z;
	//======================================================================================================//

	//======================================================================================================//
	//gather parameters
	ocrGuid_t finished_event 	= (ocrGuid_t)paramv[0];

	//PRINTF("twoel_reduction %d\n", iter);
	//======================================================================================================//

	//
	twoel_ij_eq_kl(tol2e_over_schwmax, 0, 0, 0);
	twoel_i_eq_j_and_k_eq_l(tol2e_over_schwmax, 0, 0, 0);
	twoel_i_eq_j_eq_k_eq_l(tol2e_over_schwmax, 0, 0, 0);


	//perform reduction
	size_t t, i, j;
	size_t i_off;
	size_t nbfn_squared = nbfn * nbfn;
	size_t t_off = 0;

	t_off = 0;
	for (t = 0; t < threads + 1; t++)
	{
		i_off=0;
		for (i = 0; i < nbfn; i++)
		{
			for (j = 0; j < nbfn; j++)
			{
				g_fock[i_off + j] += t_fock[t_off + i_off + j];
				t_fock[t_off + i_off + j] = 0.0;
			}
			i_off += nbfn;
		}
		t_off += nbfn_squared;
	}

	for (t = 0; t < threads + 1; t++)
	{
		*icut1 += twoel_context[t].icut1;
		*icut2 += twoel_context[t].icut2;
		*icut3 += twoel_context[t].icut3;
		*icut4 += twoel_context[t].icut4;

		twoel_context[t].icut1 = 0;
		twoel_context[t].icut2 = 0;
		twoel_context[t].icut3 = 0;
		twoel_context[t].icut4 = 0;
	}

	#ifdef USE_CACHE
	reset_cache_index();
	#endif

	//compute value to be returned to main function
	*etwo = 0.50 * contract_matrices(g_fock, g_dens);

	//satisfy dependency
	ocrEventSatisfy(finished_event, NULL_GUID);

	return NULL_GUID;
}
//======================================================================================================//

//======================================================================================================//
//twoel_complement:
//	Executes complementary functions on each iteration.
//	Ends the program or enables the twoel_symm EDTs of the next iteration
//======================================================================================================//
ocrGuid_t twoel_complement ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[])
{
	//======================================================================================================//
	//gather global data
	twoel_param_t *my_twoel_params 	= (twoel_param_t*) (depv[0].ptr);

	//first, take the time for twoel operation = symm + complement
	double *ttwoel					= my_twoel_params->ttwoel;
	*ttwoel = *ttwoel + timer();

	ax					= my_twoel_params->ax;
	ay					= my_twoel_params->ay;
	az					= my_twoel_params->az;
	eigv					= my_twoel_params->eigv;
	double *energy				= my_twoel_params->energy;
	double *enrep				= my_twoel_params->enrep;
	double *eone				= my_twoel_params->eone;
	double *etwo				= my_twoel_params->etwo;
	expnt					= my_twoel_params->expnt;
	fm					= my_twoel_params->fm;
	g_dens					= my_twoel_params->g_dens;
	g_fock					= my_twoel_params->g_fock;
	g_schwarz				= my_twoel_params->g_schwarz;
	g_schwarz_max_j				= my_twoel_params->g_schwarz_max_j;
	g_orbs					= my_twoel_params->g_orbs;
	g_tfock					= my_twoel_params->g_tfock;
	g_work					= my_twoel_params->g_work;
	iky					= my_twoel_params->iky;
	long long int *icut1			= my_twoel_params->icut1;
	long long int *icut2			= my_twoel_params->icut2;
	long long int *icut3			= my_twoel_params->icut3;
	long long int *icut4			= my_twoel_params->icut4;
	double *i_timer				= my_twoel_params->i_timer;
	nbfn					= my_twoel_params->nbfn;
	nocc					= my_twoel_params->nocc;
	q					= my_twoel_params->q;
	rnorm					= my_twoel_params->rnorm;
	double *tdens				= my_twoel_params->tdens;
	double *tdgemm				= my_twoel_params->tdgemm;
	double *tdiag				= my_twoel_params->tdiag;
	double *tinit				= my_twoel_params->tinit;
	threads					= my_twoel_params->threads;
	double *tonel				= my_twoel_params->tonel;
	double *tprint				= my_twoel_params->tprint;
	twoel_context				= my_twoel_params->twoel_context;
	t_fock					= my_twoel_params->t_fock;
	x					= my_twoel_params->x;
	y					= my_twoel_params->y;
	z					= my_twoel_params->z;
	//======================================================================================================//

	//======================================================================================================//
	//gather parameters
	long iter 			= (long)paramv[0];

	//PRINTF("twoel_complement %d\n", iter);
	//======================================================================================================//

	//======================================================================================================//
	// diagonalize the fock matrix.
	double tester = diagon(iter);
	*tdiag  = *tdiag + timer();

	// make the new density matrix in g_work from orbitals in g_orbs, compute the norm of the change in
	// the density matrix and then update the density matrix in g_dens with damping.;
	makden();
	double deltad = dendif();

	double scale;
	if (iter == 0)
		scale = 0.00;
	else if (iter < 5)
		scale = (nbfn > 60) ? 0.50 : 0.00;
	else
		scale = 0.00;

	damp(scale);
	*tdens = *tdens + timer();

	*i_timer = timer2() - *i_timer;

	// add up energy and print out convergence information;
	*energy = *enrep + *eone + *etwo;
	//PRINTF(" energy=%15.8f, enrep= %9.7f, eone=%9.7f, etwo=%10.4f\n", *energy, *enrep, *eone, *etwo);
	PRINTF(" iter= %3d, energy=%15.8f, deltad= %9.7f, deltaf=%9.7f, time=%10.4f\n", (int)iter, *energy, deltad, tester, *i_timer);
	*tprint = *tprint + timer();

	// if converged exit iteration loop;
	double mybreak = 0;
	//if (deltad < tol) break;
	if (deltad < tol) mybreak = 1;
	#ifdef BOOKKEEPING
	// something has gone wrong--print what you know and quit;
	if (*icut3 == 0)
	{
		PRINTF("no two-electron integrals computed!\n");
		PRINTF("SCF failed to converge in %d iters\n", (int)iter);
		//break;
		mybreak = 1;
	}
	#endif
	//======================================================================================================//

	//======================================================================================================//
	//no more iterations to be performed
	if(iter == mxiter-1 || mybreak == 1)
	{
		if (iter == mxiter-1)
		{
			PRINTF("SCF failed to converge in %d iters\n", (int)iter);
			iter--;
		}

		//...v....1....v....2....v....3....v....4....v....5....v....6....v....7..;
		//
		//     finished ... print out eigenvalues and occupied orbitals;
		//

		// print out timing information;

		PRINTF("\n\nfinal energy = %18.11f\n", *energy);
		PRINTF("\neigenvalues\n\n");
		output(eigv, 0, MIN(nbfn, nocc + 5), 0, 1, nbfn, 1, 1);

		PRINTF("      init       onel      twoel       diag       dens       print       eigen    \n");
		PRINTF("      ----       ----      -----       ----       ----       ----        ----    \n");

		PRINTF("%10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f", *tinit, *tonel, *ttwoel, *tdiag, *tdens, *tprint, *tdgemm);

		//printf("twoel = %10.2f", ttwoel);

		double totsec = *tinit + *tonel + *ttwoel + *tdiag + *tdens + *tprint;
		PRINTF("\n elapsed time in seconds %10.2f\n\n", totsec);

		#ifdef USE_CACHE
		PRINTF("Total number of cache values:%lu\n", cache_size());
		PRINTF("Total cache capacity:%lu\n\n", cache_capacity());
		#endif

		// print out information on # integrals evaluated each iteration;
		long long int nints = *icut1 + *icut2 + *icut3 + *icut4;
		//PRINTF("nints=%d icut1=%d icut2=%d icut3=%d icut4=%d\n\n", (int)nints ,(int)*icut1, (int)*icut2, (int)*icut3, (int)*icut4);
		double frac = (double)(*icut3) / (double)nints;

		PRINTF("No. of integrals screened or computed (all iters) \n\n");
		PRINTF("-------------------------------------\n");
		PRINTF("  failed #ij test   failed #j test  failed #kl test    #compute           #total       fraction\n");
		PRINTF("  ---------------  ---------------  ---------------  ---------------  ---------------  --------\n");
		PRINTF("  %15lld  %15lld  %15lld  %15lld  %15lld  %9.6f\n\n", *icut1, *icut4, *icut2, *icut3, nints, frac);

		long long int maxint = nbfn;
		maxint = pow(maxint, 4) * (iter + 1);

		if (nints != maxint)
		{
			PRINTF("Inconsistent number of integrals, should be %lld\n", maxint);
			PRINTF("Note: largest 32-bit int is 2,147,483,647\n");
		}

		//release DBs
		closearrays();

		//shutdown the runtime if this is the last iteration
		ocrShutdown();
	}
	//======================================================================================================//

	//======================================================================================================//
	//perform the next iteration
	else
	{
		*i_timer = timer2();

		// make the one particle contribution to the fock matrix and get the partial contribution to the energy;
		memcpy(g_fock, g_fock_const, nbfn * nbfn * sizeof(double));
		*eone = 0.50 * contract_matrices(g_fock, g_dens);
		*tonel = *tonel + timer();

		g_i=0;
		g_ij=0;
		g_kl=0;

		//create EDTs for the next iteration
		create_new_iteration();
	}
	//======================================================================================================//

	return NULL_GUID;
}

//======================================================================================================//

//======================================================================================================//
void twoel_ocr()
{
	g_i=0;
	g_ij=0;
	g_kl=0;

	int i;
	iter=0;

	double tol2e_over_schwmax = tol2e / schwmax;
	//======================================================================================================//
	//guids for symm edts
	ocrGuid_t symm_template;
	ocrEdtTemplateCreate(&symm_template, twoel_symm, EDT_PARAM_UNK, EDT_PARAM_UNK);

	ocrGuid_t symm_affinity=NULL_GUID;
	//======================================================================================================//

	//======================================================================================================//
	//guids for reduction edts
	ocrGuid_t redu_template;
	ocrEdtTemplateCreate(&redu_template, twoel_reduction, EDT_PARAM_UNK, EDT_PARAM_UNK);

	ocrGuid_t redu_affinity=NULL_GUID;
	//======================================================================================================//

	//======================================================================================================//
	//guids for complement edts
	ocrGuid_t comp_template;
	ocrEdtTemplateCreate(&comp_template, twoel_complement, EDT_PARAM_UNK, EDT_PARAM_UNK);

	ocrGuid_t comp_affinity=NULL_GUID;
	//======================================================================================================//

	//======================================================================================================//
	//context for each thread
	size_t size = (threads + 1) * nbfn * nbfn * sizeof(double);

	PTR_T t_fock_ptr;
 	ocrGuid_t t_fock_db;
	ocrGuid_t t_fock_affinity=NULL_GUID;
  	DBCREATE( &t_fock_db, &t_fock_ptr, sizeof(double)*size, FLAGS, t_fock_affinity, NO_ALLOC);
	if(t_fock_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for t_fock, using malloc instead !!!!!!\n\n");
		t_fock = (double*)malloc(sizeof(double)*size);
	}
	else
	{
		t_fock = (double*)t_fock_ptr;
	}

	for (i=0; i<size; i++)
	{
		t_fock[i] = 0.0;
	}

	PTR_T twoel_context_ptr;
 	ocrGuid_t twoel_context_db;
	ocrGuid_t twoel_context_affinity=NULL_GUID;
	DBCREATE( &twoel_context_db, &twoel_context_ptr, (threads + 1) * sizeof(twoel_context_t)*size, FLAGS, twoel_context_affinity, NO_ALLOC);

	if(twoel_context_ptr==NULL)
	{
		PRINTF("\n!!!!!! DBCREATE failed for twoel_context, using malloc instead !!!!!!\n\n");
		twoel_context = (twoel_context_t*)malloc((threads + 1) * sizeof(twoel_context_t)*size);
	}
	else
	{
		twoel_context = (twoel_context_t*)twoel_context_ptr;
	}


	for (i = 0; i < (threads + 1); i++)
	{
		twoel_context[i].fock = t_fock + (i * nbfn * nbfn);
	}
	//======================================================================================================//

	//======================================================================================================//
	//create db for passing arguments to the twoel function. This will be passed to the function as the
	//db of the dependency on slot 0.
	my_twoel_params->ax		 	= ax;
	my_twoel_params->ay		 	= ay;
	my_twoel_params->az		 	= az;
	my_twoel_params->eigv 			= eigv;
	my_twoel_params->energy 		= &energy;
	my_twoel_params->enrep	 		= &enrep;
	my_twoel_params->eone 			= &eone;
	my_twoel_params->etwo 			= &etwo;
	my_twoel_params->expnt 			= expnt;
	my_twoel_params->fm 			= fm;
	my_twoel_params->g_dens 		= g_dens;
	my_twoel_params->g_fock 		= g_fock;
	my_twoel_params->g_schwarz 		= g_schwarz;
	my_twoel_params->g_schwarz_max_j 	= g_schwarz_max_j;
	my_twoel_params->g_orbs 		= g_orbs;
	my_twoel_params->g_tfock 		= g_tfock;
	my_twoel_params->g_work 		= g_work;
	my_twoel_params->iky		 	= iky;
	my_twoel_params->icut1		 	= &icut1;
	my_twoel_params->icut2		 	= &icut2;
	my_twoel_params->icut3		 	= &icut3;
	my_twoel_params->icut4		 	= &icut4;
	my_twoel_params->i_timer	 	= &i_timer;
	my_twoel_params->nbfn 			= nbfn;
	my_twoel_params->nocc 			= nocc;
	my_twoel_params->q		 	= q;
	my_twoel_params->rnorm		 	= rnorm;
	my_twoel_params->tdens			= &tdens;
	my_twoel_params->tdgemm			= &tdgemm;
	my_twoel_params->tdiag			= &tdiag;
	my_twoel_params->tinit			= &tinit;
	my_twoel_params->threads		= threads;
	my_twoel_params->tonel 			= &tonel;
	my_twoel_params->tprint			= &tprint;
	my_twoel_params->ttwoel			= &ttwoel;
	my_twoel_params->tol2e_over_schwmax 	= tol2e_over_schwmax;
	my_twoel_params->twoel_context 		= twoel_context;
	my_twoel_params->t_fock 		= t_fock;
	my_twoel_params->x		 	= x;
	my_twoel_params->y		 	= y;
	my_twoel_params->z		 	= z;
	//======================================================================================================//

	//======================================================================================================//
	//parameters to be passed to twoel_symm
	u64 symm_params[4];

	//parameters to be passed to twoel_reduction
	u64 redu_params;

	//parameters to be passed to twoel_complement
	u64 comp_params;
	//======================================================================================================//

	//======================================================================================================//
	//perform operations previous to the first set of twoel_symm

	i_timer = timer2();

	// make the one particle contribution to the fock matrix and get the partial contribution to the energy;
	memcpy(g_fock, g_fock_const, nbfn * nbfn * sizeof(double));
	eone = 0.50 * contract_matrices(g_fock, g_dens);
	tonel = tonel + timer();
	//======================================================================================================//

	//======================================================================================================//
	//Create EDTs and associate the dependencies for the first iteration
	//======================================================================================================//
	//compute limits of computation for each thread for static scheduling
	int nbfnDivthreads = nbfn / threads;
	int nbfnModthreads = nbfn % threads;
	int lastStep;
	if( nbfnModthreads == 0)
	{
		lastStep = nbfnDivthreads;
	}
	else
	{
		lastStep = nbfnDivthreads + nbfnModthreads;
	}
	int start=0, step=nbfnDivthreads;
	//======================================================================================================//

	//======================================================================================================//
	//the first set of twoel_symm dont depend on a previous towel_reduction, so they can start as soon as created
	for(i=0; i<threads; i++)
	{
		//id for this EDT
		symm_params[0]=(u64)i;

		//limits of computation for this EDT
		symm_params[1]=start;
		if(i<threads-1)
		{
			symm_params[2]=step;
		}
		else
		{
			symm_params[2]=lastStep;
		}
		start += step;

		//pointer to the event that will satisfy the dependency for the reduction function
		ocrGuid_t temp_guid;
		ocrEventCreate(&temp_guid, OCR_EVENT_STICKY_T, TRUE);
		symm_params[3]=GUIDTOU64(temp_guid);
		*(symm_finished+iter*threads+i) = temp_guid;

		ocrEdtCreate(	symm_guid+iter*threads+i,
				symm_template,
				4, (u64*)(symm_params),
				1, NULL, 0,
				symm_affinity, NULL);


		//Since twoel_params_guid has already an associated db, this dependency is satisfied immediately.
		ocrAddDependence(my_twoel_params_db, *(symm_guid+iter*threads+i), 0, DB_MODE_ITW);
	}
	//======================================================================================================//


	//======================================================================================================//
	//create edt for twoel_reduction with THREADS+1 number of dependencies

	//pointer to the event that will satisfy the dependency for the reduction function
	ocrEventCreate(reduction_finished+iter, OCR_EVENT_STICKY_T, TRUE);
	redu_params=GUIDTOU64(reduction_finished[iter]);

	//create EDT
	ocrEdtCreate(	redu_guid+iter,
			redu_template,
			1, (u64*)&redu_params,
			threads+1, NULL, 0,
			redu_affinity, NULL);

	//Since twoel_params_guid has already an associated db, this dependency is satisfied immediately.
	ocrAddDependence(my_twoel_params_db, redu_guid[iter], 0, DB_MODE_ITW);

	//associate the dependencies between the instances of twoel_symm (source) and twoel_reduction (destination)
	for(i=0; i<threads; i++)
	{
		//associate dependencies with events
		ocrAddDependence(*(symm_finished+iter*threads+i), redu_guid[iter], i+1, DB_MODE_ITW);
	}
	//======================================================================================================//


	//======================================================================================================//
	//create edt for twoel_comp with 2 dependencies: the twoel_reduction of the same iteration and the global data

	//id for this instance
	comp_params=(u64)iter;

	//create EDT
	ocrEdtCreate(	comp_guid+iter,
			comp_template,
			1, (u64*)(&comp_params),
			2, NULL, 0,
			comp_affinity, NULL);

	//Since twoel_params_guid has already an associated db, this dependency is satisfied immediately.
	ocrAddDependence(my_twoel_params_db, comp_guid[iter], 0, DB_MODE_ITW);

	//associate the dependencies between the instances of twoel_symm (source) and twoel_reduction (destination)
	ocrAddDependence(reduction_finished[iter], comp_guid[iter], 1, DB_MODE_ITW);
	//======================================================================================================//
}
//======================================================================================================//

//======================================================================================================//
void create_new_iteration()
{
	int i;
	iter++;

	//======================================================================================================//
	//guids for symm edts
	ocrGuid_t symm_template;
	ocrEdtTemplateCreate(&symm_template, twoel_symm, EDT_PARAM_UNK, EDT_PARAM_UNK);

	ocrGuid_t symm_affinity=NULL_GUID;
	//======================================================================================================//

	//======================================================================================================//
	//guids for reduction edts
	ocrGuid_t redu_template;
	ocrEdtTemplateCreate(&redu_template, twoel_reduction, EDT_PARAM_UNK, EDT_PARAM_UNK);

	ocrGuid_t redu_affinity=NULL_GUID;
	//======================================================================================================//

	//======================================================================================================//
	//guids for complement edts
	ocrGuid_t comp_template;
	ocrEdtTemplateCreate(&comp_template, twoel_complement, EDT_PARAM_UNK, EDT_PARAM_UNK);

	ocrGuid_t comp_affinity=NULL_GUID;
	//======================================================================================================//

	//======================================================================================================//
	//parameters to be passed to twoel_symm
	u64 symm_params[4];

	//parameters to be passed to twoel_reduction
	u64 redu_params;

	//parameters to be passed to twoel_complement
	u64 comp_params;
	//======================================================================================================//

	//======================================================================================================//
	i_timer = timer2();
	//======================================================================================================//


	//======================================================================================================//
	//Create EDTs and associate the dependencies for the current iteration

	//======================================================================================================//
	//compute limits of computation for each thread for static scheduling
	int nbfnDivthreads = nbfn / threads;
	int nbfnModthreads = nbfn % threads;
	int lastStep;
	if( nbfnModthreads == 0)
	{
		lastStep = nbfnDivthreads;
	}
	else
	{
		lastStep = nbfnDivthreads + nbfnModthreads;
	}
	int start=0, step=nbfnDivthreads;
	//======================================================================================================//

	//======================================================================================================//
	for(i=0; i<threads; i++)
	{
		//id for this EDT
		symm_params[0]=(u64)i;

		//limits of computation for this EDT
		symm_params[1]=start;
		if(i<threads-1)
		{
			symm_params[2]=step;
		}
		else
		{
			symm_params[2]=lastStep;
		}
		start += step;

		//pointer to the event that will satisfy the dependency for the reduction function
		ocrGuid_t temp_guid;
		ocrEventCreate(&temp_guid, OCR_EVENT_STICKY_T, TRUE);
		symm_params[3]=GUIDTOU64(temp_guid);
		*(symm_finished+iter*threads+i) = temp_guid;

		//create EDT
		ocrEdtCreate(	symm_guid+iter*threads+i,
				symm_template,
				4, (u64*)(symm_params),
				1, NULL, 0,
				symm_affinity, NULL);


		//Since twoel_params_guid has already an associated db, this dependency is satisfied immediately.
		ocrAddDependence(my_twoel_params_db, *(symm_guid+iter*threads+i), 0, DB_MODE_ITW);
	}
	//======================================================================================================//

	//======================================================================================================//
	//create edt for twoel_reduction with THREADS+1 number of dependencies

	//pointer to the event that will satisfy the dependency for the complement function
	ocrEventCreate(reduction_finished+iter, OCR_EVENT_STICKY_T, TRUE);
	redu_params=GUIDTOU64(reduction_finished[iter]);

	//create EDT
	ocrEdtCreate(	redu_guid+iter,
			redu_template,
			1, (u64*)&redu_params,
			threads+1, NULL, 0,
			redu_affinity, NULL);

	//Since twoel_params_guid has already an associated db, this dependency is satisfied immediately.
	ocrAddDependence(my_twoel_params_db, redu_guid[iter], 0, DB_MODE_ITW);

	//associate the dependencies between the instances of twoel_symm (source) and twoel_reduction (destination)
	for(i=0; i<threads; i++)
	{
		//associate dependencies with events
		ocrAddDependence(*(symm_finished+iter*threads+i), redu_guid[iter], i+1, DB_MODE_ITW);
	}
	//======================================================================================================//

	//======================================================================================================//
	//create edt for twoel_comp with 2 dependencies: the twoel_reduction of the same iteration and the global data

	//id for this instance
	comp_params=(u64)iter;

	//create EDT
	ocrEdtCreate(	comp_guid+iter,
			comp_template,
			1, (u64*)(&comp_params),
			2, NULL, 0,
			comp_affinity, NULL);

	//Since twoel_params_guid has already an associated db, this dependency is satisfied immediately.
	ocrAddDependence(my_twoel_params_db, comp_guid[iter], 0, DB_MODE_ITW);

	//associate the dependencies between the instances of twoel_symm (source) and twoel_reduction (destination)
	ocrAddDependence(reduction_finished[iter], comp_guid[iter], 1, DB_MODE_ITW);
	//======================================================================================================//
}
//======================================================================================================//
