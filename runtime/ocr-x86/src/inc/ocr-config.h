/**
 * @brief Configuration for the OCR runtime
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_CONFIG_H__
#define __OCR_CONFIG_H__

// Allocator
#define ENABLE_ALLOCATOR_TLSF 1

// Comp-platform
#define ENABLE_COMP_PLATFORM_PTHREAD 1
//#define ENABLE_COMP_PLATFORM_FSIM 1

// Comp-target
#define ENABLE_COMP_TARGET_HC 1

// Datablock
#define ENABLE_DATABLOCK_REGULAR 1

// Event
#define ENABLE_EVENT_HC 1

// GUID provider
#define ENABLE_GUID_PTR 1

// Mem-platform
#define ENABLE_MEM_PLATFORM_MALLOC 1

// Mem-target
#define ENABLE_MEM_TARGET_SHARED 1

// Policy domain
#define ENABLE_POLICY_DOMAIN_HC 1

// Scheduler
#define ENABLE_SCHEDULER_HC 1

// Task
#define ENABLE_TASK_HC 1

// Worker
#define ENABLE_WORKER_HC 1

// Workpile
#define ENABLE_WORKPILE_HC 1

#endif /* __OCR_CONFIG_H__ */


