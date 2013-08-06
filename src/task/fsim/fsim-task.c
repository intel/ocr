/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "task/fsim/fsim-task.h"
#include "debug.h"
#include "ocr-macros.h"

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
