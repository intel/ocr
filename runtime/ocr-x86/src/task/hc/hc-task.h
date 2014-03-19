/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __HC_TASK_H__
#define __HC_TASK_H__

#include "ocr-config.h"
#if defined(ENABLE_TASK_HC) || defined(ENABLE_TASKTEMPLATE_HC)

#include "hc/hc.h"
#include "ocr-task.h"
#include "utils/ocr-utils.h"

#ifdef ENABLE_TASKTEMPLATE_HC
/*! \brief Event Driven Task(EDT) template implementation
 */
typedef struct {
    ocrTaskTemplate_t base;
} ocrTaskTemplateHc_t;

typedef struct {
    ocrTaskTemplateFactory_t baseFactory;
} ocrTaskTemplateFactoryHc_t;

ocrTaskTemplateFactory_t * newTaskTemplateFactoryHc(ocrParamList_t* perType, u32 factoryId);
#endif /* ENABLE_TASKTEMPLATE_HC */

#ifdef ENABLE_TASK_HC

/*! \brief Event Driven Task(EDT) implementation for OCR Tasks
 */
typedef struct {
    ocrTask_t base;
    regNode_t * signalers; /**< Does not grow, set once when the task is created */
    ocrGuid_t* unkDbs;     /**< Contains the list of DBs dynamically acquired (through DB create) */
    u32 countUnkDbs;       /**< Count in unkDbs */
    u32 maxUnkDbs;         /**< Maximum number in unkDbs */
    volatile u32 frontierSlot; /**< Slot of the execution frontier
                                  This excludes once events */
    volatile u32 slotSatisfiedCount; /**< Number of slots satisfied */
    volatile u32 lock; /**< TODO: We can probably do with just atomics on frontierSlot and slotSatisfiedCount */
} ocrTaskHc_t;

typedef struct {
    ocrTaskFactory_t baseFactory;
} ocrTaskFactoryHc_t;

ocrTaskFactory_t * newTaskFactoryHc(ocrParamList_t* perType, u32 factoryId);
#endif /* ENABLE_TASK_HC */

#endif /* ENABLE_TASK_HC or ENABLE_TASKTEMPLATE_HC */
#endif /* __HC_TASK_H__ */
