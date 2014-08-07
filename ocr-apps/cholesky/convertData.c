/*
 * Simple code to convert a text file for the matrix into
 * a binary format that is layed out in chunks of tileSize*tileSize
 * to be placed in DRAM so that it can easily be read by the XE
 */

#include <stdlib.h>
#include <stdio.h>

static double** readMatrix(int matrixSize, FILE* in) {
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

int main(int argc, const char *argv[]) {
    if(argc != 4) {
        printf("Usage: %s matrixSize tileSize fileName\n", argv[0]);
        return 1;
    }

    FILE *in, *out;
    int matrixSize = -1;
    int tileSize = -1;
    int numTiles = -1;

    matrixSize = atoi(argv[1]);
    tileSize = atoi(argv[2]);

    numTiles = matrixSize/tileSize;

    if(matrixSize % tileSize != 0) {
        printf("Incorrect tile size %d for matrix of size %d\n", tileSize, matrixSize);
        return 1;
    }

    in = fopen(argv[3], "r");
    out = fopen("cholesky_out.bin", "wb");
    if(!in || !out) {
        printf("Cannot open input or output files.\n");
        return 1;
    }
    double buf[tileSize*tileSize];
    int bufCount = 0;
    int i, j, A_i, A_j;

    // First put matrixSize and tileSize
    //bufInt[0] = matrixSize;
    //bufInt[1] = tileSize;
    //fwrite(bufInt, sizeof(int), 2, out);

    // Now read the matrix into a temporary place
    double **matrix = readMatrix(matrixSize, in);

    // Lay it out in tiles so that we can easily DMA it
    // into the various data-blocks
    for(i = 0; i < numTiles; ++i) {
        for(j = 0; j <= i; ++j) {
            bufCount = 0;
            for(A_i = i*tileSize; A_i < (i+1)*tileSize; ++A_i) {
                for(A_j = j*tileSize; A_j < (j+1)*tileSize; ++A_j) {
                    buf[bufCount++] = matrix[A_i][A_j];
                }
            }
            fwrite(buf, sizeof(double), tileSize*tileSize, out);
        }
    }

    fclose(in);
    fclose(out);

    return 0;
}
