/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr.h"
#include <stdlib.h>
#include <math.h>

#ifdef TG_ARCH
#include <misc.h>
#else
#include <stdio.h>
static double** readMatrix( int matrixSize, FILE* in );
#endif


// Uncomment the below two lines to include EDT profiling information
// Also see OCR_ENABLE_EDT_PROFILING in common.mk
//#include "app-profiling.h"
//#include "db-weights.h"


#define FLAGS DB_PROP_NONE
#define PROPERTIES EDT_PROP_NONE

ocrGuid_t sequential_cholesky_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int index = 0, iB = 0, jB = 0, kB = 0;

    u64 *func_args = paramv;
    int k = (int) func_args[0];
    int tileSize = (int) func_args[1];
    ocrGuid_t out_lkji_kkkp1_event_guid = (ocrGuid_t) func_args[2];

    double* aBlock = (double*) (depv[0].ptr);

    PRINTF("RUNNING sequential_cholesky %d with 0x%llx to satisfy\n", k, (u64)(out_lkji_kkkp1_event_guid));

    void* lBlock_db;
    ocrGuid_t out_lkji_kkkp1_db_guid;
    ocrGuid_t out_lkji_kkkp1_db_affinity = NULL_GUID;

    ocrDbCreate(&out_lkji_kkkp1_db_guid, &lBlock_db,
             sizeof(double)*tileSize*tileSize, FLAGS, out_lkji_kkkp1_db_affinity, NO_ALLOC);

    double* lBlock = (double*) lBlock_db;

    for( kB = 0 ; kB < tileSize ; ++kB ) {
        if( aBlock[kB*tileSize+kB] <= 0 ) {
            PRINTF("Not a symmetric positive definite (SPD) matrix, got %f for %d\n", aBlock[kB*tileSize+kB], kB);
            ASSERT(0);
            ocrShutdown();
            return NULL_GUID;
        } else {
            lBlock[kB*tileSize+kB] = sqrt(aBlock[kB*tileSize+kB]);
        }

        for(jB = kB + 1; jB < tileSize ; ++jB ) {
            lBlock[jB*tileSize+kB] = aBlock[jB*tileSize+kB]/lBlock[kB*tileSize+kB];
        }

        for(jB = kB + 1; jB < tileSize ; ++jB ) {
            for(iB = jB ; iB < tileSize ; ++iB ) {
                aBlock[iB*tileSize+jB] -= lBlock[iB*tileSize+kB] * lBlock[jB*tileSize+kB];
            }
        }
    }

    ocrEventSatisfy(out_lkji_kkkp1_event_guid, out_lkji_kkkp1_db_guid);

    return NULL_GUID;
}

ocrGuid_t trisolve_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int index, iB, jB, kB;

    u64 *func_args = paramv;
    int k = (int) func_args[0];
    int j = (int) func_args[1];
    int tileSize = (int) func_args[2];
    ocrGuid_t out_lkji_jkkp1_event_guid = (ocrGuid_t) func_args[3];

    PRINTF("RUNNING trisolve (%d, %d)\n", k, j);

    double* aBlock = (double*) (depv[0].ptr);

    double* liBlock = (double*) (depv[1].ptr);

    ocrGuid_t out_lkji_jkkp1_db_guid;
    ocrGuid_t out_lkji_jkkp1_db_affinity = NULL_GUID;
    void* loBlock_db;

    ocrDbCreate(&out_lkji_jkkp1_db_guid, &loBlock_db,
             sizeof(double)*tileSize*tileSize, FLAGS, out_lkji_jkkp1_db_affinity, NO_ALLOC);

    double * loBlock = (double*) loBlock_db;

    for( kB = 0; kB < tileSize ; ++kB ) {
        for( iB = 0; iB < tileSize ; ++iB ) {
            loBlock[iB*tileSize+kB] = aBlock[iB*tileSize+kB] / liBlock[kB*tileSize+kB];
        }

        for( jB = kB + 1 ; jB < tileSize; ++jB ) {
            for( iB = 0; iB < tileSize; ++iB ) {
                aBlock[iB*tileSize+jB] -= liBlock[jB*tileSize+kB] * loBlock[iB*tileSize+kB];
            }
        }
    }

    ocrEventSatisfy(out_lkji_jkkp1_event_guid, out_lkji_jkkp1_db_guid);

    return NULL_GUID;
}

ocrGuid_t update_diagonal_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int index, iB, jB, kB;
    double temp = 0;

    u64 *func_args = paramv;
    int k = (int) func_args[0];
    int j = (int) func_args[1];
    int i = (int) func_args[2];
    int tileSize = (int) func_args[3];
    ocrGuid_t out_lkji_jjkp1_event_guid = (ocrGuid_t) func_args[4];

    PRINTF("RUNNING update_diagonal (%d, %d, %d)\n", k, j, i);

    double* aBlock = (double*) (depv[0].ptr);

    double* l2Block = (double*) (depv[1].ptr);

    for( jB = 0; jB < tileSize ; ++jB ) {
        for( kB = 0; kB < tileSize ; ++kB ) {
            temp = 0 - l2Block[jB*tileSize+kB];
            for( iB = jB; iB < tileSize; ++iB ) {
                aBlock[iB*tileSize+jB] += temp * l2Block[iB*tileSize+kB];
            }
        }
    }

    ocrEventSatisfy(out_lkji_jjkp1_event_guid, depv[0].guid);

    return NULL_GUID;
}

ocrGuid_t update_nondiagonal_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    double temp;
    int index, jB, kB, iB;

    u64 *func_args = paramv;
    int k = (int) func_args[0];
    int j = (int) func_args[1];
    int i = (int) func_args[2];
    int tileSize = (int) func_args[3];
    ocrGuid_t out_lkji_jikp1_event_guid = (ocrGuid_t) func_args[4];

    PRINTF("RUNNING update_nondiagonal (%d, %d, %d)\n", k, j, i);

    double* aBlock = (double*) (depv[0].ptr);

    double* l1Block = (double*) (depv[1].ptr);

    double* l2Block = (double*) (depv[2].ptr);

    for( jB = 0; jB < tileSize ; ++jB ) {
        for( kB = 0; kB < tileSize ; ++kB ) {
            temp = 0 - l2Block[jB*tileSize+kB];
            for(iB = 0; iB < tileSize; ++iB)
                aBlock[iB*tileSize+jB] += temp * l1Block[iB*tileSize+kB];
        }
    }

    ocrEventSatisfy(out_lkji_jikp1_event_guid, depv[0].guid);

    return NULL_GUID;
}

ocrGuid_t wrap_up_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int i, j, i_b, j_b;
    double* temp;

#ifdef RMD
    VERIFY(1, "Done with cholesky, shutdown (TODO: Dump the output)\n");
#else
    FILE* out = fopen("cholesky.out", "w");

    u64 *func_args = paramv;
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
#endif

    ocrShutdown();

    return NULL_GUID;
}

inline static void sequential_cholesky_task_prescriber (ocrGuid_t edtTemp, int k,
        int tileSize, ocrGuid_t*** lkji_event_guids) {
    ocrGuid_t seq_cholesky_task_guid;

    u64 func_args[3];
    func_args[0] = k;
    func_args[1] = tileSize;
    func_args[2] = (u64)(lkji_event_guids[k][k][k+1]);

    ocrGuid_t affinity = NULL_GUID;
    ocrEdtCreate(&seq_cholesky_task_guid, edtTemp, 3, func_args, 1, NULL, PROPERTIES, affinity, NULL);

    ocrAddDependence(lkji_event_guids[k][k][k], seq_cholesky_task_guid, 0, DB_MODE_ITW);
}

inline static void trisolve_task_prescriber ( ocrGuid_t edtTemp, int k, int j, int tileSize,
        ocrGuid_t*** lkji_event_guids) {
    ocrGuid_t trisolve_task_guid;

    u64 func_args[4];
    func_args[0] = k;
    func_args[1] = j;
    func_args[2] = tileSize;
    func_args[3] = (u64)(lkji_event_guids[j][k][k+1]);


    ocrGuid_t affinity = NULL_GUID;
    ocrEdtCreate(&trisolve_task_guid, edtTemp, 4, func_args, 2, NULL, PROPERTIES, affinity, NULL);

    ocrAddDependence(lkji_event_guids[j][k][k], trisolve_task_guid, 0, DB_MODE_ITW);
    ocrAddDependence(lkji_event_guids[k][k][k+1], trisolve_task_guid, 1, DB_MODE_ITW);
}

inline static void update_nondiagonal_task_prescriber ( ocrGuid_t edtTemp, int k, int j, int i,
        int tileSize, ocrGuid_t*** lkji_event_guids) {
    ocrGuid_t update_nondiagonal_task_guid;

    u64 func_args[5];
    func_args[0] = k;
    func_args[1] = j;
    func_args[2] = i;
    func_args[3] = tileSize;
    func_args[4] = (u64)(lkji_event_guids[j][i][k+1]);

    ocrGuid_t affinity = NULL_GUID;
    ocrEdtCreate(&update_nondiagonal_task_guid, edtTemp, 5, func_args, 3, NULL, PROPERTIES, affinity, NULL);

    ocrAddDependence(lkji_event_guids[j][i][k], update_nondiagonal_task_guid, 0, DB_MODE_ITW);
    ocrAddDependence(lkji_event_guids[j][k][k+1], update_nondiagonal_task_guid, 1, DB_MODE_ITW);
    ocrAddDependence(lkji_event_guids[i][k][k+1], update_nondiagonal_task_guid, 2, DB_MODE_ITW);
}


inline static void update_diagonal_task_prescriber ( ocrGuid_t edtTemp, int k, int j, int i,
        int tileSize, ocrGuid_t*** lkji_event_guids) {
    ocrGuid_t update_diagonal_task_guid;

    u64 func_args[5];
    func_args[0] = k;
    func_args[1] = j;
    func_args[2] = i;
    func_args[3] = tileSize;
    func_args[4] = (u64)(lkji_event_guids[j][j][k+1]);

    ocrGuid_t affinity = NULL_GUID;
    ocrEdtCreate(&update_diagonal_task_guid, edtTemp, 5, func_args, 2, NULL, PROPERTIES, affinity, NULL);

    ocrAddDependence(lkji_event_guids[j][j][k], update_diagonal_task_guid, 0, DB_MODE_ITW);
    ocrAddDependence(lkji_event_guids[j][k][k+1], update_diagonal_task_guid, 1, DB_MODE_ITW);
}

inline static void wrap_up_task_prescriber ( ocrGuid_t edtTemp, int numTiles, int tileSize,
        ocrGuid_t*** lkji_event_guids ) {
    int i,j,k;
    ocrGuid_t wrap_up_task_guid;

    u64 func_args[2];
    func_args[0]=(int)numTiles;
    func_args[1]=(int)tileSize;

    ocrGuid_t affinity = NULL_GUID;
    ocrEdtCreate(&wrap_up_task_guid, edtTemp, 2, func_args, (numTiles+1)*numTiles/2, NULL, PROPERTIES, affinity, NULL);

    int index = 0;
    for ( i = 0; i < numTiles; ++i ) {
        k = 1;
        for ( j = 0; j <= i; ++j ) {
            ocrAddDependence(lkji_event_guids[i][j][k], wrap_up_task_guid, index++, DB_MODE_ITW);
            ++k;
        }
    }
}

inline static ocrGuid_t*** allocateCreateEvents ( int numTiles ) {
    int i,j,k;
    ocrGuid_t*** lkji_event_guids;
    void* lkji_event_guids_db = NULL;
    ocrGuid_t lkji_event_guids_db_guid;

    ocrDbCreate(&lkji_event_guids_db_guid, &lkji_event_guids_db, sizeof(ocrGuid_t **)*numTiles,
                 FLAGS, NULL_GUID, NO_ALLOC);

    lkji_event_guids = (ocrGuid_t ***) (lkji_event_guids_db);
    for( i = 0 ; i < numTiles ; ++i ) {
        ocrDbCreate(&lkji_event_guids_db_guid, (void *)&lkji_event_guids[i],
                    sizeof(ocrGuid_t *)*(i+1),
                    FLAGS, NULL_GUID, NO_ALLOC);
        for( j = 0 ; j <= i ; ++j ) {
            ocrDbCreate(&lkji_event_guids_db_guid, (void *)&lkji_event_guids[i][j],
                    sizeof(ocrGuid_t)*(numTiles+1),
                    FLAGS, NULL_GUID, NO_ALLOC);
            for( k = 0 ; k <= numTiles ; ++k ) {
                ocrEventCreate(&(lkji_event_guids[i][j][k]), OCR_EVENT_STICKY_T, TRUE);
}
        }
    }

    return lkji_event_guids;
}

#ifdef RMD
inline static void satisfyInitialTiles(int numTiles, int tileSize, u64 startBlob,
                                       ocrGuid_t*** lkji_event_guids) {
    int i,j;
    int T_i, T_j;
    ocrGuid_t db_guid;
    ocrGuid_t db_affinity;
    void* temp_db;

    for( i = 0 ; i < numTiles ; ++i ) {
        for( j = 0 ; j <= i ; ++j ) {
            ocrDbCreate(&db_guid, &temp_db, sizeof(double)*tileSize*tileSize,
                     FLAGS, db_affinity, NO_ALLOC);

            hal_memCopy(temp_db, startBlob, sizeof(double)*tileSize*tileSize, 1);
            ocrEventSatisfy(lkji_event_guids[i][j][0], db_guid);
            ocrDbRelease(db_guid);
            startBlob += tileSize*tileSize*sizeof(double);
        }
    }
    hal_fence();

}
#else
inline static void satisfyInitialTiles(int numTiles, int tileSize, double** matrix,
                                       ocrGuid_t*** lkji_event_guids) {
    int i,j,index;
    int A_i, A_j, T_i, T_j;

    for( i = 0 ; i < numTiles ; ++i ) {
        for( j = 0 ; j <= i ; ++j ) {
            ocrGuid_t db_guid;
            ocrGuid_t db_affinity = NULL_GUID;
            void* temp_db;
            ocrGuid_t tmpdb_guid;
            ocrDbCreate(&db_guid, &temp_db, sizeof(double)*tileSize*tileSize,
                     FLAGS, db_affinity, NO_ALLOC);

            double* temp = (double*) temp_db;
            double** temp2D;

            ocrDbCreate(&tmpdb_guid, (void *)&temp2D, sizeof(double*)*tileSize,
                        FLAGS, NULL_GUID, NO_ALLOC);

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
            ocrDbDestroy(tmpdb_guid);
        }
    }
}
#endif

ocrGuid_t mainEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    int matrixSize = -1;
    int tileSize = -1;
    int numTiles = -1;
    int i, j, k;
    double **matrix, ** temp;
    u64 argc;
    u64 offsets[4];

    void *programArg = depv[0].ptr;
    u64* dbAsU64 = (u64*)programArg;
    argc = dbAsU64[0];

    if ( argc !=  4 ) {
        PRINTF("Usage: ./cholesky matrixSize tileSize fileName (found %lld args)\n", argc);
        ocrShutdown();
        return 1;
    }

    for (i=0; i< argc; i++) {
        offsets[i] = dbAsU64[i+1];
    }
    char *dbAsChar = (char*)programArg;

    matrixSize = atoi(dbAsChar+offsets[1]);
    tileSize = atoi(dbAsChar+offsets[2]);

    if ( matrixSize % tileSize != 0 ) {
        PRINTF("Incorrect tile size %d for the matrix of size %d \n", tileSize, matrixSize);
        ocrShutdown();
        return NULL_GUID;
    }
    PRINTF("Matrixsize %d, tileSize %d\n", matrixSize, tileSize);

#ifndef RMD
    FILE *in;
    in = fopen(dbAsChar+offsets[3], "r");
    if( !in ) {
        PRINTF("Cannot find file: %s\n", dbAsChar+offsets[3]);
        ocrShutdown();
        return NULL_GUID;
    }
    matrix = readMatrix( matrixSize, in );
#endif

    numTiles = matrixSize/tileSize;

    ocrGuid_t*** lkji_event_guids = allocateCreateEvents(numTiles);

    ocrGuid_t templateSeq, templateTrisolve,
              templateUpdateNonDiag, templateUpdate, templateWrap;

    ocrEdtTemplateCreate(&templateSeq, sequential_cholesky_task, 3, 1);
    ocrEdtTemplateCreate(&templateTrisolve, trisolve_task, 4, 2);
    ocrEdtTemplateCreate(&templateUpdateNonDiag, update_nondiagonal_task, 5, 3);
    ocrEdtTemplateCreate(&templateUpdate, update_diagonal_task, 5, 2);
    ocrEdtTemplateCreate(&templateWrap, wrap_up_task, 2, (numTiles+1)*numTiles/2);

    PRINTF("Going to satisfy initial tiles\n");
#ifdef RMD
    satisfyInitialTiles( numTiles, tileSize, BLOB_START, lkji_event_guids);
#else
    satisfyInitialTiles( numTiles, tileSize, matrix, lkji_event_guids);
#endif

    for ( k = 0; k < numTiles; ++k ) {
        PRINTF("Prescribing sequential task %d\n", k);
        sequential_cholesky_task_prescriber ( templateSeq, k, tileSize,
                                              lkji_event_guids);

        for( j = k + 1 ; j < numTiles ; ++j ) {
            trisolve_task_prescriber ( templateTrisolve,
                                       k, j, tileSize, lkji_event_guids);

            for( i = k + 1 ; i < j ; ++i ) {
                update_nondiagonal_task_prescriber ( templateUpdateNonDiag,
                                                     k, j, i, tileSize, lkji_event_guids);
            }
            update_diagonal_task_prescriber ( templateUpdate,
                                              k, j, i, tileSize, lkji_event_guids);
        }
    }

    wrap_up_task_prescriber ( templateWrap, numTiles, tileSize, lkji_event_guids );

    PRINTF("Wrapping up mainEdt\n");
    return NULL_GUID;
}

#ifndef RMD
static double** readMatrix( int matrixSize, FILE* in ) {
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
#endif
