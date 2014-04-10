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

#include "profiler.h"

#ifdef OCR_RUNTIME_PROFILER


#include <stdio.h>
#include <pthread.h>

// TODO: FIXME
extern pthread_key_t _profilerThreadData;

/* _profiler functions */
void _profilerInit(_profiler *self, profilerEvent_t event, u64 prevTicks) {
    self->accumulatorTicks = self->startTicks = self->endTicks = 0UL;
    self->countResume = 0;
    self->onStackCount = 1;
    self->myEvent = event;
    self->active = self->isPaused = 0;

    self->myData = (_profilerData*)pthread_getspecific(_profilerThreadData);
    if(self->myData) {
        self->active = true;
        if(self->myData->level >= 1) {
            ASSERT(prevTicks != 0);
            if(self->myData->stack[self->myData->level-1]->myEvent == event) {
                ++(self->myData->stack[self->myData->level-1]->onStackCount);
                self->endTicks = prevTicks; // We cheat to keep track of this overhead
                                            // startTicks will be udpated when we return from here
                                            // and we will be able to remove startTicks - endTicks
                                            // from our "parent"
            } else {
                _profilerPause(self->myData->stack[self->myData->level-1], prevTicks); // It's a "fake" pause in the sense that we still want this time in the parent's total
                ++(self->myData->level);
                ASSERT(self->myData->level < MAX_PROFILER_LEVEL);
                // We push ourself on the stack
                self->myData->stack[self->myData->level-1] = self;
                // _gettime will be called after we returned to remove the return overhead
            }
        } else {
            ++(self->myData->level);
            ASSERT(self->myData->level < MAX_PROFILER_LEVEL);
            // We push ourself on the stack
            self->myData->stack[self->myData->level-1] = self;
            // _gettime will be called after we returned to remove the return overhead
        }
    }
}

_profiler* _profilerDestroy(_profiler *self, u64 end) {
    u8 removedFromStack = 0;
    if(self->active && !self->isPaused) {
        ASSERT(self->myData->stack[self->myData->level-1]->onStackCount >= 1);
        if(--(self->myData->stack[self->myData->level-1]->onStackCount) == 0) {
            --(self->myData->level);
            self->myData->stack[self->myData->level] = NULL;
            removedFromStack = 1;

            self->endTicks = end;
            ASSERT(self->endTicks > self->startTicks);
            self->accumulatorTicks += self->endTicks - self->startTicks;
            self->isPaused = true;
        } else {
            ASSERT(self->startTicks > self->endTicks); // Our cheat to measure the overhead of profilerInit
            // We pause the "parent" to remove the overhead of this call
            _profilerPause(self->myData->stack[self->myData->level-1], end);
            // We compute the time to "fix-up" by
            ASSERT(self->accumulatorTicks == 0);
            ASSERT(self->onStackCount == 1); // This is 1 because it should not have changed at all
            self->isPaused = true;
            self->accumulatorTicks = self->startTicks - self->endTicks;
        }
    }

    if(!self->active) {
        return NULL;
    }

    ASSERT(self->active && self->isPaused);


    if(self->accumulatorTicks < (1+self->countResume)*self->myData->overheadTimer) {
        self->accumulatorTicks = 0;
    } else {
        self->accumulatorTicks -= (1+self->countResume)*self->myData->overheadTimer;
    }

    u64 accumulatorMs = self->accumulatorTicks/PROFILE_KHZ;
    u32 accumulatorNs = (u32)(1000000.0*((double)self->accumulatorTicks/PROFILE_KHZ - (double)accumulatorMs));

    if(removedFromStack) {
        // First the self counter
        _profilerSelfEntryAddTime(&(self->myData->selfEvents[self->myEvent]), accumulatorMs, accumulatorNs);

        // We also update the child counter
        if(self->myData->level >= 1) {
            // We have a parent so we update there
            _profiler *parentEventPtr = self->myData->stack[self->myData->level-1];
            u32 parentEvent = parentEventPtr->myEvent;
            ASSERT(parentEvent != self->myEvent);
            _profilerChildEntryAddTime(
                &(self->myData->childrenEvents[parentEvent][self->myEvent<parentEvent?self->myEvent:(self->myEvent-1)]),
                accumulatorMs, accumulatorNs);

            // We also add the time directly to the parent as the parent has been paused to avoid counting
            // the overhead of creating the child profiling object
            _profilerAddTime(self->myData->stack[self->myData->level-1], self->accumulatorTicks);

            if(self->myData->level >= 2) {
                // We have a parent of parent
                // We add the time in the [grandParent, parent] entry to indicate that
                // the time that the parent spent in the grandparent
                // was due to a child of parent (us in this case)
                _profiler *grandParentEventPtr = self->myData->stack[self->myData->level-2];
                u32 grandParentEvent = grandParentEventPtr->myEvent;
                ASSERT(grandParentEvent != parentEvent);
                _profilerChildEntryAddChildTime(
                    &(self->myData->childrenEvents[grandParentEvent][parentEvent<grandParentEvent?parentEvent:(parentEvent-1)]),
                    accumulatorMs, accumulatorNs);
            }
            return parentEventPtr;
        }
    } else {
        // Not removed from stack which means that this is a collapsed call
        ASSERT(self->myEvent == self->myData->stack[self->myData->level-1]->myEvent);
        _profilerFixup(self->myData->stack[self->myData->level-1], self->accumulatorTicks);
        return self->myData->stack[self->myData->level-1]; // We resume our "parent" from the pause earlier in this function
    }
    return NULL;
}

// This returns ns for 1000 tries!!
#define DO_OVERHEAD_DIFF(start, end, res) \
    if(end.tv_nsec < start.tv_nsec) { \
        end.tv_nsec += 1000000000L; \
        --end.tv_sec; \
    } \
    res = (end.tv_sec-start.tv_sec)*1000000L + (end.tv_nsec - start.tv_nsec)/1000;

/* _profilerData functions */
void _profilerDataInit(_profilerData *self) {
    self->level = 0;
    self->overheadTimer = 0;
    // Calibrate overhead
    u64 start, end, temp1, temp2;
    _gettime(start);
    u32 i;
    for(i=0; i<999; ++i) {
        asm volatile (
            "rdtscp"
            : "=a" (temp1), "=d" (temp2)
            :
            : "ecx"
            );
    }
    _gettime(end);
    ASSERT(end > start);
    self->overheadTimer = (end - start)/1000;
    fprintf(stderr, "Got RDTSCP overhead of %lu ticks\n", self->overheadTimer);
}

void _profilerDataDestroy(void* _self) {
    _profilerData *self = (_profilerData*)_self;

    // This will dump the profile and delete everything. This can be called
    // when the thread is exiting (and the TLS is destroyed)
    u32 i;
    for(i=0; i<(u32)MAX_EVENTS; ++i) {
        fprintf(self->output, "DEF %s %u\n", _profilerEventNames[i], i);
    }

    // Print out the entries
    for(i=0; i<(u32)MAX_EVENTS; ++i) {
        u32 j;
        for(j=0; j<(u32)MAX_EVENTS; ++j) {
            if(j == i) {
                _profilerSelfEntry *entry = &(self->selfEvents[i]);
                if(entry->count == 0) continue; // Skip entries with no content
                fprintf(self->output, "ENTRY %u:%u = count(%lu), sum(%lu.%06u), sumSq(%lu.%06u)\n",
                        i, j, entry->count, entry->sumMs, entry->sumNs, entry->sumSqMs, entry->sumSqNs);
            } else {
                // Child entry
                _profilerChildEntry *entry = &(self->childrenEvents[i][j<i?j:(j-1)]);
                if(entry->count == 0) continue;
                fprintf(self->output,
                        "ENTRY %u:%u = count(%lu), sum(%lu.%06u), sumSq(%lu.%06u), sumChild(%lu.%06u), sumSqChild(%lu.%06u)\n",
                        i, j, entry->count, entry->sumMs, entry->sumNs, entry->sumSqMs,
                        entry->sumSqNs, entry->sumInChildrenMs, entry->sumInChildrenNs,
                        entry->sumSqInChildrenMs, entry->sumSqInChildrenNs);
            }
        }
    }
    fclose(self->output);
}

#endif /* OCR_RUNTIME_PROFILER */
