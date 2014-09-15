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
#define ENABLE_BUILDER_ONLY

// Allocator
#define ENABLE_ALLOCATOR_NULL


// CommApi
#define ENABLE_COMM_API_HANDLELESS

// Comm-platform
#define ENABLE_COMM_PLATFORM_XE

// Comp-platform
//#define ENABLE_COMP_PLATFORM_PTHREAD
#define ENABLE_COMP_PLATFORM_FSIM

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

// HAL layer to use
#define HAL_X86_64

// Mem-platform
#define ENABLE_MEM_PLATFORM_FSIM

// Mem-target
#define ENABLE_MEM_TARGET_SHARED

// Policy domain
#define ENABLE_POLICY_DOMAIN_XE

// Scheduler
#define ENABLE_SCHEDULER_XE

// SAL layer to use
#define SAL_LINUX

// Sysboot layer to use
// None - this is a builder

// Task
#define ENABLE_TASK_HC

// Task template
#define ENABLE_TASKTEMPLATE_HC

// Worker
#define ENABLE_WORKER_XE

// Workpile
#define ENABLE_WORKPILE_XE

// Build the OCR-lib support
#define ENABLE_EXTENSION_LIB
#endif /* __OCR_CONFIG_H__ */
