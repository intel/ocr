/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sal.h"
#include "ocr-types.h"

void sal_exit(u64 errorCode) {
    // TODO: Call into the PD
}

void sal_abort() {
    // TODO: Call into the PD
}

void sal_assert(bool cond) {
    if(!cond) sal_abort();
}

void sal_printf(const char* string, ...) {
    // TODO: Call into the PD
}


