/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __HC_TASK_H__
#define __HC_TASK_H__

#include "hc/hc.h"
#include "ocr-task.h"
#include "ocr-utils.h"

/*! \brief Event Driven Task(EDT) template implementation
 */
typedef struct {
    ocrTaskTemplate_t base;
} ocrTaskTemplateHc_t;

/*! \brief Event Driven Task(EDT) implementation for OCR Tasks
 */
typedef struct {
    ocrTask_t base;
    regNode_t * waiters;
    regNode_t * signalers; // Does not grow, set once when the task is created
} ocrTaskHc_t;

typedef struct {
    ocrTaskTemplateFactory_t baseFactory;
} ocrTaskTemplateFactoryHc_t;

ocrTaskTemplateFactory_t * newTaskTemplateFactoryHc(ocrParamList_t* perType);

typedef struct {
    ocrTaskFactory_t baseFactory;
} ocrTaskFactoryHc_t;

ocrTaskFactory_t * newTaskFactoryHc(ocrParamList_t* perType);
#endif /* __HC_TASK_H__ */
