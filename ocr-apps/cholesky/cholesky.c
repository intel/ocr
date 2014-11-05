/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifndef TG_ARCH
#include <getopt.h>
#include <string.h>
#include <time.h>
static double** readMatrix( u32 matrixSize, FILE* in );

#else
int compare_string(char *, char *);
#endif


// Uncomment the below two lines to include EDT profiling information
// Also see OCR_ENABLE_EDT_PROFILING in common.mk
//#include "app-profiling.h"
//#include "db-weights.h"


#define FLAGS DB_PROP_NONE
#define PROPERTIES EDT_PROP_NONE

ocrGuid_t sequential_cholesky_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u32 index = 0, iB = 0, jB = 0, kB = 0;

    u64 *func_args = paramv;
    u32 k = (u32) func_args[0];
    u32 tileSize = (u32) func_args[1];
    u32 printStatOut = (u32) func_args[2];
    ocrGuid_t out_lkji_kkkp1_event_guid = (ocrGuid_t) func_args[3];

    double* aBlock = (double*) (depv[0].ptr);

    if (printStatOut == 1) PRINTF("RUNNING sequential_cholesky %d with 0x%llx to satisfy\n", k, (u64)(out_lkji_kkkp1_event_guid));

    void* lBlock_db;
    ocrGuid_t out_lkji_kkkp1_db_guid;
    ocrGuid_t out_lkji_kkkp1_db_affinity = NULL_GUID;

    ocrDbCreate(&out_lkji_kkkp1_db_guid, &lBlock_db,
             sizeof(double)*tileSize*tileSize, FLAGS, out_lkji_kkkp1_db_affinity, NO_ALLOC);

    double* lBlock = (double*) lBlock_db;

    for( kB = 0 ; kB < tileSize ; ++kB ) {
        if( aBlock[kB*tileSize+kB] <= 0 ) {
            PRINTF("Not a symmetric positive definite (SPD) matrix, got %f for %d\n", aBlock[kB*tileSize+kB], kB);
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
    u32 index, iB, jB, kB;

    u64 *func_args = paramv;
    u32 k = (u32) func_args[0];
    u32 j = (u32) func_args[1];
    u32 tileSize = (u32) func_args[2];
    u32 printStatOut = (u32) func_args[3];
    ocrGuid_t out_lkji_jkkp1_event_guid = (ocrGuid_t) func_args[4];

    if (printStatOut == 1) PRINTF("RUNNING trisolve (%d, %d)\n", k, j);

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
    u32 index, iB, jB, kB;
    double temp = 0;

    u64 *func_args = paramv;
    u32 k = (u32) func_args[0];
    u32 j = (u32) func_args[1];
    u32 i = (u32) func_args[2];
    u32 tileSize = (u32) func_args[3];
    u32 printStatOut = (u32) func_args[4];
    ocrGuid_t out_lkji_jjkp1_event_guid = (ocrGuid_t) func_args[5];

    if (printStatOut == 1) PRINTF("RUNNING update_diagonal (%d, %d, %d)\n", k, j, i);

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
    u32 index, jB, kB, iB;

    u64 *func_args = paramv;
    u32 k = (u32) func_args[0];
    u32 j = (u32) func_args[1];
    u32 i = (u32) func_args[2];
    u32 tileSize = (u32) func_args[3];
    u32 printStatOut = (u32) func_args[4];
    ocrGuid_t out_lkji_jikp1_event_guid = (ocrGuid_t) func_args[5];

    if (printStatOut == 1) PRINTF("RUNNING update_nondiagonal (%d, %d, %d)\n", k, j, i);

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
    u32 i, j, i_b, j_b;
    double* temp;

    u64 *func_args = paramv;
    u32 numTiles = (u32) func_args[0];
    u32 tileSize = (u32) func_args[1];

#ifndef TG_ARCH
    u32 outSelLevel = (u32) func_args[2];

    struct timeval a;
    if (outSelLevel == 5)
    {
        FILE* outCSV = fopen("ocr_cholesky_stats.csv", "a");
        if (!outCSV)
        {
            PRINTF("Cannot find file: %s\n", "ocr_cholesky_stats.csv");
            ocrShutdown();
            return NULL_GUID;
        }

        gettimeofday(&a, 0);
        fprintf(outCSV, "%f\n", (a.tv_sec * 1000000 + a.tv_usec) * 1.0 / 1000000);
        fclose(outCSV);
        outSelLevel = 2;
    }
#endif

    FILE* out = fopen("cholesky.out", "w");

    for ( i = 0; i < numTiles; ++i ) {
        for( i_b = 0; i_b < tileSize; ++i_b) {
            for ( j = 0; j <= i; ++j ) {
                temp = (double*) (depv[i*(i+1)/2+j].ptr);
                if(i != j) {
                    for(j_b = 0; j_b < tileSize; ++j_b) {

                        #ifndef TG_ARCH
                        switch(outSelLevel)
                        {
                        case 0:
                            printf("%lf ", temp[i_b*tileSize+j_b]);
                            break;
                        case 1:
                            fprintf(out, "%lf ", temp[i_b*tileSize+j_b]);
                            break;
                        case 2:
                            fwrite(&temp[i_b*tileSize+j_b], sizeof(double), 1, out);
                            break;
                        case 3:
                            fprintf(out, "%lf ", temp[i_b*tileSize+j_b]);
                            printf("%lf ", temp[i_b*tileSize+j_b]);
                            break;
                        case 4:
                            fwrite(&temp[i_b*tileSize+j_b], sizeof(double), 1, out);
                            printf("%lf ", temp[i_b*tileSize+j_b]);
                            break;
                        }
                        #else
                        fwrite(&temp[i_b*tileSize+j_b], sizeof(double), 1, out);
                        #endif
                    }
                } else {
                    for(j_b = 0; j_b <= i_b; ++j_b) {

                        #ifndef TG_ARCH
                        switch(outSelLevel)
                        {
                        case 0:
                            printf("%lf ", temp[i_b*tileSize+j_b]);
                            break;
                        case 1:
                            fprintf(out, "%lf ", temp[i_b*tileSize+j_b]);
                            break;
                        case 2:
                            fwrite(&temp[i_b*tileSize+j_b], sizeof(double), 1, out);
                            break;
                        case 3:
                            fprintf(out, "%lf ", temp[i_b*tileSize+j_b]);
                            printf("%lf ", temp[i_b*tileSize+j_b]);
                            break;
                        case 4:
                            fwrite(&temp[i_b*tileSize+j_b], sizeof(double), 1, out);
                            printf("%lf ", temp[i_b*tileSize+j_b]);
                            break;
                        }
                        #else
                        fwrite(&temp[i_b*tileSize+j_b], sizeof(double), 1, out);
                        #endif
                    }

                }
            }
        }
    }
    fclose(out);

    ocrShutdown();

    return NULL_GUID;
}

inline static void sequential_cholesky_task_prescriber (ocrGuid_t edtTemp, u32 k, u32 tileSize,
                                                        u32 printStatOut, ocrGuid_t*** lkji_event_guids) {
    ocrGuid_t seq_cholesky_task_guid;

    u64 func_args[4];
    func_args[0] = k;
    func_args[1] = tileSize;
    func_args[2] = printStatOut;
    func_args[3] = (u64)(lkji_event_guids[k][k][k+1]);

    ocrGuid_t affinity = NULL_GUID;
    ocrEdtCreate(&seq_cholesky_task_guid, edtTemp, 4, func_args, 1, NULL, PROPERTIES, affinity, NULL);

    ocrAddDependence(lkji_event_guids[k][k][k], seq_cholesky_task_guid, 0, DB_MODE_ITW);
}

inline static void trisolve_task_prescriber ( ocrGuid_t edtTemp, u32 k, u32 j, u32 tileSize,
                                              u32 printStatOut, ocrGuid_t*** lkji_event_guids) {
    ocrGuid_t trisolve_task_guid;

    u64 func_args[5];
    func_args[0] = k;
    func_args[1] = j;
    func_args[2] = tileSize;
    func_args[3] = printStatOut;
    func_args[4] = (u64)(lkji_event_guids[j][k][k+1]);

    ocrGuid_t affinity = NULL_GUID;
    ocrEdtCreate(&trisolve_task_guid, edtTemp, 5, func_args, 2, NULL, PROPERTIES, affinity, NULL);

    ocrAddDependence(lkji_event_guids[j][k][k], trisolve_task_guid, 0, DB_MODE_ITW);
    ocrAddDependence(lkji_event_guids[k][k][k+1], trisolve_task_guid, 1, DB_MODE_ITW);
}

inline static void update_nondiagonal_task_prescriber ( ocrGuid_t edtTemp, u32 k, u32 j, u32 i, u32 tileSize,
                                                        u32 printStatOut, ocrGuid_t*** lkji_event_guids) {
    ocrGuid_t update_nondiagonal_task_guid;

    u64 func_args[6];
    func_args[0] = k;
    func_args[1] = j;
    func_args[2] = i;
    func_args[3] = tileSize;
    func_args[4] = printStatOut;
    func_args[5] = (u64)(lkji_event_guids[j][i][k+1]);

    ocrGuid_t affinity = NULL_GUID;
    ocrEdtCreate(&update_nondiagonal_task_guid, edtTemp, 6, func_args, 3, NULL, PROPERTIES, affinity, NULL);

    ocrAddDependence(lkji_event_guids[j][i][k], update_nondiagonal_task_guid, 0, DB_MODE_ITW);
    ocrAddDependence(lkji_event_guids[j][k][k+1], update_nondiagonal_task_guid, 1, DB_MODE_ITW);
    ocrAddDependence(lkji_event_guids[i][k][k+1], update_nondiagonal_task_guid, 2, DB_MODE_ITW);
}


inline static void update_diagonal_task_prescriber ( ocrGuid_t edtTemp, u32 k, u32 j, u32 i, u32 tileSize,
                                                     u32 printStatOut, ocrGuid_t*** lkji_event_guids) {
    ocrGuid_t update_diagonal_task_guid;

    u64 func_args[6];
    func_args[0] = k;
    func_args[1] = j;
    func_args[2] = i;
    func_args[3] = tileSize;
    func_args[4] = printStatOut;
    func_args[5] = (u64)(lkji_event_guids[j][j][k+1]);

    ocrGuid_t affinity = NULL_GUID;
    ocrEdtCreate(&update_diagonal_task_guid, edtTemp, 6, func_args, 2, NULL, PROPERTIES, affinity, NULL);

    ocrAddDependence(lkji_event_guids[j][j][k], update_diagonal_task_guid, 0, DB_MODE_ITW);
    ocrAddDependence(lkji_event_guids[j][k][k+1], update_diagonal_task_guid, 1, DB_MODE_ITW);
}

inline static void wrap_up_task_prescriber ( ocrGuid_t edtTemp, u32 numTiles, u32 tileSize,
                                             u32 outSelLevel, ocrGuid_t*** lkji_event_guids ) {
    u32 i,j,k;
    ocrGuid_t wrap_up_task_guid;

    u64 func_args[3];
    func_args[0]=(u32)numTiles;
    func_args[1]=(u32)tileSize;
    func_args[2]=(u32)outSelLevel;

    ocrGuid_t affinity = NULL_GUID;
    ocrEdtCreate(&wrap_up_task_guid, edtTemp, 3, func_args, (numTiles+1)*numTiles/2, NULL, PROPERTIES, affinity, NULL);

    u32 index = 0;
    for ( i = 0; i < numTiles; ++i ) {
        k = 1;
        for ( j = 0; j <= i; ++j ) {
            ocrAddDependence(lkji_event_guids[i][j][k], wrap_up_task_guid, index++, DB_MODE_ITW);
            ++k;
        }
    }
}

inline static ocrGuid_t*** allocateCreateEvents ( u32 numTiles ) {
    u32 i,j,k;
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

#ifdef TG_ARCH
inline static void satisfyInitialTiles(u32 numTiles, u32 tileSize,
                                       ocrGuid_t*** lkji_event_guids) {
    u32 i,j;
    u32 T_i, T_j;
    ocrGuid_t db_guid;
    ocrGuid_t db_affinity;
    void* temp_db;
    FILE *fin;

    fin = fopen("inputfile", "r");
    if(fin == NULL) PRINTF("Error opening input file\n");
    for( i = 0 ; i < numTiles ; ++i ) {
        for( j = 0 ; j <= i ; ++j ) {
            ocrDbCreate(&db_guid, &temp_db, sizeof(double)*tileSize*tileSize,
                     FLAGS, db_affinity, NO_ALLOC);

            fread(temp_db, sizeof(double)*tileSize*tileSize, 1, fin);
            ocrEventSatisfy(lkji_event_guids[i][j][0], db_guid);
            ocrDbRelease(db_guid);
        }
    }
    hal_fence();
    fclose(fin);

}
#else
inline static void satisfyInitialTiles(u32 numTiles, u32 tileSize, double** matrix,
                                       ocrGuid_t*** lkji_event_guids) {
    u32 i,j,index;
    u32 A_i, A_j, T_i, T_j;

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

            // Split the matrix u32o tiles and write it u32o the item space at time 0.
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
    u32 matrixSize = -1;
    u32 tileSize = -1;
    u32 numTiles = -1;
    u32 i, j, k, c;
    u32 outSelLevel = 2;
    u32 printStatOut = 0;
    double **matrix, ** temp;
    u64 argc;

    void *programArg = depv[0].ptr;
    argc = getArgc(programArg);

    char *nparamv[argc];
    char *fileNameIn, *fileNameOut = "cholesky.out";

#ifndef TG_ARCH

    for (i=0; i< argc; i++)
    {
        nparamv[i] = getArgv(programArg, i);
    }


    if ( argc == 1)
    {
        PRINTF("Cholesky\n");
        PRINTF("__________________________________________________________________________________________________\n");
        PRINTF("Solves an OCR version of a Tiled Cholesky Decomposition with non-MKL math kernels\n\n");
        PRINTF("Usage:\n");
        PRINTF("\tcholesky {Arguments}\n\n");
        PRINTF("Arguments:\n");
        PRINTF("\t--ds -- Specify the Size of the Input Matrix\n");
        PRINTF("\t--ts -- Specify the Tile Size\n");
        PRINTF("\t--fi -- Specify the Input File Name of the Matrix\n");
        PRINTF("\t--ps -- Print status updates on algorithm computation to stdout\n");
        PRINTF("\t\t0: Disable (default)\n");
        PRINTF("\t\t1: Enable\n");
        PRINTF("\t--ol -- Output Selection Level:\n");
        PRINTF("\t\t0: Print solution to stdout\n");
        PRINTF("\t\t1: Write solution to text file\n");
        PRINTF("\t\t2: Write solution to binary file (default)\n");
        PRINTF("\t\t3: Write solution to text file and print to stdout\n");
        PRINTF("\t\t4: Write solution to binary file and print to stdout\n");
        PRINTF("\t\t5: Write algorithm timing data to ocr_cholesky_stats.csv and write solution to binary file\n");

        ocrShutdown();
        return NULL_GUID;
    }
    else
    {
        // Reads in 5 arguments, datasize, tilesize, input matrix file name, and output matrix filename
        while (1)
        {
            static struct option long_options[] =
                {
                    {"ds", required_argument, 0, 'a'},
                    {"ts", required_argument, 0, 'b'},
                    {"fi", required_argument, 0, 'c'},
                    {"ps", required_argument, 0, 'd'},
                    {"ol", required_argument, 0, 'e'},
                    {0, 0, 0, 0}
                };

            u32 option_index = 0;

            c = getopt_long(argc, nparamv, "a:b:c:d:e", long_options, &option_index);

            if (c == -1) // Detect the end of the options
                break;
            switch (c)
            {
            case 'a':
                //PRINTF("Option a: matrixSize with value '%s'\n", optarg);
                matrixSize = (u32) atoi(optarg);
                break;
            case 'b':
                //PRINTF("Option b: tileSize with value '%s'\n", optarg);
                tileSize = (u32) atoi(optarg);
                break;
            case 'c':
                //PRINTF("Option c: fileNameIn with value '%s'\n", optarg);
                fileNameIn = optarg;
                break;
            case 'd':
                //PRINTF("Option d: printStatOut with value '%s'\n", optarg);
                printStatOut = (u32) atoi(optarg);
                break;
            case 'e':
                //PRINTF("Option e: outSelLevel with value '%s'\n", optarg);
                outSelLevel = (u32) atoi(optarg);
                break;
            default:
                PRINTF("ERROR: Invalid argument switch\n\n");
                PRINTF("Cholesky\n");
                PRINTF("__________________________________________________________________________________________________\n");
                PRINTF("Solves an OCR-only version of a Tiled Cholesky Decomposition with non-MKL math kernels\n\n");
                PRINTF("Usage:\n");
                PRINTF("\tcholesky {Arguments}\n\n");
                PRINTF("Arguments:\n");
                PRINTF("\t--ds -- Specify the Size of the Input Matrix\n");
                PRINTF("\t--ts -- Specify the Tile Size\n");
                PRINTF("\t--fi -- Specify the Input File Name of the Matrix\n");
                PRINTF("\t--ps -- Print status updates on algorithm computation to stdout\n");
                PRINTF("\t\t0: Disable (default)\n");
                PRINTF("\t\t1: Enable\n");
                PRINTF("\t--ol -- Output Selection Level:\n");
                PRINTF("\t\t0: Print solution to stdout\n");
                PRINTF("\t\t1: Write solution to text file\n");
                PRINTF("\t\t2: Write solution to binary file (default)\n");
                PRINTF("\t\t3: Write solution to text file and print to stdout\n");
                PRINTF("\t\t4: Write solution to binary file and print to stdout\n");
                PRINTF("\t\t5: Write algorithm timing data to ocr_cholesky_stats.csv and write solution to binary file\n");

                ocrShutdown();
                return NULL_GUID;
            }
        }
    }

    if(matrixSize == -1 || tileSize == -1)
    {
        PRINTF("Must specify matrix size and tile size\n");
        ocrShutdown();
        return NULL_GUID;
    }
    else if(matrixSize % tileSize != 0)
    {
        PRINTF("Incorrect tile size %d for the matrix of size %d \n", tileSize, matrixSize);
        ocrShutdown();
        return NULL_GUID;
    }

    PRINTF("Matrixsize %d, tileSize %d\n", matrixSize, tileSize);
    numTiles = matrixSize/tileSize;

    struct timeval a;
    if(outSelLevel == 5)
    {

        FILE* outCSV = fopen("ocr_cholesky_stats.csv", "r");
        if( !outCSV )
        {

            outCSV = fopen("ocr_cholesky_stats.csv", "w");

            if( !outCSV ) {
                PRINTF("Cannot find file: %s\n", "ocr_cholesky_stats.csv");
                ocrShutdown();
                return NULL_GUID;
            }

            fprintf(outCSV, "MatrixSize,TileSize,NumTile,PreAllocTime,PreAlgorithmTime,PostAlgorithmTime\n");
        }
        else
        {
            outCSV = fopen("ocr_cholesky_stats.csv", "a+");
        }

        fprintf(outCSV, "%d,%d,%d,", matrixSize, tileSize, numTiles);
        gettimeofday(&a, 0);
        fprintf(outCSV, "%f,", (a.tv_sec*1000000+a.tv_usec)*1.0/1000000);
        fclose(outCSV);
    }

    FILE *in;
    in = fopen(fileNameIn, "r");
    if( !in ) {
        PRINTF("Cannot find file: %s\n", fileNameIn);
        ocrShutdown();
        return NULL_GUID;
    }

    matrix = readMatrix( matrixSize, in );

    if(outSelLevel == 5)
    {
        FILE* outCSV = fopen("ocr_cholesky_stats.csv", "a");
        if( !outCSV ) {
            PRINTF("Cannot find file: %s\n", "ocr_cholesky_stats.csv");
            ocrShutdown();
            return NULL_GUID;
        }
        gettimeofday(&a, 0);
        fprintf(outCSV, "%f,", (a.tv_sec*1000000+a.tv_usec)*1.0/1000000);
        fclose(outCSV);
    }

#else

    for (i=0; i < argc; i++)
    {
        if (compare_string(getArgv(programArg, i), "--ds")  == 0)
            matrixSize = (u32) atoi(getArgv(programArg, i+1));

        else if (compare_string(getArgv(programArg, i), "--ts")  == 0)
            tileSize = (u32) atoi(getArgv(programArg, i+1));

        else if (compare_string(getArgv(programArg, i), "--ps")  == 0)
            printStatOut = (u32) atoi(getArgv(programArg, i+1));

//        else if (compare_string(getArgv(programArg, i), "--ol") == 0)
//            outSelLevel = (u32) atoi(getArgv(programArg, i+1));
    }

    if ( matrixSize <= 0 || tileSize <= 0 )
    {
        PRINTF("ERROR: Invalid argument switch\n\n");
        PRINTF("Cholesky\n");
        PRINTF("__________________________________________________________________________________________________\n");
        PRINTF("Solves an OCR-only version of a Tiled Cholesky Decomposition with non-MKL math kernels\n\n");
        PRINTF("Usage:\n");
        PRINTF("\tcholesky {Arguments}\n\n");
        PRINTF("Arguments:\n");
        PRINTF("\t--ds -- Specify the Size of the Input Matrix\n");
        PRINTF("\t--ts -- Specify the Tile Size\n");
        PRINTF("\t--fi -- Specify the Input File Name of the Matrix\n");
        PRINTF("\t--ps -- Print status updates on algorithm computation to stdout\n");
        PRINTF("\t\t0: Disable (default)\n");
        PRINTF("\t\t1: Enable\n");

        ocrShutdown();
        return NULL_GUID;
    }

    else if(matrixSize % tileSize != 0)
    {
        PRINTF("Incorrect tile size %d for the matrix of size %d \n", tileSize, matrixSize);
        ocrShutdown();
        return NULL_GUID;
    }

    PRINTF("Matrixsize %d, tileSize %d\n", matrixSize, tileSize);
    numTiles = matrixSize/tileSize;
#endif

    ocrGuid_t*** lkji_event_guids = allocateCreateEvents(numTiles);

    ocrGuid_t templateSeq, templateTrisolve,
              templateUpdateNonDiag, templateUpdate, templateWrap;

    ocrEdtTemplateCreate(&templateSeq, sequential_cholesky_task, 4, 1);
    ocrEdtTemplateCreate(&templateTrisolve, trisolve_task, 5, 2);
    ocrEdtTemplateCreate(&templateUpdateNonDiag, update_nondiagonal_task, 6, 3);
    ocrEdtTemplateCreate(&templateUpdate, update_diagonal_task, 6, 2);
    ocrEdtTemplateCreate(&templateWrap, wrap_up_task, 3, (numTiles+1)*numTiles/2);


    if (printStatOut == 1) PRINTF("Going to satisfy initial tiles\n");
#ifdef TG_ARCH
    satisfyInitialTiles( numTiles, tileSize, lkji_event_guids);
#else
    satisfyInitialTiles( numTiles, tileSize, matrix, lkji_event_guids);
#endif

    for ( k = 0; k < numTiles; ++k ) {
        if (printStatOut == 1) PRINTF("Prescribing sequential task %d\n", k);
        sequential_cholesky_task_prescriber ( templateSeq, k, tileSize, printStatOut, lkji_event_guids);

        for( j = k + 1 ; j < numTiles ; ++j ) {
            trisolve_task_prescriber ( templateTrisolve, k, j, tileSize, printStatOut, lkji_event_guids);

            for( i = k + 1 ; i < j ; ++i ) {
                update_nondiagonal_task_prescriber ( templateUpdateNonDiag, k, j, i,
                                                     tileSize, printStatOut, lkji_event_guids);
            }
            update_diagonal_task_prescriber ( templateUpdate, k, j, i, tileSize,
                                              printStatOut, lkji_event_guids);
        }
    }

    wrap_up_task_prescriber ( templateWrap, numTiles, tileSize, outSelLevel, lkji_event_guids );

    if (printStatOut == 1) PRINTF("Wrapping up mainEdt\n");
    return NULL_GUID;
}

#ifndef TG_ARCH
static double** readMatrix( u32 matrixSize, FILE* in ) {
    u32 i,j;
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
#else
int compare_string(char *first, char *second)
{
    while(*first==*second)
   {
       if ( *first == '\0' || *second == '\0' )
          break;

       first++;
       second++;
    }

    if( *first == '\0' && *second == '\0' )
       return 0;
    else
       return -1;
}

#endif
