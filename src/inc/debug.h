/**
 * @brief Some debug utilities for OCR
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <pthread.h>
#include <stdio.h>
#include <assert.h>


#define PRINTF(format, ...) do { fprintf(stderr, format, ## __VA_ARGS__); } while(0);


#ifdef OCR_DEBUG
/**
 * @brief No debugging messages are printed
 *
 * Note that you can still be in debugging mode
 * and have no messages printed as this would allow
 * all ASSERTs to be checked
 */
#define DEBUG_LVL_NONE      0
#define OCR_DEBUG_0_STR "NONE"

/**
 * @brief Only warnings are printed
 */
#define DEBUG_LVL_WARN      1
#define OCR_DEBUG_1_STR "WARN"

/**
 * @brief Warnings and informational
 * messages are printed
 *
 * Default debug level if nothing is specified
 * for #OCR_DEBUG_LVL (compile time)
 */
#define DEBUG_LVL_INFO      2
#define OCR_DEBUG_2_STR "INFO"

/**
 * @brief Warnings, informational
 * messages and verbose messages are printed
 */
#define DEBUG_LVL_VERB      3
#define OCR_DEBUG_3_STR "VERB"

/**
 * @brief Everything is printed
 */
#define DEBUG_LVL_VVERB     4
#define OCR_DEBUG_4_STR "VVERB"

#ifndef OCR_DEBUG_LVL
/**
 * @brief Debug level
 *
 * This controls the verbosity of the
 * debug messages in debug mode
 */
#define OCR_DEBUG_LVL DEBUG_LVL_INFO
#endif /* OCR_DEBUG_LVL */

#ifdef OCR_DEBUG_ALLOCATOR
#define OCR_DEBUG_ALLOCATOR 1
#else
#define OCR_DEBUG_ALLOCATOR 0
#endif
#define OCR_DEBUG_ALLOCATOR_STR "ALLOC"
#ifndef DEBUG_LVL_ALLOCATOR
#define DEBUG_LVL_ALLOCATOR OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_COMP_PLATFORM
#define OCR_DEBUG_COMP_PLATFORM 1
#else
#define OCR_DEBUG_COMP_PLATFORM 0
#endif
#define OCR_DEBUG_COMP_PLATFORM_STR "COMP-PLAT"
#ifndef DEBUG_LVL_COMP_PLATFORM
#define DEBUG_LVL_COMP_PLATFORM OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_COMP_TARGET
#define OCR_DEBUG_COMP_TARGET 1
#else
#define OCR_DEBUG_COMP_TARGET 0
#endif
#define OCR_DEBUG_COMP_TARGET_STR "COMP-TARG"
#ifndef DEBUG_LVL_COMP_TARGET
#define DEBUG_LVL_COMP_TARGET OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_DATABLOCK
#define OCR_DEBUG_DATABLOCK 1
#else
#define OCR_DEBUG_DATABLOCK 0
#endif
#define OCR_DEBUG_DATABLOCK_STR "DB"
#ifndef DEBUG_LVL_DATABLOCK
#define DEBUG_LVL_DATABLOCK OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_EVENT
#define OCR_DEBUG_EVENT 1
#else
#define OCR_DEBUG_EVENT 0
#endif
#define OCR_DEBUG_EVENT_STR "EVT"
#ifndef DEBUG_LVL_EVENT
#define DEBUG_LVL_EVENT OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_GUID
#define OCR_DEBUG_GUID 1
#else
#define OCR_DEBUG_GUID 0
#endif
#define OCR_DEBUG_GUID_STR "GUID"
#ifndef DEBUG_LVL_GUID
#define DEBUG_LVL_GUID OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_INIPARSING
#define OCR_DEBUG_INIPARSING 1
#else
#define OCR_DEBUG_INIPARSING 0
#endif
#define OCR_DEBUG_INIPARSING_STR "INI-PARSING"
#ifndef DEBUG_LVL_INIPARSING
#define DEBUG_LVL_INIPARSING OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_MACHINE
#define OCR_DEBUG_MACHINE 1
#else
#define OCR_DEBUG_MACHINE 0
#endif
#define OCR_DEBUG_MACHINE_STR "MACHINE-DESC"
#ifndef DEBUG_LVL_MACHINE
#define DEBUG_LVL_MACHINE OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_MEM_PLATFORM
#define OCR_DEBUG_MEM_PLATFORM 1
#else
#define OCR_DEBUG_MEM_PLATFORM 0
#endif
#define OCR_DEBUG_MEM_PLATFORM_STR "MEM-PLAT"
#ifndef DEBUG_LVL_MEM_PLATFORM
#define DEBUG_LVL_MEM_PLATFORM OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_MEM_TARGET
#define OCR_DEBUG_MEM_TARGET 1
#else
#define OCR_DEBUG_MEM_TARGET 0
#endif
#define OCR_DEBUG_MEM_TARGET_STR "MEM-TARG"
#ifndef DEBUG_LVL_MEM_TARGET
#define DEBUG_LVL_MEM_TARGET OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_POLICY
#define OCR_DEBUG_POLICY 1
#else
#define OCR_DEBUG_POLICY 0
#endif
#define OCR_DEBUG_POLICY_STR "POLICY"
#ifndef DEBUG_LVL_POLICY
#define DEBUG_LVL_POLICY OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_SCHEDULER
#define OCR_DEBUG_SCHEDULER 1
#else
#define OCR_DEBUG_SCHEDULER 0
#endif
#define OCR_DEBUG_SCHEDULER_STR "SCHED"
#ifndef DEBUG_LVL_SCHEDULER
#define DEBUG_LVL_SCHEDULER OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_STATS
#define OCR_DEBUG_STATS 1
#else
#define OCR_DEBUG_STATS 0
#endif
#define OCR_DEBUG_STATS_STR "STATS"
#ifndef DEBUG_LVL_STATS
#define DEBUG_LVL_STATS OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_SYNC
#define OCR_DEBUG_SYNC 1
#else
#define OCR_DEBUG_SYNC 0
#endif
#define OCR_DEBUG_SYNC_STR "SYNC"
#ifndef DEBUG_LVL_SYNC
#define DEBUG_LVL_SYNC OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_TASK
#define OCR_DEBUG_TASK 1
#else
#define OCR_DEBUG_TASK 0
#endif
#define OCR_DEBUG_TASK_STR "EDT"
#ifndef DEBUG_LVL_TASK
#define DEBUG_LVL_TASK OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_WORKER
#define OCR_DEBUG_WORKER 1
#else
#define OCR_DEBUG_WORKER 0
#endif
#define OCR_DEBUG_WORKER_STR "WORKER"
#ifndef DEBUG_LVL_WORKER
#define DEBUG_LVL_WORKER OCR_DEBUG_LVL
#endif

#ifdef OCR_DEBUG_WORKPILE
#define OCR_DEBUG_WORKPILE 1
#else
#define OCR_DEBUG_WORKPILE 0
#endif
#define OCR_DEBUG_WORKPILE_STR "WORKPILE"
#ifndef DEBUG_LVL_WORKPILE
#define DEBUG_LVL_WORKPILE OCR_DEBUG_LVL
#endif


// Imply OCR_STATUS
#define OCR_STATUS
// Imply ASSERTs
#define OCR_ASSERT

#define DO_DEBUG_TYPE(type, level) \
    if(OCR_DEBUG_##type  && level <= DEBUG_LVL_##type) {                               \
        static const char* __type __attribute__((unused)) = OCR_DEBUG_##type##_STR;    \
        static const char* __level __attribute__((unused)) = OCR_DEBUG_##level##_STR;


// TODO: Re-add the worker thing once I figure out a way to not make it segfault
#define DEBUG(format, ...) do { fprintf(stderr, "%s(%s) W 0: " format, __type, __level, /*(u64)getCurrentWorkerContext()->sourceObj,*/ ## __VA_ARGS__); } while(0);
#define DPRINTF_TYPE(type, level, format, ...) do {                                              \
        if(OCR_DEBUG_##type && level <= DEBUG_LVL_##type) {                                      \
            fprintf(stderr, OCR_DEBUG_##type##_STR "(" OCR_DEBUG_##level##_STR ") W 0: " format, \
                    /*(u64)getCurrentWorkerContext()->sourceObj, */ ## __VA_ARGS__);             \
        }                                                                                        \
    } while(0);

#else
#define DO_DEBUG_TYPE(level) if(0) {
#define DEBUG(format, ...)
#define DPRINTF_TYPE(type, level, format, ...)
#endif /* OCR_DEBUG */

#define DO_DEBUG_TYPE_INT(type, level) DO_DEBUG_TYPE(type, level)
#define DO_DEBUG(level) DO_DEBUG_TYPE_INT(DEBUG_TYPE, level)

#define DPRINTF_TYPE_INT(type, level, format, ...) DPRINTF_TYPE(type, level, format, ## __VA_ARGS__)
#define DPRINTF(level, format, ...) DPRINTF_TYPE_INT(DEBUG_TYPE, level, format, ## __VA_ARGS__)

#define END_DEBUG }

#ifdef OCR_STATUS
#define STATUS(format, ...)                                             \
    PRINTF("##OCR-STATUS %s:%d " format, __FILE__, __LINE__,##__VA_ARGS__);
#else
#define STATUS(format, ...)
#endif /* OCR_STATUS */

#define STATUS0(format) STATUS("%s" format, "")

#ifdef OCR_ASSERT
#define ASSERT(a) do { assert(a); } while(0);
#define RESULT_ASSERT(a, op, b) do { assert((a) op (b)); } while(0);
#define RESULT_TRUE(a) do { assert(a); } while(0);
#else
#define ASSERT(a)
#define RESULT_ASSERT(a, op, b) do { a; } while(0);
#define RESULT_TRUE(a) do { a; } while(0);
#endif /* OCR_ASSERT */

#endif /* __DEBUG_H__ */
