/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_GUID_COUNTED_MAP

#include "debug.h"
#include "guid/counted/counted-map-guid.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"

// Default hashtable's number of buckets
#define DEFAULT_NB_BUCKETS 10

// Guid is composed of : (LOCID KIND COUNTER)
#define GUID_BIT_SIZE 64
#define GUID_LOCID_SIZE 6 // Warning! 2^6 locId max, bump that up for more.
#define GUID_KIND_SIZE 4 // Warning! check ocrGuidKind struct definition for correct size 
#define GUID_COUNTER_SIZE (GUID_BIT_SIZE-(GUID_KIND_SIZE+GUID_LOCID_SIZE))

#define GUID_LOCID_MASK (((((u64)1)<<GUID_LOCID_SIZE)-1)<<(GUID_COUNTER_SIZE+GUID_KIND_SIZE))
#define GUID_LOCID_SHIFT_RIGHT (GUID_BIT_SIZE-GUID_LOCID_SIZE)

#define GUID_KIND_MASK (((((u64)1)<<GUID_KIND_SIZE)-1)<< GUID_COUNTER_SIZE)
#define GUID_KIND_SHIFT_RIGHT (GUID_LOCID_SHIFT_RIGHT-GUID_KIND_SIZE)

#define KIND_LOCATION 0
#define LOCID_LOCATION GUID_KIND_SIZE

// GUID 'id' counter, atomically incr when a new GUID is requested
static u64 guidCounter = 0;

void countedMapDestruct(ocrGuidProvider_t* self) {
    destructHashtable(((ocrGuidProviderCountedMap_t *) self)->guidImplTable);
    runtimeChunkFree((u64)self, NULL);
}

void countedMapBegin(ocrGuidProvider_t *self, ocrPolicyDomain_t *pd) {
    // Nothing to do
}

void countedMapStart(ocrGuidProvider_t *self, ocrPolicyDomain_t *pd) {
    self->pd = pd;
    //Initialize the map now that we have an assigned policy domain
    ocrGuidProviderCountedMap_t * derived = (ocrGuidProviderCountedMap_t *) self;
    derived->guidImplTable = newHashtableModulo(pd, DEFAULT_NB_BUCKETS);
}

void countedMapStop(ocrGuidProvider_t *self) {
    // Nothing to do
}

void countedMapFinish(ocrGuidProvider_t *self) {
    // Nothing to do
}

/**
 * @brief Utility function to extract a kind from a GUID.
 */
static ocrGuidKind getKindFromGuid(ocrGuid_t guid) {
    return (ocrGuidKind) ((guid & GUID_KIND_MASK) >> GUID_COUNTER_SIZE);
}

static u64 locationToLocId(ocrLocation_t location) {
    // No support for location yet 
    return (u64) 0;
}

/**
 * @brief Utility function to generate a new GUID.
 */
static u64 generateNextGuid(ocrGuidProvider_t* self, ocrGuidKind kind) {
    u64 locId = (u64) locationToLocId(self->pd->myLocation);
    u64 locIdShifted = locId << LOCID_LOCATION;
    u64 kindShifted = kind << KIND_LOCATION;
    u64 guid = (locIdShifted | kindShifted) << GUID_COUNTER_SIZE;
    //PERF: Can privatize the counter by working out something with thread-id
    u64 newCount = hal_xadd64(&guidCounter, 1);
    // double check if we overflow the guid's counter size
    ASSERT(newCount < ((u64)1<<GUID_COUNTER_SIZE));
    guid |= newCount;
    return guid;
}

/**
 * @brief Generate a guid for 'val' by increasing the guid counter.
 */
static u8 countedMapGetGuid(ocrGuidProvider_t* self, ocrGuid_t* guid, u64 val, ocrGuidKind kind) {
    // Here no need to allocate
    u64 newGuid = generateNextGuid(self, kind);
    hashtablePut(((ocrGuidProviderCountedMap_t *) self)->guidImplTable, (void *) newGuid, (void *) val);
    *guid = (ocrGuid_t) newGuid;
    return 0;
}

u8 countedMapCreateGuid(ocrGuidProvider_t* self, ocrFatGuid_t *fguid, u64 size, ocrGuidKind kind) {
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *policy = NULL;
    getCurrentEnv(&policy, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_ALLOC
    msg.type = PD_MSG_MEM_ALLOC | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(size) = size; // allocate 'size' payload as metadata
    PD_MSG_FIELD(properties) = 0; // TODO:  What flags should be defined?  Where are symbolic constants for them defined?
    PD_MSG_FIELD(type) = GUID_MEMTYPE;

    RESULT_PROPAGATE(policy->fcts.processMessage (policy, &msg, true));

    void * ptr = (void *)PD_MSG_FIELD(ptr);
    // Fill in the GUID value and in the fatGuid
    // and registers its associated metadata ptr
    countedMapGetGuid(self, &(fguid->guid), (u64) ptr, kind);
    // Update the fat GUID's metaDataPtr
    fguid->metaDataPtr = ptr;
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

/**
 * @brief Returns the value associated with a guid and its kind if requested.
 */
static u8 countedMapGetVal(ocrGuidProvider_t* self, ocrGuid_t guid, u64* val, ocrGuidKind* kind) {
    *val = (u64) hashtableGet(((ocrGuidProviderCountedMap_t *) self)->guidImplTable, (void *) guid);
    if(kind) {
        *kind = getKindFromGuid(guid);
    }
    return 0;
}

/**
 * @brief Get the 'kind' of the guid pointed object.
 */
static u8 countedMapGetKind(ocrGuidProvider_t* self, ocrGuid_t guid, ocrGuidKind* kind) {
    *kind = getKindFromGuid(guid);
    return 0;
}

u8 countedMapReleaseGuid(ocrGuidProvider_t *self, ocrFatGuid_t fatGuid, bool releaseVal) {
    ocrGuid_t guid = fatGuid.guid;
    // If there's metaData associated with guid we need to deallocate memory
    if(releaseVal && (fatGuid.metaDataPtr != NULL)) {
        ocrPolicyMsg_t msg;
        ocrPolicyDomain_t *policy = NULL;
        getCurrentEnv(&policy, NULL, NULL, &msg);
    #define PD_MSG (&msg)
    #define PD_TYPE PD_MSG_MEM_UNALLOC
        msg.type = PD_MSG_MEM_UNALLOC | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD(ptr) = fatGuid.metaDataPtr;
        PD_MSG_FIELD(type) = GUID_MEMTYPE;
        RESULT_PROPAGATE(policy->fcts.processMessage (policy, &msg, true));
    #undef PD_MSG
    #undef PD_TYPE
    }
    // In any case, we need to recycle the guid
    ocrGuidProviderCountedMap_t * derived = (ocrGuidProviderCountedMap_t *) self;
    hashtableRemove(derived->guidImplTable, (void *)guid);
    return 0;
}

static ocrGuidProvider_t* newGuidProviderCountedMap(ocrGuidProviderFactory_t *factory,
                                             ocrParamList_t *perInstance) {
    ocrGuidProvider_t *base = (ocrGuidProvider_t*) runtimeChunkAlloc(sizeof(ocrGuidProviderCountedMap_t), NULL);
    base->fcts = factory->providerFcts;
    base->pd = NULL;
    base->id = factory->factoryId;
    return base;
}

/****************************************************/
/* OCR GUID PROVIDER COUNTED MAP FACTORY            */
/****************************************************/

static void destructGuidProviderFactoryCountedMap(ocrGuidProviderFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrGuidProviderFactory_t *newGuidProviderFactoryCountedMap(ocrParamList_t *typeArg, u32 factoryId) {
    ocrGuidProviderFactory_t *base = (ocrGuidProviderFactory_t*) 
        runtimeChunkAlloc(sizeof(ocrGuidProviderFactoryCountedMap_t), (void *)1);

    base->instantiate = &newGuidProviderCountedMap;
    base->destruct = &destructGuidProviderFactoryCountedMap;
    base->factoryId = factoryId;
    base->providerFcts.destruct = &countedMapDestruct;
    base->providerFcts.begin = &countedMapBegin;
    base->providerFcts.start = &countedMapStart;
    base->providerFcts.stop = &countedMapStop;
    base->providerFcts.finish = &countedMapFinish;
    base->providerFcts.getGuid = &countedMapGetGuid;
    base->providerFcts.createGuid = &countedMapCreateGuid;
    base->providerFcts.getVal = &countedMapGetVal;
    base->providerFcts.getKind = &countedMapGetKind;
    base->providerFcts.releaseGuid = &countedMapReleaseGuid;
    return base;
}

#endif /* ENABLE_GUID_COUNTED_MAP */
