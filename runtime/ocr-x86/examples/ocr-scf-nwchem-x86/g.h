/*
 * g.h
 *
 *  Created on: Jan 22, 2013
 *      Author: adam
 */

#ifndef G_H_
#define G_H_

#include "cscc.h"
#include "integ.h"
#include <stdlib.h>
#include <math.h>

typedef struct {
  double facr2;
  double expnt;
  double x;
  double y;
  double z;
  double rnorm;
} precalc;

extern precalc * g_precalc;
extern double* g_cache;
extern size_t g_cache_index;
extern size_t g_cache_size;
extern size_t g_cached_values;
//extern double fm[2001][5];
extern double *fm;
extern double rdelta, delta, delo2;

// computes f0 to a relative accuracy of better than 4.e-13 for all t. Uses 4th order taylor
// expansion on grid out to t=28.0 asymptotic expansion accurate for t greater than 28
static inline double  f0(double t) {
  if (t >= 28.0) { return 0.88622692545276 / sqrt(t);}

  int    n    = (int) ((t + delo2) * rdelta);
  double x    = delta * (double) n - t;
  //double *fmn = fm[n];
  double *fmn = fm+n*5;

  return fmn[0] + x * (fmn[1] + x * 0.5  * (fmn[2] + x * (1./3.) * (fmn[3] + x * 0.25 * fmn[4])));
}

// Compute the two electon integral (ij|kl) over normalized; primitive 1s gaussians;
// This variant uses precomputed lookup tables to retrieve the values which only depend
// on i,j or k,l to reduce computations and memory accesses. In addition, the i,j and k,l
// values are passed in as a precomputed index into a matrix.
static inline double g_fast(int ij_ind, int kl_ind) {
  double f0val;
  precalc* preij = &g_precalc[ij_ind];
  precalc* prekl = &g_precalc[kl_ind];

  double expntIJ = preij->expnt;
  double expntKL = prekl->expnt;

  double exijkl = preij->facr2 * prekl->facr2;
  double denom  = expntIJ * expntKL * sqrt(expntIJ + expntKL);
  double fac    = (expntIJ) * (expntKL) / (expntIJ + expntKL);

  double xp   = preij->x;
  double yp   = preij->y;
  double zp   = preij->z;
  double xq   = prekl->x;
  double yq   = prekl->y;
  double zq   = prekl->z;
  double rpq2 = (xp - xq) * (xp - xq) + (yp - yq) * (yp - yq) + (zp - zq) * (zp - zq);

  f0val = f0(fac * rpq2);
  return (two_pi_pow_2_5 / denom) * exijkl * f0val * preij->rnorm * prekl->rnorm;
}

// Compute the two electon integral (ij|kl) over normalized; primitive 1s gaussians;
// This is kept mostly for reference, in order to actually remove it, makesz() must
// be modified to call g_fast instead
static inline double g(int i, int j, int k, int l) {
  double f0val;
  double dxij = x[i] - x[j];
  double dyij = y[i] - y[j];
  double dzij = z[i] - z[j];
  double dxkl = x[k] - x[l];
  double dykl = y[k] - y[l];
  double dzkl = z[k] - z[l];

  double rab2 = dxij * dxij + dyij * dyij + dzij * dzij;
  double rcd2 = dxkl * dxkl + dykl * dykl + dzkl * dzkl;

  double expntIJ = expnt[i] + expnt[j];
  double expntKL = expnt[k] + expnt[l];
  double oneOverExpntIJ = 1.0/expntIJ;
  double oneOverExpntKL = 1.0/expntKL;

  double facij  = expnt[i] * expnt[j] * oneOverExpntIJ;
  double fackl  = expnt[k] * expnt[l] * oneOverExpntKL;
  double exijkl = exprjh(-facij * rab2 - fackl * rcd2);
  double denom  = expntIJ * expntKL * sqrt(expntIJ + expntKL);
  double fac    = (expntIJ) * (expntKL) / (expntIJ + expntKL);

  double xp   = (x[i] * expnt[i] + x[j] * expnt[j]) * oneOverExpntIJ;
  double yp   = (y[i] * expnt[i] + y[j] * expnt[j]) * oneOverExpntIJ;
  double zp   = (z[i] * expnt[i] + z[j] * expnt[j]) * oneOverExpntIJ;
  double xq   = (x[k] * expnt[k] + x[l] * expnt[l]) * oneOverExpntKL;
  double yq   = (y[k] * expnt[k] + y[l] * expnt[l]) * oneOverExpntKL;
  double zq   = (z[k] * expnt[k] + z[l] * expnt[l]) * oneOverExpntKL;
  double rpq2 = (xp - xq) * (xp - xq) + (yp - yq) * (yp - yq) + (zp - zq) * (zp - zq);

  f0val = f0(fac * rpq2);
  return (two_pi_pow_2_5 / denom) * exijkl * f0val * rnorm[i] * rnorm[j] * rnorm[k] * rnorm[l];
}

// macros to give gcc hints to help optimize the branches required
//when caching results from g()
#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif


/* The caching system implemented here relies on the fact that the g(i,j,k,l)
 * is called in the same order each iteration. This allows us to simply store
 * the return values in a list without requiring any special data structures
 * to get the correct result from the arguments. This will not work for
 * parallel computations, but it may be possible to do something similar
 * where each unit of work has a separate linearly ordered cache.
 * (Unless nodes can work-steal from other nodes)
 */

// This caches the values from the g() variant which uses a lookup table
static inline double g_cached_fast(int ij_ind, int kl_ind) {
  if (unlikely(g_cache_index >= g_cache_size)) {
    if (g_cache_size == 0) g_cache_size = 8 * 1024 * 1024;
    g_cache_size <<= 1;
    g_cache = realloc(g_cache, g_cache_size * sizeof(double));
  }

  if (unlikely(g_cache_index >= g_cached_values)) {
    double ret = g_fast(ij_ind, kl_ind);
    g_cache[g_cache_index++] = ret;
    g_cached_values++;
    return ret;
  }

  return g_cache[g_cache_index++];
}

// This caches the values from the standard g() function
static inline double g_cached(int i, int j, int k, int l) {
  if (unlikely(g_cache_index >= g_cache_size)) {
    if (g_cache_size == 0) g_cache_size = 1024 * 1024;
    g_cache_size <<= 1;
    g_cache = realloc(g_cache, g_cache_size * sizeof(double));
  }

  if (unlikely(g_cache_index >= g_cached_values)) {
    double ret = g(i, j, k, l);
    g_cache[g_cache_index++] = ret;
    g_cached_values++;
    return ret;
  }

  return g_cache[g_cache_index++];
}

// must be called after each iteration to reset the order of
// returned values back to the beginning
static inline void reset_cache_index(void) {
  g_cache_index = 0;
}

// get some info about the cache's size
static inline size_t cache_size(void) {return g_cached_values; }
static inline size_t cache_capacity(void) { return g_cache_size; }


#endif /* G_H_ */
