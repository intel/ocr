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

// Define this if building the PD builder program
// If this is defined, this will exclude everything
// that does not contribute to building the policy domain
//#define ENABLE_BUILDER_ONLY

// Allocator
#define ENABLE_ALLOCATOR_TLSF

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

// GUID provider
#define ENABLE_GUID_PTR

// HAL layer to use
#define HAL_FSIM_XE

// Mem-platform
//#define ENABLE_MEM_PLATFORM_MALLOC

// Mem-target
//#define ENABLE_MEM_TARGET_SHARED

// Policy domain
#define ENABLE_POLICY_DOMAIN_XE

// Scheduler
#define ENABLE_SCHEDULER_XE

// SAL layer to use
#define SAL_FSIM_XE

// XE tool-chain used
#define TOOL_CHAIN_XE

// Sysboot layer to use
#define ENABLE_SYSBOOT_FSIM

// Task
#define ENABLE_TASK_XE

// Task template
#define ENABLE_TASKTEMPLATE_XE

// Worker
#define ENABLE_WORKER_XE

// Workpile
//#define ENABLE_WORKPILE_HC

#endif /* __OCR_CONFIG_H__ */


