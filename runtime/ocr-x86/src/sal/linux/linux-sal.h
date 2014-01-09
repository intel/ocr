/**
 * @brief SAL platform for Linux
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __SAL_LINUX_H__
#define __SAL_LINUX_H__

#include "ocr-config.h"
#ifdef ENABLE_SAL_LINUX

#include "ocr-sal.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"


typedef struct {
    ocrSal_t base;
} ocrSalLinux_t;

typedef struct {
    ocrSalFactory_t base;
} ocrSalFactoryLinux_t;

ocrSalFactory_t* newSalFactoryLinux(ocrParamList_t *perType);

#endif /* ENABLE_SAL_LINUX */
#endif /* __SAL_LINUX_H__ */
