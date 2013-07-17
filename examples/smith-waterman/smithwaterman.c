/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "ocr.h"

#define GAP_PENALTY -1
#define TRANSITION_PENALTY -2
#define TRANSVERSION_PENALTY -4
#define MATCH 2

#define FLAGS 0xdead
#define PROPERTIES 0xdead

struct timeval startTimer;

enum Nucleotide {GAP=0, ADENINE, CYTOSINE, GUANINE, THYMINE};

signed char char_mapping ( char c ) {
    signed char to_be_returned = -1;
    switch(c) {
        case '_': to_be_returned = GAP; break;
        case 'A': to_be_returned = ADENINE; break;
        case 'C': to_be_returned = CYTOSINE; break;
        case 'G': to_be_returned = GUANINE; break;
        case 'T': to_be_returned = THYMINE; break;
    }
    return to_be_returned;
}

static char alignment_score_matrix[5][5] =
{
    {GAP_PENALTY,GAP_PENALTY,GAP_PENALTY,GAP_PENALTY,GAP_PENALTY},
    {GAP_PENALTY,MATCH,TRANSVERSION_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY},
    {GAP_PENALTY,TRANSVERSION_PENALTY, MATCH,TRANSVERSION_PENALTY,TRANSITION_PENALTY},
    {GAP_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY, MATCH,TRANSVERSION_PENALTY},
    {GAP_PENALTY,TRANSVERSION_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY, MATCH}
};

size_t clear_whitespaces_do_mapping ( signed char* buffer, long size ) {
    size_t non_ws_index = 0, traverse_index = 0;

    while ( traverse_index < (size_t)size ) {
        char curr_char = buffer[traverse_index];
        switch ( curr_char ) {
            case 'A': case 'C': case 'G': case 'T':
                /*this used to be a copy not also does mapping*/
                buffer[non_ws_index++] = char_mapping(curr_char);
                break;
        }
        ++traverse_index;
    }
    return non_ws_index;
}

signed char* read_file( FILE* file, size_t* n_chars ) {
    fseek (file, 0L, SEEK_END);
    long file_size = ftell (file);
    fseek (file, 0L, SEEK_SET);

    signed char *file_buffer = (signed char *)malloc((1+file_size)*sizeof(signed char));

    fread(file_buffer, sizeof(signed char), file_size, file);
    file_buffer[file_size] = '\n';

    /* shams' sample inputs have newlines in them */
    *n_chars = clear_whitespaces_do_mapping(file_buffer, file_size);
    return file_buffer;
}

typedef struct {
    ocrGuid_t bottom_row_event_guid;
    ocrGuid_t right_column_event_guid;
    ocrGuid_t bottom_right_event_guid;
} Tile_t;

ocrGuid_t smith_waterman_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int index, ii, jj;

    /* Unbox parameters */
    int i = (int) paramv[0];
    int j = (int) paramv[1];
    int tile_width = (int) paramv[2];
    int tile_height = (int) paramv[3];
    Tile_t** tile_matrix = (Tile_t**) paramv[4];
    signed char* string_1 = (signed char* ) paramv[5];
    signed char* string_2 = (signed char* ) paramv[6];
    int n_tiles_height = (int) paramv[7];
    int n_tiles_width = (int)paramv[8];

    /* Get the input datablock data pointers acquired from dependences */
    int* left_tile_right_column = (int *) depv[0].ptr;
    int* above_tile_bottom_row = (int *) depv[1].ptr;
    int* diagonal_tile_bottom_right = (int *) depv[2].ptr;

    /* Allocate a haloed local matrix for calculating 'this' tile*/
    int  * curr_tile_tmp = (int*)malloc(sizeof(int)*(1+tile_width)*(1+tile_height));
    /* 2D-ify it for readability */
    int ** curr_tile = (int**)malloc(sizeof(int*)*(1+tile_height));
    for (index = 0; index < tile_height+1; ++index) {
        curr_tile[index] = &curr_tile_tmp[index*(1+tile_width)];
    }

    /* Initialize halo from neighbouring tiles */
    /* Set local_tile[0][0] (top left) from the bottom right of the northwest tile */
    curr_tile[0][0] = diagonal_tile_bottom_right[0];

    /* Set local_tile[i+1][0] (left column) from the right column of the left tile */
    for ( index = 1; index < tile_height+1; ++index ) {
        curr_tile[index][0] = left_tile_right_column[index-1];
    }

    /* Set local_tile[0][j+1] (top row) from the bottom row of the above tile */
    for ( index = 1; index < tile_width+1; ++index ) {
        curr_tile[0][index] = above_tile_bottom_row[index-1];
    }

    /* Run a smith-waterman on the local tile */
    for ( ii = 1; ii < tile_height+1; ++ii ) {
        for ( jj = 1; jj < tile_width+1; ++jj ) {
            signed char char_from_1 = string_1[(j-1)*tile_width+(jj-1)];
            signed char char_from_2 = string_2[(i-1)*tile_height+(ii-1)];

            /* Get score from northwest, north and west */
            int diag_score = curr_tile[ii-1][jj-1] + alignment_score_matrix[char_from_2][char_from_1];
            int left_score = curr_tile[ii  ][jj-1] + alignment_score_matrix[char_from_1][GAP];
            int  top_score = curr_tile[ii-1][jj  ] + alignment_score_matrix[GAP][char_from_2];

            int bigger_of_left_top = (left_score > top_score) ? left_score : top_score;

            /* Set the local tile[i][j] to the maximum value of northwest, north and west */
            curr_tile[ii][jj] = (bigger_of_left_top > diag_score) ? bigger_of_left_top : diag_score;
        }
    }

    /* Allocate datablock for bottom right of the local tile */
    ocrGuid_t db_guid_i_j_br;
    void* db_guid_i_j_br_data;
    ocrDbCreate( &db_guid_i_j_br, &db_guid_i_j_br_data, sizeof(int), FLAGS, NULL_GUID, NO_ALLOC );

    /* Satisfy the bottom right event of local tile with the data block allocated above */
    int* curr_bottom_right = (int*)db_guid_i_j_br_data;
    curr_bottom_right[0] = curr_tile[tile_height][tile_width];

    ocrDbRelease(db_guid_i_j_br); // For now, no auto-release it seems...
    ocrEventSatisfy(tile_matrix[i][j].bottom_right_event_guid, db_guid_i_j_br);

    /* Allocate datablock for right column of the local tile */
    ocrGuid_t db_guid_i_j_rc;
    void* db_guid_i_j_rc_data;
    ocrDbCreate( &db_guid_i_j_rc, &db_guid_i_j_rc_data, sizeof(int)*tile_height, FLAGS, NULL_GUID, NO_ALLOC );

    /* Satisfy the right column event of local tile with the data block allocated above */
    int* curr_right_column = (int*)db_guid_i_j_rc_data;
    for ( index = 0; index < tile_height; ++index ) {
        curr_right_column[index] = curr_tile[index+1][tile_width];
    }
    ocrDbRelease(db_guid_i_j_rc);
    ocrEventSatisfy(tile_matrix[i][j].right_column_event_guid, db_guid_i_j_rc);

    /* Allocate datablock for bottom row of the local tile */
    ocrGuid_t db_guid_i_j_brow;
    void* db_guid_i_j_brow_data;
    ocrDbCreate( &db_guid_i_j_brow, &db_guid_i_j_brow_data, sizeof(int)*tile_width, FLAGS, NULL_GUID, NO_ALLOC );

    /* Satisfy the bottom row event of local tile with the data block allocated above */
    int* curr_bottom_row = (int*)db_guid_i_j_brow_data;
    for ( index = 0; index < tile_width; ++index ) {
        curr_bottom_row[index] = curr_tile[tile_height][index+1];
    }
    ocrDbRelease(db_guid_i_j_brow);
    ocrEventSatisfy(tile_matrix[i][j].bottom_row_event_guid, db_guid_i_j_brow);

    free(curr_tile);
    free(curr_tile_tmp);
    /* We can also free all the input DBs we get */
    ocrDbDestroy(depv[0].guid);
    ocrDbDestroy(depv[1].guid);
    ocrDbDestroy(depv[2].guid);
    /* If this is the last tile (bottom right most tile), finish */
    if ( i == n_tiles_height && j == n_tiles_width ) {
        fprintf(stdout, "score: %d\n", curr_bottom_row[tile_width-1]);
        struct timeval stopTimer;
        gettimeofday(&stopTimer, 0);
        printf("The computation took %f seconds\r\n",((stopTimer.tv_sec - startTimer.tv_sec)*1000000+(stopTimer.tv_usec - startTimer.tv_usec))*1.0/1000000);

        ocrShutdown();
    }
    return NULL_GUID;
}

static void initialize_border_values( Tile_t** tile_matrix, int n_tiles_width, int n_tiles_height, int tile_width, int tile_height ) {
    int i, j;
    /* Create a datablock for the bottom right element for tile[0][0] */
    ocrGuid_t db_guid_0_0_br;
    void* db_guid_0_0_br_data;
    ocrDbCreate( &db_guid_0_0_br, &db_guid_0_0_br_data, sizeof(int), FLAGS, NULL_GUID, NO_ALLOC );
    /* Satisfy the bottom right event for tile[0][0] with the respective datablock */
    int* allocated = (int*)db_guid_0_0_br_data;
    allocated[0] = 0;
    ocrDbRelease(db_guid_0_0_br);
    ocrEventSatisfy(tile_matrix[0][0].bottom_right_event_guid, db_guid_0_0_br);

    /* Create datablocks for the bottom right elements and bottom rows for tiles[0][j]
     * and Satisfy the bottom row event for tile[0][j] with the respective datablock */
    for ( j = 1; j < n_tiles_width + 1; ++j ) {
        ocrGuid_t db_guid_0_j_brow;
        void* db_guid_0_j_brow_data;
        ocrDbCreate( &db_guid_0_j_brow, &db_guid_0_j_brow_data, sizeof(int)*tile_width, FLAGS, NULL_GUID, NO_ALLOC );

        allocated = (int*)db_guid_0_j_brow_data;
        for( i = 0; i < tile_width ; ++i ) {
            allocated[i] = GAP_PENALTY*((j-1)*tile_width+i+1);
        }
        ocrDbRelease(db_guid_0_j_brow);
        ocrEventSatisfy(tile_matrix[0][j].bottom_row_event_guid, db_guid_0_j_brow);

        ocrGuid_t db_guid_0_j_br;
        void* db_guid_0_j_br_data;
        ocrDbCreate( &db_guid_0_j_br, &db_guid_0_j_br_data, sizeof(int), FLAGS, NULL_GUID, NO_ALLOC );
        allocated = (int*)db_guid_0_j_br_data;
        allocated[0] = GAP_PENALTY*(j*tile_width); //sagnak: needed to handle tilesize 2

        ocrDbRelease(db_guid_0_j_br);
        ocrEventSatisfy(tile_matrix[0][j].bottom_right_event_guid, db_guid_0_j_br);
    }

    /* Create datablocks for the right columns for tiles[i][0]
     * and Satisfy the right column event for tile[i][0] with the respective datablock */
    for ( i = 1; i < n_tiles_height + 1; ++i ) {
        ocrGuid_t db_guid_i_0_rc;
        void* db_guid_i_0_rc_data;
        ocrDbCreate( &db_guid_i_0_rc, &db_guid_i_0_rc_data, sizeof(int)*tile_height, FLAGS, NULL_GUID, NO_ALLOC );
        allocated = (int*)db_guid_i_0_rc_data;
        for ( j = 0; j < tile_height ; ++j ) {
            allocated[j] = GAP_PENALTY*((i-1)*tile_height+j+1);
        }
        ocrDbRelease(db_guid_i_0_rc);
        ocrEventSatisfy(tile_matrix[i][0].right_column_event_guid, db_guid_i_0_rc);

        ocrGuid_t db_guid_i_0_br;
        void* db_guid_i_0_br_data;
        ocrDbCreate( &db_guid_i_0_br, &db_guid_i_0_br_data, sizeof(int), FLAGS, NULL_GUID, NO_ALLOC );

        allocated = (int*)db_guid_i_0_br_data;
        allocated[0] = GAP_PENALTY*(i*tile_height); //sagnak: needed to handle tilesize 2

        ocrDbRelease(db_guid_i_0_br);
        ocrEventSatisfy(tile_matrix[i][0].bottom_right_event_guid, db_guid_i_0_br);
    }
}

static void ioHandling ( void* marshalled, int* p_n_tiles_height, int* p_n_tiles_width, int* p_tile_width, int* p_tile_height, signed char** p_string_1, signed char** p_string_2) {
    u64 argc = ((u64*) marshalled)[0];

    *p_tile_width = (int) atoi ((char*)(marshalled+((u64*) marshalled)[4])); //argv[3], offset in bytes
    *p_tile_height = (int) atoi ((char*)(marshalled+((u64*) marshalled)[5])); //argv[4], offset in bytes

    if ( argc < 5 ) {
        fprintf(stderr, "Usage: %s fileName1 fileName2 tileWidth tileHeight\n", ((char*)(marshalled+((u64*) marshalled)[1]))/*argv[0]*/);
        exit(1);
    }

    char* file_name_1 = ((char*)(marshalled+((u64*) marshalled)[2])); //argv[1], offset in bytes
    char* file_name_2 = ((char*)(marshalled+((u64*) marshalled)[3])); //argv[2], offset in bytes

    FILE* file_1 = fopen(file_name_1, "r");
    if (!file_1) { fprintf(stderr, "could not open file %s\n",file_name_1); exit(1); }
    size_t n_char_in_file_1 = 0;
    *p_string_1 = read_file(file_1, &n_char_in_file_1);
    fprintf(stdout, "Size of input string 1 is %zu\n", n_char_in_file_1 );

    FILE* file_2 = fopen(file_name_2, "r");
    if (!file_2) { fprintf(stderr, "could not open file %s\n",file_name_2); exit(1); }
    size_t n_char_in_file_2 = 0;
    *p_string_2 = read_file(file_2, &n_char_in_file_2);
    fprintf(stdout, "Size of input string 2 is %zu\n", n_char_in_file_2 );

    fprintf(stdout, "Tile width is %d\n", *p_tile_width);
    fprintf(stdout, "Tile height is %d\n", *p_tile_height);

    *p_n_tiles_width = n_char_in_file_1 / *p_tile_width;
    *p_n_tiles_height = n_char_in_file_2 / *p_tile_height;

    fprintf(stdout, "Imported %d x %d tiles.\n", *p_n_tiles_width, *p_n_tiles_height);

    fprintf(stdout, "Allocating tile matrix\n");
}

ocrGuid_t mainEdt ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    assert ( 0 == paramc );
    assert ( 1 == depc );

    int n_tiles_height;
    int n_tiles_width;
    int tile_width;
    int tile_height;
    signed char* string_1;
    signed char* string_2;

    ioHandling(depv[0].ptr, &n_tiles_height, &n_tiles_width, &tile_width, &tile_height, &string_1, &string_2);

    int i, j;

    Tile_t** tile_matrix = (Tile_t **) malloc(sizeof(Tile_t*)*(n_tiles_height+1));
    for ( i = 0; i < n_tiles_height+1; ++i ) {
        tile_matrix[i] = (Tile_t *) malloc(sizeof(Tile_t)*(n_tiles_width+1));
        for ( j = 0; j < n_tiles_width+1; ++j ) {
            /* Create readiness events for every tile */
            ocrEventCreate(&(tile_matrix[i][j].bottom_row_event_guid ), OCR_EVENT_STICKY_T, true);
            ocrEventCreate(&(tile_matrix[i][j].right_column_event_guid ), OCR_EVENT_STICKY_T, true);
            ocrEventCreate(&(tile_matrix[i][j].bottom_right_event_guid ), OCR_EVENT_STICKY_T, true);
        }
    }

    fprintf(stdout, "Allocated tile matrix\n");

    gettimeofday(&startTimer, 0);

    initialize_border_values(tile_matrix, n_tiles_width, n_tiles_height, tile_width, tile_height);

    ocrGuid_t smith_waterman_task_template_guid;
    ocrEdtTemplateCreate(&smith_waterman_task_template_guid, smith_waterman_task, 9 /*paramc*/, 3 /*depc*/);

    for ( i = 1; i < n_tiles_height+1; ++i ) {
        for ( j = 1; j < n_tiles_width+1; ++j ) {

            /* Box function parameters and put them on the heap for lifetime */
            u64 *paramv = (u64*)malloc(9*sizeof(u64));
            paramv[0]=(u64) i;
            paramv[1]=(u64) j;
            paramv[2]=(u64) tile_width;
            paramv[3]=(u64) tile_height;
            paramv[4]=(u64) tile_matrix;
            paramv[5]=(u64) string_1;
            paramv[6]=(u64) string_2;
            paramv[7]=(u64) n_tiles_height;
            paramv[8]=(u64) n_tiles_width;

            /* Create an event-driven tasks of smith_waterman tasks */
            ocrGuid_t task_guid;
            ocrEdtCreate(&task_guid, smith_waterman_task_template_guid,
                9 /*paramc, sagnak but does not the template know of this already?*/, paramv,
                3 /*depc, sagnak but does not the template know of this already?*/, NULL /*depv*/,
                PROPERTIES, NULL_GUID /*affinity*/, NULL /*outputEvent*/);

            /* add dependency to the EDT from the west tile's right column ready event */
            ocrAddDependence(tile_matrix[i][j-1].right_column_event_guid, task_guid, 0, DB_MODE_RO);

            /* add dependency to the EDT from the north tile's bottom row ready event */
            ocrAddDependence(tile_matrix[i-1][j].bottom_row_event_guid, task_guid, 1, DB_MODE_RO);

            /* add dependency to the EDT from the northwest tile's bottom right ready event */
            ocrAddDependence(tile_matrix[i-1][j-1].bottom_right_event_guid, task_guid, 2, DB_MODE_RO);

            /* schedule task*/
            // ocrEdtSchedule(task_guid);
        }
    }

    return NULL_GUID;
}
