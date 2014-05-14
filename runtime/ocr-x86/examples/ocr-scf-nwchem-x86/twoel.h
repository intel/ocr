#ifndef TWOEL_H
#define TWOEL_H


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <sys/time.h>

#include "cscc.h"
#include "integ.h"
#include "diagonalize.h"
#include "g.h"
#include "scf.h"

//======================================================================================================//
//these functions were moved to twoel.c since they need to be in the same
//file as the EDTs in order to return the correct value
double timer(void);
double timer2(void);

double dendif(void);
double diagon(int iter);
//======================================================================================================//

void twoel_ocr();
void create_new_iteration();

#define OFF(a, b)             ((a ##_off) + (b))
#define DENS(a, b)            (g_dens_ ## a ## _ ## b)

#define DENS_DECL(a, b)       double g_dens_ ## a ## _ ## b = g_dens[OFF(a,b)]

#ifdef BAD_CACHE_UTILIZATION
#define DENS_DECL_SYMM(a, b)  double g_dens_ ## a ## _ ## b = g_dens[OFF(a,b)]; \
                              double g_dens_ ## b ## _ ## a = g_dens[OFF(b,a)]
#else
#define DENS_DECL_SYMM(a, b)  double g_dens_ ## a ## _ ## b = g_dens[OFF(a,b)]; \
                              double g_dens_ ## b ## _ ## a = g_dens_ ## a ## _ ## b
#endif

#define TOL_DECL(a, b)        double tol2e_over_g_schwarz_ ## a ## _ ## b = tol2e / g_schwarz[OFF(a,b)]


#define UPDATE(a, b, c, d) fock[OFF(a,b)] += (gg * DENS(c,d)); \
                           fock[OFF(a,c)] -= (0.50 * gg * DENS(b,d));

#define UPDATE1(a, b, c, d)    fock[OFF(a,b)] += (gg * DENS(c,d));
#define UPDATE2(a, b, c, d)    fock[OFF(a,c)] -= (0.50 * gg * DENS(b,d));

#define UPDATE_COULOMB1(a, b, c, d)  fock[OFF(a,b)] += (gg * DENS(c,d))
#define UPDATE_COULOMB2(a, b, c, d)  fock[OFF(a,b)] += (gg * DENS(c,d)) + (gg * DENS(c,d))
#define UPDATE_EXCHANGE(a, b, c, d)  fock[OFF(a,c)] -= (0.50 * gg * DENS(b,d));

#ifndef NO_BOOKKEEPING
#define CUT_DECLS       long long int temp_icut1 = 0; \
                        long long int temp_icut2 = 0; \
                        long long int temp_icut3 = 0; \
                        long long int temp_icut4 = 0
#define CUT_WRITEBACK   twoel_context[id].icut1 += temp_icut1; \
                        twoel_context[id].icut2 += temp_icut2; \
                        twoel_context[id].icut3 += temp_icut3; \
                        twoel_context[id].icut4 += temp_icut4

#define CUT1(N)   temp_icut1 += N
#define CUT2(N)   temp_icut2 += N
#define CUT3(N)   temp_icut3 += N
#define CUT4(N)   temp_icut4 += N
#else
#define CUT1(N)
#define CUT2(N)
#define CUT3(N)
#define CUT4(N)
#define CUT_DECLS
#define CUT_WRITEBACK
#endif

#ifndef NO_PRECALC
#define G(a, b, c, d) g_fast(OFF(a, b), OFF(c, d))
#else
#define G(a, b, c, d) g(a, b, c, d)
#endif

#define CHUNK_SIZE 1
#define N3_CHUNK_SIZE 4

#define SYMM_N3

#define NUM_MUT 16



#endif /* TWOEL_H */
