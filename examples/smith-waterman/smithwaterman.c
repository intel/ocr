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

#include <stdio.h>
#include <stdlib.h>

#include "ocr.h"

//TODO need this because we don't use the user api yet
#include "ocr-runtime.h"

#define GAP_PENALTY -1
#define TRANSITION_PENALTY -2
#define TRANSVERSION_PENALTY -4
#define MATCH 2

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

    void * bottom_row;
    void * right_column;
    void * bottom_right;
} Tile_t;


u8 smith_waterman_task (u32 paramc, u64 * params, void ** paramv, u32 n_dbs, ocrEdtDep_t* dbs ) {
    int index, ii, jj;
    void* func_args_db = (void*)(dbs[3].ptr);
    intptr_t *func_args = (intptr_t *)func_args_db;
    int i = (int) func_args[0];
    int j = (int) func_args[1];
    int tile_width = (int) func_args[2];
    int tile_height = (int) func_args[3];
    Tile_t** tile_matrix = (Tile_t**) func_args[4];
    signed char* string_1 = (signed char* ) func_args[5];
    signed char* string_2 = (signed char* ) func_args[6];
    int n_tiles_height = (int) func_args[7];
    int n_tiles_width = (int)func_args[8];

    void* left_tile_right_column_db = (void*)(dbs[0].ptr);
    void* above_tile_bottom_row_db = (void*)(dbs[1].ptr);
    void* diagonal_tile_bottom_right_db = (void*)(dbs[2].ptr);

    int* left_tile_right_column = (int *) left_tile_right_column_db;
    int* above_tile_bottom_row = (int *) above_tile_bottom_row_db;
    int* diagonal_tile_bottom_right = (int *) diagonal_tile_bottom_right_db;

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

    int* curr_bottom_right = (int*)malloc(sizeof(int));
    curr_bottom_right[0] = curr_tile[tile_height][tile_width];

    ocr_event_t* t_i_j_br = (ocr_event_t *) deguidify(tile_matrix[i][j].bottom_right_event_guid);
    void *db_i_j_br = (void*) curr_bottom_right;
    ocrGuid_t db_guid_i_j_br = guidify(db_i_j_br);
    t_i_j_br->put(t_i_j_br, db_guid_i_j_br);

    int* curr_right_column = (int*)malloc(sizeof(int)*tile_height);
    for ( index = 0; index < tile_height; ++index ) {
        curr_right_column[index] = curr_tile[index+1][tile_width];
    }

    ocr_event_t* t_i_j_rc = (ocr_event_t *) deguidify(tile_matrix[i][j].right_column_event_guid);
    void *db_i_j_rc = (void*) curr_right_column;
    ocrGuid_t db_guid_i_j_rc = guidify(db_i_j_rc);
    t_i_j_rc->put(t_i_j_rc, db_guid_i_j_rc);

    int* curr_bottom_row = (int*)malloc(sizeof(int)*tile_width);
    for ( index = 0; index < tile_width; ++index ) {
        curr_bottom_row[index] = curr_tile[tile_height][index+1];
    }

    ocr_event_t* t_i_j_brow = (ocr_event_t *) deguidify(tile_matrix[i][j].bottom_row_event_guid);
    void *db_i_j_brow = (void*) curr_bottom_row;
    ocrGuid_t db_guid_i_j_brow = guidify(db_i_j_brow);
    t_i_j_brow->put(t_i_j_brow,db_guid_i_j_brow);

    free(curr_tile);
    free(curr_tile_tmp);
    if ( i == n_tiles_height && j == n_tiles_width ) {
        fprintf(stdout, "score: %d\n", curr_bottom_row[tile_width-1]);
        ocrFinish();
    }
    return 0;
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

    int* allocated = (int*)malloc(sizeof(int));
    allocated[0] = 0;

    ocr_event_t* t_0_0_br = (ocr_event_t *) deguidify(tile_matrix[0][0].bottom_right_event_guid);
    void *db_0_0_br = (void*) allocated;
    ocrGuid_t db_guid_0_0_br = guidify(db_0_0_br);
    t_0_0_br->put(t_0_0_br,db_guid_0_0_br);

    for ( j = 1; j < n_tiles_width + 1; ++j ) {
        allocated = (int*)malloc(sizeof(int)*tile_width);
        for( i = 0; i < tile_width ; ++i ) {
            allocated[i] = GAP_PENALTY*((j-1)*tile_width+i+1);
        }

        ocr_event_t* t_0_j_brow = (ocr_event_t *) deguidify(tile_matrix[0][j].bottom_row_event_guid);
        void *db_0_j_brow = (void*) allocated;
        ocrGuid_t db_guid_0_j_brow = guidify(db_0_j_brow);
        t_0_j_brow->put(t_0_j_brow,db_guid_0_j_brow);

        allocated = (int*)malloc(sizeof(int));
        allocated[0] = GAP_PENALTY*(j*tile_width); //sagnak: needed to handle tilesize 2

        ocr_event_t* t_0_j_br = (ocr_event_t *) deguidify(tile_matrix[0][j].bottom_right_event_guid);
        void *db_0_j_br = (void*) allocated;
        ocrGuid_t db_guid_0_j_br = guidify(db_0_j_br);
        t_0_j_br->put(t_0_j_br,db_guid_0_j_br);
    }

    for ( i = 1; i < n_tiles_height + 1; ++i ) {
        allocated = (int*)malloc(sizeof(int)*tile_height);
        for ( j = 0; j < tile_height ; ++j ) {
            allocated[j] = GAP_PENALTY*((i-1)*tile_height+j+1);
        }
        ocr_event_t* t_i_0_rc = (ocr_event_t *) deguidify(tile_matrix[i][0].right_column_event_guid);
        void *db_i_0_rc = (void*) allocated;
        ocrGuid_t db_guid_i_0_rc = guidify(db_i_0_rc);
        t_i_0_rc->put(t_i_0_rc,db_guid_i_0_rc);

        allocated = (int*)malloc(sizeof(int));
        allocated[0] = GAP_PENALTY*(i*tile_height); //sagnak: needed to handle tilesize 2

        ocr_event_t* t_i_0_br = (ocr_event_t *) deguidify(tile_matrix[i][0].bottom_right_event_guid);
        void *db_i_0_br = (void*) allocated;
        ocrGuid_t db_guid_i_0_br = guidify(db_i_0_br);
        t_i_0_br->put(t_i_0_br,db_guid_i_0_br);
    }


    for ( i = 1; i < n_tiles_height+1; ++i ) {
        for ( j = 1; j < n_tiles_width+1; ++j ) {

            ocrGuid_t task_guid = taskFactory->create(taskFactory, smith_waterman_task, 0, NULL, NULL, 4);
            ocr_task_t* task = (ocr_task_t*) deguidify(task_guid);

            ocr_event_t* e_1 = (ocr_event_t *) deguidify(tile_matrix[i][j-1].right_column_event_guid);
            task->add_dependency(task, e_1, 0);

            ocr_event_t* e_2 = (ocr_event_t *) deguidify(tile_matrix[i-1][j].bottom_row_event_guid);
            task->add_dependency(task, e_2, 1);

            ocr_event_t* e_3 = (ocr_event_t *) deguidify(tile_matrix[i-1][j-1].bottom_right_event_guid);
            task->add_dependency(task, e_3, 2);

            ocrGuid_t func_args_event = eventFactory->create(eventFactory, OCR_EVENT_STICKY_T, true);
            ocr_event_t* func_args_event_p = (ocr_event_t *) deguidify(func_args_event);
            intptr_t *func_args = (intptr_t *)malloc(9*sizeof(intptr_t));
            func_args[0]=i;
            func_args[1]=j;
            func_args[2]=tile_width;
            func_args[3]=tile_height;
            func_args[4]=(intptr_t) tile_matrix;
            func_args[5]=(intptr_t) string_1;
            func_args[6]=(intptr_t) string_2;
            func_args[7]=n_tiles_height;
            func_args[8]=n_tiles_width;
            void* func_args_db = (void*) func_args;
            ocrGuid_t func_args_db_guid = guidify(func_args_db);

            task->add_dependency(task, func_args_event_p, 3);

            func_args_event_p->put(func_args_event_p,func_args_db_guid);

            ocrEdtSchedule(task_guid);
        }
    }

    ocrCleanup();
    return 0;
}

