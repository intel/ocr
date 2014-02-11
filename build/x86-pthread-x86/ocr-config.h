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

// Constants used in the runtime

// Define this if building the PD builder program
// If this is defined, this will exclude everything
// that does not contribute to building the policy domain
//#define ENABLE_BUILDER_ONLY

// Allocator
#define ENABLE_ALLOCATOR_TLSF

// Comm-api
#define ENABLE_COMM_API_HANDLELESS

// Comm-platform
#define ENABLE_COMM_PLATFORM_NULL

// Comp-platform
#define ENABLE_COMP_PLATFORM_PTHREAD
//#define ENABLE_COMP_PLATFORM_FSIM

// Comp-target
#define ENABLE_COMP_TARGET_PASSTHROUGH

// Datablock
#define ENABLE_DATABLOCK_REGULAR

// Event
#define ENABLE_EVENT_HC

// External things (mostly needed by the INI parser)
#define ENABLE_EXTERNAL_DICTIONARY
#define ENABLE_EXTERNAL_INIPARSER

// GUID provider
#define ENABLE_GUID_PTR
#define ENABLE_GUID_COUNTED_MAP

// HAL layer to use
#define HAL_X86_64

// SAL layer to use
#define SAL_LINUX

// Mem-platform
#define ENABLE_MEM_PLATFORM_MALLOC

// Mem-target
#define ENABLE_MEM_TARGET_SHARED

// Policy domain
#define ENABLE_POLICY_DOMAIN_HC

// Scheduler
#define ENABLE_SCHEDULER_HC

// Sysboot layer to use
#define ENABLE_SYSBOOT_LINUX

// Task
#define ENABLE_TASK_HC

// Task template
#define ENABLE_TASKTEMPLATE_HC

// Worker
#define ENABLE_WORKER_HC

// Workpile
#define ENABLE_WORKPILE_HC

// Build the OCR-lib support
#define ENABLE_OCR_LIB
#endif /* __OCR_CONFIG_H__ */

