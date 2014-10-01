/*
 * Simple code to convert the binary output file to
 * a readable one
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


int main(int argc, const char *argv[]) {
//    if(argc != 5) {
//        printf("Usage: %s matrixSize tileSize binaryFileName textFileName\n", argv[0]);
//        return 1;
//    }

    FILE *in, *out;
    int matrixSize = -1;
    int tileSize = -1;
    int numTiles = -1;

    int index;
    for (index=0; index < argc; index++)
    {
       if (compare_string(argv[index], "--ds")  == 0)
            matrixSize = atoi(argv[index+1]);

        else if (compare_string(argv[index], "--ts")  == 0)
            tileSize = atoi(argv[index+1]);
    }

    numTiles = matrixSize/tileSize;

    in = fopen(argv[argc - 2], "rb");
    out = fopen(argv[argc - 1], "w");
    if(!in || !out) {
        printf("Cannot open input or output files.\n");
        return 1;
    }
    int i, j, i_b, j_b;
    double val;
    for ( i = 0; i < numTiles; ++i ) {
        for( i_b = 0; i_b < tileSize; ++i_b) {
            for ( j = 0; j <= i; ++j ) {
                if(i != j) {
                    for(j_b = 0; j_b < tileSize; ++j_b) {
                        fread(&val, sizeof(double), 1, in);
                        fprintf(out, "%lf ", val);
                    }
                } else {
                    for(j_b = 0; j_b <= i_b; ++j_b) {
                        fread(&val, sizeof(double), 1, in);
                        fprintf(out, "%lf ", val);
                    }
                }
            }
        }
    }
    fclose(in);
    fclose(out);

    return 0;
}
