/**
 * @brief OCR implementation of messages sent by the statistics gathering
 * framework
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifdef OCR_ENABLE_STATISTICS
#include "debug.h"
#include "ocr-config.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-stat-user.h"
#include "ocr-statistics.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_TYPE STATS

#define DEFINE_MESSAGE(name, funcCreate, funcDestruct, funcDump)                                 \
    ocrStatsMessage_t* newStatsMessage_##name() {                                             \
    ocrSTMESS_##name##_t* self = (ocrSTMESS_##name##_t*)malloc(sizeof(ocrSTMESS_##name##_t)); \
    self->base.create = & funcCreate;                                                        \
    self->base.destruct = & funcDestruct;                                                    \
    self->base.dump = & funcDump;

#define END_DEFINE_MESSAGE return (ocrStatsMessage_t*)self; }

#define DEFINE_FILTER(name, funcCreate, funcDestruct, funcDump, funcNotify, funcMerge)           \
    ocrStatsFilter_t* newStatsFilter_##name() {                                               \
    ocrSTFILT_##name##_t* self = (ocrSTFILT_##name##_t*)malloc(sizeof(ocrSTFILT_##name##_t)); \
    self->base.create = & funcCreate;                                                        \
    self->base.destruct = & funcDestruct;                                                    \
    self->base.dump = & funcDump;                                                            \
    self->base.notify = & funcNotify;                                                        \
    self->base.merge = & funcMerge;


#define END_DEFINE_FILTER return (ocrStatsFilter_t*)self; }

// Simple message
static void simpleMessageCreate(ocrStatsMessage_t *self, ocrStatsEvt_t type, u64 tick, ocrGuid_t src,
                            ocrGuid_t dest, void* configuration) {

    return ocrStatsMessageCreate(self, type, tick, src, dest, configuration);
}

static void simpleMessageDestruct(ocrStatsMessage_t *self) {
    return ocrStatsMessageDestruct(self);
}

static char* simpleMessageDump(ocrStatsMessage_t *self) {
    return NULL;
}

DEFINE_MESSAGE(simple, simpleMessageCreate, simpleMessageDestruct, simpleMessageDump)
END_DEFINE_MESSAGE


// Simple filter that just records all events
static void simpleFilterCreate(ocrStatsFilter_t *self, ocrStatsFilter_t *parent, void* configuration) {
    FILTER_TYPE(simple)* rself = (FILTER_TYPE(simple)*)self;

    rself->base.parent = parent;
    rself->count = 0;
    rself->maxCount = 8; // Make a configurable number
    DPRINTF(DEBUG_LVL_VERB, "Created a simple filter @ 0x%lx with parent 0x%lx\n", (u64)self, (u64)parent);
    rself->messages = (intSimpleMessageNode_t*)malloc(sizeof(intSimpleMessageNode_t)*rself->maxCount);
}

static void simpleFilterDestruct(ocrStatsFilter_t *self) {
    FILTER_TYPE(simple)* rself = (FILTER_TYPE(simple)*)self;

    DPRINTF(DEBUG_LVL_VERB, "Destroying simple filter @ 0x%lx\n", (u64)self);
    // We alert our parent
    if(rself->base.parent) {
        DPRINTF(DEBUG_LVL_VVERB, "Filter @ 0x%lx merging with parent 0x%lx\n", (u64)self, (u64)(rself->base.parent));
        return rself->base.parent->merge(rself->base.parent, self, 1);
    } else {
        // Here, we destroy ourself
        free(rself->messages);
        free(self);
    }
}

static u64 simpleFilterDump(ocrStatsFilter_t *self, char** out, u64 chunk, void* configuration) {
    FILTER_TYPE(simple)* rself = (FILTER_TYPE(simple)*)self;

    if(rself->count == 0) return 0;

    ASSERT(chunk < rself->count);

    ocrGuidKind srcK, destK;

    *out = (char*)malloc(sizeof(char)*82); // The output message should always fit in 80 chars given the format

    intSimpleMessageNode_t *tmess = &(rself->messages[chunk]);

    ocrStat
    guidKind(getCurrentPD(),tmess->src, &srcK);
    guidKind(getCurrentPD(),tmess->dest, &destK);

    snprintf(*out, sizeof(char)*82, "%ld : T %d 0x%lx(%d) -> 0x%lx(%d) ", tmess->tick,
             tmess->type, tmess->src, srcK, tmess->dest, destK);

    if(chunk < rself->count - 1)
        return chunk+1;
    return 0;
}

static void simpleFilterNotify(ocrStatsFilter_t *self, const ocrStatsMessage_t *mess) {
    FILTER_TYPE(simple)* rself = (FILTER_TYPE(simple)*)self;
    if(rself->count + 1 == rself->maxCount) {
        // Allocate more space
        rself->maxCount *= 2;
        intSimpleMessageNode_t *t = (intSimpleMessageNode_t*)malloc(sizeof(intSimpleMessageNode_t)*rself->maxCount);
        memcpy(t, rself->messages, (rself->count - 1)*sizeof(intSimpleMessageNode_t));
        rself->messages = t;
    }
    DPRINTF(DEBUG_LVL_VVERB, "Filter @ 0x%lx received message (%ld): %ld -> %ld of type %d\n",
            (u64)self, rself->count, mess->src, mess->dest, (u32)mess->type);
    intSimpleMessageNode_t* tmess = &(rself->messages[rself->count++]);
    tmess->tick = mess->tick;
    tmess->src = mess->src;
    tmess->dest = mess->dest;
    tmess->type = mess->type;
}

static void simpleFilterMerge(ocrStatsFilter_t *self, ocrStatsFilter_t *other, u8 toKill) {
    ASSERT(0); // Not implemented for now
}

DEFINE_FILTER(simple, simpleFilterCreate, simpleFilterDestruct, simpleFilterDump, simpleFilterNotify, simpleFilterMerge)
END_DEFINE_FILTER

// Filter that dumps things in a file (not very optimized and global lock)
static void fileDumpFilterCreate(ocrStatsFilter_t *self, ocrStatsFilter_t *parent, void* configuration) {
    FILTER_TYPE(filedump)* rself = (FILTER_TYPE(filedump)*)self;

    ASSERT(parent == NULL);
    rself->base.parent = NULL;
    rself->lock = GocrLockFactory->instantiate(GocrLockFactory, NULL);

    // Open the file
    if(configuration)
        rself->outFile = fopen((char*)configuration, "w");
    else
        rself->outFile = fopen("ocrStats.log", "w");
    DPRINTF(DEBUG_LVL_VERB, "Created a file-dump filter @ 0x%lx with file 0x%lx\n", (u64)self, (u64)rself->outFile);
    ASSERT(rself->outFile);
}

static void fileDumpFilterDestruct(ocrStatsFilter_t *self) {
    FILTER_TYPE(filedump)* rself = (FILTER_TYPE(filedump)*)self;

    DPRINTF(DEBUG_LVL_VERB, "Destroying file-dump filter @ 0x%lx\n", (u64)self);
    // We close out the output file
    if(rself->outFile) {
        fclose(rself->outFile);
    }
    // Also destroy the lock
    rself->lock->destruct(rself->lock);

    // Destroy ourself as well
    free(self);
}

static u64 fileDumpFilterDump(ocrStatsFilter_t *self, char** out, u64 chunk, void* configuration) {
    ASSERT(0); // Should never be called

    return 0;
}

static void fileDumpFilterNotify(ocrStatsFilter_t *self, const ocrStatsMessage_t *mess) {
    ASSERT(0); // Should never be called
}

static void fileDumpFilterMerge(ocrStatsFilter_t *self, ocrStatsFilter_t *other, u8 toKill) {
    // This is when we need to basically dump all information from this filter in the file
    FILTER_TYPE(filedump)* rself = (FILTER_TYPE(filedump)*)self;

    DPRINTF(DEBUG_LVL_VERB, "Filter @ 0x%lx received merging from filter 0x%lx\n", (u64)self, (u64)other);

    if(rself->outFile) {
        char* outString;
        u64 nextChunk = 0;
        rself->lock->lock(rself->lock);
        do {
            nextChunk = other->dump(other, &outString, nextChunk, NULL);
            if(outString) {
                fprintf(rself->outFile, "%s\n", outString);
                free(outString);
            }
        } while(nextChunk);
        rself->lock->unlock(rself->lock);
    }

    if(toKill) {
        other->parent = NULL;
        other->destruct(other);
    }
}

DEFINE_FILTER(filedump, fileDumpFilterCreate, fileDumpFilterDestruct, fileDumpFilterDump,
           fileDumpFilterNotify, fileDumpFilterMerge)
END_DEFINE_FILTER


#endif /* OCR_ENABLE_STATISTICS */
