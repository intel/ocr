/* Copyright (c) 2012, Rice University

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

*/

#include "ocr.h"
#include "math.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define FLAGS 0xdead
#define PROPERTIES 0xdead



u8 sequential_cholesky_task ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
	int index = 0, iB = 0, jB = 0, kB = 0, jBB = 0;

	intptr_t *func_args = *paramv;
	int k = (int) func_args[0];
	int tileSize = (int) func_args[1];
	ocrGuid_t out_lkji_kkkp1_event_guid = (ocrGuid_t) func_args[2];

	double* aBlock = (double*) (depv[0].ptr);
	double** aBlock2D = (double**) malloc(sizeof(double*)*tileSize);
	for( index = 0; index < tileSize; ++index )
		aBlock2D[index] = &(aBlock[index*tileSize]);

	void* lBlock_db;
	ocrGuid_t out_lkji_kkkp1_db_guid;
	ocrDbCreate( &out_lkji_kkkp1_db_guid, &lBlock_db, sizeof(double)*tileSize*tileSize, FLAGS, NULL, NO_ALLOC );

	double* lBlock = (double*) lBlock_db;
	double** lBlock2D = (double**) malloc(sizeof(double*)*tileSize);
	for( index = 0; index < tileSize; ++index )
		lBlock2D[index] = &(lBlock[index*tileSize]);

	for( kB = 0 ; kB < tileSize ; ++kB ) {
		if( aBlock2D[ kB ][ kB ] <= 0 ) {
			fprintf(stderr,"Not a symmetric positive definite (SPD) matrix\n"); exit(1);
		} else {
			lBlock2D[ kB ][ kB ] = sqrt( aBlock2D[ kB ][ kB ] );
		}

		for(jB = kB + 1; jB < tileSize ; ++jB )
			lBlock2D[ jB ][ kB ] = aBlock2D[ jB ][ kB ] / lBlock2D[ kB ][ kB ];

		for(jBB= kB + 1; jBB < tileSize ; ++jBB )
			for(iB = jBB ; iB < tileSize ; ++iB )
				aBlock2D[ iB ][ jBB ] -= lBlock2D[ iB ][ kB ] * lBlock2D[ jBB ][ kB ];
	}

	ocrEventSatisfy(out_lkji_kkkp1_event_guid, out_lkji_kkkp1_db_guid);

	free(lBlock2D);
	free(aBlock2D);
}

u8 trisolve_task ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
	int index, iB, jB, kB;

	intptr_t *func_args = *paramv;
	int k = (int) func_args[0];
	int j = (int) func_args[1];
	int tileSize = (int) func_args[2];
	ocrGuid_t out_lkji_jkkp1_event_guid = (ocrGuid_t) func_args[3];

	double* aBlock = (double*) (depv[0].ptr);
	double** aBlock2D = (double**) malloc(sizeof(double*)*tileSize);
	for( index = 0; index < tileSize; ++index )
		aBlock2D[index] = &(aBlock[index*tileSize]);

	double* liBlock = (double*) (depv[1].ptr);
	double** liBlock2D = (double**) malloc(sizeof(double*)*tileSize);
	for( index = 0; index < tileSize; ++index )
		liBlock2D[index] = &(liBlock[index*tileSize]);

	ocrGuid_t out_lkji_jkkp1_db_guid;
	void* loBlock_db;
	ocrDbCreate( &out_lkji_jkkp1_db_guid, &loBlock_db, sizeof(double)*tileSize*tileSize, FLAGS, NULL, NO_ALLOC);

	double * loBlock = (double*) loBlock_db;
	double** loBlock2D = (double**) malloc(sizeof(double*)*tileSize);
	for( index = 0; index < tileSize; ++index )
		loBlock2D[index] = &(loBlock[index*tileSize]);

	for( kB = 0; kB < tileSize ; ++kB ) {
		for( iB = 0; iB < tileSize ; ++iB )
			loBlock2D[ iB ][ kB ] = aBlock2D[ iB ][ kB ] / liBlock2D[ kB ][ kB ];

		for( jB = kB + 1 ; jB < tileSize; ++jB )
			for( iB = 0; iB < tileSize; ++iB )
				aBlock2D[ iB ][ jB ] -= liBlock2D[ jB ][ kB ] * loBlock2D[ iB ][ kB ];
	}

	ocrEventSatisfy(out_lkji_jkkp1_event_guid, out_lkji_jkkp1_db_guid);

	free(loBlock2D);
	free(liBlock2D);
	free(aBlock2D);
}

u8 update_diagonal_task ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
	int index, iB, jB, kB;
	double temp = 0;

	intptr_t *func_args = *paramv;
	int k = (int) func_args[0];
	int j = (int) func_args[1];
	int i = (int) func_args[2];
	int tileSize = (int) func_args[3];
	ocrGuid_t out_lkji_jjkp1_event_guid = (ocrGuid_t) func_args[4];

	double* aBlock = (double*) (depv[0].ptr);
	double** aBlock2D = (double**) malloc(sizeof(double*)*tileSize);
	for( index = 0; index < tileSize; ++index )
		aBlock2D[index] = &(aBlock[index*tileSize]);

	double* l2Block = (double*) (depv[1].ptr);
	double** l2Block2D = (double**) malloc(sizeof(double*)*tileSize);
	for( index = 0; index < tileSize; ++index )
		l2Block2D[index] = &(l2Block[index*tileSize]);

	for( jB = 0; jB < tileSize ; ++jB ) {
		for( kB = 0; kB < tileSize ; ++kB ) {
			temp = 0 - l2Block2D[ jB ][ kB ];
			for( iB = jB; iB < tileSize; ++iB )
				aBlock2D[ iB ][ jB ] += temp * l2Block2D[ iB ][ kB ];
		}
	}

	ocrEventSatisfy(out_lkji_jjkp1_event_guid, depv[0].guid);

	free(l2Block2D);
	free(aBlock2D);
}

u8 update_nondiagonal_task ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
	double temp;
	int index, jB, kB, iB;

	intptr_t *func_args = *paramv;
	int k = (int) func_args[0];
	int j = (int) func_args[1];
	int i = (int) func_args[2];
	int tileSize = (int) func_args[3];
	ocrGuid_t out_lkji_jikp1_event_guid = (ocrGuid_t) func_args[4];

	double* aBlock = (double*) (depv[0].ptr);
	double** aBlock2D = (double**) malloc(sizeof(double*)*tileSize);
	for( index = 0; index < tileSize; ++index )
		aBlock2D[index] = &(aBlock[index*tileSize]);

	double* l1Block = (double*) (depv[1].ptr);
	double** l1Block2D = (double**) malloc(sizeof(double*)*tileSize);
	for( index = 0; index < tileSize; ++index )
		l1Block2D[index] = &(l1Block[index*tileSize]);

	double* l2Block = (double*) (depv[2].ptr);
	double** l2Block2D = (double**) malloc(sizeof(double*)*tileSize);
	for( index = 0; index < tileSize; ++index )
		l2Block2D[index] = &(l2Block[index*tileSize]);

	for( jB = 0; jB < tileSize ; ++jB ) {
		for( kB = 0; kB < tileSize ; ++kB ) {
			temp = 0 - l2Block2D[ jB ][ kB ];
			for( iB = 0; iB < tileSize ; ++iB )
				aBlock2D[ iB ][ jB ] += temp * l1Block2D[ iB ][ kB ];
		}
	}

	ocrEventSatisfy(out_lkji_jikp1_event_guid, depv[0].guid);

	free(l2Block2D);
	free(l1Block2D);
	free(aBlock2D);
}

u8 wrap_up_task ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
	int i, j, i_b, j_b;
	double* temp;
	FILE* out = fopen("cholesky.out", "w");

	intptr_t *func_args = *paramv;
	int numTiles = (int) func_args[0];
	int tileSize = (int) func_args[1];

	for ( i = 0; i < numTiles; ++i ) {
		for( i_b = 0; i_b < tileSize; ++i_b) {
			for ( j = 0; j <= i; ++j ) {
				temp = (double*) (depv[i*(i+1)/2+j].ptr);
				if(i != j) {
					for(j_b = 0; j_b < tileSize; ++j_b) {
						fprintf( out, "%lf ", temp[i_b*tileSize+j_b]);
					}
				} else {
					for(j_b = 0; j_b <= i_b; ++j_b) {
						fprintf( out, "%lf ", temp[i_b*tileSize+j_b]);
					}
				}
			}
		}
	}
	ocrFinish();
}

inline static void sequential_cholesky_task_prescriber ( int k, int tileSize, ocrGuid_t*** lkji_event_guids) {
	ocrGuid_t seq_cholesky_task_guid;

	intptr_t **p_func_args = (intptr_t **)malloc(sizeof(intptr_t*));
	intptr_t *func_args = (intptr_t *)malloc(4*sizeof(intptr_t));
	func_args[0] = k;
	func_args[1] = tileSize;
	func_args[2] = (ocrGuid_t)lkji_event_guids[k][k][k+1];
	*p_func_args = func_args;

	ocrEdtCreate(&seq_cholesky_task_guid, sequential_cholesky_task, 3, NULL, (void**)p_func_args, PROPERTIES, 1, NULL);

	ocrAddDependence(lkji_event_guids[k][k][k], seq_cholesky_task_guid, 0);
	ocrEdtSchedule(seq_cholesky_task_guid);
}

inline static void trisolve_task_prescriber ( int k, int j, int tileSize, ocrGuid_t*** lkji_event_guids) {
	ocrGuid_t trisolve_task_guid;

	intptr_t **p_func_args = (intptr_t **)malloc(sizeof(intptr_t*));
	intptr_t *func_args = (intptr_t *)malloc(5*sizeof(intptr_t));
	func_args[0] = k;
	func_args[1] = j;
	func_args[2] = tileSize;
	func_args[3] = (ocrGuid_t)lkji_event_guids[j][k][k+1];
	*p_func_args = func_args;

	ocrEdtCreate(&trisolve_task_guid, trisolve_task, 4, NULL, (void**)p_func_args, PROPERTIES, 2, NULL);

	ocrAddDependence(lkji_event_guids[j][k][k], trisolve_task_guid, 0);
	ocrAddDependence(lkji_event_guids[k][k][k+1], trisolve_task_guid, 1);
	ocrEdtSchedule(trisolve_task_guid);
}

inline static void update_nondiagonal_task_prescriber ( int k, int j, int i, int tileSize, ocrGuid_t*** lkji_event_guids) { 
	ocrGuid_t update_nondiagonal_task_guid;

	intptr_t **p_func_args = (intptr_t **)malloc(sizeof(intptr_t*));
	intptr_t *func_args = (intptr_t *)malloc(6*sizeof(intptr_t));
	func_args[0] = k;
	func_args[1] = j;
	func_args[2] = i;
	func_args[3] = tileSize;
	func_args[4] = (ocrGuid_t)lkji_event_guids[j][i][k+1];
	*p_func_args = func_args;

	ocrEdtCreate(&update_nondiagonal_task_guid, update_nondiagonal_task, 5, NULL, (void**)p_func_args, PROPERTIES, 3, NULL);

	ocrAddDependence(lkji_event_guids[j][i][k], update_nondiagonal_task_guid, 0);
	ocrAddDependence(lkji_event_guids[j][k][k+1], update_nondiagonal_task_guid, 1);
	ocrAddDependence(lkji_event_guids[i][k][k+1], update_nondiagonal_task_guid, 2);

	ocrEdtSchedule(update_nondiagonal_task_guid);
}


inline static void update_diagonal_task_prescriber ( int k, int j, int i, int tileSize, ocrGuid_t*** lkji_event_guids) { 
	ocrGuid_t update_diagonal_task_guid;

	intptr_t **p_func_args = (intptr_t **)malloc(sizeof(intptr_t*));
	intptr_t *func_args = (intptr_t *)malloc(6*sizeof(intptr_t));
	func_args[0] = k;
	func_args[1] = j;
	func_args[2] = i;
	func_args[3] = tileSize;
	func_args[4] = (ocrGuid_t)lkji_event_guids[j][j][k+1];
	*p_func_args = func_args;

	ocrEdtCreate(&update_diagonal_task_guid, update_diagonal_task, 5, NULL, (void**)p_func_args, PROPERTIES, 2, NULL);

	ocrAddDependence(lkji_event_guids[j][j][k], update_diagonal_task_guid, 0);
	ocrAddDependence(lkji_event_guids[j][k][k+1], update_diagonal_task_guid, 1);

	ocrEdtSchedule(update_diagonal_task_guid);
}

inline static void wrap_up_task_prescriber ( int numTiles, int tileSize, ocrGuid_t*** lkji_event_guids ) {
	int i,j,k;
	ocrGuid_t wrap_up_task_guid;

	intptr_t **p_func_args = (intptr_t **)malloc(sizeof(intptr_t*));
	intptr_t *func_args = (intptr_t *)malloc(3*sizeof(intptr_t));
	func_args[0]=(int)numTiles;
	func_args[1]=(int)tileSize;
	*p_func_args = func_args;

	ocrEdtCreate(&wrap_up_task_guid, wrap_up_task, 2, NULL, (void**)p_func_args, PROPERTIES, (numTiles+1)*numTiles/2, NULL);

	int index = 0;
	for ( i = 0; i < numTiles; ++i ) {
		k = 1;
		for ( j = 0; j <= i; ++j ) {
			ocrAddDependence(lkji_event_guids[i][j][k], wrap_up_task_guid, index++);
			++k;
		}
	}

	ocrEdtSchedule(wrap_up_task_guid);
}

inline static ocrGuid_t*** allocateCreateEvents ( int numTiles ) {
	int i,j,k;
	ocrGuid_t*** lkji_event_guids;

	lkji_event_guids = (ocrGuid_t ***) malloc(sizeof(ocrGuid_t **)*numTiles);
	for( i = 0 ; i < numTiles ; ++i ) {
		lkji_event_guids[i] = (ocrGuid_t **) malloc(sizeof(ocrGuid_t *)*(i+1));
		for( j = 0 ; j <= i ; ++j ) {
			lkji_event_guids[i][j] = (ocrGuid_t *) malloc(sizeof(ocrGuid_t)*(numTiles+1));
			for( k = 0 ; k <= numTiles ; ++k )
				ocrEventCreate(&(lkji_event_guids[i][j][k]), OCR_EVENT_STICKY_T, true);
		}
	}

	return lkji_event_guids;
}

inline static double** readMatrix( int matrixSize, FILE* in ) {
	int i,j;
	double **A = (double**) malloc (sizeof(double*)*matrixSize);

	for( i = 0; i < matrixSize; ++i)
		A[i] = (double*) malloc(sizeof(double)*matrixSize);

	for( i = 0; i < matrixSize; ++i ) {
		for( j = 0; j < matrixSize-1; ++j )
			fscanf(in, "%lf ", &A[i][j]);
		fscanf(in, "%lf\n", &A[i][j]);
	}
	return A;
}

inline static void satisfyInitialTiles( int numTiles, int tileSize, double** matrix, ocrGuid_t*** lkji_event_guids) {
	int i,j,index;
	int A_i, A_j, T_i, T_j;

	for( i = 0 ; i < numTiles ; ++i ) {
		for( j = 0 ; j <= i ; ++j ) {
			void* temp_db;
			ocrGuid_t db_guid;
			ocrDbCreate( &db_guid, &temp_db, sizeof(double)*tileSize*tileSize, FLAGS, NULL, NO_ALLOC );

			double* temp = (double*) temp_db;
			double** temp2D = (double**) malloc(sizeof(double*)*tileSize);

			for( index = 0; index < tileSize; ++index )
				temp2D [index] = &(temp[index*tileSize]);

			// Split the matrix into tiles and write it into the item space at time 0.
			// The tiles are indexed by tile indices (which are tag values).
			for( A_i = i*tileSize, T_i = 0 ; T_i < tileSize; ++A_i, ++T_i ) {
				for( A_j = j*tileSize, T_j = 0 ; T_j < tileSize; ++A_j, ++T_j ) {
					temp2D[ T_i ][ T_j ] = matrix[ A_i ][ A_j ];
				}
			}
			ocrEventSatisfy(lkji_event_guids[i][j][0], db_guid);
			free(temp2D);
		}
	}
}

int main( int argc, char* argv[] ) {
	OCR_INIT(&argc, argv, sequential_cholesky_task, trisolve_task, update_nondiagonal_task, update_diagonal_task, wrap_up_task);

	int i, j, k;
	double **matrix, ** temp;
	FILE *in;

	int matrixSize = -1;
	int tileSize = -1;
	int numTiles = -1;

	if ( argc !=  4 ) {
		printf("Usage: ./cholesky matrixSize tileSize fileName (found %d args)\n", argc);
		return 1;
	}

	matrixSize = atoi(argv[1]);
	tileSize = atoi(argv[2]);

	if ( matrixSize % tileSize != 0 ) {
		printf("Incorrect tile size %d for the matrix of size %d \n", tileSize, matrixSize);
		return 1;
	}

	in = fopen(argv[3], "r");
	if( !in ) {
		printf("Cannot find file: %s\n", argv[3]);
		return 1;
	}

	numTiles = matrixSize/tileSize;

	ocrGuid_t*** lkji_event_guids = allocateCreateEvents(numTiles);

	matrix = readMatrix( matrixSize, in );

	satisfyInitialTiles( numTiles, tileSize, matrix, lkji_event_guids);

	for ( k = 0; k < numTiles; ++k ) {
		sequential_cholesky_task_prescriber ( k, tileSize, lkji_event_guids);

		for( j = k + 1 ; j < numTiles ; ++j ) {
			trisolve_task_prescriber ( k, j, tileSize, lkji_event_guids);

			for( i = k + 1 ; i < j ; ++i ) {
				update_nondiagonal_task_prescriber ( k, j, i, tileSize, lkji_event_guids);
			}
			update_diagonal_task_prescriber ( k, j, i, tileSize, lkji_event_guids);
		}
	}

	wrap_up_task_prescriber ( numTiles, tileSize, lkji_event_guids );

	ocrCleanup();
	return 0;
}
