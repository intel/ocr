/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_SCHEDULER_H__
#define __OCR_SCHEDULER_H__

#include "ocr-runtime-types.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

struct _ocrPolicyDomain_t;

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

typedef struct _paramListSchedulerFact_t {
    ocrParamList_t base;
} paramListSchedulerFact_t;

typedef struct _paramListSchedulerInst_t {
    ocrParamList_t base;
} paramListSchedulerInst_t;


/****************************************************/
/* OCR SCHEDULER                                    */
/****************************************************/

struct _ocrScheduler_t;
struct _ocrCost_t;
struct _ocrPolicyCtx_t;
struct _ocrMsgHandler_t;

typedef struct _ocrSchedulerFcts_t {
    void (*destruct)(struct _ocrScheduler_t *self);

    void (*begin)(struct _ocrScheduler_t *self, struct _ocrPolicyDomain_t * PD);

    void (*start)(struct _ocrScheduler_t *self, struct _ocrPolicyDomain_t * PD);

    void (*stop)(struct _ocrScheduler_t *self);

    void (*finish)(struct _ocrScheduler_t *self);

    // TODO: Check this call
    // u8 (*yield)(struct _ocrScheduler_t *self, ocrGuid_t workerGuid,
    //                    ocrGuid_t yieldingEdtGuid, ocrGuid_t eventToYieldForGuid,
    //                    ocrGuid_t * returnGuid, struct _ocrPolicyCtx_t *context);

    /**
     * @brief Requests EDTs from this scheduler
     *
     * This call requests EDTs from the scheduler. The EDTs are returned in the
     * EDTs array.
     *
     * @param self[in]          Pointer to this scheduler
     * @param count[in/out]     As input contains either:
     *                            - the maximum number of EDTs requested if edts[0] is NULL_GUID
     *                            - the number of EDTs in edts (requested GUIDs). This
     *                              is also the maximum number of EDTs to be returned
     *                          As output, contains the number of EDTs returned
     * @param edts[in/out]      As input contains the GUIDs of the EDTs requested or NULL_GUID.
     *                          As output, contains the EDTs given by the scheduler to the
     *                          caller. Note that the array needs to be allocated by
     *                          the caller and of sufficient size
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*takeEdt)(struct _ocrScheduler_t *self, u32 *count, ocrFatGuid_t *edts);

    /**
     * @brief Gives EDTs to this scheduler
     *
     * This call requests that the scheduler now handles the EDTs passed to it. The
     * scheduler may refuse some of the EDTs passed to it
     *
     * @param self[in]          Pointer to this scheduler
     * @param count[in/out]     As input, contains the number of EDTs passed to the scheduler
     *                          As output, contains the number of EDTs still left in the array
     * @param edts[in/out]      As input, contains the EDTs passed to the scheduler. As output,
     *                          contains the EDTs that have not been accepted by the
     *                          scheduler from 0 to count excluded.
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*giveEdt)(struct _ocrScheduler_t *self, u32 *count, ocrFatGuid_t *edts);

    /**
     * @brief Requests communication handler from this scheduler.
     *
     * This call requests communication handler from the scheduler.
     * The pointers are returned in the handlers array.
     *
     * @param self[in]          Pointer to this scheduler
     * @param count[in/out]     Number of handlers. As input, contains the maximum number
     *                          of handlers to be returned. As output contains the number
     *                          of handlers returned.
     * @param handlers[in/out]  As output, handlers given to the caller by the callee where
     *                          the fatGuid's metaDataPtr contains a pointer to a handler.
     *                          The fatGuid's guid is set to NULL_GUID.
     *                          (array needs to be allocated by the caller and of sufficient size).
     * @param properties[in]    Properties for the take
     *
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*takeComm)(struct _ocrScheduler_t *self, u32 *count, ocrFatGuid_t * handlers, u32 properties);

    /**
     * @brief Gives communication handler to this scheduler.
     *
     * @param self[in]          Pointer to this scheduler
     * @param count[in/out]     As input, contains the number of handlers passed to the scheduler
     *                          As output, contains the number of handlers still left in the array
     * @param handlers[in/out]  As input, contains handlers passed to the scheduler through the fatGuid's
     *                          metaDataPtr. The fatGuid's guid is set to NULL_GUID. As output,
     *                          contains handlers that have not been accepted by the scheduler
     *                          from 0 to count excluded.
     * @param properties[in]    Properties for the give
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*giveComm)(struct _ocrScheduler_t *self, u32 *count, ocrFatGuid_t * handlers, u32 properties);

    // TODO: We will need to add the DB functions here
} ocrSchedulerFcts_t;

struct _ocrWorkpile_t;

/*! \brief Represents OCR schedulers.
 *
 *  Currently, we allow scheduler interface to have work taken from them or given to them
 */
typedef struct _ocrScheduler_t {
    ocrFatGuid_t fguid;
    struct _ocrPolicyDomain_t *pd;

    struct _ocrWorkpile_t **workpiles;
    u64 workpileCount;

    ocrSchedulerFcts_t fcts;
} ocrScheduler_t;


/****************************************************/
/* OCR SCHEDULER FACTORY                            */
/****************************************************/

typedef struct _ocrSchedulerFactory_t {
    ocrScheduler_t* (*instantiate) (struct _ocrSchedulerFactory_t * factory,
                                    ocrParamList_t *perInstance);
    void (*initialize) (struct _ocrSchedulerFactory_t * factory, ocrScheduler_t *self, ocrParamList_t *perInstance);
    void (*destruct)(struct _ocrSchedulerFactory_t * factory);

    ocrSchedulerFcts_t schedulerFcts;
} ocrSchedulerFactory_t;

void initializeSchedulerOcr(ocrSchedulerFactory_t * factory, ocrScheduler_t * self, ocrParamList_t *perInstance);

#endif /* __OCR_SCHEDULER_H__ */
