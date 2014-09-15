/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#define __OCR__
#include <stdio.h>
// TODO: Need to remove when we have all the calls we need (str* mostly)
#include <string.h>
// TODO: Remove when we have sqrt
#include <math.h>
#include <sys/time.h>
#include <ocr.h>
#include <extensions/ocr-affinity.h>

#define debug(i,a,b...) do { PRINTF("[%d] %s:%d: " a "\n", i, __FILE__, __LINE__, ##b); } while(0)

int myrank, numranks = -1;

struct timeval a,b;

#define FLAGS DB_PROP_SINGLE_ASSIGNMENT
#define PROPERTIES EDT_PROP_NONE
#define UNUSED __attribute__((unused))

ocrGuid_t shutdown_template;

/* number of entries in an array */
#define LENGTH(a) (sizeof((a))/sizeof((a)[0]))

/* Assign data to compute nodes */
#define OWNER(i,j) (i % numranks)

/*
 * The global guid array is formatted as follows:
 * * [0] self-reference (the guid of the global guid array)
 * * [1] the sequential solver edt template guid
 * * [2] the triangle solver edt template guid
 * * [3] the diagonal update edt template guid
 * * [4] the non-diagonal update edt template guid
 * * [5+] a packed array of tile-related guids
 *
 * This array is passed to all of the tile operation EDTs as the first entry in
 * depv[].  It is used for scheduling the next task, and specifying the input
 * dependencies.
 *
 * For each tile, there is one Event GUID which is depended upon by other
 * compute nodes.  The Event GUID is satisfied with a DB GUID once the tile is
 * finalized (as determined by task_done()).
 *
 * This version of Cholesky follows strict SSA semantics for data block access,
 * so every phase/version of a tile is a separate DB with a separate GUID.
 * This is easier for Cholesky than for some other computations, as incomplete
 * tiles are never depended upon by remote consumers in Cholesky.
 */
enum { GLOBAL_SELF, GLOBAL_LOCAL, GLOBAL_SEQ, GLOBAL_TRISOLVE, GLOBAL_UPDATE, GLOBAL_UPDATE_NON_DIAG, _GLOBAL_PACKED_OFFSET };
enum { K_EV, _K_MAX };
#define GLOBALIDX(i,j,k) ((((i)+1)*(i)/2 + (j)) * _K_MAX + k + _GLOBAL_PACKED_OFFSET)
#define GLOBAL(E,i,j,k) ((E)[GLOBALIDX(i,j,k)])

static void task_done(int i, int j, int k, int tileSize, const ocrGuid_t *globals, const ocrGuid_t data_guid) {
    ocrGuid_t task_guid, affinity = globals[GLOBAL_LOCAL];
    u64 func_args[4];
    func_args[0] = i;
    func_args[1] = j;
    func_args[2] = k+1;
    func_args[3] = tileSize;
//    PRINTF("[%d] task_done globals=%p globals_guid=%ld\n", myrank, globals, data_guid);

    if(k == j) {
        PRINTF("[%d] tile %d,%d finalized at version %d\n", myrank, i, j, k+1);
        /* tile is finalized, post the OUT event */
        ocrEventSatisfy(GLOBAL(globals,i,j,K_EV), data_guid);
        return;
    } else
    if(k == j-1) {
        /* Next up is the final event for this tile */
        if(i == j) {
//            PRINTF("[%d] tile %d,%d next up: potrf template: %ld\n", myrank, i, j, globals[GLOBAL_SEQ]);
            ocrEdtCreate(&task_guid, globals[GLOBAL_SEQ], 4, func_args, 2, NULL, PROPERTIES, affinity, NULL);
        } else {
//            PRINTF("[%d] tile %d,%d next up: trsm template: %ld\n", myrank, i, j, globals[GLOBAL_TRISOLVE]);
            ocrEdtCreate(&task_guid, globals[GLOBAL_TRISOLVE], 4, func_args, 3, NULL, PROPERTIES, affinity, NULL);
            ocrAddDependence(GLOBAL(globals,k+1,k+1,K_EV), task_guid, 2, DB_MODE_RO);  /* up */
        }
    } else {
        /* Next up is an intermediate computation */
        if(i == j) {
//            PRINTF("[%d] tile %d,%d next up: syrk template: %ld\n", myrank, i, j, globals[GLOBAL_UPDATE]);
            ocrEdtCreate(&task_guid, globals[GLOBAL_UPDATE], 4, func_args, 3, NULL, PROPERTIES, affinity, NULL);
            ocrAddDependence(GLOBAL(globals,i,k+1,K_EV), task_guid, 2, DB_MODE_RO); /* left */
        } else {
//            PRINTF("[%d] tile %d,%d next up: gemm template: %ld\n", myrank, i, j, globals[GLOBAL_UPDATE_NON_DIAG]);
            ocrEdtCreate(&task_guid, globals[GLOBAL_UPDATE_NON_DIAG], 4, func_args, 4, NULL, PROPERTIES, affinity, NULL);
            ocrAddDependence(GLOBAL(globals,i,k+1,K_EV), task_guid, 2, DB_MODE_RO); /* left */
            ocrAddDependence(GLOBAL(globals,j,k+1,K_EV), task_guid, 3, DB_MODE_RO); /* diagonal */
        }
    }
    ocrAddDependence(data_guid,task_guid,0,DB_MODE_ITW);
    ocrAddDependence(globals[GLOBAL_SELF],task_guid,1,DB_MODE_RO);
}

static ocrGuid_t sequential_cholesky_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT(paramc == 4);
    ASSERT(depc   == 2);

    int i = (int)paramv[0];
    int j = (int)paramv[1];
    int k = (int)paramv[2];
    int tileSize = (int)paramv[3];

    int iB = 0, jB = 0, kB = 0;
    const double* aBlock = (double*) (depv[0].ptr);
    const ocrGuid_t *globals = (ocrGuid_t*)(depv[1].ptr);
    PRINTF("[%d] RUNNING sequential_cholesky %d\n", myrank, k);

    ocrGuid_t out_guid;
    double *out;
    ocrDbCreate(&out_guid, (void**)&out, tileSize*tileSize*sizeof(double), FLAGS, globals[GLOBAL_LOCAL], NO_ALLOC);
    memcpy(out, aBlock, tileSize*tileSize*sizeof(double));

    for( kB = 0 ; kB < tileSize ; ++kB ) {
        if( aBlock[kB*tileSize+kB] <= 0 ) {
            PRINTF("Not a symmetric positive definite (SPD) matrix\n");
            ASSERT(0);
            ocrShutdown();
            return NULL_GUID;
        } else {
            out[kB*tileSize+kB] = sqrt(out[kB*tileSize+kB]);
        }

        for(jB = kB + 1; jB < tileSize ; ++jB ) {
            out[jB*tileSize+kB] = out[jB*tileSize+kB]/out[kB*tileSize+kB];
        }

        for(jB = kB + 1; jB < tileSize ; ++jB ) {
            for(iB = jB ; iB < tileSize ; ++iB ) {
                out[iB*tileSize+jB] -= out[iB*tileSize+kB] * out[jB*tileSize+kB];
            }
        }
    }
    ocrDbRelease(out_guid);
    task_done(i, j, k, tileSize, globals, out_guid);

    return NULL_GUID;
}

static ocrGuid_t trisolve_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT(paramc == 4);
    ASSERT(depc   == 3);

    int i = (int)paramv[0];
    int j = (int)paramv[1];
    int k = (int)paramv[2];
    int tileSize = (int)paramv[3];

    int iB, jB, kB;
    const double* aBlock = (double*) (depv[0].ptr);
    const ocrGuid_t *globals = (ocrGuid_t*)(depv[1].ptr);
    const double* liBlock = (double*) (depv[2].ptr);
//    PRINTF("[%d] RUNNING trisolve (%d, %d, %d)\n", myrank, i, j, k);

    ocrGuid_t out_guid;
    double *out;
    ocrDbCreate(&out_guid, (void**)&out, tileSize*tileSize*sizeof(double), FLAGS, globals[GLOBAL_LOCAL], NO_ALLOC);
    memcpy(out, aBlock, tileSize*tileSize*sizeof(double));

    for( kB = 0; kB < tileSize ; ++kB ) {
        for( iB = 0; iB < tileSize ; ++iB ) {
            out[iB*tileSize+kB] /= liBlock[kB*tileSize+kB];
        }

        for( jB = kB + 1 ; jB < tileSize; ++jB ) {
            for( iB = 0; iB < tileSize; ++iB ) {
                out[iB*tileSize+jB] -= liBlock[jB*tileSize+kB] * out[iB*tileSize+kB];
            }
        }
    }

    ocrDbRelease(out_guid);
    task_done(i, j, k, tileSize, globals, out_guid);

    return NULL_GUID;
}

static ocrGuid_t update_diagonal_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT(paramc == 4);
    ASSERT(depc   == 3);

    int i = (int)paramv[0];
    int j = (int)paramv[1];
    int k = (int)paramv[2];
    int tileSize = (int)paramv[3];

    int iB, jB, kB;
    const double* aBlock = (double*) (depv[0].ptr);
    const ocrGuid_t *globals = (ocrGuid_t*)(depv[1].ptr);
    const double* l2Block = (double*) (depv[2].ptr);
//    PRINTF("[%d] RUNNING update_diagonal (%d, %d, %d)\n", myrank, k, j, i);

    ocrGuid_t out_guid;
    double *out;
    ocrDbCreate(&out_guid, (void**)&out, tileSize*tileSize*sizeof(double), FLAGS, globals[GLOBAL_LOCAL], NO_ALLOC);
    memcpy(out, aBlock, tileSize*tileSize*sizeof(double));

    for( jB = 0; jB < tileSize ; ++jB ) {
        for( kB = 0; kB < tileSize ; ++kB ) {
            double temp = 0 - l2Block[jB*tileSize+kB];
            for( iB = jB; iB < tileSize; ++iB ) {
                out[iB*tileSize+jB] += temp * l2Block[iB*tileSize+kB];
            }
        }
    }
    ocrDbRelease(out_guid);
    task_done(i, j, k, tileSize, globals, out_guid);

    return NULL_GUID;
}

static ocrGuid_t update_nondiagonal_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT(paramc == 4);
    ASSERT(depc   == 4);

    int i = (int)paramv[0];
    int j = (int)paramv[1];
    int k = (int)paramv[2];
    int tileSize = (int)paramv[3];

    int jB, kB, iB;
    const double* aBlock = (double*) (depv[0].ptr);
    const ocrGuid_t *globals = (ocrGuid_t*)(depv[1].ptr);
    const double* l1Block = (double*) (depv[2].ptr);
    const double* l2Block = (double*) (depv[3].ptr);
//    PRINTF("[%d] RUNNING update_nondiagonal (%d, %d, %d)\n", myrank, k, j, i);

    ocrGuid_t out_guid;
    double *out;
    ocrDbCreate(&out_guid, (void**)&out, tileSize*tileSize*sizeof(double), FLAGS, globals[GLOBAL_LOCAL], NO_ALLOC);
    memcpy(out, aBlock, tileSize*tileSize*sizeof(double));

    for( jB = 0; jB < tileSize ; ++jB ) {
        for( kB = 0; kB < tileSize ; ++kB ) {
            double temp = 0 - l2Block[jB*tileSize+kB];
            for(iB = 0; iB < tileSize; ++iB)
                out[iB*tileSize+jB] += temp * l1Block[iB*tileSize+kB];
        }
    }

    ocrDbRelease(out_guid);
    task_done(i, j, k, tileSize, globals, out_guid);

    return NULL_GUID;
}

static ocrGuid_t wrap_up_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int i, j, i_b, j_b;
    FILE *out = fopen("cholesky.out", "w");

    u64 *func_args = paramv;
    int numTiles = (int) func_args[0];
    int tileSize = (int) func_args[1];

    for ( i = 0; i < numTiles; ++i ) {
        for( i_b = 0; i_b < tileSize; ++i_b) {
            for ( j = 0; j <= i; ++j ) {
                int idx = i*(i+1)/2+j+1;
                const double *temp = (double*) (depv[idx].ptr);
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
    fclose(out);

    gettimeofday(&b,0);
    PRINTF("[%d] The computation took %f seconds\r\n", myrank,
        ((b.tv_sec - a.tv_sec)*1000000+(b.tv_usec - a.tv_usec))*1.0/1000000);

    ocrShutdown();
    return NULL_GUID;
}

inline static void wrap_up_task_prescriber ( ocrGuid_t edtTemp, int numTiles, int tileSize,
                                             ocrGuid_t* globals) {
    int i, j;
    ocrGuid_t wrap_up_task_guid;

    u64 func_args[3];
    func_args[0]=(int)numTiles;
    func_args[1]=(int)tileSize;

    ocrEdtCreate(&wrap_up_task_guid, edtTemp, 2, func_args, (numTiles+1)*numTiles/2+1, NULL, PROPERTIES, globals[GLOBAL_LOCAL], NULL);

    int index = 1;
    for ( i = 0; i < numTiles; ++i ) {
        for ( j = 0; j <= i; ++j ) {
            ocrAddDependence(GLOBAL(globals,i,j,K_EV), wrap_up_task_guid, index++, DB_MODE_RO);
        }
    }
    ocrAddDependence(globals[0], wrap_up_task_guid, 0, DB_MODE_RO);
}


/* Setup is a bit weird, distributed OCR is under development.  Here's how it works:
 *
 * Stage 1: Each node creates local events and DBs, and pass them to node 0
 * Stage 2: Node 0 builds the full table of Event and DB GUIDs and broadcasts it
 * Stage 3: Each node sets up local (proxy) events for remote Events, with an
 *          explicit copy task inbetween, does the rest of the local setup, and
 *          starts everything running
 */

static ocrGuid_t setup_1_create_local(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]);
static ocrGuid_t setup_2_aggregate   (u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]);
static ocrGuid_t setup_3_spawn       (u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]);
static ocrGuid_t satisfy_initial_tile(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]);
//static ocrGuid_t copy_tile           (u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]);
static ocrGuid_t readMatrix(int matrixSize, FILE* in);
static ocrGuid_t setup_1_template, setup_2_template, setup_3_template, filename_db;

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int matrixSize = -1;
    int tileSize = -1;
    int numTiles = -1;
    int i;
    u64 argc;

    const void *programArg = depv[0].ptr;
    u64* dbAsU64 = (u64*)programArg;
    argc = dbAsU64[0];

    if ( argc !=  4 ) {
        PRINTF("Usage: ./cholesky matrixSize tileSize fileName (found %ld args)\n", argc);
        ocrShutdown();
        return NULL_GUID;
    }

    // TODO: Replace with MALLOC when it becomes available again
    u64* offsets = (u64*)malloc(argc*sizeof(u64));
    for (i=0; i< argc; i++)
        offsets[i] = dbAsU64[i+1];
    char *dbAsChar = (char*)programArg;

    matrixSize = atoi(dbAsChar+offsets[1]);
    tileSize = atoi(dbAsChar+offsets[2]);

    if ( matrixSize % tileSize != 0 ) {
        PRINTF("Incorrect tile size %d for the matrix of size %d \n", tileSize, matrixSize);
        ocrShutdown();
        return NULL_GUID;
    }

    numTiles = matrixSize/tileSize;

    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ocrGuid_t affinities[affinityCount];
    ocrAffinityGet(AFFINITY_PD, &affinityCount, affinities);

    for(i = 0; i < affinityCount; i++)
        debug(0, "affinity[%d] = %ld", i, affinities[i]);

    char *dbfn, *fn = dbAsChar+offsets[3];
    ocrDbCreate(&filename_db, (void**)&dbfn, strlen(fn)+1, FLAGS, affinities[0], NO_ALLOC);
    strcpy(dbfn, fn);
    ocrDbRelease(filename_db);

    debug(myrank, "registering EDTs");
    ocrEdtTemplateCreate(&setup_1_template, setup_1_create_local, 6, 0);
    ocrEdtTemplateCreate(&setup_2_template, setup_2_aggregate   , 2, affinityCount);
    ocrEdtTemplateCreate(&setup_3_template, setup_3_spawn       , 3, 2);

    ocrGuid_t setup_1_edt, setup_2_edt, affinity = affinities[0];
    u64 args2[] = {numTiles, tileSize};
    ocrEdtCreate(&setup_2_edt, setup_2_template, LENGTH(args2), args2, affinityCount, NULL, PROPERTIES, affinity, NULL);
    u64 args1[] = {numTiles, tileSize, setup_2_edt, affinityCount, 0, 0};

    debug(myrank, "spawn setup 1, %ld node job", affinityCount);

    /* Spawn the distributed GUID creation stuff */
    for(i = 0; i < affinityCount; i++) {
        affinity = affinities[i];
        args1[4] = i;
        args1[5] = affinity;
        ocrEdtCreate(&setup_1_edt, setup_1_template, LENGTH(args1), args1, 0, NULL, PROPERTIES, affinity, NULL);
    }
    return NULL_GUID;
}

/* Runs on all nodes.  Creates a partial GUID array containing Event and DB GUIDs for tiles
 * the local node owns. (Only the Event GUIDs are used remotely.) */
static ocrGuid_t setup_1_create_local(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT(paramc == 6);
    ASSERT(depc   == 0);
    int numTiles = (int)paramv[0];
    int tileSize = (int)paramv[1];
    ocrGuid_t cb = (ocrGuid_t)paramv[2];
    ocrGuid_t *globals = NULL, db_globals = 0;
    numranks = paramv[3];
    myrank   = paramv[4];
    ocrGuid_t my_affinity = paramv[5];
    debug(myrank, "in setup 1 (affinity %ld), %d node job", my_affinity, numranks);

    int i, j, me = myrank;

    ocrDbCreate(&db_globals, (void**)&globals, sizeof(ocrGuid_t)*GLOBALIDX(numTiles-1,numTiles-1,_K_MAX),
             FLAGS, NULL_GUID, NO_ALLOC);
    memset(globals, 0, sizeof(ocrGuid_t)*GLOBALIDX(numTiles-1,numTiles-1,_K_MAX));
    for( i = 0 ; i < numTiles ; ++i ) {
        for( j = 0 ; j <= i ; ++j ) {
            void *ptr;
            if(OWNER(i,j) != me)
                continue;
//            ocrDbCreate(&GLOBAL(globals,i,j,K_DB), &ptr, sizeof(double)*tileSize*tileSize, FLAGS, NULL_GUID, NO_ALLOC);
            ocrEventCreate(&GLOBAL(globals,i,j,K_EV), OCR_EVENT_STICKY_T, 1);
        }
    }
    ocrDbRelease(db_globals);
    /* Pass my chunk of GUIDs, and my node ID, back to node 0 */
    ocrAddDependence(db_globals, cb, me, DB_MODE_RO);
    return NULL_GUID;
}

/* At this point, each node's array has their own GUIDs in the Event and
 * DB slots for tiles they own, and NULLs everywhere else */

/* Runs on node 0.  Takes all nodes' partial GUID arrays as inputs.  Builds a
 * full table and broadcasts it to all of the nodes. */
static ocrGuid_t setup_2_aggregate(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT(paramc == 2);
    int numTiles = (int)paramv[0];
    int tileSize = (int)paramv[1];
    ocrGuid_t *globals = NULL, db_globals = 0;
    int i, j;
    ASSERT(numranks > -1);
    ocrDbCreate(&db_globals, (void**)&globals, sizeof(ocrGuid_t)*GLOBALIDX(numTiles-1,numTiles-1,_K_MAX),
             FLAGS, NULL_GUID, NO_ALLOC);
    for( i = 0 ; i < numTiles ; ++i ) {
        for( j = 0 ; j <= i ; ++j ) {
            int owner = OWNER(i,j);
            GLOBAL(globals,i,j,K_EV) = GLOBAL(((ocrGuid_t*)depv[owner].ptr),i,j,K_EV);
//            GLOBAL(globals,i,j,K_DB) = GLOBAL(((ocrGuid_t*)depv[owner].ptr),i,j,K_DB);
        }
    }
    ocrDbRelease(db_globals);
    ocrGuid_t affinities[numranks];
    u64 num_affinities = numranks;
    ocrAffinityGet(AFFINITY_PD, &num_affinities, affinities);
    ASSERT(num_affinities == numranks);

    /* Spawn the distributed DAG creation stuff */
    for(i = 0; i < numranks; i++) {
        ocrGuid_t deps[] = { db_globals, filename_db };
        ocrGuid_t setup_3_edt, affinity = affinities[i];
        u64 args[] = {numTiles, tileSize, affinity};
        PRINTF("[%d] creating edt from template %lx for node %d\n", myrank, setup_3_template, i);
        ocrEdtCreate(&setup_3_edt, setup_3_template, LENGTH(args), args, 2, deps, PROPERTIES, affinity, NULL);
    }
    return NULL_GUID;
}

/* At this point, node 0 has a fully built GUID array containing DB and Event GUIDs
 * for every tile in the matrix. */

/* Runs on all nodes.  Takes a copy of node 0's full GUID array as input.  Makes
 * a local copy for passing to local tasks, adds some local EDT Template GUIDs and
 * fixes things up a bit. */
static ocrGuid_t setup_3_spawn(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT(paramc == 3);
    ASSERT(depc   == 2);
    int numTiles       = (int)paramv[0];
    int tileSize       = (int)paramv[1];
    ocrGuid_t nodeguid = (ocrGuid_t)paramv[2];
    const char *filename = (char*)depv[1].ptr;
    int i, j, me = myrank;
    const ocrGuid_t *aggregated_guids = (ocrGuid_t *)depv[0].ptr;
    ocrGuid_t *globals, db_globals;
    ocrGuid_t satisfy_initial_template;
    ocrGuid_t matrix;
//    ocrGuid_t copy_template;
    FILE *in;

    /* Create a local DB with a copy of the aggregated guid list from node 0. */
    ocrDbCreate(&db_globals, (void**)&globals, sizeof(ocrGuid_t)*GLOBALIDX(numTiles-1,numTiles-1,_K_MAX),
             FLAGS, nodeguid, NO_ALLOC);
    memcpy(globals,aggregated_guids,sizeof(ocrGuid_t)*GLOBALIDX(numTiles-1,numTiles-1,_K_MAX));
    /* Make inline "copy" EDTs to avoid multiple local consumers of a remote data block (and resulting cloning issues) */
//    ocrEdtTemplateCreate(&copy_template, copy_tile, 1, 1);
//    for( i = 0 ; i < numTiles ; ++i ) {
//        for( j = 0 ; j <= i ; ++j ) {
//            int owner = OWNER(i,j);
//            if(owner != myrank) {
//                ocrGuid_t local_event, edt, out_event;
//                ocrEdtCreate(&edt, copy_template, 1, (u64[]){ tileSize*tileSize*sizeof(double) },
//                             1, NULL, EDT_PROP_NONE, nodeguid, &out_event);
//                ocrEventCreate(&local_event, OCR_EVENT_STICKY_T, 1);
//                ocrAddDependence(out_event, local_event, 0, DB_MODE_RO);
//                ocrAddDependence(GLOBAL(globals,i,j,K_EV), edt, 0, DB_MODE_RO);
//                GLOBAL(globals,i,j,K_EV) = local_event;
//            }
//        }
//    }

    in = fopen(filename, "r");
    if( !in ) {
        PRINTF("[%d] Cannot find file %s\n", myrank, filename);
        ocrShutdown();
        return NULL_GUID;
    }
    matrix = readMatrix( tileSize*numTiles, in );
    fclose(in);

    globals[GLOBAL_SELF]  = db_globals;
    globals[GLOBAL_LOCAL] = nodeguid;

    ocrGuid_t templateSeq, templateTrisolve, templateUpdateNonDiag, templateUpdate, templateWrap;

    ocrEdtTemplateCreate(&templateSeq          , sequential_cholesky_task, 4, 2);
    ocrEdtTemplateCreate(&templateTrisolve     , trisolve_task           , 4, 3);
    ocrEdtTemplateCreate(&templateUpdate       , update_diagonal_task    , 4, 3);
    ocrEdtTemplateCreate(&templateUpdateNonDiag, update_nondiagonal_task , 4, 4);
    ocrEdtTemplateCreate(&templateWrap, wrap_up_task, 2, (numTiles+1)*numTiles/2+1);

//    PRINTF("[%d] globals is %p, guid %ld\n", myrank, globals, db_globals);
    globals[GLOBAL_SEQ]             = templateSeq;
//    PRINTF("[%d] creating template %ld for sequential_cholesky_task\n", myrank, globals[GLOBAL_SEQ]);
    globals[GLOBAL_TRISOLVE]        = templateTrisolve;
//    PRINTF("[%d] creating template %ld for trisolve_task\n", myrank, globals[GLOBAL_TRISOLVE]);
    globals[GLOBAL_UPDATE]          = templateUpdate;
//    PRINTF("[%d] creating template %ld for update_diagonal_task\n", myrank, globals[GLOBAL_UPDATE]);
    globals[GLOBAL_UPDATE_NON_DIAG] = templateUpdateNonDiag;
//    PRINTF("[%d] creating template %ld for update_nondiagonal_task\n", myrank, globals[GLOBAL_UPDATE_NON_DIAG]);

    ocrEdtTemplateCreate(&satisfy_initial_template, satisfy_initial_tile, 4, 2);
    for(i = 0; i < numTiles; i++) {
        for(j = 0; j <= i; j++) {
            int owner = OWNER(i,j);
            if(owner == me) {
                ocrGuid_t initial;
                u64 args[] = {numTiles, tileSize, i, j};
                ocrGuid_t deps[] = {db_globals, matrix};
                /* no fancy stuff required, the producer will satisfy the local event with the DB directly */
                ocrEdtCreate(&initial, satisfy_initial_template, LENGTH(args), args, 2, deps, PROPERTIES, globals[GLOBAL_LOCAL], NULL);
            }
        }
    }

    gettimeofday(&a,0);

    if(!myrank)
        wrap_up_task_prescriber ( templateWrap, numTiles, tileSize, globals);

    return NULL_GUID;
}

static ocrGuid_t satisfy_initial_tile(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT(paramc == 4);
    ASSERT(depc   == 2);
    int numTiles = (int)paramv[0];
    int tileSize = (int)paramv[1];
    int Tilei    = (int)paramv[2];
    int Tilej    = (int)paramv[3];
    const ocrGuid_t *globals = (ocrGuid_t *)depv[0].ptr;
    const double    *in      = (double*)    depv[1].ptr;

    int matrixSize = numTiles * tileSize;
    int Li, Lj, i, j, DBi = 0;

    ocrGuid_t out_guid;
    double *out;
    ocrDbCreate(&out_guid, (void**)&out, tileSize*tileSize*sizeof(double), FLAGS, globals[GLOBAL_LOCAL], NO_ALLOC);

    // Split the matrix into tiles and write it into the item space at time 0.
    // The tiles are indexed by tile indices (which are tag values).
    for(i = Tilei*tileSize, Li = 0; Li < tileSize; i++, Li++) {
        for(j = Tilej*tileSize, Lj = 0 ; Lj < tileSize; j++, Lj++) {
            out[DBi++] = in[i*matrixSize + j];
        }
    }
    ocrDbRelease(out_guid);
    task_done(Tilei, Tilej, -1, tileSize, globals, out_guid);
    return NULL_GUID;
}

static ocrGuid_t readMatrix( int matrixSize, FILE* in ) {
    int i,j,index;
    ocrGuid_t rv = 0, affinity = 0;
    double *buffer;
    ocrDbCreate(&rv, (void**)&buffer, sizeof(double)*matrixSize*matrixSize, FLAGS, affinity, NO_ALLOC);
    PRINTF("[%d] readMatrix: rv=%#lx buffer=%p\n", myrank, rv, buffer);
    if(!rv || !buffer) {
        PRINTF("Failed to alloc DB of %zd bytes.\n"
               "Please fix your OCR config file.\n", sizeof(double)*matrixSize*matrixSize);
        ocrShutdown();
    }
    for(index = 0, i = 0; i < matrixSize; ++i ) {
        for( j = 0; j < matrixSize; ++j )
            fscanf(in, "%lf ", &buffer[index++]);
        fscanf(in, "%lf\n", &buffer[index++]);
    }
    ocrDbRelease(rv);
    return rv;
}
