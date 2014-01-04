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
    ocrGuidImpl_t * guidInst = self->pd->pdMalloc(self->pd, sizeof(ocrGuidImpl_t));
    guidInst->guid = (ocrGuid_t)val;
    guidInst->kind = kind;
    *guid = (u64) guidInst;
    return 0;
}

u8 ptrCreateGuid(ocrGuidProvider_t* self, ocrFatGuid_t *fguid, u64 size, ocrGuidKind kind) {
    // This is very stupid right now, we use pdMalloc/pdFree which means that
    // the metadata will not move easily but we can change this by asking the PD to
    // allocate memory for the metadata if needed.
    ocrGuidImpl_t *guidInst = self->pd->pdMalloc(self->pd, sizeof(ocrGuidImpl_t) + size);
    guidInst->guid = (ocrGuid_t)((u64)guidInst + sizeof(ocrGuidImpl_t));
    guidInst->kind = kind;
    fguid->guid = (u64)guidInst;
    fguid->metaDataPtr = (void*)((u64)guidInst + sizeof(ocrGuidImpl_t));
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
    self->pd->pdFree(self->pd, (void*)guid.guid);
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
        runtimeChunkAlloc(sizeof(ocrGuidProviderFactoryPtr_t), NULL);
    
    base->instantiate = &newGuidProviderPtr;
    base->destruct = &destructGuidProviderFactoryPtr;
    base->factoryId = factoryId;
    base->providerFcts.destruct = &ptrDestruct;
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
