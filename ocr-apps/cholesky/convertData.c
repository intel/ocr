/*
 * Simple code to convert a text file for the matrix into
 * a binary format that is layed out in chunks of tileSize*tileSize
 * to be placed in DRAM so that it can easily be read by the XE
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

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

    FILE *in, *out;
    int matrixSize = -1, tileSize = -1, numTiles = -1;
    int outSelLevel = -1;
    int c;
    int printStatOut = 0;

    char *fileNameIn, *fileNameOut = "cholesky_out.bin";

    if ( argc == 1) {
        printf("Cholesky\n");
        printf("__________________________________________________________________________________________________\n");
        printf("Solves an OCR version of a Tiled Cholesky Decomposition with non-MKL math kernels\n\n");
        printf("Usage:\n");
        printf("\tcholesky {Arguments}\n\n");
        printf("Arguments:\n");
        printf("\t--ds -- Specify the Size of the Input Matrix\n");
        printf("\t--ts -- Specify the Tile Size\n");
        printf("\t--fi -- Specify the Input File Name of the Matrix\n");
        printf("\t--ps -- Print status updates on algorithm computation to stdout\n");
        printf("\t\t0: Disable (default)\n");
        printf("\t\t1: Enable\n");

        return 1;
    }
    else
    {
        // Reads in 5 arguments, input matrix file name, output matrix filename, datasize, tilesize, and
        while (1)
        {
            static struct option long_options[] =
                {
                    {"ds", required_argument, 0, 'a'},
                    {"ts", required_argument, 0, 'b'},
                    {"fi", required_argument, 0, 'c'},
                    {"ps", required_argument, 0, 'd'},
                    {0, 0, 0, 0}
                };

            int option_index = 0;

            c = getopt_long(argc, argv, "a:b:c:d", long_options, &option_index);

            if (c == -1) // Detect the end of the options
                break;
            switch (c)
            {
            case 'a':
                //printf("Option a: matrixSize with value '%s'\n", optarg);
                matrixSize = (int) atoi(optarg);
                break;
            case 'b':
                //printf("Option b: tileSize with value '%s'\n", optarg);
                tileSize = (int) atoi(optarg);
                break;
            case 'c':
                //printf("Option c: fileNameIn with value '%s'\n", optarg);
                fileNameIn = optarg;
                break;
            case 'd':
                //PRINTF("Option d: printStatOut with value '%s'\n", optarg);
                printStatOut = (int) atoi(optarg);
                break;
            default:
                printf("ERROR: Invalid argument switch\n\n");
                printf("Cholesky\n");
                printf("__________________________________________________________________________________________________\n");
                printf("Solves an OCR-only version of a Tiled Cholesky Decomposition with non-MKL math kernels\n\n");
                printf("Usage:\n");
                printf("\tcholesky {Arguments}\n\n");
                printf("Arguments:\n");
                printf("\t--ds -- Specify the Size of the Input Matrix\n");
                printf("\t--ts -- Specify the Tile Size\n");
                printf("\t--fi -- Specify the Input File Name of the Matrix\n");
                printf("\t--es -- Print status updates on algorithm computation to stdout\n");
                printf("\t\t0: Disable (default)\n");
                printf("\t\t1: Enable\n");

                return 1;
            }
        }
    }

    if(matrixSize == -1 || tileSize == -1)
    {
        printf("Must specify matrix size and tile size\n");
        return 1;
    }
    else if(matrixSize % tileSize != 0)
    {
        printf("Incorrect tile size %d for the matrix of size %d \n", tileSize, matrixSize);
        return 1;
    }

    numTiles = matrixSize/tileSize;

    in = fopen(fileNameIn, "r");
    out = fopen(fileNameOut, "wb");
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
