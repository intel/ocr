/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

// Serial version of Cooley-Tukey FFT in plain C.
//

#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "timer.h"

//#define PI 3.1415926

void ditfft2(float *X_real, float *X_imag, float *x_in, int N, int step) {
    if(N == 1) {
        X_real[0] = x_in[0];
        X_imag[0] = 0;
    } else {
        // DFT even side
        ditfft2(X_real, X_imag, x_in, N/2, 2 * step);
        ditfft2(X_real+N/2, X_imag+N/2, x_in+step, N/2, 2 * step);
        int k;
        for(k=0;k<N/2;k++) {
            float t_real = X_real[k];
            float t_imag = X_imag[k];
            float twiddle_real = cos(-2 * M_PI * k / N);
            float twiddle_imag = sin(-2 * M_PI * k / N);
            float xr = X_real[k+N/2];
            float xi = X_imag[k+N/2];

            // (a+bi)(c+di) = (ac - bd) + (bc + ad)i
            X_real[k] = t_real +
                (twiddle_real*xr - twiddle_imag*xi);
            X_imag[k] = t_imag +
                (twiddle_imag*xr + twiddle_real*xi);
            X_real[k+N/2] = t_real -
                (twiddle_real*xr - twiddle_imag*xi);
            X_imag[k+N/2] = t_imag -
                (twiddle_imag*xr + twiddle_real*xi);
        }
    }
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Specify size.\n");
        exit(0);
    }

    int iterations;
    if(argc > 2) {
        iterations = atoi(argv[2]);
    } else {
        iterations = 1;
    }
    int N = atoi(argv[1]);
    float *x_in = (float*)malloc(sizeof(float) * N);
    float *X_real = (float*)malloc(sizeof(float) * N);
    float *X_imag = (float*)malloc(sizeof(float) * N);
    int i;

    for(i=0;i<N;i++) {
        x_in[i] = 0;
    }
    x_in[1] = 1;
    x_in[3] = 2;
    x_in[6] = 5;

    double t = mysecond();

    for(i=0;i<iterations;i++) {
        ditfft2(X_real, X_imag, x_in, N, 1);
    }

    printf("Completed %d iterations in %f seconds\n",iterations,mysecond()-t);

    for(i=0;i<N;i++) {
        //printf("%d [%f + %fi]\n",i,X_real[i],X_imag[i]);
    }
}
