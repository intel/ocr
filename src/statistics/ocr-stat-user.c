/**
 * @brief OCR implementation of messages sent by the statistics gathering
 * framework
 * @authors Romain Cledat, Intel Corporation
 * @date 2013-04-17
 * Copyright (c) 2013, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-stat-user.h"
#include "ocr-statistics.h"
#include "ocr-config.h"
#include "debug.h"
#include "ocr-policy-domain-getter.h"

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
    DO_DEBUG(DEBUG_LVL_VERB)
        DEBUG("Created a simple filter @ 0x%lx with parent 0x%lx\n", (u64)self, (u64)parent);
    END_DEBUG
    rself->messages = (intSimpleMessageNode_t*)malloc(sizeof(intSimpleMessageNode_t)*rself->maxCount);
}

static void simpleFilterDestruct(ocrStatsFilter_t *self) {
    FILTER_TYPE(simple)* rself = (FILTER_TYPE(simple)*)self;

    DO_DEBUG(DEBUG_LVL_VERB)
        DEBUG("Destroying simple filter @ 0x%lx\n", (u64)self);
    END_DEBUG
    // We alert our parent
    if(rself->base.parent) {
        DO_DEBUG(DEBUG_LVL_VVERB)
            DEBUG("Filter @ 0x%lx merging with parent 0x%lx\n", (u64)self, (u64)(rself->base.parent));
        END_DEBUG
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
    DO_DEBUG(DEBUG_LVL_VVERB)
        DEBUG("Filter @ 0x%lx received message (%ld): %ld -> %ld of type %d\n",
              (u64)self, rself->count, mess->src, mess->dest, (u32)mess->type);
    END_DEBUG
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
    DO_DEBUG(DEBUG_LVL_VERB)
        DEBUG("Created a file-dump filter @ 0x%lx with file 0x%lx\n", (u64)self, (u64)rself->outFile);
    END_DEBUG
    ASSERT(rself->outFile);
}

static void fileDumpFilterDestruct(ocrStatsFilter_t *self) {
    FILTER_TYPE(filedump)* rself = (FILTER_TYPE(filedump)*)self;

    DO_DEBUG(DEBUG_LVL_VERB)
        DEBUG("Destroying file-dump filter @ 0x%lx\n", (u64)self);
    END_DEBUG
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

    DO_DEBUG(DEBUG_LVL_VERB)
        DEBUG("Filter @ 0x%lx received merging from filter 0x%lx\n", (u64)self, (u64)other);
    END_DEBUG

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
