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
#include <sys/time.h>

//TODO need this because we don't use the user api yet
#include "ocr-runtime.h"


void sequential_cholesky_task ( size_t n_dbs, const std::vector<ocrGuid_t>& dbs ) {
    int index = 0, iB = 0, jB = 0, kB = 0, jBB = 0;

    ocr::Data* func_args_db = (ocr::Data*) deguidify(dbs[1]);
    intptr_t *func_args = (intptr_t *)func_args_db;
    int k = (int) func_args[0];
    int tileSize = (int) func_args[1];
    ocrGuid_t out_lkji_kkkp1_event_guid = (ocrGuid_t) func_args[2];
    ocrGuid_t*** lkji_db = (ocrGuid_t***) func_args[3];

    ocr::Data* aBlock_db = (ocr::Data*) deguidify(dbs[0]);
    double** aBlock = (double**) aBlock_db;

    double** lBlock = (double**) malloc(sizeof(double*)*tileSize);
    for( index = 0; index < tileSize; ++index )
        lBlock[index] = (double*) malloc(sizeof(double)*(index+1));

    for( kB = 0 ; kB < tileSize ; ++kB ) {
        if( aBlock[ kB ][ kB ] <= 0 ) {
            fprintf(stderr,"Not a symmetric positive definite (SPD) matrix\n"); exit(1);
        } else {
            lBlock[ kB ][ kB ] = sqrt( aBlock[ kB ][ kB ] );
        }

        for(jB = kB + 1; jB < tileSize ; ++jB ) 
            lBlock[ jB ][ kB ] = aBlock[ jB ][ kB ] / lBlock[ kB ][ kB ];

        for(jBB= kB + 1; jBB < tileSize ; ++jBB ) 
            for(iB = jBB ; iB < tileSize ; ++iB ) 
                aBlock[ iB ][ jBB ] -= lBlock[ iB ][ kB ] * lBlock[ jBB ][ kB ];
    }

    ocr::Event* out_lkji_kkkp1_event = (ocr::Event*) deguidify(out_lkji_kkkp1_event_guid);
    ocr::Data *out_lkji_kkkp1_db = (ocr::Data*) lBlock;
    ocrGuid_t out_lkji_kkkp1_db_guid = guidify<ocr::Data>(out_lkji_kkkp1_db);
    ocrGuid_t worker_guid = ocr_get_current_worker_guid();
    lkji_db[k][k][k+1] = out_lkji_kkkp1_db_guid;
    out_lkji_kkkp1_event->put(out_lkji_kkkp1_db_guid, worker_guid);
}

void trisolve_task ( size_t n_dbs, const std::vector<ocrGuid_t>& dbs ) {
    int i = 0, iB, jB, kB;

    ocr::Data* func_args_db = (ocr::Data*) deguidify(dbs[2]);
    intptr_t *func_args = (intptr_t *)func_args_db;
    int k = (int) func_args[0];
    int j = (int) func_args[1];
    int tileSize = (int) func_args[2];
    ocrGuid_t out_lkji_jkkp1_event_guid = (ocrGuid_t) func_args[3];
    ocrGuid_t*** lkji_db = (ocrGuid_t***) func_args[4];
    
    ocr::Data* aBlock_db = (ocr::Data*) deguidify(dbs[0]);
    double** aBlock = (double**) aBlock_db;

    ocr::Data* liBlock_db = (ocr::Data*) deguidify(dbs[1]);
    double** liBlock = (double**) liBlock_db;

    double ** loBlock = (double**) malloc (sizeof(double*)*tileSize);
    for(i = 0; i < tileSize; ++i)
        loBlock[i] = (double*) malloc (sizeof(double*)*tileSize);

    for( kB = 0; kB < tileSize ; ++kB ) {
        for( iB = 0; iB < tileSize ; ++iB )
            loBlock[ iB ][ kB ] = aBlock[ iB ][ kB ] / liBlock[ kB ][ kB ];

        for( jB = kB + 1 ; jB < tileSize; ++jB )
            for( iB = 0; iB < tileSize; ++iB )
                aBlock[ iB ][ jB ] -= liBlock[ jB ][ kB ] * loBlock[ iB ][ kB ];
    }

    ocr::Event* out_lkji_jkkp1_event = (ocr::Event*) deguidify(out_lkji_jkkp1_event_guid);
    ocr::Data *out_lkji_jkkp1_db = (ocr::Data*) loBlock;
    ocrGuid_t out_lkji_jkkp1_db_guid = guidify<ocr::Data>(out_lkji_jkkp1_db);
    ocrGuid_t worker_guid = ocr_get_current_worker_guid();
    lkji_db[j][k][k+1] = out_lkji_jkkp1_db_guid;
    out_lkji_jkkp1_event->put(out_lkji_jkkp1_db_guid, worker_guid);
}

void update_diagonal_task ( size_t n_dbs, const std::vector<ocrGuid_t>& dbs ) {
    int iB, jB, kB;
    double temp = 0;

    ocr::Data* func_args_db = (ocr::Data*) deguidify(dbs[2]);
    intptr_t *func_args = (intptr_t *)func_args_db;
    int k = (int) func_args[0];
    int j = (int) func_args[1];
    int i = (int) func_args[2];
    int tileSize = (int) func_args[3];
    ocrGuid_t out_lkji_jjkp1_event_guid = (ocrGuid_t) func_args[4];
    ocrGuid_t*** lkji_db = (ocrGuid_t***) func_args[5];
    
    ocr::Data* aBlock_db = (ocr::Data*) deguidify(dbs[0]);
    double** aBlock = (double**) aBlock_db;

    ocr::Data* l2Block_db = (ocr::Data*) deguidify(dbs[1]);
    double** l2Block = (double**) l2Block_db;

    for( jB = 0; jB < tileSize ; ++jB ) {
        for( kB = 0; kB < tileSize ; ++kB ) {
            temp = 0 - l2Block[ jB ][ kB ];
            for( iB = jB; iB < tileSize; ++iB ) 
                aBlock[ iB ][ jB ] += temp * l2Block[ iB ][ kB ];
        }
    }

    ocr::Event* out_lkji_jjkp1_event = (ocr::Event*) deguidify(out_lkji_jjkp1_event_guid);
    ocr::Data *out_lkji_jjkp1_db = (ocr::Data*) aBlock;
    ocrGuid_t out_lkji_jjkp1_db_guid = guidify<ocr::Data>(out_lkji_jjkp1_db);
    ocrGuid_t worker_guid = ocr_get_current_worker_guid();
    lkji_db[j][j][k+1] = out_lkji_jjkp1_db_guid;
    out_lkji_jjkp1_event->put(out_lkji_jjkp1_db_guid, worker_guid);
}

void update_nondiagonal_task ( size_t n_dbs, const std::vector<ocrGuid_t>& dbs ) {
    double temp;
    int jB, kB, iB;

    ocr::Data* func_args_db = (ocr::Data*) deguidify(dbs[3]);
    intptr_t *func_args = (intptr_t *)func_args_db;
    int k = (int) func_args[0];
    int j = (int) func_args[1];
    int i = (int) func_args[2];
    int tileSize = (int) func_args[3];
    ocrGuid_t out_lkji_jikp1_event_guid = (ocrGuid_t) func_args[4];
    ocrGuid_t*** lkji_db = (ocrGuid_t***) func_args[5];

    ocr::Data* aBlock_db = (ocr::Data*) deguidify(dbs[0]);
    double** aBlock = (double**) aBlock_db;

    ocr::Data* l1Block_db = (ocr::Data*) deguidify(dbs[1]);
    double** l1Block = (double**) l1Block_db;

    ocr::Data* l2Block_db = (ocr::Data*) deguidify(dbs[2]);
    double** l2Block = (double**) l2Block_db;

    for( jB = 0; jB < tileSize ; ++jB ) {
        for( kB = 0; kB < tileSize ; ++kB ) {
            temp = 0 - l2Block[ jB ][ kB ];
            for( iB = 0; iB < tileSize ; ++iB ) 
                aBlock[ iB ][ jB ] += temp * l1Block[ iB ][ kB ];
        }
    }

    ocr::Event* out_lkji_jikp1_event = (ocr::Event*) deguidify(out_lkji_jikp1_event_guid);
    ocr::Data *out_lkji_jikp1_db = (ocr::Data*) aBlock;
    ocrGuid_t out_lkji_jikp1_db_guid = guidify<ocr::Data>(out_lkji_jikp1_db);
    ocrGuid_t worker_guid = ocr_get_current_worker_guid();
    lkji_db[j][i][k+1] = out_lkji_jikp1_db_guid;
    out_lkji_jikp1_event->put(out_lkji_jikp1_db_guid, worker_guid);
}

void wrap_up_task ( size_t n_dbs, const std::vector<ocrGuid_t>& dbs ) {
    int i, j, k, i_b, j_b;
    double** temp;
	FILE* out = fopen("cholesky.out", "w");

    ocr::Data* func_args_db = (ocr::Data*) deguidify(dbs[1]);
    intptr_t *func_args = (intptr_t *)func_args_db;
    int numTiles = (int) func_args[0];
    int tileSize = (int) func_args[1];
    ocrGuid_t*** lkji_db = (ocrGuid_t***) func_args[2];

	for ( i = 0; i < numTiles; ++i ) {
		for( i_b = 0; i_b < tileSize; ++i_b) {
			k = 1;
			for ( j = 0; j <= i; ++j ) {
				temp = (double**) (ocr::Data*) deguidify(lkji_db[i][j][k]);
				if(i != j) {
					for(j_b = 0; j_b < tileSize; ++j_b) {
						fprintf( out, "%lf ", temp[i_b][j_b]);
					}
				} else {
					for(j_b = 0; j_b <= i_b; ++j_b) {
						fprintf( out, "%lf ", temp[i_b][j_b]);
					}
				}
				++k;
			}
		}
	}
    ocrShutdown();
}

int main( int argc, char* argv[] ) {
    //TODO this should use the API to register function pointers
    ocrInit(argc, argv, 0, NULL);

	int i, j, k, ii;
	double **A, ** temp;
	int A_i, A_j, T_i, T_j;
	FILE *in;
	int i_b, j_b;

	int matrixSize = -1;
	int tileSize = -1;
	int numTiles = -1;

	if ( argc !=  4 ) {
		printf("Usage: ./cholesky matrixSize tileSize fileName (found %d args)\n", argc);
		return 1;
	}

	matrixSize = atoi(argv[1]);
	tileSize = atoi(argv[2]);
	if ( matrixSize % tileSize != 0) {
		printf("Incorrect tile size %d for the matrix of size %d \n", tileSize, matrixSize);
		return 1;
	}

	numTiles = matrixSize/tileSize;

	ocrGuid_t*** lkji_events;
	ocrGuid_t*** lkji_db;

	lkji_events = (ocrGuid_t ***) malloc(sizeof(ocrGuid_t **)*numTiles);
	lkji_db = (ocrGuid_t ***) malloc(sizeof(ocrGuid_t **)*numTiles);
	for( i = 0 ; i < numTiles ; ++i ) {
		lkji_events[i] = (ocrGuid_t **) malloc(sizeof(ocrGuid_t *)*(i+1));
		lkji_db[i] = (ocrGuid_t **) malloc(sizeof(ocrGuid_t *)*(i+1));
		for( j = 0 ; j <= i ; ++j ) {
			lkji_events[i][j] = (ocrGuid_t *) malloc(sizeof(ocrGuid_t)*(numTiles+1));
			lkji_db[i][j] = (ocrGuid_t *) malloc(sizeof(ocrGuid_t)*(numTiles+1));
			for( k = 0 ; k <= numTiles ; ++k )
				lkji_events[i][j][k] = eventFactory->create();
		}
	}

	in = fopen(argv[3], "r");
	if( !in ) {
		printf("Cannot find file: %s\n", argv[3]);
		return 1;
	}

	A = (double**) malloc (sizeof(double*)*matrixSize);
	for( i = 0; i < matrixSize; ++i)
		A[i] = (double*) malloc(sizeof(double)*matrixSize);

	for( i = 0; i < matrixSize; ++i ) {
		for( j = 0; j < matrixSize-1; ++j )
			fscanf(in, "%lf ", &A[i][j]);
		fscanf(in, "%lf\n", &A[i][j]);
	}

	for( i = 0 ; i < numTiles ; ++i ) {
        for( j = 0 ; j <= i ; ++j ) {
            // Allocate memory for the tiles.
            temp = (double**) malloc (sizeof(double*)*tileSize);
            for( ii = 0; ii < tileSize; ++ii )
                temp[ii] = (double*) malloc (sizeof(double)*tileSize);
            // Split the matrix into tiles and write it into the item space at time 0.
            // The tiles are indexed by tile indices (which are tag values).
            for( A_i = i*tileSize, T_i = 0 ; T_i < tileSize; ++A_i, ++T_i ) {
                for( A_j = j*tileSize, T_j = 0 ; T_j < tileSize; ++A_j, ++T_j ) {
                    temp[ T_i ][ T_j ] = A[ A_i ][ A_j ];
                }
            }

            ocr::Event* lkji_events_i_j_0 = (ocr::Event*) deguidify(lkji_events[i][j][0]);
            ocr::Data *lkji_i_j_0_db = (ocr::Data*) temp;
            ocrGuid_t lkji_i_j_0_db_guid = guidify<ocr::Data>(lkji_i_j_0_db);
            ocrGuid_t worker_guid = ocr_get_current_worker_guid();
            lkji_db[i][j][0] = lkji_i_j_0_db_guid;
            lkji_events_i_j_0->put(lkji_i_j_0_db_guid, worker_guid);
        }
	}

    for ( k = 0; k < numTiles; ++k ) {
        { // sequential_cholesky_task
            ocr::EventList el;
            el.enlist(lkji_events[k][k][k]);

            ocrGuid_t func_args_event = eventFactory->create();
            ocr::Event* func_args_event_p = (ocr::Event*) deguidify(func_args_event);
            intptr_t *func_args = (intptr_t *)malloc(4*sizeof(intptr_t));
            func_args[0]=k;
            func_args[1]=tileSize;
            func_args[2]=(ocrGuid_t)lkji_events[k][k][k+1];
            func_args[3]=(intptr_t)lkji_db;
            ocr::Data* func_args_db = (ocr::Data*) func_args;
            ocrGuid_t func_args_db_guid = guidify<ocr::Data>(func_args_db);

            el.enlist(func_args_event);
            ocrGuid_t worker_guid = ocr_get_current_worker_guid();
            func_args_event_p->put(func_args_db_guid, worker_guid);

            taskFactory->create(sequential_cholesky_task, &el, worker_guid);
        }
        
        for( j = k + 1 ; j < numTiles ; ++j ) {
            { // trisolve_task
                ocr::EventList el;
                el.enlist(lkji_events[j][k][k]);
                el.enlist(lkji_events[k][k][k+1]);

                ocrGuid_t func_args_event = eventFactory->create();
                ocr::Event* func_args_event_p = (ocr::Event*) deguidify(func_args_event);
                intptr_t *func_args = (intptr_t *)malloc(5*sizeof(intptr_t));
                func_args[0]=k;
                func_args[1]=j;
                func_args[2]=tileSize;
                func_args[3]=(ocrGuid_t)lkji_events[j][k][k+1];
                func_args[4]=(intptr_t)lkji_db;
                ocr::Data* func_args_db = (ocr::Data*) func_args;
                ocrGuid_t func_args_db_guid = guidify<ocr::Data>(func_args_db);

                el.enlist(func_args_event);
                ocrGuid_t worker_guid = ocr_get_current_worker_guid();
                func_args_event_p->put(func_args_db_guid, worker_guid);

                taskFactory->create(trisolve_task, &el, worker_guid);
            }

            for( i = k + 1 ; i < j ; ++i ) {
                ocr::EventList el;
                el.enlist(lkji_events[j][i][k]);
                el.enlist(lkji_events[j][k][k+1]);
                el.enlist(lkji_events[i][k][k+1]);

                ocrGuid_t func_args_event = eventFactory->create();
                ocr::Event* func_args_event_p = (ocr::Event*) deguidify(func_args_event);
                intptr_t *func_args = (intptr_t *)malloc(6*sizeof(intptr_t));
                func_args[0]=k;
                func_args[1]=j;
                func_args[2]=i;
                func_args[3]=tileSize;
                func_args[4]=(ocrGuid_t)lkji_events[j][i][k+1];
                func_args[5]=(intptr_t)lkji_db;
                ocr::Data* func_args_db = (ocr::Data*) func_args;
                ocrGuid_t func_args_db_guid = guidify<ocr::Data>(func_args_db);

                el.enlist(func_args_event);
                ocrGuid_t worker_guid = ocr_get_current_worker_guid();
                func_args_event_p->put(func_args_db_guid, worker_guid);

                taskFactory->create(update_nondiagonal_task, &el, worker_guid);
            }

            {
                ocr::EventList el;
                el.enlist(lkji_events[j][j][k]);
                el.enlist(lkji_events[j][k][k+1]);

                ocrGuid_t func_args_event = eventFactory->create();
                ocr::Event* func_args_event_p = (ocr::Event*) deguidify(func_args_event);
                intptr_t *func_args = (intptr_t *)malloc(6*sizeof(intptr_t));
                func_args[0]=k;
                func_args[1]=j;
                func_args[2]=i;
                func_args[3]=tileSize;
                func_args[4]=(ocrGuid_t)lkji_events[j][j][k+1];
                func_args[5]=(intptr_t)lkji_db;
                ocr::Data* func_args_db = (ocr::Data*) func_args;
                ocrGuid_t func_args_db_guid = guidify<ocr::Data>(func_args_db);

                el.enlist(func_args_event);
                ocrGuid_t worker_guid = ocr_get_current_worker_guid();
                func_args_event_p->put(func_args_db_guid, worker_guid);

                taskFactory->create(update_diagonal_task, &el, worker_guid);
            }
        }
    }

    { //wrap_up_task
        ocr::EventList el;
        el.enlist(lkji_events[numTiles-1][numTiles-1][numTiles]);

        ocrGuid_t func_args_event = eventFactory->create();
        ocr::Event* func_args_event_p = (ocr::Event*) deguidify(func_args_event);
        intptr_t *func_args = (intptr_t *)malloc(3*sizeof(intptr_t));
        func_args[0]=(int)numTiles;
        func_args[1]=(int)tileSize;
        func_args[2]=(intptr_t)lkji_db;
        ocr::Data* func_args_db = (ocr::Data*) func_args;
        ocrGuid_t func_args_db_guid = guidify<ocr::Data>(func_args_db);

        el.enlist(func_args_event);
        ocrGuid_t worker_guid = ocr_get_current_worker_guid();
        func_args_event_p->put(func_args_db_guid, worker_guid);

        taskFactory->create(wrap_up_task, &el, worker_guid);
    }

    ocrCleanup();
	return 0;
}
