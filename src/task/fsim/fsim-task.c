/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "task/fsim/fsim-task.h"
#include "debug.h"
#include "ocr-macros.h"

#if 0
// TODO sagnak, copy paste of HC tasks, so need the same stuff here too :(
typedef ocrTaskHc_t ocrTaskFSIM_t;

typedef struct _ocrMessageTaskFSIM_t {
    ocrTaskHc_t base;
    ocrPolicyCtx_t message;
} ocrMessageTaskFSIM_t;



typedef ocrMessageTaskFactoryHc_t ocrMessageTaskFactoryFSIM_t;

typedef struct {
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
#endif

/*************************************************************************************/

void destructTaskTemplateFactoryFSIM(ocrTaskTemplateFactory_t* base) {
    ocrTaskTemplateFactoryFSIM_t* derived = (ocrTaskTemplateFactoryFSIM_t*) base;
    free(derived);
}

ocrTaskTemplate_t * newTaskTemplateFSIM (ocrTaskTemplateFactory_t* factory,
                                      ocrEdt_t executePtr, u32 paramc, u32 depc, ocrParamList_t *perInstance) {
    ocrTaskTemplateFSIM_t* template = (ocrTaskTemplateFSIM_t*) checkedMalloc(template, sizeof(ocrTaskTemplateFSIM_t));
    ocrTaskTemplate_t * base = (ocrTaskTemplate_t *) template;
    base->paramc = paramc;
    base->depc = depc;
    base->executePtr = executePtr;
    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_EDT_TEMPLATE);
    return base;
}

ocrTaskTemplateFactory_t * newTaskTemplateFactoryFSIM(ocrParamList_t* perType) {
    ocrTaskTemplateFactoryFSIM_t* derived = (ocrTaskTemplateFactoryFSIM_t*) checkedMalloc(derived, sizeof(ocrTaskTemplateFactoryFSIM_t));
    ocrTaskTemplateFactory_t* base = (ocrTaskTemplateFactory_t*) derived;
    base->instantiate = newTaskTemplateFSIM;
    base->destruct =  destructTaskTemplateFactoryFSIM;
    //TODO What taskTemplateFcts is supposed to do ?
    return base;
}

void destructMessageTaskTemplateFactoryFSIM(ocrTaskTemplateFactory_t* base) {
    ocrMessageTaskTemplateFactoryFSIM_t* derived = (ocrMessageTaskTemplateFactoryFSIM_t*) base;
    free(derived);
}

ocrTaskTemplate_t * newMessageTaskTemplateFSIM (ocrTaskTemplateFactory_t* factory,
                                      ocrEdt_t executePtr, u32 paramc, u32 depc, ocrParamList_t *perInstance) {
    ocrMessageTaskTemplateFSIM_t* template = (ocrMessageTaskTemplateFSIM_t*) checkedMalloc(template, sizeof(ocrMessageTaskTemplateFSIM_t));
    ocrTaskTemplate_t * base = (ocrTaskTemplate_t *) template;
    base->paramc = paramc;
    base->depc = depc;
    base->executePtr = executePtr;
    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_EDT_TEMPLATE);
    return base;
}

ocrTaskTemplateFactory_t * newMessageTaskTemplateFactoryFSIM(ocrParamList_t* perType) {
    ocrMessageTaskTemplateFSIM_t* derived = (ocrMessageTaskTemplateFSIM_t*) checkedMalloc(derived, sizeof(ocrMessageTaskTemplateFSIM_t));
    ocrTaskTemplateFactory_t* base = (ocrTaskTemplateFactory_t*) derived;
    base->instantiate = newMessageTaskTemplateFSIM;
    base->destruct =  destructMessageTaskTemplateFactoryFSIM;
    //TODO What taskTemplateFcts is supposed to do ?
    return base;
}
