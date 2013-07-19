/**
 * @brief Trivial implementation of GUIDs
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "debug.h"
#include "guid/ptr/ptr-guid.h"
#include "ocr-macros.h"

#include <stdlib.h>

typedef struct {
    ocrGuid_t guid;
    ocrGuidKind kind;
} ocrGuidImpl_t;

static void ptrDestruct(ocrGuidProvider_t* self) {
    free(self);
    return;
}

static u8 ptrGetGuid(ocrGuidProvider_t* self, ocrGuid_t* guid, u64 val, ocrGuidKind kind) {
    ocrGuidImpl_t * guidInst = malloc(sizeof(ocrGuidImpl_t));
    guidInst->guid = (ocrGuid_t)val;
    guidInst->kind = kind;
    *guid = (u64) guidInst;
    return 0;
}

static u8 ptrGetVal(ocrGuidProvider_t* self, ocrGuid_t guid, u64* val, ocrGuidKind* kind) {
    ocrGuidImpl_t * guidInst = (ocrGuidImpl_t *) guid;
    *val = (u64) guidInst->guid;
    if(kind)
        *kind = guidInst->kind;
    return 0;
}

static u8 ptrGetKind(ocrGuidProvider_t* self, ocrGuid_t guid, ocrGuidKind* kind) {
    ocrGuidImpl_t * guidInst = (ocrGuidImpl_t *) guid;
    *kind = guidInst->kind;
    return 0;
}

static u8 ptrReleaseGuid(ocrGuidProvider_t *self, ocrGuid_t guid) {
    free((ocrGuidImpl_t*) guid);
    return 0;
}

static ocrGuidProvider_t* newGuidProviderPtr(ocrGuidProviderFactory_t *factory,
                                             ocrParamList_t *perInstance) {
    ocrGuidProvider_t *base = (ocrGuidProvider_t*)checkedMalloc(
        base, sizeof(ocrGuidProviderPtr_t));
    base->fctPtrs = &(factory->providerFcts);
    return base;
}

/****************************************************/
/* OCR GUID PROVIDER PTR FACTORY                    */
/****************************************************/

static void destructGuidProviderFactoryPtr(ocrGuidProviderFactory_t *factory) {
    free(factory);
}

ocrGuidProviderFactory_t *newGuidProviderFactoryPtr(ocrParamList_t *typeArg) {
    ocrGuidProviderFactory_t *base = (ocrGuidProviderFactory_t*)
        checkedMalloc(base, sizeof(ocrGuidProviderFactoryPtr_t));
    base->instantiate = &newGuidProviderPtr;
    base->destruct = &destructGuidProviderFactoryPtr;
    base->providerFcts.destruct = &ptrDestruct;
    base->providerFcts.getGuid = &ptrGetGuid;
    base->providerFcts.getVal = &ptrGetVal;
    base->providerFcts.getKind = &ptrGetKind;
    base->providerFcts.releaseGuid = &ptrReleaseGuid;

    return base;
}
