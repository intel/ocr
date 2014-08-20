/* Modified in 2014 by Romain Cledat (now at Intel). The original
 * license (BSD) is below. This file is also subject to the license
 * aggrement located in the file LICENSE and cannot be distributed
 * without it. This notice cannot be removed or modified
 */

/* Copyright (c) 2011, Romain Cledat
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Georgia Institute of Technology nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ROMAIN CLEDAT BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __OCR_RUNTIME_PROFILER_H__
#define __OCR_RUNTIME_PROFILER_H__

#include "ocr-config.h"
#ifdef OCR_RUNTIME_PROFILER
#include "ocr-types.h"

#ifndef ENABLE_COMP_PLATFORM_PTHREAD
#error "The current runtime profiler is only compatible with the pthread comp-platform at this time"
#endif

#include <stdio.h>

// This file will be automatically generated and will contain the
// profilerEvent_t enum as well as the _profilerEventNames array
// It is automatically generated from all the START_PROFILE calls
#include "profilerAutoGen.h"

#ifndef MAX_PROFILER_LEVEL
#define MAX_PROFILER_LEVEL 512
#endif

//timeMs = a/PROFILE_KHZ;
//timeNs = (unsigned int)(1000000.0*((double)a/PROFILE_KHZ - (double)timeMs));

#define _gettime(ticks)                          \
    do {                                         \
        u64 d;                                   \
        asm volatile (                           \
            "rdtscp \n\t"                        \
            : "=a" (ticks), "=d" (d)             \
            : /* no input */                     \
            : "ecx"                              \
            );                                   \
        ticks |= (d << 32);                      \
    } while(0);


typedef struct __profilerChildEntry {
    u64 count;
    u64 sumMs, sumSqMs, sumInChildrenMs, sumSqInChildrenMs;
    u32 sumNs, sumSqNs, sumInChildrenNs, sumSqInChildrenNs;
} _profilerChildEntry;

typedef struct __profilerSelfEntry {
    u64 count;
    u64 sumMs, sumSqMs;
    u32 sumNs, sumSqNs;
} _profilerSelfEntry;

struct __profilerData;

typedef struct __profiler {
    u64 accumulatorTicks, startTicks, endTicks;
    struct __profilerData *myData;
    u32 countResume;
    u32 onStackCount;
    profilerEvent_t myEvent;
    u8 active, isPaused;
} _profiler;

typedef struct __profilerData {
    _profiler* stack[MAX_PROFILER_LEVEL];
    FILE *output;
    u64 overheadTimer;
    u32 level;

    _profilerSelfEntry selfEvents[MAX_EVENTS];
    _profilerChildEntry childrenEvents[MAX_EVENTS][MAX_EVENTS-1]; // We already have the self entry
} _profilerData;

/* _profilerChildEntry functions */
static inline void _profilerChildEntryReset(_profilerChildEntry *self) {
    self->count = self->sumMs = self->sumSqMs = self->sumInChildrenMs
        = self->sumSqInChildrenMs = 0UL;
    self->sumNs = self->sumSqNs = self->sumInChildrenNs
        = self->sumSqInChildrenNs = 0;
}

static inline void _profilerChildEntryAddTime(_profilerChildEntry *self,
                                              u32 timeMs, u32 timeNs) {
    ++self->count;
    self->sumNs += timeNs;
    if(self->sumNs >= 1000000) {
        self->sumNs -= 1000000;
        ++self->sumMs;
    }
    self->sumMs += timeMs;

    self->sumSqNs += (u32)((double)timeNs/(double)1e6*(double)timeNs);
    while(self->sumSqNs >= 1000000) {
        self->sumSqNs -= 1000000;
        ++self->sumSqMs;
    }
    self->sumSqMs += timeMs*timeMs;
}

static inline void _profilerChildEntryAddChildTime(_profilerChildEntry *self,
                                                   u32 timeMs, u32 timeNs) {
    self->sumInChildrenNs += timeNs;
    if(self->sumInChildrenNs >= 1000000) {
        self->sumInChildrenNs -= 1000000;
        ++self->sumInChildrenMs;
    }

    self->sumInChildrenMs += timeMs;

    self->sumSqInChildrenNs += (u32)((double)timeNs/(double)1e6*(double)timeNs);
    while(self->sumSqInChildrenNs >= 1000000) {
        self->sumSqInChildrenNs -= 1000000;
        ++self->sumSqInChildrenMs;
    }
    self->sumSqInChildrenMs += timeMs*timeMs;
}

/* _profilerSelfEntry functions */
static inline void _profilerSelfEntryReset(_profilerSelfEntry *self) {
    self->count = self->sumMs = self->sumSqMs = 0;
    self->sumNs = self->sumSqNs = 0;
}

static inline void _profilerSelfEntryAddTime(_profilerSelfEntry *self,
                                             u64 timeMs, u32 timeNs) {
    ++self->count;

    self->sumNs += timeNs;
    if(self->sumNs >= 1000000) {
        self->sumNs -= 1000000;
        ++self->sumMs;
    }
    self->sumMs += timeMs;

    self->sumSqNs += (u32)((double)timeNs/(double)1e6*(double)timeNs);
    while(self->sumSqNs >= 1000000) {
        self->sumSqNs -= 1000000;
        ++self->sumSqMs;
    }
    self->sumSqMs += timeMs*timeMs;
}

/* _profiler inline functions */
static inline void _profilerPause(_profiler *self, u64 realEndTicks) __attribute__((always_inline));
static inline void _profilerPause(_profiler *self, u64 realEndTicks) {
    // Code looks weird to try to get _gettime as early as possible
    if(realEndTicks == 0) {
        _gettime(self->endTicks);
        if(self->active && !self->isPaused) {
            --(self->myData->level);
            self->myData->stack[self->myData->level] = NULL;
            // Here: self->endTicks > self->startTicks
            self->accumulatorTicks += self->endTicks - self->startTicks;
            self->isPaused = 1;
        }
    } else {
        self->endTicks = realEndTicks;
        if(self->active && !self->isPaused) {
            // Here: self->endTicks > self->startTicks
            self->accumulatorTicks += self->endTicks - self->startTicks;
            self->isPaused = 1;
        }
    }
}

static inline void _profilerResume(_profiler *self, u8 isFakePause) __attribute__((always_inline));
static inline void _profilerResume(_profiler *self, u8 isFakePause) {
    if(self->active && self->isPaused) {
        if(!isFakePause) {
            ++(self->myData->level);
            self->myData->stack[self->myData->level-1] = self; // Put ourself back on the stack
        }
        self->isPaused = 0;
        ++self->countResume;
        _gettime(self->startTicks);
    }
}

static inline void _profilerFixup(_profiler *self, u64 ticks) __attribute__((always_inline));
static inline void _profilerFixup(_profiler *self, u64 ticks) {
    // Here: self->accumulatorTicks > ticks
    self->accumulatorTicks -= ticks;
}

static inline void _profilerAddTime(_profiler *self, u64 ticks) __attribute__((always_inline));
static inline void _profilerAddTime(_profiler *self, u64 ticks) {
    self->accumulatorTicks += ticks;
}

/* Non-inline profiler functions */
void _profilerInit(_profiler *self, profilerEvent_t ev, u64 lastTick);
_profiler* _profilerDestroy(_profiler *self, u64 tick);

/* Non-inline profilerData functions */
void _profilerDataInit(_profilerData *self);
void _profilerDataDestroy(void * self);

/* the overhead we are not accounting for is:
  *    - overhead of _gettime
 */
#define START_PROFILE(eventId)                                          \
    u64 _tempTicks;                                                     \
    _gettime(_tempTicks)                                                \
    /*_sync_synchronize();*/                                            \
    _profiler *_flightweight = (_profiler*)malloc(sizeof(_profiler));   \
    _profilerInit(_flightweight, (profilerEvent_t)eventId, _tempTicks); \
    _gettime(_flightweight->startTicks);


#define PAUSE_PROFILE                           \
    do { _profilerPause(_flightweight); } while(0);

#define RESUME_PROFILE                          \
    do { _profilerResume(_flightweight, 0); } while(0);

#define RETURN_PROFILE(val)                                             \
    do {                                                                \
        _gettime(_tempTicks);                                           \
        /*_sync_synchronize(); */                                       \
        _profiler *_tres = _profilerDestroy(_flightweight, _tempTicks); \
        free(_flightweight);                                            \
        if(_tres) _profilerResume(_tres, 1);                            \
        return val;                                                     \
    } while(0);

#define EXIT_PROFILE                                                    \
    do {                                                                \
        _gettime(_tempTicks);                                           \
        /*_sync_synchronize();*/                                        \
        _profiler *_tres = _profilerDestroy(_flightweight, _tempTicks); \
        free(_flightweight);                                            \
        if(_tres) _profilerResume(_tres, 1);                            \
    } while(0);

#else

#define START_PROFILE(name)
#define PAUSE_PROFILE
#define RESUME_PROFILE

#define RETURN_PROFILE(val) return val;
#define EXIT_PROFILE
#endif /* OCR_RUNTIME_PROFILER */

#endif
