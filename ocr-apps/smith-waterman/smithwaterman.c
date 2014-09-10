/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr.h"

#include <stdio.h>
#include <stdlib.h>

#define GAP_PENALTY -1
#define TRANSITION_PENALTY -2
#define TRANSVERSION_PENALTY -4
#define MATCH 2

#define FLAGS 0xdead
#define PROPERTIES 0xdead

enum Nucleotide {GAP=0, ADENINE, CYTOSINE, GUANINE, THYMINE};

s8 char_mapping ( char c ) {
    s8 to_be_returned = -1;
    switch(c) {
    case '_':
        to_be_returned = GAP;
        break;
    case 'A':
        to_be_returned = ADENINE;
        break;
    case 'C':
        to_be_returned = CYTOSINE;
        break;
    case 'G':
        to_be_returned = GUANINE;
        break;
    case 'T':
        to_be_returned = THYMINE;
        break;
    }
    return to_be_returned;
}

static char alignment_score_matrix[5][5] = {
    {GAP_PENALTY,GAP_PENALTY,GAP_PENALTY,GAP_PENALTY,GAP_PENALTY},
    {GAP_PENALTY,MATCH,TRANSVERSION_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY},
    {GAP_PENALTY,TRANSVERSION_PENALTY, MATCH,TRANSVERSION_PENALTY,TRANSITION_PENALTY},
    {GAP_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY, MATCH,TRANSVERSION_PENALTY},
    {GAP_PENALTY,TRANSVERSION_PENALTY,TRANSITION_PENALTY,TRANSVERSION_PENALTY, MATCH}
};

u32 clear_whitespaces_do_mapping ( s8* buffer, u32 size ) {
    u32 non_ws_index = 0, traverse_index = 0;

    while ( traverse_index < (u32)size ) {
        char curr_char = buffer[traverse_index];
        switch ( curr_char ) {
        case 'A':
        case 'C':
        case 'G':
        case 'T':
            /*this used to be a copy not also does mapping*/
            buffer[non_ws_index++] = char_mapping(curr_char);
            break;
        }
        ++traverse_index;
    }
    return non_ws_index;
}

#ifdef TG_ARCH
s8* read_file( s8* filestart, u32* n_chars ) {
    static FILE *file = NULL;
    s8* file_buffer;

    if(file==NULL) file = fopen((const char *)filestart, "r");
    u32 file_size = *n_chars;
    ocrGuid_t filebuf;
    ocrDbCreate(&filebuf, (void **)&file_buffer, sizeof(s8)*(1+file_size), FLAGS, NULL_GUID, NO_ALLOC);
    fread(file_buffer, sizeof(s8), file_size, file);

    // Clean up what has been read
    *n_chars =  clear_whitespaces_do_mapping(file_buffer, file_size);
    return file_buffer;
}
#else
s8* read_file( s8* filename, u32* n_chars ) {
    FILE* file = fopen(filename, "r");

    if (!file) {
        PRINTF("could not open file %s\n",filename);
        return NULL;
    }
    fseek (file, 0L, SEEK_END);
    s32 file_size = ftell (file);
    fseek (file, 0L, SEEK_SET);
    ocrGuid_t filebuf;

    s8 *file_buffer;
    ocrDbCreate( &filebuf, (void **)&file_buffer, sizeof(s8)*(1+file_size), FLAGS, NULL_GUID, NO_ALLOC );

    fread(file_buffer, sizeof(s8), file_size, file);
    file_buffer[file_size] = '\n';

    /* shams' sample inputs have newlines in them */
    *n_chars = clear_whitespaces_do_mapping(file_buffer, file_size);
    return file_buffer;
}
#endif

typedef struct {
    ocrGuid_t bottom_row_event_guid;
    ocrGuid_t right_column_event_guid;
    ocrGuid_t bottom_right_event_guid;
} Tile_t;

ocrGuid_t smith_waterman_task ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    s32 index, ii, jj;

    /* Get the input datablock data pos32ers acquired from dependences */
    s32* left_tile_right_column = (s32 *) depv[0].ptr;
    s32* above_tile_bottom_row = (s32 *) depv[1].ptr;
    s32* diagonal_tile_bottom_right = (s32 *) depv[2].ptr;
    u64* dbparamv = (u64 *) depv[3].ptr;

    /* Unbox parameters */
    s32 i = (s32) dbparamv[0];
    s32 j = (s32) dbparamv[1];
    s32 tile_width = (s32) dbparamv[2];
    s32 tile_height = (s32) dbparamv[3];
    Tile_t** tile_matrix = (Tile_t**) dbparamv[4];
    s8* string_1 = (s8* ) dbparamv[5];
    s8* string_2 = (s8* ) dbparamv[6];
    s32 n_tiles_height = (s32) dbparamv[7];
    s32 n_tiles_width = (s32)dbparamv[8];

    s32  * curr_tile_tmp;
    ocrGuid_t db_curr_tile_tmp, db_curr_tile;

    /* Allocate a haloed local matrix for calculating 'this' tile*/
    ocrDbCreate(&db_curr_tile_tmp, (void **)&curr_tile_tmp, sizeof(u32)*(1+tile_width)*(1+tile_height), FLAGS, NULL_GUID, NO_ALLOC);
    s32 ** curr_tile;
    /* 2D-ify it for readability */
    ocrDbCreate(&db_curr_tile, (void **)&curr_tile, sizeof(u32 *)*(1+tile_height), FLAGS, NULL_GUID, NO_ALLOC);

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
            s8 char_from_1 = string_1[(j-1)*tile_width+(jj-1)];
            s8 char_from_2 = string_2[(i-1)*tile_height+(ii-1)];

            /* Get score from northwest, north and west */
            s32 diag_score = curr_tile[ii-1][jj-1] + alignment_score_matrix[char_from_2][char_from_1];
            s32 left_score = curr_tile[ii  ][jj-1] + alignment_score_matrix[char_from_1][GAP];
            s32  top_score = curr_tile[ii-1][jj  ] + alignment_score_matrix[GAP][char_from_2];

            s32 bigger_of_left_top = (left_score > top_score) ? left_score : top_score;

            /* Set the local tile[i][j] to the maximum value of northwest, north and west */
            curr_tile[ii][jj] = (bigger_of_left_top > diag_score) ? bigger_of_left_top : diag_score;
        }
    }

    /* Allocate datablock for bottom right of the local tile */
    ocrGuid_t db_guid_i_j_br;
    void* db_guid_i_j_br_data;
    ocrDbCreate( &db_guid_i_j_br, &db_guid_i_j_br_data, sizeof(s32), FLAGS, NULL_GUID, NO_ALLOC );

    /* Satisfy the bottom right event of local tile with the data block allocated above */
    s32* curr_bottom_right = (s32*)db_guid_i_j_br_data;
    curr_bottom_right[0] = curr_tile[tile_height][tile_width];

    ocrDbRelease(db_guid_i_j_br); // For now, no auto-release it seems...
    ocrEventSatisfy(tile_matrix[i][j].bottom_right_event_guid, db_guid_i_j_br);

    /* Allocate datablock for right column of the local tile */
    ocrGuid_t db_guid_i_j_rc;
    void* db_guid_i_j_rc_data;
    ocrDbCreate( &db_guid_i_j_rc, &db_guid_i_j_rc_data, sizeof(s32)*tile_height, FLAGS, NULL_GUID, NO_ALLOC );

    /* Satisfy the right column event of local tile with the data block allocated above */
    s32* curr_right_column = (s32*)db_guid_i_j_rc_data;
    for ( index = 0; index < tile_height; ++index ) {
        curr_right_column[index] = curr_tile[index+1][tile_width];
    }
    ocrDbRelease(db_guid_i_j_rc);
    ocrEventSatisfy(tile_matrix[i][j].right_column_event_guid, db_guid_i_j_rc);

    /* Allocate datablock for bottom row of the local tile */
    ocrGuid_t db_guid_i_j_brow;
    void* db_guid_i_j_brow_data;
    ocrDbCreate( &db_guid_i_j_brow, &db_guid_i_j_brow_data, sizeof(s32)*tile_width, FLAGS, NULL_GUID, NO_ALLOC );

    /* Satisfy the bottom row event of local tile with the data block allocated above */
    s32* curr_bottom_row = (s32*)db_guid_i_j_brow_data;
    for ( index = 0; index < tile_width; ++index ) {
        curr_bottom_row[index] = curr_tile[tile_height][index+1];
    }
    ocrDbRelease(db_guid_i_j_brow);
    ocrEventSatisfy(tile_matrix[i][j].bottom_row_event_guid, db_guid_i_j_brow);

    ocrDbDestroy(db_curr_tile);
    ocrDbDestroy(db_curr_tile_tmp);
    /* We can also free all the input DBs we get */
    ocrDbDestroy(depv[0].guid);
    ocrDbDestroy(depv[1].guid);
    ocrDbDestroy(depv[2].guid);
    /* If this is the last tile (bottom right most tile), finish */
    if ( i == n_tiles_height && j == n_tiles_width ) {
        PRINTF("score: %d\n", curr_bottom_row[tile_width-1]);
        ocrShutdown();
    }
    return NULL_GUID;
}

static void initialize_border_values( Tile_t** tile_matrix, s32 n_tiles_width, s32 n_tiles_height, s32 tile_width, s32 tile_height ) {
    s32 i, j;
    /* Create a datablock for the bottom right element for tile[0][0] */
    ocrGuid_t db_guid_0_0_br;
    void* db_guid_0_0_br_data;
    ocrDbCreate( &db_guid_0_0_br, &db_guid_0_0_br_data, sizeof(s32), FLAGS, NULL_GUID, NO_ALLOC );
    /* Satisfy the bottom right event for tile[0][0] with the respective datablock */
    s32* allocated = (s32*)db_guid_0_0_br_data;
    allocated[0] = 0;
    ocrDbRelease(db_guid_0_0_br);
    ocrEventSatisfy(tile_matrix[0][0].bottom_right_event_guid, db_guid_0_0_br);

    /* Create datablocks for the bottom right elements and bottom rows for tiles[0][j]
     * and Satisfy the bottom row event for tile[0][j] with the respective datablock */
    for ( j = 1; j < n_tiles_width + 1; ++j ) {
        ocrGuid_t db_guid_0_j_brow;
        void* db_guid_0_j_brow_data;
        ocrDbCreate( &db_guid_0_j_brow, &db_guid_0_j_brow_data, sizeof(s32)*tile_width, FLAGS, NULL_GUID, NO_ALLOC );

        allocated = (s32*)db_guid_0_j_brow_data;
        for( i = 0; i < tile_width ; ++i ) {
            allocated[i] = GAP_PENALTY*((j-1)*tile_width+i+1);
        }
        ocrDbRelease(db_guid_0_j_brow);
        ocrEventSatisfy(tile_matrix[0][j].bottom_row_event_guid, db_guid_0_j_brow);

        ocrGuid_t db_guid_0_j_br;
        void* db_guid_0_j_br_data;
        ocrDbCreate( &db_guid_0_j_br, &db_guid_0_j_br_data, sizeof(s32), FLAGS, NULL_GUID, NO_ALLOC );
        allocated = (s32*)db_guid_0_j_br_data;
        allocated[0] = GAP_PENALTY*(j*tile_width); //sagnak: needed to handle tilesize 2

        ocrDbRelease(db_guid_0_j_br);
        ocrEventSatisfy(tile_matrix[0][j].bottom_right_event_guid, db_guid_0_j_br);
    }

    /* Create datablocks for the right columns for tiles[i][0]
     * and Satisfy the right column event for tile[i][0] with the respective datablock */
    for ( i = 1; i < n_tiles_height + 1; ++i ) {
        ocrGuid_t db_guid_i_0_rc;
        void* db_guid_i_0_rc_data;
        ocrDbCreate( &db_guid_i_0_rc, &db_guid_i_0_rc_data, sizeof(s32)*tile_height, FLAGS, NULL_GUID, NO_ALLOC );
        allocated = (s32*)db_guid_i_0_rc_data;
        for ( j = 0; j < tile_height ; ++j ) {
            allocated[j] = GAP_PENALTY*((i-1)*tile_height+j+1);
        }
        ocrDbRelease(db_guid_i_0_rc);
        ocrEventSatisfy(tile_matrix[i][0].right_column_event_guid, db_guid_i_0_rc);

        ocrGuid_t db_guid_i_0_br;
        void* db_guid_i_0_br_data;
        ocrDbCreate( &db_guid_i_0_br, &db_guid_i_0_br_data, sizeof(s32), FLAGS, NULL_GUID, NO_ALLOC );

        allocated = (s32*)db_guid_i_0_br_data;
        allocated[0] = GAP_PENALTY*(i*tile_height); //sagnak: needed to handle tilesize 2

        ocrDbRelease(db_guid_i_0_br);
        ocrEventSatisfy(tile_matrix[i][0].bottom_right_event_guid, db_guid_i_0_br);
    }
}

static u32 __attribute__ ((noinline)) ioHandling ( void* marshalled, s32* p_n_tiles_height, s32* p_n_tiles_width, s32* p_tile_width, s32* p_tile_height, s8** p_string_1, s8** p_string_2) {
    u64 argc = getArgc(marshalled);

    if(argc < 5) {
#ifdef TG_ARCH
        PRINTF("Usage: %s tileWidth tileHeight string1Length string2Length\n", getArgv(marshalled, 0)/*argv[0]*/);
#else
        PRINTF("Usage: %s tileWidth tileHeight fileName1 fileName2\n", getArgv(marshalled, 0)/*argv[0]*/);
#endif
        return 1;
    }

    u32 n_char_in_file_1 = 0;
    u32 n_char_in_file_2 = 0;
    s8 *file_name_1;
    s8 *file_name_2;

#ifdef TG_ARCH
    *p_tile_width = (s32) atoi(getArgv(marshalled, 1));
    *p_tile_height = (s32) atoi(getArgv(marshalled, 2));
    n_char_in_file_1 = (s32) atoi(getArgv(marshalled, 3));
    n_char_in_file_2 = (s32) atoi(getArgv(marshalled, 4));
    file_name_1 = NULL; // Doesn't matter anyway
    file_name_2 = NULL; // since the filename is immaterial
#else
    *p_tile_width = (s32) atoi(getArgv(marshalled, 1));
    *p_tile_height = (s32) atoi(getArgv(marshalled, 2));
    file_name_1 = getArgv(marshalled, 3);
    file_name_2 = getArgv(marshalled, 4);
#endif

    *p_string_1 = read_file(file_name_1, &n_char_in_file_1);
    if(*p_string_1 == NULL) return 1;
    PRINTF("Size of input string 1 is %d\n", n_char_in_file_1 );

    *p_string_2 = read_file(file_name_2, &n_char_in_file_2);
    if(*p_string_2 == NULL) return 1;
    PRINTF("Size of input string 2 is %d\n", n_char_in_file_2 );

    PRINTF("Tile width is %d\n", *p_tile_width);
    PRINTF("Tile height is %d\n", *p_tile_height);

    *p_n_tiles_width = n_char_in_file_1 / *p_tile_width;
    *p_n_tiles_height = n_char_in_file_2 / *p_tile_height;

    PRINTF("Imported %d x %d tiles.\n", *p_n_tiles_width, *p_n_tiles_height);

    PRINTF("Allocating tile matrix\n");
    return 0;
}

ocrGuid_t mainEdt ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT ( 0 == paramc );
    ASSERT ( 1 == depc );

    s32 n_tiles_height;
    s32 n_tiles_width;
    s32 tile_width;
    s32 tile_height;
    s8* string_1;
    s8* string_2;

    if(ioHandling(depv[0].ptr, &n_tiles_height, &n_tiles_width, &tile_width, &tile_height, &string_1, &string_2))
    {
        ocrShutdown();
        return NULL_GUID;
    }

    s32 i, j;

    ocrGuid_t db_tmp;
    Tile_t** tile_matrix;

    ocrDbCreate(&db_tmp, (void **)&tile_matrix, sizeof(Tile_t*)*(n_tiles_height+1), FLAGS, NULL_GUID, NO_ALLOC);
    for ( i = 0; i < n_tiles_height+1; ++i ) {
        ocrDbCreate(&db_tmp, (void **)&tile_matrix[i], sizeof(Tile_t)*(n_tiles_width+1), FLAGS, NULL_GUID, NO_ALLOC);
        for ( j = 0; j < n_tiles_width+1; ++j ) {
            /* Create readiness events for every tile */
            ocrEventCreate(&(tile_matrix[i][j].bottom_row_event_guid ), OCR_EVENT_STICKY_T, true);
            ocrEventCreate(&(tile_matrix[i][j].right_column_event_guid ), OCR_EVENT_STICKY_T, true);
            ocrEventCreate(&(tile_matrix[i][j].bottom_right_event_guid ), OCR_EVENT_STICKY_T, true);
        }
    }

    PRINTF("Allocated tile matrix\n");

    initialize_border_values(tile_matrix, n_tiles_width, n_tiles_height, tile_width, tile_height);

    ocrGuid_t smith_waterman_task_template_guid;
    ocrEdtTemplateCreate(&smith_waterman_task_template_guid, smith_waterman_task, 0 /*paramc*/, 4 /*depc*/);

    for ( i = 1; i < n_tiles_height+1; ++i ) {
        for ( j = 1; j < n_tiles_width+1; ++j ) {
            ocrGuid_t db_paramv;
            /* Box function parameters and put them on the heap for lifetime */
            u64 *paramv;
            ocrDbCreate(&db_paramv, (void **)&paramv, 9*sizeof(u64), FLAGS, NULL_GUID, NO_ALLOC);
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
                         0 /*paramc, sagnak but does not the template know of this already?*/, paramv,
                         4 /*depc, sagnak but does not the template know of this already?*/, NULL /*depv*/,
                         PROPERTIES, NULL_GUID /*affinity*/, NULL /*outputEvent*/);

            /* add dependency to the EDT from the west tile's right column ready event */
            ocrAddDependence(tile_matrix[i][j-1].right_column_event_guid, task_guid, 0, DB_MODE_RO);

            /* add dependency to the EDT from the north tile's bottom row ready event */
            ocrAddDependence(tile_matrix[i-1][j].bottom_row_event_guid, task_guid, 1, DB_MODE_RO);

            /* add dependency to the EDT from the northwest tile's bottom right ready event */
            ocrAddDependence(tile_matrix[i-1][j-1].bottom_right_event_guid, task_guid, 2, DB_MODE_RO);

            /* add dependency to the EDT from the paramv datablock */
            ocrAddDependence(db_paramv, task_guid, 3, DB_MODE_RO);

            /* schedule task*/
            // ocrEdtSchedule(task_guid);
        }
    }

    return NULL_GUID;
}
