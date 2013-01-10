#include <stdio.h>
#include <stdlib.h>
#include "ocr.h"

//TODO need this because we don't use the user api yet
#include "ocr-runtime.h"

#define GAP_PENALTY -1
#define TRANSITION_PENALTY -2
#define TRANSVERSION_PENALTY -4
#define MATCH 2

#define FLAGS 0xdead
#define PROPERTIES 0xdead

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

u8 smith_waterman_task ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int index, ii, jj;

//    void* func_args_db = (void*)(depv[3].ptr);
//    intptr_t *func_args = (intptr_t *)func_args_db;
//    int i = (int) func_args[0];
//    int j = (int) func_args[1];
//    int tile_width = (int) func_args[2];
//    int tile_height = (int) func_args[3];
//    Tile_t** tile_matrix = (Tile_t**) func_args[4];
//    signed char* string_1 = (signed char* ) func_args[5];
//    signed char* string_2 = (signed char* ) func_args[6];
//    int n_tiles_height = (int) func_args[7];
//    int n_tiles_width = (int)func_args[8];

    intptr_t* typed_paramv = *paramv;
    int i = (int) typed_paramv[0];
    int j = (int) typed_paramv[1];
    int tile_width = (int) typed_paramv[2];
    int tile_height = (int) typed_paramv[3];
    Tile_t** tile_matrix = (Tile_t**) typed_paramv[4];
    signed char* string_1 = (signed char* ) typed_paramv[5];
    signed char* string_2 = (signed char* ) typed_paramv[6];
    int n_tiles_height = (int) typed_paramv[7];
    int n_tiles_width = (int)typed_paramv[8];

    int* left_tile_right_column = (int *) depv[0].ptr;
    int* above_tile_bottom_row = (int *) depv[1].ptr;
    int* diagonal_tile_bottom_right = (int *) depv[2].ptr;

    int  * curr_tile_tmp = (int*)malloc(sizeof(int)*(1+tile_width)*(1+tile_height));
    int ** curr_tile = (int**)malloc(sizeof(int*)*(1+tile_height));
    for (index = 0; index < tile_height+1; ++index) {
        curr_tile[index] = &curr_tile_tmp[index*(1+tile_width)];
    }

    curr_tile[0][0] = diagonal_tile_bottom_right[0];
    for ( index = 1; index < tile_height+1; ++index ) {
        curr_tile[index][0] = left_tile_right_column[index-1];
    }

    for ( index = 1; index < tile_width+1; ++index ) {
        curr_tile[0][index] = above_tile_bottom_row[index-1];
    }

    for ( ii = 1; ii < tile_height+1; ++ii ) {
        for ( jj = 1; jj < tile_width+1; ++jj ) {
            signed char char_from_1 = string_1[(j-1)*tile_width+(jj-1)];
            signed char char_from_2 = string_2[(i-1)*tile_height+(ii-1)];

            int diag_score = curr_tile[ii-1][jj-1] + alignment_score_matrix[char_from_2][char_from_1];
            int left_score = curr_tile[ii  ][jj-1] + alignment_score_matrix[char_from_1][GAP];
            int  top_score = curr_tile[ii-1][jj  ] + alignment_score_matrix[GAP][char_from_2];

            int bigger_of_left_top = (left_score > top_score) ? left_score : top_score;
            curr_tile[ii][jj] = (bigger_of_left_top > diag_score) ? bigger_of_left_top : diag_score;
        }
    }

    ocrGuid_t db_guid_i_j_br;
    void* db_guid_i_j_br_data;
    ocrDbCreate( &db_guid_i_j_br, &db_guid_i_j_br_data, sizeof(int), FLAGS, NULL, NO_ALLOC );

    int* curr_bottom_right = (int*)db_guid_i_j_br_data;
    curr_bottom_right[0] = curr_tile[tile_height][tile_width];
    ocrEventSatisfy(tile_matrix[i][j].bottom_right_event_guid, db_guid_i_j_br);

    ocrGuid_t db_guid_i_j_rc;
    void* db_guid_i_j_rc_data;
    ocrDbCreate( &db_guid_i_j_rc, &db_guid_i_j_rc_data, sizeof(int)*tile_height, FLAGS, NULL, NO_ALLOC );

    int* curr_right_column = (int*)db_guid_i_j_rc_data;
    for ( index = 0; index < tile_height; ++index ) {
        curr_right_column[index] = curr_tile[index+1][tile_width];
    }
    ocrEventSatisfy(tile_matrix[i][j].right_column_event_guid, db_guid_i_j_rc);

    ocrGuid_t db_guid_i_j_brow;
    void* db_guid_i_j_brow_data;
    ocrDbCreate( &db_guid_i_j_brow, &db_guid_i_j_brow_data, sizeof(int)*tile_width, FLAGS, NULL, NO_ALLOC );

    int* curr_bottom_row = (int*)db_guid_i_j_brow_data;
    for ( index = 0; index < tile_width; ++index ) {
        curr_bottom_row[index] = curr_tile[tile_height][index+1];
    }

    ocrEventSatisfy(tile_matrix[i][j].bottom_row_event_guid, db_guid_i_j_brow);

    free(curr_tile);
    free(curr_tile_tmp);
    if ( i == n_tiles_height && j == n_tiles_width ) {
        fprintf(stdout, "score: %d\n", curr_bottom_row[tile_width-1]);
        ocrFinish();
    }
}

int main ( int argc, char* argv[] ) {

    OCR_INIT(&argc, argv, smith_waterman_task );
    int i, j;

    int tile_width = (int) atoi (argv[3]);
    int tile_height = (int) atoi (argv[4]);

    int n_tiles_width;
    int n_tiles_height;

    if ( argc < 5 ) {
        fprintf(stderr, "Usage: %s fileName1 fileName2 tileWidth tileHeight\n", argv[0]);
        exit(1);
    }

    signed char* string_1;
    signed char* string_2;

    char* file_name_1 = argv[1];
    char* file_name_2 = argv[2];

    FILE* file_1 = fopen(file_name_1, "r");
    if (!file_1) { fprintf(stderr, "could not open file %s\n",file_name_1); exit(1); }
    size_t n_char_in_file_1 = 0;
    string_1 = read_file(file_1, &n_char_in_file_1);
    fprintf(stdout, "Size of input string 1 is %zu\n", n_char_in_file_1 );

    FILE* file_2 = fopen(file_name_2, "r");
    if (!file_2) { fprintf(stderr, "could not open file %s\n",file_name_2); exit(1); }
    size_t n_char_in_file_2 = 0;
    string_2 = read_file(file_2, &n_char_in_file_2);
    fprintf(stdout, "Size of input string 2 is %zu\n", n_char_in_file_2 );

    fprintf(stdout, "Tile width is %d\n", tile_width);
    fprintf(stdout, "Tile height is %d\n", tile_height);

    n_tiles_width = n_char_in_file_1/tile_width;
    n_tiles_height = n_char_in_file_2/tile_height;

    fprintf(stdout, "Imported %d x %d tiles.\n", n_tiles_width, n_tiles_height);

    fprintf(stdout, "Allocating tile matrix\n");

    // sagnak: all workers allocate their own copy of tile matrix
    Tile_t** tile_matrix = (Tile_t **) malloc(sizeof(Tile_t*)*(n_tiles_height+1));
    for ( i = 0; i < n_tiles_height+1; ++i ) {
        tile_matrix[i] = (Tile_t *) malloc(sizeof(Tile_t)*(n_tiles_width+1));
        for ( j = 0; j < n_tiles_width+1; ++j ) {
            ocrEventCreate(&(tile_matrix[i][j].bottom_row_event_guid ), OCR_EVENT_STICKY_T, true);
            ocrEventCreate(&(tile_matrix[i][j].right_column_event_guid ), OCR_EVENT_STICKY_T, true);
            ocrEventCreate(&(tile_matrix[i][j].bottom_right_event_guid ), OCR_EVENT_STICKY_T, true);
        }
    }

    fprintf(stdout, "Allocated tile matrix\n");
    ocrGuid_t db_guid_0_0_br;
    void* db_guid_0_0_br_data;
    ocrDbCreate( &db_guid_0_0_br, &db_guid_0_0_br_data, sizeof(int), FLAGS, NULL, NO_ALLOC );
    int* allocated = (int*)db_guid_0_0_br_data;
    allocated[0] = 0;
    ocrEventSatisfy(tile_matrix[0][0].bottom_right_event_guid, db_guid_0_0_br);

    for ( j = 1; j < n_tiles_width + 1; ++j ) {
        ocrGuid_t db_guid_0_j_brow;
        void* db_guid_0_j_brow_data;
        ocrDbCreate( &db_guid_0_j_brow, &db_guid_0_j_brow_data, sizeof(int)*tile_width, FLAGS, NULL, NO_ALLOC );

        allocated = (int*)db_guid_0_j_brow_data;
        for( i = 0; i < tile_width ; ++i ) {
            allocated[i] = GAP_PENALTY*((j-1)*tile_width+i+1);
        }

        ocrEventSatisfy(tile_matrix[0][j].bottom_row_event_guid, db_guid_0_j_brow);

        ocrGuid_t db_guid_0_j_br;
        void* db_guid_0_j_br_data;
        ocrDbCreate( &db_guid_0_j_br, &db_guid_0_j_br_data, sizeof(int), FLAGS, NULL, NO_ALLOC );
        allocated = (int*)db_guid_0_j_br_data;
        allocated[0] = GAP_PENALTY*(j*tile_width); //sagnak: needed to handle tilesize 2

        ocrEventSatisfy(tile_matrix[0][j].bottom_right_event_guid, db_guid_0_j_br);
    }

    for ( i = 1; i < n_tiles_height + 1; ++i ) {
        ocrGuid_t db_guid_i_0_rc;
        void* db_guid_i_0_rc_data;
        ocrDbCreate( &db_guid_i_0_rc, &db_guid_i_0_rc_data, sizeof(int)*tile_height, FLAGS, NULL, NO_ALLOC );
        allocated = (int*)db_guid_i_0_rc_data;
        for ( j = 0; j < tile_height ; ++j ) {
            allocated[j] = GAP_PENALTY*((i-1)*tile_height+j+1);
        }
        ocrEventSatisfy(tile_matrix[i][0].right_column_event_guid, db_guid_i_0_rc);

        ocrGuid_t db_guid_i_0_br;
        void* db_guid_i_0_br_data;
        ocrDbCreate( &db_guid_i_0_br, &db_guid_i_0_br_data, sizeof(int), FLAGS, NULL, NO_ALLOC );

        allocated = (int*)db_guid_i_0_br_data;
        allocated[0] = GAP_PENALTY*(i*tile_height); //sagnak: needed to handle tilesize 2

        ocrEventSatisfy(tile_matrix[i][0].bottom_right_event_guid, db_guid_i_0_br);
    }


    for ( i = 1; i < n_tiles_height+1; ++i ) {
        for ( j = 1; j < n_tiles_width+1; ++j ) {

            intptr_t **p_paramv = (intptr_t **)malloc(sizeof(intptr_t*));
            intptr_t *paramv = (intptr_t *)malloc(9*sizeof(intptr_t));
            paramv[0]=(intptr_t)i;
            paramv[1]=(intptr_t)j;
            paramv[2]=(intptr_t)tile_width;
            paramv[3]=(intptr_t)tile_height;
            paramv[4]=(intptr_t) tile_matrix;
            paramv[5]=(intptr_t) string_1;
            paramv[6]=(intptr_t) string_2;
            paramv[7]=(intptr_t)n_tiles_height;
            paramv[8]=(intptr_t)n_tiles_width;
            *p_paramv = paramv;
            ocrGuid_t task_guid;

            ocrEdtCreate(&task_guid, smith_waterman_task, 9, NULL, (void **) p_paramv, PROPERTIES, 3, NULL);

            ocrAddDependence(tile_matrix[i][j-1].right_column_event_guid, task_guid, 0);
            ocrAddDependence(tile_matrix[i-1][j].bottom_row_event_guid, task_guid, 1);
            ocrAddDependence(tile_matrix[i-1][j-1].bottom_right_event_guid, task_guid, 2);

            ocrEdtSchedule(task_guid);
        }
    }

    ocrCleanup();
    return 0;
}
