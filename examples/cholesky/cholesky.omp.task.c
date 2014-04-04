#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include <omp.h>

void sequential_cholesky ( int k, int tileSize, double** in_lkji_kkk, double*** out_lkji_kkkp1 ) {
    int index = 0, iB = 0, jB = 0, kB = 0, jBB = 0;
    double** aBlock = in_lkji_kkk;
    double** lBlock;
    lBlock = (double**) malloc(sizeof(double*)*tileSize);
    for( index = 0; index < tileSize; ++index ) {
        lBlock[index] = (double*) malloc(sizeof(double)*(index+1));
    }

    for( kB = 0 ; kB < tileSize ; ++kB ) {
        if( aBlock[ kB ][ kB ] <= 0 ) {
            fprintf(stderr,"Not a symmetric positive definite (SPD) matrix\n");
            exit(1);
        } else {
            lBlock[ kB ][ kB ] = sqrt( aBlock[ kB ][ kB ] );
        }

        for(jB = kB + 1; jB < tileSize ; ++jB ) {
            lBlock[ jB ][ kB ] = aBlock[ jB ][ kB ] / lBlock[ kB ][ kB ];
        }

        for(jBB= kB + 1; jBB < tileSize ; ++jBB ) {
            for(iB = jBB ; iB < tileSize ; ++iB ) {
                aBlock[ iB ][ jBB ] -= lBlock[ iB ][ kB ] * lBlock[ jBB ][ kB ];
            }
        }
    }
    *out_lkji_kkkp1 = lBlock;
}

void trisolve ( int k, int j, int tileSize, double** in_lkji_jkk, double** in_lkji_kkkp1, double*** out_lkji_jkkp1 ) {
    int i = 0;
    int iB, jB, kB;
    double** aBlock = in_lkji_jkk;
    double** liBlock = in_lkji_kkkp1;

    double ** loBlock;
    loBlock = (double**) malloc (sizeof(double*)*tileSize);
    for(i = 0; i < tileSize; ++i) {
        loBlock[i] = (double*) malloc (sizeof(double*)*tileSize);
    }

    for( kB = 0; kB < tileSize ; ++kB ) {
        for( iB = 0; iB < tileSize ; ++iB ) {
            loBlock[ iB ][ kB ] = aBlock[ iB ][ kB ] / liBlock[ kB ][ kB ];
        }

        for( jB = kB + 1 ; jB < tileSize; ++jB ) {
            for( iB = 0; iB < tileSize; ++iB ) {
                aBlock[ iB ][ jB ] -= liBlock[ jB ][ kB ] * loBlock[ iB ][ kB ];
            }
        }
    }
    *out_lkji_jkkp1 = loBlock;
}

void update_diagonal ( int k, int j, int i, int tileSize, double** in_lkji_jjk, double** in_lkji_jkkp1, double*** out_lkji_jjkp1 ) {
    int iB, jB, kB;
    double temp = 0;
    double** aBlock = in_lkji_jjk;
    double** l2Block = in_lkji_jkkp1;

    for( jB = 0; jB < tileSize ; ++jB ) {
        for( kB = 0; kB < tileSize ; ++kB ) {
            temp = 0 - l2Block[ jB ][ kB ];
            for( iB = jB; iB < tileSize; ++iB ) {
                aBlock[ iB ][ jB ] += temp * l2Block[ iB ][ kB ];
            }
        }
    }
    *out_lkji_jjkp1 = aBlock;
}

void update_nondiagonal ( int k, int j, int i, int tileSize, double** in_lkji_jik, double** in_lkji_ikkp1, double** in_lkji_jkkp1, double*** out_lkji_jikp1 ) {
    double temp;
    int jB, kB, iB;
    double**  aBlock = in_lkji_jik;
    double** l1Block = in_lkji_jkkp1;
    double** l2Block = in_lkji_ikkp1;

    for( jB = 0; jB < tileSize ; ++jB ) {
        for( kB = 0; kB < tileSize ; ++kB ) {
            temp = 0 - l2Block[ jB ][ kB ];
            for( iB = 0; iB < tileSize ; ++iB ) {
                aBlock[ iB ][ jB ] += temp * l1Block[ iB ][ kB ];
            }
        }
    }
    *out_lkji_jikp1 = aBlock;
}

int main(int argc, char** argv) {
    int i, j, k, ii;
    double **A, ** temp;
    int A_i, A_j, T_i, T_j;
    FILE *in, *out;
    int i_b, j_b;

    int matrixSize = -1;
    int tileSize = -1;
    int numTiles = -1;

    double ***** lkji;
    if ( argc !=  4 ) {
        printf("Usage: ./Cholesky matrixSize tileSize fileName (found %d args)\n", argc);
        return 1;
    }

    matrixSize = atoi(argv[1]);
    tileSize = atoi(argv[2]);
    if ( matrixSize % tileSize != 0) {
        printf("Incorrect tile size %d for the matrix of size %d \n", tileSize, matrixSize);
        return 1;
    }

    numTiles = matrixSize/tileSize;


    lkji = (double *****) malloc(sizeof(double****)*numTiles);
    for( i = 0 ; i < numTiles ; ++i ) {
        lkji[i] = (double****) malloc(sizeof(double***)*(i+1));;
        for( j = 0 ; j <= i ; ++j ) {
            lkji[i][j] = (double***) malloc(sizeof(double**)*(numTiles+1));
        }
    }


    in = fopen(argv[3], "r");
    if( !in ) {
        printf("Cannot find file: %s\n", argv[3]);
        return 1;
    }
    A = (double**) malloc (sizeof(double*)*matrixSize);
    for( i = 0; i < matrixSize; ++i) {
        A[i] = (double*) malloc(sizeof(double)*matrixSize);
    }

    for( i = 0; i < matrixSize; ++i ) {
        for( j = 0; j < matrixSize-1; ++j ) {
            fscanf(in, "%lf ", &A[i][j]);
        }
        fscanf(in, "%lf\n", &A[i][j]);
    }

    for( i = 0 ; i < numTiles ; ++i ) {
        for( j = 0 ; j <= i ; ++j ) {
            // Allocate memory for the tiles.
            temp = (double**) malloc (sizeof(double*)*tileSize);
            for( ii = 0; ii < tileSize; ++ii ) {
                temp[ii] = (double*) malloc (sizeof(double)*tileSize);
            }
            // Split the matrix into tiles and write it into the item space at time 0.
            // The tiles are indexed by tile indices (which are tag values).
            for( A_i = i*tileSize, T_i = 0 ; T_i < tileSize; ++A_i, ++T_i ) {
                for( A_j = j*tileSize, T_j = 0 ; T_j < tileSize; ++A_j, ++T_j ) {
                    temp[ T_i ][ T_j ] = A[ A_i ][ A_j ];
                }
            }

            lkji[i][j][0] = temp;
        }
    }

    struct timeval a;
    struct timeval b;
    gettimeofday(&a, 0);

    for ( k = 0; k < numTiles; ++k ) {
        sequential_cholesky (k, tileSize, lkji[k][k][k], &(lkji[k][k][k+1]));

        #pragma omp parallel for private(j) shared(k,tileSize,numTiles,lkji) schedule(static)
        for( j = k + 1 ; j < numTiles ; ++j ) {
            trisolve (k, j, tileSize , lkji[j][k][k], lkji[k][k][k+1], &(lkji[j][k][k+1]));
        }

        #pragma omp parallel
        {
            #pragma omp single
            {
                for( j = k + 1 ; j < numTiles ; ++j ) {
                    #pragma omp task firstprivate(j)
                    {
                        //printf("thread %d running up diag with %d\n", omp_get_thread_num(), j);
                        update_diagonal ( k, j, j, tileSize , lkji[j][j][k], lkji[j][k][k+1], &(lkji[j][j][k+1]));
                        //printf("thread %d done running up diag with %d\n", omp_get_thread_num(), j);
                    }
                    for( i = k + 1 ; i < j ; ++i ) {
                        #pragma omp task firstprivate(j,i)
                        {
                            //printf("thread %d running up nondiag with %d,%d\n", omp_get_thread_num(), j,i);
                            update_nondiagonal ( k, j, i, tileSize, lkji[j][i][k], lkji[i][k][k+1], lkji[j][k][k+1], &(lkji[j][i][k+1]));
                            //printf("thread %d done running up nondiag with %d,%d\n", omp_get_thread_num(), j,i);
                        }
                    }
                }
            }
            #pragma omp taskwait
        }
    }

    gettimeofday(&b, 0);
    printf("The computation took %f seconds\r\n",((b.tv_sec - a.tv_sec)*1000000+(b.tv_usec - a.tv_usec))*1.0/1000000);

    out = fopen("cholesky.out", "w");
    for ( i = 0; i < numTiles; ++i ) {
        for( i_b = 0; i_b < tileSize; ++i_b) {
            int k = 1;
            for ( j = 0; j <= i; ++j ) {
                temp = lkji[i][j][k];
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

    fclose(out);
    for( i = 0; i < matrixSize; ++i ) {
        free(A[i]);
    }
    free(A);
    return 0;
}
