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
//PERF: This parameter heavily impacts the GUID provider scalability !
#define DEFAULT_NB_BUCKETS 1000

// Guid is composed of : (LOCID KIND COUNTER)
#define GUID_BIT_SIZE 64
#define GUID_LOCID_SIZE 7 // Warning! 2^7 locId max, bump that up for more.
#define GUID_KIND_SIZE 5 // Warning! check ocrGuidKind struct definition for correct size

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
    //destructHashtable(((ocrGuidProviderCountedMap_t *) self)->guidImplTable);
    runtimeChunkFree((u64)self, NULL);
}

void countedMapBegin(ocrGuidProvider_t *self, ocrPolicyDomain_t *pd) {
    // Nothing to do
}

void countedMapStart(ocrGuidProvider_t *self, ocrPolicyDomain_t *pd) {
    self->pd = pd;
    //Initialize the map now that we have an assigned policy domain
    ocrGuidProviderCountedMap_t * derived = (ocrGuidProviderCountedMap_t *) self;
    derived->guidImplTable = newHashtableBucketLockedModulo(pd, DEFAULT_NB_BUCKETS);
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

/**
 * @brief Utility function to extract a kind from a GUID.
 */
static u64 extractLocIdFromGuid(ocrGuid_t guid) {
    return (u64) ((guid & GUID_LOCID_MASK) >> GUID_LOCID_SHIFT_RIGHT);
}

static ocrLocation_t locIdtoLocation(u64 locId) {
    //TODO: We assume there will be a mapping between a location
    //      and an 'id' stored in the guid. For now identity.
    return (ocrLocation_t) (locId);
}

static u64 locationToLocId(ocrLocation_t location) {
    //TODO: We assume there will be a mapping between a location
    //      and an 'id' stored in the guid. For now identity.
    u64 locId = (u64) ((int)location);
    // Make sure we're not overflowing location size
    ASSERT((locId < (1<<GUID_LOCID_SIZE)) && "GUID location ID overflows");
    return locId;
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
    ASSERT((newCount < ((u64)1<<GUID_COUNTER_SIZE)) && "GUID counter overflows");
    guid |= newCount;
    return guid;
}

/**
 * @brief Generate a guid for 'val' by increasing the guid counter.
 */
static u8 countedMapGetGuid(ocrGuidProvider_t* self, ocrGuid_t* guid, u64 val, ocrGuidKind kind) {
    // Here no need to allocate
    u64 newGuid = generateNextGuid(self, kind);
    hashtableConcBucketLockedPut(((ocrGuidProviderCountedMap_t *) self)->guidImplTable, (void *) newGuid, (void *) val);
    *guid = (ocrGuid_t) newGuid;
    return 0;
}

/**
 * @brief Allocates a piece of memory that embeds both
 * the guid and some meta-data payload behind it
 * fatGuid's metaDataPtr will point to.
 */
u8 countedMapCreateGuid(ocrGuidProvider_t* self, ocrFatGuid_t *fguid, u64 size, ocrGuidKind kind) {
    PD_MSG_STACK(msg);
    ocrPolicyDomain_t *policy = NULL;
    getCurrentEnv(&policy, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_ALLOC
    msg.type = PD_MSG_MEM_ALLOC | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_I(size) = size; // allocate 'size' payload as metadata
    PD_MSG_FIELD_I(properties) = 0; // TODO:  What flags should be defined?  Where are symbolic constants for them defined?
    PD_MSG_FIELD_I(type) = GUID_MEMTYPE;

    RESULT_PROPAGATE(policy->fcts.processMessage (policy, &msg, true));

    void * ptr = (void *)PD_MSG_FIELD_O(ptr);
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
    *val = (u64) hashtableConcBucketLockedGet(((ocrGuidProviderCountedMap_t *) self)->guidImplTable, (void *) guid);
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

/**
 * @brief Resolve location of a GUID
 */
u8 countedMapGetLocation(ocrGuidProvider_t* self, ocrGuid_t guid, ocrLocation_t* location) {
    //Resolve the actual location of the GUID
    *location = (ocrLocation_t) locIdtoLocation(extractLocIdFromGuid(guid));
    return 0;
}

/**
 * @brief Associate an already existing GUID to a value.
 * This is useful in the context of distributed-OCR to register
 * a local metadata represent for a foreign GUID.
 */
u8 countedMapRegisterGuid(ocrGuidProvider_t* self, ocrGuid_t guid, u64 val) {
    hashtableConcBucketLockedPut(((ocrGuidProviderCountedMap_t *) self)->guidImplTable, (void *) guid, (void *) val);
    return 0;
}

u8 countedMapReleaseGuid(ocrGuidProvider_t *self, ocrFatGuid_t fatGuid, bool releaseVal) {
    ocrGuid_t guid = fatGuid.guid;
    // If there's metaData associated with guid we need to deallocate memory
    if(releaseVal && (fatGuid.metaDataPtr != NULL)) {
        PD_MSG_STACK(msg);
        ocrPolicyDomain_t *policy = NULL;
        getCurrentEnv(&policy, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_UNALLOC
        msg.type = PD_MSG_MEM_UNALLOC | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD_I(allocatingPD.guid) = NULL_GUID;
        PD_MSG_FIELD_I(allocatingPD.metaDataPtr) = NULL;
        PD_MSG_FIELD_I(allocator.guid) = NULL_GUID;
        PD_MSG_FIELD_I(allocator.metaDataPtr) = NULL;
        PD_MSG_FIELD_I(ptr) = fatGuid.metaDataPtr;
        PD_MSG_FIELD_I(type) = GUID_MEMTYPE;
        RESULT_PROPAGATE(policy->fcts.processMessage (policy, &msg, true));
#undef PD_MSG
#undef PD_TYPE
    }
    // In any case, we need to recycle the guid
    ocrGuidProviderCountedMap_t * derived = (ocrGuidProviderCountedMap_t *) self;
    hashtableConcBucketLockedRemove(derived->guidImplTable, (void *)guid, NULL);
    return 0;
}

static ocrGuidProvider_t* newGuidProviderCountedMap(ocrGuidProviderFactory_t *factory,
        ocrParamList_t *perInstance) {
    ocrGuidProvider_t *base = (ocrGuidProvider_t*) runtimeChunkAlloc(sizeof(ocrGuidProviderCountedMap_t), PERSISTENT_CHUNK);
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
                                     runtimeChunkAlloc(sizeof(ocrGuidProviderFactoryCountedMap_t), NONPERSISTENT_CHUNK);

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
    base->providerFcts.getLocation = FUNC_ADDR(u8 (*)(ocrGuidProvider_t*, ocrGuid_t, ocrLocation_t*), countedMapGetLocation);
    base->providerFcts.registerGuid = FUNC_ADDR(u8 (*)(ocrGuidProvider_t*, ocrGuid_t, u64), countedMapRegisterGuid);
    base->providerFcts.releaseGuid = &countedMapReleaseGuid;
    return base;
}

#endif /* ENABLE_GUID_COUNTED_MAP */
