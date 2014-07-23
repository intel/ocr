/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

// Basic, serial implementation of Cooley-Tukey algorithm.
//
// Used to verify results of more complex OCR implementations.
//


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
            double twiddle_real = cos(-2 * M_PI * k / N);
            double twiddle_imag = sin(-2 * M_PI * k / N);
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
