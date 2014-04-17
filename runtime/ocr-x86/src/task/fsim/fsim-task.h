/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __FSIM_TASK_H__
#define __FSIM_TASK_H__

#include "task/hc/hc-task.h"
#include "ocr-policy-domain.h"

// TODO sagnak, copy paste of HC tasks, so need the same stuff here too :(
typedef ocrTaskHc_t ocrTaskFSIM_t;
typedef struct _ocrMessageTaskFSIM_t {
    ocrTaskHc_t base;
    ocrPolicyCtx_t* message;
} ocrMessageTaskFSIM_t;



typedef ocrTaskFactoryHc_t ocrTaskFactoryFSIM_t;
typedef struct _ocrMessageTaskFactoryFSIM {
    ocrTaskFactory_t baseFactory;
    // singleton the factory passes on to each task instance
    ocrTaskFcts_t taskFctPtrs;
} ocrMessageTaskFactoryFSIM_t;



ocrTaskFactory_t * newTaskFactoryFSIM (ocrParamList_t* perType);
ocrTaskFactory_t * newMessageTaskFactoryFSIM (ocrParamList_t* perType);



typedef ocrTaskTemplateHc_t ocrTaskTemplateFSIM_t;
typedef struct {
    ocrTaskTemplate_t base;
} ocrMessageTaskTemplateFSIM_t;



typedef ocrTaskTemplateFactoryHc_t ocrTaskTemplateFactoryFSIM_t;
typedef struct {
    ocrTaskTemplateFactory_t baseFactory;
} ocrMessageTaskTemplateFactoryFSIM_t;



ocrTaskTemplateFactory_t * newTaskTemplateFactoryFSIM(ocrParamList_t* perType);

ocrTaskTemplateFactory_t * newMessageTaskTemplateFactoryFSIM(ocrParamList_t* perType);

#endif /* __FSIM_TASK_H__ */

