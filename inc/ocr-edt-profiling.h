/**
 * @cond IGNORED_FILES
 *
 * This file is ignored by Doxygen
 */

/**
 * @brief Profiling support
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_EDT_PROFILING_H__
#define __OCR_EDT_PROFILING_H__

#include <ocr-types.h>

struct _profileStruct {
    u8     *fname;
    u8     numIcCoeff;  // Num of coefficients for instruction count
    u8     numFpCoeff;  // Num of coefficients for floating point ops
    u8     numRdCoeff;  // Num of coefficients for reads
    u8     numWrCoeff;  // Num of coefficients for writes
    double icVar;       // Variance in instruction count model
    double fpVar;       // Variance in floating point model
    double rdVar;       // Variance in reads model
    double wrVar;       // Variance in writes model
    double *icCoeff;    // Coefficients of instruction count polynomial
    double *fpCoeff;    // Coefficients of floating point polynomial
    double *rdCoeff;    // Coefficients of reads polynomial
    double *wrCoeff;    // Coefficients of writes polynomial
} profileStruct;

struct _dbWeightStruct {
    u8     *fname;
    u16    slots[2];    // Most dominant slots
    u8     weights[2];  // Corresponding weights to most dominant slots, %
    u8     sd[2];       // Corresponding sd to most dominant slots, 0-100
    u8     coverage;    // Coverage of most dominant 2 slots
} dbWeightStruct;

#endif /* __OCR_EDT_PROFILING_H__ */
/**
 * @endcond
 */
