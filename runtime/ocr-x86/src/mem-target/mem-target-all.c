/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "mem-target/mem-target-all.h"
#include "debug.h"

const char * memtarget_types[] = {
    "shared",
    NULL
};

inline ocrMemTargetFactory_t *newMemTargetFactory(memTargetType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_MEM_TARGET_SHARED
    case memTargetShared_id:
        return newMemTargetFactoryShared(typeArg);
#endif
    default:
        ASSERT(0);
        return NULL;
    };
}

void initializeMemTargetOcr(ocrMemTargetFactory_t * factory, ocrMemTarget_t * self, ocrParamList_t *perInstance) {
    self->fguid.guid = UNINITIALIZED_GUID;
    self->fguid.metaDataPtr = self;
    self->pd = NULL;

    self->size = ((paramListMemTargetInst_t *)perInstance)->size;
    self->level = ((paramListMemTargetInst_t *)perInstance)->level;
    self->startAddr = self->endAddr = 0ULL;
    self->memories = NULL;
    self->memoryCount = 0;
    self->fcts = factory->targetFcts;
}
