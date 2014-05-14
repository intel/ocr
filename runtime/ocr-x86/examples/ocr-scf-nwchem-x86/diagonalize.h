#ifndef DIAGONALIZE_H
#define DIAGONALIZE_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>


#ifdef USE_MKL
#include <mkl_lapacke.h>
#elif defined USE_LAPACK
#include <lapacke.h>
#endif

#include "cscc.h"

double my_pythag(double a, double b);
int my_ga_rrreduc(int nm, int n, double *a, double *b, double *fv2);
void my_ga_tred2(int nm, int n, double *a, double *d, double *e, double *z);
int my_ga_tql2(int nm, int n, double *d, double *e, double *z);
void my_ga_rrrebak(int nm, int n, double *b, double *dl, int m, double *z);
int my_rsg(int nm, int n, double *a, double *b, double *w, double *z, double *fv1, double *fv2);
int my_rs(int nm, int n, double *a, double *w, double *z, double *fv1, double *fv2);
void  Eigen_gen(double * g_a, double * g_s, double * g_v, double * evals);
void  Eigen_std(double * g_a, double * g_v, double * evals);

#endif
