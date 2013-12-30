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
#define ENABLE_ALLOCATOR_TLSF

// Comp-platform
#define ENABLE_COMP_PLATFORM_PTHREAD
//#define ENABLE_COMP_PLATFORM_FSIM

// Comp-target
#define ENABLE_COMP_TARGET_HC

// Datablock
#define ENABLE_DATABLOCK_REGULAR

// Event
#define ENABLE_EVENT_HC

// GUID provider
#define ENABLE_GUID_PTR

// Mem-platform
#define ENABLE_MEM_PLATFORM_MALLOC

// Mem-target
#define ENABLE_MEM_TARGET_SHARED

// Policy domain
#define ENABLE_POLICY_DOMAIN_HC

// Scheduler
#define ENABLE_SCHEDULER_HC

// SAL layer to use
#define ENABLE_SAL_X86

// Task
#define ENABLE_TASK_HC

// Worker
#define ENABLE_WORKER_HC

// Workpile
#define ENABLE_WORKPILE_HC

// HAL layer to use
#define HAL_FILE "hal/ocr-hal-x86_64.h"

#endif /* __OCR_CONFIG_H__ */


