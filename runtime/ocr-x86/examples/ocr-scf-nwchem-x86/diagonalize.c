#include "diagonalize.h"

// Finds dsqrt(a**2+b**2) without overflow or destructive underflow
double my_pythag(double a, double b) {
  double p, r, t, s, u;

  p = MAX(fabs(a), fabs(b));
  if (p ==  0.0) return p;

  r = MIN(fabs(a), fabs(b)) / p;
  r = r * r;

  while (r > 0.0) {
     s = r / (4.0 + r);
     u = 1.0 + 2.0 * s;
     p = u * p;
     t = s / u;
     r = t * t * r;
  }

  return p;
}


int my_ga_rrreduc(int nm, int n, double *a, double *b, double *fv2) {

//     this subroutine is a translation of the algol procedure rrreduc1,
//     num. math. 11, 99-110(1968) by martin and wilkinson.
//     handbook for auto. comp., vol.ii-linear algebra, 303-314(1971).
//
//     this subroutine rrreduces the generalized symmetric eigenproblem
//     ax=(lambda)bx, where b is positive definite, to the standard
//     symmetric eigenproblem using the cholesky factorization of b.
//
//     on input
//
//        nm  must be set to the row dimension of two-dimensional arrays
//        n   is the order of the matrices a and b
//        a and b contain the real symmetric input matrices.
//
//     on output
//
//        a contains in its full lower triangle the full lower triangle
//          of the symmetric matrix derived from the rrreduction to the
//          standard form.  the strict upper triangle of a is unaltered.
//
//        b contains in its strict lower triangle the strict lower
//          triangle of its cholesky factor l.  the full upper
//          triangle of b is unaltered.
//
//        fv2 contains the diagonal elements of l.
//
//        ierr is set to
//          zero       for normal return,
//          7*n+1      if b is not positive definite.

  int i, j, k;
  double x, y=0;

//     .......... form l in the arrays b and dl ..........
  for (i = 0; i < n; i++) {
  for (j = i; j < n; j++) {

      x = b[OFFSET(i,j)];
      for (k = 0; k < i; k++) x = x - b[OFFSET(i,k)] * b[OFFSET(j,k)];

      if (j == i) {
         if (x <= 0.0) return 7 * n + 1;
         else { y = sqrt(x); fv2[i] = y; }
      } else b[OFFSET(j,i)] = x / y;

  } }

//     .......... form the transpose of the upper triangle of inv(l)*a
//                in the lower triangle of the array a ..........
  for (i = 0; i < n; i++) {
  for (j = i; j < n; j++) {

    double x = a[OFFSET(i,j)];
    for (k = 0; k < i; k++) x = x - b[OFFSET(i,k)] * a[OFFSET(j,k)];

    a[OFFSET(j,i)] = x / fv2[i];
  } }

//     .......... pre-multiply by inv(l) and overwrite ..........
  for (j = 0; j < n; j ++) {
  for (i = j; i < n; i ++) {
    double x = a[OFFSET(i,j)];

    if (i != j)
       for (k = j; k < i; k ++) x = x - a[OFFSET(k,j)] * b[OFFSET(i,k)];

    if (j > 0)
       for (k = 0; k < j; k ++) x = x - a[OFFSET(j,k)] * b[OFFSET(i,k)];

    a[OFFSET(i,j)] = x / fv2[i];
  } }

  return 0;
}


void my_ga_tred2(int nm, int n, double *a, double *d, double *e, double *z) {

//     this subroutine is a translation of the algol procedure tred2,
//     num. math. 11, 181-195(1968) by martin, reinsch, and wilkinson.
//     handbook for auto. comp., vol.ii-linear algebra, 212-226(1971).
//
//     this subroutine reduces a real symmetric matrix to a symmetric
//     tridiagonal matrix using and accumulating orthogonal similarity
//     transformations.
//
//     on input
//
//        nm  must be set to the row dimension of two-dimensional arrays.
//        n   is the order of the matrix.
//        a   contains the real symmetric input matrix.
//
//     on output
//
//        d contains the diagonal elements of the tridiagonal matrix.
//        e contains the subdiagonal elements of the tridiagonal matrix in its
//             last n - 1 positions.  e(0) is set to zero.
//        z contains the orthogonal transformation matrix produced in the reduction.
//        a and z may coincide.  if distinct, a is unaltered.

  int i, j, k;
  double f, g, h, hh, scale;

  for (i = 0; i < n; i++) {
      for (j = i; j < n; j++) z[OFFSET(j,i)] = a[OFFSET(j,i)];
      for (j = 0; j < i; j++) z[OFFSET(j,i)] = 0.0;
      d[i] = a[OFFSET(n-1,i)];
  }

//     .......... for i = n - 1 step - 1 until 1 do -- ..........
  for (i = n - 1; i >= 1; i--) {
      h     = 0.0;
      scale = 0.0;

//     .......... scale row (algol tol then not needed) ..........
      if (i != 1) for (k = 0; k < i; k++) scale = scale + fabs(d[k]);

      if (scale == 0.0) {
         e[i] = d[i - 1];
         for (j = 0; j < i; j++) { d[j] = z[OFFSET(i-1,j)]; z[OFFSET(i,j)] = 0.0; z[OFFSET(j,i)] = 0.0; }

      } else {

         for (k = 0; k < i; k++) {
             d[k] = d[k] / scale;
             h = h + d[k] * d[k];
         }

         f        = d[i - 1];
         g        = - copysign(sqrt(h), f);
         e[i]     = scale * g;
         h        = h - f * g;
         d[i - 1] = f - g;

//     .......... form a*u ..........
         for (j = 0; j < i; j++) e[j] = 0.0;

         for (j = 0; j < i; j++) {
             f       = d[j];
             z[OFFSET(j,i)] = f;
             g       = e[j] + z[OFFSET(j,j)] * f;

             for (k = j + 1; k < i; k++) {
                 g    = g + z[OFFSET(k,j)] * d[k];
                 e[k] = e[k] + z[OFFSET(k,j)] * f;
             }

             e[j] = g;
         }

//     .......... form p ..........
         f = 0.0;
         for (j = 0; j < i; j++) { e[j] = e[j] / h; f = f + e[j] * d[j]; }

//     .......... form q ..........
         hh = f / (h + h);
         for (j = 0; j < i; j++) e[j] = e[j] - hh * d[j];

//     .......... form rrreduced a ..........
         for (j = 0; j < i; j++) {
             f = d[j];
             g = e[j];

             for (k = j; k < i; k++) z[OFFSET(k,j)] = z[OFFSET(k,j)] - f * e[k] - g * d[k];

             d[j]    = z[OFFSET(i-1,j)];
             z[OFFSET(i,j)] = 0.0;
      }  }

       d[i] = h;
   }

//     .......... accumulation of transformation matrices ..........
   for (i = 1; i < n; i++) {
       z[OFFSET(n-1,i-1)] = z[OFFSET(i-1,i-1)];
       z[OFFSET(i-1,i-1)] = 1.0;
       h               = d[i];

       if (h != 0.0) {

          for (k = 0; k < i; k++) d[k] = z[OFFSET(k,i)] / h;

          for (j = 0; j < i; j++) {
             g = 0.0;
             for (k = 0; k < i; k++) g       = g + z[OFFSET(k,i)] * z[OFFSET(k,j)];
             for (k = 0; k < i; k++) z[OFFSET(k,j)] = z[OFFSET(k,j)] - g * d[k];
       }
       }

       for (k = 0; k < i; k++) z[OFFSET(k,i)] = 0.0;
   }

  for (i = 0; i < n; i++) {
    d[i] = z[OFFSET(n-1,i)];
    z[OFFSET(n-1,i)] = 0.0;
  }

  z[OFFSET(n-1,n-1)] = 1.0;
  e[0]            = 0.0;
}


int my_ga_tql2(int nm, int n, double *d, double *e, double *z) {
//
//     this subroutine is a translation of the algol procedure tql2,
//     num. math. 11, 293-306(1968) by bowdler, martin, reinsch, and wilkinson.
//     handbook for auto. comp., vol.ii-linear algebra, 227-240(1971).
//
//     this subroutine finds the eigenvalues and eigenvectors of a symmetric tridiagonal
//     matrix by the ql method. The eigenvectors of a full symmetric matrix can also be
//     found if  tred2  has been used to rrreduce this full matrix to tridiagonal form.
//
//     on input
//
//        nm must be set to the row dimension of two-dimensional arrays
//        n  is the order of the matrix.
//        d  contains the diagonal elements of the input matrix.
//        e  contains the subdiagonal elements of the input matrix
//                 in its last n-1 positions.  e(0) is arbitrary.
//        z  contains the transformation matrix produced in the reduction by tred2,
//                 if performed. If the eigenvectors of the tridiagonal matrix are
//                 desired, z must contain the identity matrix.
//
//      on output
//
//        d  contains the eigenvalues in ascending order. If an error exit is made, the
//                 eigenvalues are correct but unordered for indices 1,2,...,ierr-1.
//        e  has been destroyed.
//        z  contains orthonormal eigenvectors of the symmetric tridiagonal (or full) matrix.
//                 If an error exit is made, z contains the eigenvectors associated with the
//                 stored eigenvalues.
//        ierr is set to
//                 zero       for normal return,
//                 j          if the j-th eigenvalue has not been determined after 30 iterations.

  int i, ii, j, k, l, l1, l2, m;
  double c, c2, c3=0, dl1, el1, f, g, h, p, r, s, s2=0, tst1, tst2;

  if (n == 1) return 0;
  for (i = 1; i < n; i++) e[i - 1] = e[i];

  f        = 0.0;
  tst1     = 0.0;
  e[n - 1] = 0.0;

  for (l = 0; l < n; l ++) {
      h = fabs(d[l]) + fabs(e[l]);
      if (tst1 < h) tst1 = h;

//     .......... look for small sub-diagonal element ..........
      for (m = l; m < n; m++) {
          tst2 = fabs(e[m]) + tst1;
          if (tst2 == tst1) break;
      }
      if (m == n) m--;
      if (m == l) { d[l] = d[l] + f; continue; }

      for (j = 0; j < 30; j++) {      // iterate at most 30 times

//     .......... form shift ..........
          l1    = l  + 1;
          l2    = l1 + 1;
          g     = d[l];
          p     = (d[l1] - g) / (2.0 * e[l]);
          r     = my_pythag(p, 1.0);
          d[l]  = e[l] / (p + copysign(r, p));
          d[l1] = e[l] * (p + copysign(r, p));
          dl1   = d[l1];
          h     = g - d[l];

          for (i = l2; i < n; i++) d[i] = d[i] - h;

//     .......... ql transformation ..........
          f   = f + h;
          p   = d[m];
          c   = 1.0;
          c2  = c;
          el1 = e[l1];
          s   = 0.0;

//     .......... for i = m - 1 step -1 until l do -- ..........
          for (i = m - 1; i >= l; i--) {
              c3       = c2;
              c2       = c;
              s2       = s;
              g        = c * e[i];
              h        = c * p;
              r        = my_pythag(p, e[i]);
              e[i + 1] = s * r;
              s        = e[i] / r;
              c        = p / r;
              p        = c * d[i] - s * g;
              d[i + 1] = h + s * (c * g + s * d[i]);

//     .......... form vector ..........
              for (k = 0; k < n; k++) {
                  h           = z[OFFSET(k,i+1)];
                  z[OFFSET(k,i+1)] = s * z[OFFSET(k,i)] + c * h;
                  z[OFFSET(k,i)] = c * z[OFFSET(k,i)] - s * h;
          }   }

          p    = -s * s2 * c3 * el1 * e[l] / dl1;
          e[l] = s * p;
          d[l] = c * p;
          tst2 = tst1 + fabs(e[l]);
          if (tst2 <= tst1) break;
      }

      if (j == 30) return l;     // no convergence
      d[l] = d[l] + f;
  }

//     .......... order eigenvalues and eigenvectors ..........
  for (ii = 1; ii < n; ii++) {
      i = ii - 1;
      k = ii - 1;
      p = d[i];

      for (j = ii; j < n; j++) { if (d[j] >= p) continue; k = j; p = d[j]; }

      if (k == i) continue;
      d[k] = d[i];
      d[i] = p;

      for (j = 0; j < n; j++) { p = z[OFFSET(j,i)]; z[OFFSET(j,i)] = z[OFFSET(j,k)]; z[OFFSET(j,k)] = p; }
  }

  return 0;
}


void my_ga_rrrebak(int nm, int n, double *b, double *dl, int m, double *z) {
//
//     this subroutine is a translation of the algol procedure rrrebaka,
//     num. math. 11, 99-110(1968) by martin and wilkinson.
//     handbook for auto. comp., vol.ii-linear algebra, 303-314(1971).
//
//     this subroutine forms the eigenvectors of a generalized symmetric eigensystem
//     by back transforming those of the derived symmetric matrix determined by rrreduc.
//
//     on input
//
//        nm  must be set to the row dimension of two-dimensional arrays.
//        n   is the order of the matrix system.
//        b  contains information about the similarity transformation (cholesky decomposition)
//                 used in the rrreduction by  rrreduc in its strict lower triangle.
//        dl contains further information about the transformation.
//        m  is the number of eigenvectors to be back transformed.
//        z  contains the eigenvectors to be back transformed in its first m columns.
//
//     on output
//
//        z contains the transformed eigenvectors in its first m columns.

  int i, j, k;
  double x;

  if (m == 0) return;

  for (j = 0; j < m; j++) {

      for (i = n - 1; i >= 0; i--) {
          x = z[OFFSET(i,j)];
          for (k = i + 1; k < n; k++) x = x - b[OFFSET(k,i)] * z[OFFSET(k,j)];
          z[OFFSET(i,j)] = x / dl[i];
} }   }

int my_rsg(int nm, int n, double *a, double *b, double *w, double *z, double *fv1, double *fv2) {
//
//     this subroutine calls the recommended sequence of
//     subroutines from the eigensystem subroutine package (eispack)
//     to find the eigenvalues and eigenvectors (if desired)
//     for the real symmetric generalized eigenproblem  ax = (lambda)bx.
//
//     on input
//
//        nm  must be set to the row dimension of the two-dimensional arrays.
//        n   is the order of the matrices  a  and  b.
//        a   contains a real symmetric matrix.
//        b   contains a positive definite real symmetric matrix.
//
//     on output
//
//        w  contains the eigenvalues in ascending order.
//        z  contains the eigenvectors
//        ierr  is an integer output variable set equal to the error completion code (0, no error)
//        fv1 and fv2 are temporary storage arrays.

  if (n > nm) return 10 * n;

  int ierr = my_ga_rrreduc(nm, n, a, b, fv2);
  if (ierr != 0) return ierr;

// find both eigenvalues and eigenvectors
  my_ga_tred2(nm, n, a, w, fv1, z);

  ierr = my_ga_tql2(nm, n, w, fv1, z);
  if (ierr != 0) return ierr;

  my_ga_rrrebak(nm, n, b, fv2, n, z);
  return ierr;
}


int my_rs(int nm, int n, double *a, double *w, double *z, double *fv1, double *fv2) {
//
//     this subroutine finds the eigenvalues and eigenvectors
//     for the real symmetric matrix
//
//     on input
//
//        nm  must be set to the row dimension of the two-dimensional arrays.
//        n   is the order of the matrices  a  and  b.
//        a   contains a real symmetric matrix.
//
//     on output
//
//        w  contains the eigenvalues in ascending order.
//        z  contains the eigenvectors
//        ierr  is an integer output variable set equal to the error completion code (0, no error)
//        fv1 and fv2 are temporary storage arrays.

  int ierr;
  if (n > nm) return 10 * n;

// find both eigenvalues and eigenvectors
  my_ga_tred2(nm, n, a, w, fv1, z);
  ierr = my_ga_tql2(nm, n, w, fv1, z);

  return ierr;
}

#ifndef USE_LAPACK

//     Solve the generalized eigen-value problem returning
//     all eigen-vectors and values in ascending order
void  Eigen_gen(double * g_a, double * g_s, double * g_v, double * evals) {

//  Make local copies of g_a and g_s to preserve values and allocate scratch space
  //double *  a  = (double *) malloc(nbfn * nbfn * sizeof(double));
  ocrGuid_t affinity=NULL_GUID;
  PTR_T a_ptr;
  ocrGuid_t a_db;
  DBCREATE( &a_db, &a_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *a = (double*)a_ptr;

  //double *  s  = (double *) malloc(nbfn * nbfn * sizeof(double));
  PTR_T s_ptr;
  ocrGuid_t s_db;
  DBCREATE( &s_db, &s_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *s = (double*)s_ptr;

  //double * fv1 = (double  *) malloc(nbfn * sizeof(double));
  PTR_T fv1_ptr;
  ocrGuid_t fv1_db;
  DBCREATE( &fv1_db, &fv1_ptr, nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *fv1 = (double*)fv1_ptr;

  //double * fv2 = (double  *) malloc(nbfn * sizeof(double));
  PTR_T fv2_ptr;
  ocrGuid_t fv2_db;
  DBCREATE( &fv2_db, &fv2_ptr, nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *fv2 = (double*)fv2_ptr;

  memcpy(a, g_a, nbfn * nbfn * sizeof(double));
  memcpy(s, g_s, nbfn * nbfn * sizeof(double));

  int ierr = my_rsg(nbfn, nbfn, a, s, evals, g_v, fv1, fv2);
  if (ierr != 0) {printf("Error code %d\n", ierr); exit(1);}

  //free(a); free(s); free(fv1); free(fv2);
  ocrDbRelease(a_db); ocrDbRelease(s_db); ocrDbRelease(fv1_db); ocrDbRelease(fv2_db);
}


//     Solve the eigen-value problem returning all eigen-vectors
//     and values in ascending order
void  Eigen_std(double * g_a, double * g_v, double * evals) {

//  Make local copy of g_a to preserve values and allocate scratch space
  //double *  a  = (double  *) malloc(nbfn * nbfn * sizeof(double));
  ocrGuid_t affinity=NULL_GUID;
  PTR_T a_ptr;
  ocrGuid_t a_db;
  DBCREATE( &a_db, &a_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *a = (double*)a_ptr;

  //double * fv1 = (double  *) malloc(nbfn * sizeof(double));
  PTR_T fv1_ptr;
  ocrGuid_t fv1_db;
  DBCREATE( &fv1_db, &fv1_ptr, nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *fv1 = (double*)fv1_ptr;

  //double * fv2 = (double  *) malloc(nbfn * sizeof(double));
  PTR_T fv2_ptr;
  ocrGuid_t fv2_db;
  DBCREATE( &fv2_db, &fv2_ptr, nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *fv2 = (double*)fv2_ptr;
  memcpy(a, g_a, nbfn * nbfn * sizeof(double));

  int ierr = my_rs(nbfn, nbfn, a, evals, g_v, fv1, fv2);
  if (ierr != 0) {printf("Error code %d\n", ierr); exit(1);}

  ocrDbRelease(a_db); ocrDbRelease(fv1_db); ocrDbRelease(fv2_db);
}

#else

//     Solve the generalized eigen-value problem returning
//     all eigen-vectors and values in ascending order
void  Eigen_gen(double * g_a, double * g_s, double * g_v, double * evals) {

//  Make local copies of g_a and g_s to preserve values and allocate scratch space
  //double *  a  = (double *) malloc(nbfn * nbfn * sizeof(double));
  ocrGuid_t affinity=NULL_GUID;
  PTR_T a_ptr;
  ocrGuid_t a_db;
  DBCREATE( &a_db, &a_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *a = (double*)a_ptr;

  //double *  s  = (double *) malloc(nbfn * nbfn * sizeof(double));
  PTR_T s_ptr;
  ocrGuid_t s_db;
  DBCREATE( &s_db, &s_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *s = (double*)s_ptr;

  memcpy(a, g_a, nbfn * nbfn * sizeof(double));
  memcpy(s, g_s, nbfn * nbfn * sizeof(double));

  int ierr = LAPACKE_dsygv(LAPACK_ROW_MAJOR, 1, 'V', 'U', nbfn, a, nbfn, s, nbfn, evals);
  memcpy(g_v, a, nbfn * nbfn * sizeof(double));

  if (ierr != 0) {printf("Error code %d\n", ierr); exit(1);}

  ocrDbRelease(a_db); ocrDbRelease(s_db);
}

//     Solve the eigen-value problem returning all eigen-vectors
//     and values in ascending order
void  Eigen_std(double * g_a, double * g_v, double * evals) {

//  Make local copy of g_a to preserve values and allocate scratch space
  //double *  a  = (double  *) malloc(nbfn * nbfn * sizeof(double));
  ocrGuid_t affinity=NULL_GUID;
  PTR_T a_ptr;
  ocrGuid_t a_db;
  DBCREATE( &a_db, &a_ptr, nbfn * nbfn * sizeof(double), FLAGS, affinity, NO_ALLOC);
  double *a = (double*)a_ptr;

  memcpy(a, g_a, nbfn * nbfn * sizeof(double));

  int ierr = LAPACKE_dsyev(LAPACK_ROW_MAJOR, 'V', 'U', nbfn, a, nbfn, evals);
  memcpy(g_v, a, nbfn * nbfn * sizeof(double));

  if (ierr != 0) {printf("Error code %d\n", ierr); exit(1);}

  ocrDbRelease(a_db);
}

#endif
