/**
 * @brief Trivial implementation of GUIDs
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_GUID_PTR

#include "debug.h"
#include "guid/ptr/ptr-guid.h"
#include "ocr-policy-domain.h"

#include "ocr-sysboot.h"

typedef struct {
    ocrGuid_t guid;
    ocrGuidKind kind;
} ocrGuidImpl_t;

void ptrDestruct(ocrGuidProvider_t* self) {
    runtimeChunkFree((u64)self, NULL);
}

void ptrBegin(ocrGuidProvider_t *self, ocrPolicyDomain_t *pd) {
    // Nothing else to do
}

void ptrStart(ocrGuidProvider_t *self, ocrPolicyDomain_t *pd) {
    self->pd = pd;
    // Nothing else to do
}

void ptrStop(ocrGuidProvider_t *self) {
    // Nothing to do
}

void ptrFinish(ocrGuidProvider_t *self) {
    // Nothing to do
}

u8 ptrGetGuid(ocrGuidProvider_t* self, ocrGuid_t* guid, u64 val, ocrGuidKind kind) {
// Comment out call to pdMalloc for the time being, and instead send a message to process a PD_MSG_MEM_ALLOC request.
#if 0
    ocrGuidImpl_t * guidInst = self->pd->pdMalloc(self->pd, sizeof(ocrGuidImpl_t));
    guidInst->guid = (ocrGuid_t)val;
    guidInst->kind = kind;
    *guid = (u64) guidInst;
#else
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *policy = NULL;
    ocrTask_t *task = NULL;
    getCurrentEnv(&policy, NULL, &task, &msg);
#define PD_MSG(&msg)
#define PD_TYPE PD_MSG_MEM_ALLOC
    msg.type = PD_MSG_MEM_ALLOC | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    msg.srcLocation  = myLocation;
    msg.destLocation = parentLocation;
    PD_MSG_FIELD(size) = sizeof(ocrGuidImpl_t);
    PD_MSG_FIELD(properties) = 0; // TODO:  What flags should be defined?  Where are symbolic constants for them defined?
    PD_MSG_FIELD(type) = GUID_MEMTYPE;

    if (policy->processMessage (policy, &msg, true) == 0) {
        ocrGuidImpl_t * guidInst = ((ocrGuidImpl_t *) ((u64) PD_MSG_FIELD(ptr)));
        guidInst->guid = (ocrGuid_t)val;
        guidInst->kind = kind;
        *guid = (ocrGuid_t) guidInst;  // TODO:  Is this right?  Note that "(ocrGuid_t)" WAS "(u64)", and though ocrGuid_t is #defined to u64, it seems like we should use ocrGuid_t, not u64.
    } else {
        ASSERT (false);  // TODO: Deal with failed request.
    }
#undef PD_MSG
#undef PD_TYPE
#endif
    return 0;
}

u8 ptrCreateGuid(ocrGuidProvider_t* self, ocrFatGuid_t *fguid, u64 size, ocrGuidKind kind) {
// Comment out call to pdMalloc for the time being, and instead send a message to process a PD_MSG_MEM_ALLOC request.
#if 0
    // This is very stupid right now, we use pdMalloc/pdFree which means that
    // the metadata will not move easily but we can change this by asking the PD to
    // allocate memory for the metadata if needed.
    ocrGuidImpl_t *guidInst = self->pd->pdMalloc(self->pd, sizeof(ocrGuidImpl_t) + size);
    guidInst->guid = (ocrGuid_t)((u64)guidInst + sizeof(ocrGuidImpl_t));
    guidInst->kind = kind;
    fguid->guid = (u64)guidInst;
    fguid->metaDataPtr = (void*)((u64)guidInst + sizeof(ocrGuidImpl_t));
#else
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *policy = NULL;
    ocrTask_t *task = NULL;
    getCurrentEnv(&policy, NULL, &task, &msg);
#define PD_MSG(&msg)
#define PD_TYPE PD_MSG_MEM_ALLOC
    msg.type = PD_MSG_MEM_ALLOC | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    msg.srcLocation  = myLocation;
    msg.destLocation = parentLocation;
    PD_MSG_FIELD(size) = sizeof(ocrGuidImpl_t);
    PD_MSG_FIELD(properties) = 0; // TODO:  What flags should be defined?  Where are symbolic constants for them defined?
    PD_MSG_FIELD(type) = GUID_MEMTYPE;

    if (policy->processMessage (policy, &msg, true) == 0) {
        ocrGuidImpl_t * guidInst = ((ocrGuidImpl_t *) ((u64) PD_MSG_FIELD(ptr)));
        guidInst->guid = (ocrGuid_t)((u64)guidInst + sizeof(ocrGuidImpl_t));
        guidInst->kind = kind;
        fguid->guid = (ocrGuid_t)guidInst; // TODO:  Is this right?  Note that "(ocrGuid_t)" WAS "(u64)", and though ocrGuid_t is #defined to u64, it seems like we should use ocrGuid_t, not u64.
        fguid->metaDataPtr = (void*)((u64)guidInst + sizeof(ocrGuidImpl_t));
    } else {
        ASSERT (false);  // TODO: Deal with failed request.
    }
#undef PD_MSG
#undef PD_TYPE
#endif
    return 0;
}

u8 ptrGetVal(ocrGuidProvider_t* self, ocrGuid_t guid, u64* val, ocrGuidKind* kind) {
    ocrGuidImpl_t * guidInst = (ocrGuidImpl_t *) guid;
    *val = (u64) guidInst->guid;
    if(kind)
        *kind = guidInst->kind;
    return 0;
}

u8 ptrGetKind(ocrGuidProvider_t* self, ocrGuid_t guid, ocrGuidKind* kind) {
    ocrGuidImpl_t * guidInst = (ocrGuidImpl_t *) guid;
    *kind = guidInst->kind;
    return 0;
}

u8 ptrReleaseGuid(ocrGuidProvider_t *self, ocrFatGuid_t guid, bool releaseVal) {
    if(releaseVal) {
        ASSERT(guid.metaDataPtr);
        ASSERT((u64)guid.metaDataPtr == (u64)guid.guid + sizeof(ocrGuidImpl_t));
    }
// Comment out call to pdFree for the time being, and instead send a message to process a PD_MSG_MEM_UNALLOC request.
#if 0
    self->pd->pdFree(self->pd, (void*)guid.guid);
#else
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *policy = NULL;
    ocrTask_t *task = NULL;
    getCurrentEnv(&policy, NULL, &task, &msg);
#define PD_MSG(&msg)
#define PD_TYPE PD_MSG_MEM_UNALLOC
    msg.type = PD_MSG_MEM_UNALLOC | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    msg.srcLocation  = myLocation;
    msg.destLocation = parentLocation;
    PD_MSG_FIELD(ptr) = ((void *) guid.guid);
    PD_MSG_FIELD(type) = GUID_MEMTYPE;
    if (policy->processMessage (policy, &msg, true) != 0) {
        ASSERT (false);  // TODO: Deal with failed request.
    }
#undef PD_MSG
#undef PD_TYPE
#endif
    return 0;
}

ocrGuidProvider_t* newGuidProviderPtr(ocrGuidProviderFactory_t *factory,
                                      ocrParamList_t *perInstance) {
    ocrGuidProvider_t *base = (ocrGuidProvider_t*)runtimeChunkAlloc(
        sizeof(ocrGuidProviderPtr_t), NULL);
    base->fcts = factory->providerFcts;
    base->pd = NULL;
    base->id = factory->factoryId;
    return base;
}

/****************************************************/
/* OCR GUID PROVIDER PTR FACTORY                    */
/****************************************************/

void destructGuidProviderFactoryPtr(ocrGuidProviderFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrGuidProviderFactory_t *newGuidProviderFactoryPtr(ocrParamList_t *typeArg, u32 factoryId) {
    ocrGuidProviderFactory_t *base = (ocrGuidProviderFactory_t*)
        runtimeChunkAlloc(sizeof(ocrGuidProviderFactoryPtr_t), (void *)1);
    
    base->instantiate = &newGuidProviderPtr;
    base->destruct = &destructGuidProviderFactoryPtr;
    base->factoryId = factoryId;
    base->providerFcts.destruct = &ptrDestruct;
    base->providerFcts.begin = &ptrBegin;
    base->providerFcts.start = &ptrStart;
    base->providerFcts.stop = &ptrStop;
    base->providerFcts.finish = &ptrFinish;
    base->providerFcts.getGuid = &ptrGetGuid;
    base->providerFcts.createGuid = &ptrCreateGuid;
    base->providerFcts.getVal = &ptrGetVal;
    base->providerFcts.getKind = &ptrGetKind;
    base->providerFcts.releaseGuid = &ptrReleaseGuid;

    return base;
}

#endif /* ENABLE_GUID_PTR */
