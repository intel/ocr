/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_TASK_H__
#define __OCR_TASK_H__

#include "ocr-edt.h"
#include "ocr-runtime-types.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#ifdef OCR_ENABLE_EDT_PROFILING
#include "ocr-edt-profiling.h"
#endif

struct _ocrTask_t;
struct _ocrTaskTemplate_t;

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/
typedef struct _paramListTaskFact_t {
    ocrParamList_t base;
} paramListTaskFact_t;

typedef struct _paramListTaskTemplateFact_t {
    ocrParamList_t base;
} paramListTaskTemplateFact_t;

struct _ocrTaskTemplate_t;

/****************************************************/
/* OCR TASK TEMPLATE                                */
/****************************************************/

/** @brief Abstract class to represent OCR task template functions
 *
 *  This class provides the interface to call operations on task
 *  templates
 */
typedef struct ocrTaskTemplateFcts_t {
    /** @brief Virtual destructor for the task template interface
     */
    u8 (*destruct)(struct _ocrTaskTemplate_t* self);
} ocrTaskTemplateFcts_t;

/** @brief Abstract class to represent OCR task templates.
 *
 */
typedef struct _ocrTaskTemplate_t {
    ocrGuid_t guid;         /**< GUID for this task template */
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcess_t *statProcess;
#endif
    u32 paramc;             /**< Number of input parameters */
    u32 depc;               /**< Number of dependences */
    // TODO: This does not really support things like
    // moving code around and/or different ISAs. Is this
    // going to be a problem...
    ocrEdt_t executePtr;    /**< Function pointer to execute */
#ifdef OCR_ENABLE_EDT_NAMING
    const char* name;       /**< Name of the EDT */
#endif
#ifdef OCR_ENABLE_EDT_PROFILING
    struct _profileStruct *profileData;
    struct _dbWeightStruct *dbWeights;
#endif
    u32 fctId;              /**< Functions to manage this template */
} ocrTaskTemplate_t;

/****************************************************/
/* OCR TASK TEMPLATE FACTORY                        */
/****************************************************/

typedef struct _ocrTaskTemplateFactory_t {
    /**
     * @brief Create a task template
     *
     * @param[in] factory     Pointer to this factory
     * @param[in] fctPtr      Function pointer to execute
     * @param[in] paramc      Number of input parameters or EDT_PARAM_UNK or EDT_PARAM_DEF
     * @param[in] depc        Number of DB dependences or EDT_PARAM_UNK or EDT_PARAM_DEF
     * @param[in] fctName     Name of the EDT (for debugging)
     * @param[in] perInstance Instance specific parameters
     */
    ocrTaskTemplate_t* (*instantiate)(struct _ocrTaskTemplateFactory_t * factory, ocrEdt_t fctPtr,
                                      u32 paramc, u32 depc, const char* fctName,
                                      ocrParamList_t *perInstance);

    /** @brief Destructor for the TaskTemplateFactory interface
     */
    void (*destruct)(struct _ocrTaskTemplateFactory_t * factory);

    u32 factoryId;
    ocrTaskTemplateFcts_t fcts;
} ocrTaskTemplateFactory_t;

/****************************************************/
/* OCR TASK                                         */
/****************************************************/

/**
 * @brief Abstract class to represent OCR tasks function pointers
 *
 * This class provides the interface to call operations on task
 */
typedef struct _ocrTaskFcts_t {
    /**
     * @brief Virtual destructor for the Task interface
     */
    u8 (*destruct)(struct _ocrTask_t* self);

    /**
     * @brief "Satisfy" an input dependence for this EDT
     *
     * An EDT has input slots that must be satisfied before it
     * is becomes runable. This call satisfies the dependence
     * identified by 'slot'
     *
     * @param[in] self        Pointer to this task
     * @param[in] db          Optional data passed for this dependence
     * @param[in] slot        Slot satisfied
     */
    u8 (*satisfy)(struct _ocrTask_t* self, ocrFatGuid_t db, u32 slot);

    /**
     * @brief Informs the task that the event/db 'src' is linked to its
     * input slot 'slot'
     *
     * registerSignaler where src is a data-block is equivalent to calling
     * satisfy with that same data-block
     *
     * When a dependence is established between an event and
     * a task, registerWaiter will be called on the event
     * and registerSignaler will be called on the task
     *
     * @param[in] self        Pointer to this task
     * @param[in] signaler    GUID of the source (signaler)
     * @param[in] slot        Slot on self that will be satisfied by signaler
     * @param[in] isDepAdd    True if the registerSignaler is part of the initial
     *                        adding of the dependence. False if this was a
     *                        standalone signaler register.
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*registerSignaler)(struct _ocrTask_t* self, ocrFatGuid_t src, u32 slot,
                           bool isDepAdd);

    /**
     * @brief Informs the task that the event/db 'src' is no longer linked
     * to it on 'slot'
     *
     * This removes a dependence between an event/db and
     * a task
     *
     * @param[in] self        Pointer to this task
     * @param[in] signaler    GUID of the source (signaler)
     * @param[in] slot        Slot on self that will be satisfied by signaler
     * @param[in] isDepRem    True if the unregisterSignaler is part of a
     *                        dependence removal. False if standalone call
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*unregisterSignaler)(struct _ocrTask_t* self, ocrFatGuid_t src, u32 slot,
                             bool isDepRem);

    /**
     * @brief Informs the task that it has acquired a DB that it didn't know
     * about upon entry
     *
     * This happens when the user creates a data-block within an EDT. This
     * call is special in the sense that it will ALWAYS be called within
     * the context/execution of self.
     *
     * @note This call is only called when the data-block acquire
     * is user-triggered from isnide the EDT
     *
     * @param[in] self          Pointer to this task
     * @param[in] db            GUID of the DB
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*notifyDbAcquire)(struct _ocrTask_t* self, ocrFatGuid_t db);

    /**
     * @brief Symmetric call to notifyDbAcquire(): this is called when
     * the user performs a release (could be as part of a free) on
     * ANY data-block while inside the EDT
     *
     * @warning This call may be called with DBs that have not been
     * acquired using notifyDbAcquire (ie: DBs that are dependences.
     * This should not be an error
     *
     * @param[in] self          Pointer to this task
     * @param[in] db            GUID of the DB
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*notifyDbRelease)(struct _ocrTask_t* self, ocrFatGuid_t db);

    /**
     * @brief Executes this EDT
     *
     * This should be called by a worker to execute the EDT's code
     * This call should take care of acquiring any input dependences
     * and notifying any output events if needed
     *
     * @param[in] self        Pointer to this task
     * @return 0 on success and a non-zero code on failure
     */
    u8 (*execute)(struct _ocrTask_t* self);
} ocrTaskFcts_t;

#define ELS_RUNTIME_SIZE 0
#define ELS_SIZE (ELS_RUNTIME_SIZE + ELS_USER_SIZE)

/*! \brief Abstract class to represent OCR tasks.
 *
 *  This class provides the interface for the underlying implementation to conform.
 *  OCR tasks can be executed and can have their synchronization frontier furthered by Events.
 */
typedef struct _ocrTask_t {
    ocrGuid_t guid;         /**< GUID for this task (EDT) */
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcess_t *statProcess;
#endif
    ocrGuid_t templateGuid; /**< GUID for the template of this task */
    ocrEdt_t funcPtr;       /**< Function to execute */
    u64* paramv;            /**< Pointer to the paramaters; should be inside task metadata */
#ifdef OCR_ENABLE_EDT_NAMING
    const char* name;       /**< Name of the EDT (for debugging purposes */
#endif
    ocrGuid_t outputEvent;  /**< Event to notify when the EDT is done */
    ocrGuid_t finishLatch;  /**< Latch event for this EDT (if this is a finish EDT) */
    ocrGuid_t parentLatch;  /**< Inner-most latch event (not of this EDT) */
    ocrGuid_t els[ELS_SIZE];
    ocrEdtState_t state;    /**< State of the EDT */
    u32 paramc, depc;       /**< Number of parameters and dependences */
    u32 fctId;
} ocrTask_t;

/****************************************************/
/* OCR TASK FACTORY                                 */
/****************************************************/

/*! \brief Abstract factory class to create OCR tasks.
 *
 *  This class provides an interface to create Task instances with a non-static create function
 *  to allow runtime implementers to choose to have state in their derived TaskFactory classes.
 */
typedef struct _ocrTaskFactory_t {
    /*! \brief Instantiates a Task and returns its corresponding GUID
     *  \param[in]  routine A user defined function that represents the computation this Task encapsulates.
     *  \param[in]  worker_id   The Worker instance creating this Task instance
     *  \return GUID of the concrete Task that is created by this call
     *
     *  The signature of the interface restricts the user computation that can be assigned to a task as follows.
     *  The user defined computation should take a vector of GUIDs and its size as their inputs, which may be
     *  the GUIDs used to satisfy the Events enlisted in the dependence list.
     *
     */
    ocrTask_t* (*instantiate)(struct _ocrTaskFactory_t * factory, ocrFatGuid_t edtTemplate,
                              u32 paramc, u64* paramv, u32 depc, u32 properties,
                              ocrFatGuid_t affinity, ocrFatGuid_t *outputEvent,
                              ocrTask_t *curEdt, ocrParamList_t *perInstance);

    /*! \brief Virtual destructor for the TaskFactory interface
     */
    void (*destruct)(struct _ocrTaskFactory_t * factory);

    u32 factoryId;
    ocrTaskFcts_t fcts;
} ocrTaskFactory_t;
#endif /* __OCR_TASK_H__ */
