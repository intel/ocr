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

u8 sequential_cholesky_task ( u32 paramc, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int index = 0, iB = 0, jB = 0, kB = 0, jBB = 0;

    intptr_t *func_args = (intptr_t *)(depv[1].ptr);
    int k = (int) func_args[0];
    int tileSize = (int) func_args[1];
    ocrGuid_t out_lkji_kkkp1_event_guid = (ocrGuid_t) func_args[2];
    ocrGuid_t*** lkji_db = (ocrGuid_t***) func_args[3];

    double* aBlock = (double*) (depv[0].ptr);
    double** aBlock2D = (double**) malloc(sizeof(double*)*tileSize);
    for( index = 0; index < tileSize; ++index )
        aBlock2D[index] = &(aBlock[index*tileSize]);

    double** lBlock2D = (double**) malloc(sizeof(double*)*tileSize);
    double* lBlock = (double*) malloc(sizeof(double)*tileSize*tileSize);
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

    // out_lkji_kkkp1_event
    ocrEventCreate(&out_lkji_kkkp1_event_guid, OCR_EVENT_STICKY_T, true);

    ocrGuid_t out_lkji_kkkp1_db_guid;
    ocrDbCreate( &out_lkji_kkkp1_db_guid, &lBlock, sizeof(double)*tileSize*tileSize, FLAGS );

    ocrEventSatisfy(out_lkji_kkkp1_event_guid, out_lkji_kkkp1_db_guid);
    // TODO sagnak lkji_db[k][k][k+1] = out_lkji_kkkp1_db_guid;

    free(lBlock2D);
    free(aBlock2D);
}

u8 trisolve_task ( u32 paramc, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int i = 0, iB, jB, kB;

    intptr_t *func_args = (intptr_t *)(depv[2].ptr);
    int k = (int) func_args[0];
    int j = (int) func_args[1];
    int tileSize = (int) func_args[2];
    ocrGuid_t out_lkji_jkkp1_event_guid = (ocrGuid_t) func_args[3];
    ocrGuid_t*** lkji_db = (ocrGuid_t***) func_args[4];

    double* aBlock = (double*) (depv[0].ptr);
    double** aBlock2D = (double**) malloc(sizeof(double*)*tileSize);
    for( index = 0; index < tileSize; ++index )
        aBlock2D[index] = &(aBlock[index*tileSize]);

    double* liBlock = (double*) (depv[1].ptr);
    double** liBlock2D = (double**) malloc(sizeof(double*)*tileSize);
    for( index = 0; index < tileSize; ++index )
        liBlock2D[index] = &(liBlock[index*tileSize]);

    double * loBlock = (double*) malloc (sizeof(double)*tileSize*tileSize);
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

    // out_lkji_jkkp1_event;
    ocrEventCreate(&out_lkji_jkkp1_event_guid, OCR_EVENT_STICKY_T, true);

    ocrGuid_t out_lkji_jkkp1_db_guid;
    ocrDbCreate( &out_lkji_jkkp1_db_guid, &loBlock, sizeof(double)*tileSize*tileSize, FLAGS );
    // TODO sagnak lkji_db[j][k][k+1] = out_lkji_jkkp1_db_guid;

    ocrEventSatisfy(out_lkji_jkkp1_event_guid, out_lkji_jkkp1_db_guid);

    free(loBlock2D);
    free(liBlock2D);
    free(aBlock2D);
}

u8 update_diagonal_task ( u32 paramc, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int iB, jB, kB;
    double temp = 0;

    intptr_t *func_args = (intptr_t *)(depv[2].ptr);
    int k = (int) func_args[0];
    int j = (int) func_args[1];
    int i = (int) func_args[2];
    int tileSize = (int) func_args[3];
    ocrGuid_t out_lkji_jjkp1_event_guid = (ocrGuid_t) func_args[4];
    ocrGuid_t*** lkji_db = (ocrGuid_t***) func_args[5];

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

    // out_lkji_jjkp1_event
    ocrEventCreate(&out_lkji_jjkp1_event_guid, OCR_EVENT_STICKY_T, true);

    ocrGuid_t out_lkji_jjkp1_db_guid;
    ocrDbCreate( &out_lkji_jjkp1_db_guid, &aBlock, sizeof(double)*tileSize*tileSize, FLAGS );

    // TODO sagnak lkji_db[j][j][k+1] = out_lkji_jjkp1_db_guid;
    ocrEventSatisfy(out_lkji_jjkp1_event_guid, out_lkji_jjkp1_db_guid);

    free(l2Block2D);
    free(aBlock2D);
}

u8 update_nondiagonal_task ( u32 paramc, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    double temp;
    int jB, kB, iB;

    intptr_t *func_args = (intptr_t *)(depv[3].ptr);
    int k = (int) func_args[0];
    int j = (int) func_args[1];
    int i = (int) func_args[2];
    int tileSize = (int) func_args[3];
    ocrGuid_t out_lkji_jikp1_event_guid = (ocrGuid_t) func_args[4];
    ocrGuid_t*** lkji_db = (ocrGuid_t***) func_args[5];

    double** aBlock = (double*) (depv[0].ptr);
    double** aBlock2D = (double**) malloc(sizeof(double*)*tileSize);
    for( index = 0; index < tileSize; ++index )
        aBlock2D[index] = &(aBlock[index*tileSize]);

    double** l1Block = (double*) (depv[1].ptr);
    double** l1Block2D = (double**) malloc(sizeof(double*)*tileSize);
    for( index = 0; index < tileSize; ++index )
        l1Block2D[index] = &(l1Block[index*tileSize]);

    double** l2Block = (double*) (depv[2].ptr);
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

    ocrEventCreate(&out_lkji_jikp1_event_guid, OCR_EVENT_STICKY_T, true);

    ocrGuid_t out_lkji_jikp1_db_guid;
    ocrDbCreate( &out_lkji_jikp1_db_guid, &aBlock, sizeof(double)*tileSize*tileSize, FLAGS );

    // TODO sagnak lkji_db[j][i][k+1] = out_lkji_jikp1_db_guid;
    ocrEventSatisfy(out_lkji_jikp1_event_guid, out_lkji_jikp1_db_guid);

    free(l2Block2D);
    free(l1Block2D);
    free(aBlock2D);
}

u8 wrap_up_task ( u32 paramc, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int i, j, k, i_b, j_b;
    double** temp;
        FILE* out = fopen("cholesky.out", "w");

    intptr_t *func_args = (intptr_t *)(depv[1].ptr);
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
    ocrFinish();
}

int main( int argc, char* argv[] ) {
    OCR_INIT(&argc, argv, sequential_cholesky_task, trisolve_task, update_nondiagonal_task, update_diagonal_task, wrap_up_task);

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
            ocrGuid_t seq_cholesky_task_guid;
            ocrEdtCreate(&seq_cholesky_task_guid, sequential_cholesky_task, 4, (void*)&func_args, PROPERTIES, 2, NULL);

            ocrAddDependence(lkji_events[k][k][k], seq_cholesky_task_guid, 0);

            ocrGuid_t func_args_event_guid;
            ocrEventCreate(&(func_args_event_guid), OCR_EVENT_STICKY_T, true);
            ocrAddDependence(func_args_event_guid, seq_cholesky_task_guid, 0);
            ocrEdtSchedule(seq_cholesky_task_guid);

            intptr_t *func_args = (intptr_t *)malloc(4*sizeof(intptr_t));
            func_args[0]=k;
            func_args[1]=tileSize;
            func_args[2]=(ocrGuid_t)lkji_events[k][k][k+1];
            func_args[3]=(intptr_t)lkji_db;

            ocrGuid_t func_args_db_guid;
            ocrDbCreate( &func_args_db_guid, &func_args, 4*sizeof(intptr_t), FLAGS );

            ocrEventSatisfy(func_args_event_guid, func_args_db_guid);
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
