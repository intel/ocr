/**
 * @brief OCR tasks
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __TASK_ALL_H__
#define __TASK_ALL_H__

#include "debug.h"
#include "ocr-task.h"
#include "ocr-utils.h"

typedef enum _taskType_t {
    taskHc_id,
    taskFsim_id,
    taskMax_id
} taskType_t;

const char * task_types [] = {
    "HC",
    "FSIM",
    NULL
};

typedef enum _taskTemplateType_t {
    taskTemplateHc_id,
    taskTemplateFsim_id,
    taskTemplateMax_id
} taskTemplateType_t;

const char * taskTemplate_types [] = {
    "HC",
    "FSIM",
    NULL
};

// HC Task
#include "task/hc/hc-task.h"

// Add other tasks using the same pattern as above

inline ocrTaskFactory_t *newTaskFactory(taskType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case taskHc_id:
        return newTaskFactoryHc(typeArg);
    default:
        ASSERT(0);
    };
    return NULL;
}

inline ocrTaskTemplateFactory_t *newTaskTemplateFactory(taskTemplateType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case taskTemplateHc_id:
        return newTaskTemplateFactoryHc(typeArg);
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __TASK_ALL_H__ */
