#ifndef RMD_MATH_H
#define RMD_MATH_H

# define M_PI           3.14159265358979323846

inline void  __attribute__((always_inline)) sincosf(float x, float* sinx, float* cosx) {
    __asm__ (
        "sincosF32 %0, %2\n\t"
        "or32 %1, %0, 0x0\n\t" /* cosine takes the lower 32 bits */
        "slr64  %0, %0, 32\n\t"
        : "=r" (*sinx), "=r" (*cosx)
        : "r" (x)
        :
    );
}

inline double  __attribute__((always_inline)) sin(double x) {
    float sinx, cosx;
    sincosf(x, &sinx, &cosx);
    return sinx;
}

inline double  __attribute__((always_inline)) cos(double x) {
    float sinx, cosx;
    sincosf(x, &sinx, &cosx);
    return cosx;
}

inline double __attribute__((always_inline)) fabs(double x) {
    if (x<0) return -x;
    return x;
}

#endif //RMD_MATH_H
