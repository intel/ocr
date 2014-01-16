/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __SAL_ALL_H_
#define __SAL_ALL_H_

#include "debug.h"
#include "ocr-config.h"
#include "ocr-sal.h"
#include "utils/ocr-utils.h"

typedef enum _salType_t {
    salLinux_id,
    salMax_id
} salType_t;

const char * sal_types [] = {
    "LINUX",
    NULL
};

#include "sal/linux/linux-sal.h"

inline ocrSalFactory_t * newSalFactory(salType_t type, ocrParamList_t *perType) {
    switch(type) {
#ifdef ENABLE_SAL_LINUX
    case salLinux_id:
        return newSalFactoryLinux(perType);
#endif
    default:
        ASSERT(0);
    }
    return NULL;
}

#endif /* __SAL_ALL_H_ */
