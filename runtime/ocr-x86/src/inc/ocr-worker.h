/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_WORKER_H__
#define __OCR_WORKER_H__

#include "ocr-comp-target.h"
#include "ocr-runtime-types.h"
#include "ocr-scheduler.h"
#include "ocr-types.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

struct _ocrPolicyDomain_t;
struct _ocrPolicyMsg_t;
struct _ocrMsgHandler_t;

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

typedef struct _paramListWorkerFact_t {
    ocrParamList_t base;
} paramListWorkerFact_t;

typedef struct _paramListWorkerInst_t {
    ocrParamList_t base;
} paramListWorkerInst_t;

/******************************************************/
/* OCR WORKER                                         */
/******************************************************/

struct _ocrWorker_t;
struct _ocrTask_t;

typedef struct _ocrWorkerFcts_t {
    void (*destruct)(struct _ocrWorker_t *self);

    void (*begin)(struct _ocrWorker_t *self, struct _ocrPolicyDomain_t * PD);
    
    /** @brief Start Worker
     */
    void (*start)(struct _ocrWorker_t *self, struct _ocrPolicyDomain_t * PD);

    /** @brief Run Worker 
     */
    void* (*run)(struct _ocrWorker_t *self);

    /** @brief Query the worker to finish its work
     */
    void (*finish)(struct _ocrWorker_t *self);

    /** @brief Stop Worker
     */
    void (*stop)(struct _ocrWorker_t *self);

    /** @brief Check if Worker is still running
     *  @return true if the Worker is running, false otherwise
     */
    bool (*isRunning)(struct _ocrWorker_t *self);

    /**
     * @brief Send a message to another target
     *
     * This call, which executes on the compute platform
     * making the call, will send an asynchronous message to target. There is
     * no expectation of a result to be sent back but the target may send
     * "back" a one-way message at a later point.
     *
     * The exact implementation of the message sent is implementation dependent
     * and may even be a fully synchronous call but users of this call
     * should assume asynchrony. The exact communication protocol is also
     * implementation dependent.
     *
     * @param[in] self        Pointer to this
     * @param[in] target      Target to communicate with
     * @param[in/out] message Message to send. In the general case, this is
     *                        used as input and instructs the comp-target
     *                        to send the message given but it may be updated
     *                        and should be used if waitMessage is called
     *                        later. Note the pointer-to-pointer argument.
     *                        If a new message is created (and its pointer
     *                        returned in the call), the old message will *not*
     *                        be freed and it is up to the caller to properly
     *                        dispose of it (may be on the stack).
     *                        The new message, once fully used, needs to
     *                        be disposed of with pdFree
     * @return 0 on success and a non-zero error code
     */
    u8 (*sendMessage)(struct _ocrWorker_t *self, ocrLocation_t target,
                      struct _ocrPolicyMsg_t **message);

    /**
     * @brief Checks if a message has been received by the comm platform and,
     * if so, will return a pointer to that message in 'message'
     *
     * The use case for this function is for the worker "loop" to
     * periodically check if it has services to perform for other policy domains.
     *
     * If waiting for a specific message, make sure to pass the message returned
     * by sendMessage.
     *
     * @param self[in]        Pointer to this worker
     * @param message[in/out] If a message is available, its pointer will be.
     *                        If *message is NULL on input, this function will
     *                        poll for ANY messages that meet the mask property (see
     *                        below). If *message is non NULL, this function
     *                        will poll for only that specific message.
     *                        The message returned needs to be freed using pdFree.
     * @param mask[in]        If non-zero, this function will only return messages
     *                        whose 'property' variable ORed with this mask returns
     *                        a non-zero value
     * @return #POLL_MO_MESSAGE, #POLL_MORE_MESSAGE, #POLL_ERR_MASK
     */
    u8 (*pollMessage)(struct _ocrWorker_t *self,
                      struct _ocrPolicyMsg_t **message, u32 mask);
    
    /**
     * @brief Waits for a response to the message 'message'
     *
     * This call should only be called in very limited circumstances when the
     * originating call that generated the message is synchronous (for example
     * user calls to OCR).
     *
     * This call will block until message has received a response. Make sure
     * to pass the same message as the one returned by sendMessage (as it may
     * have been modified by sendMessage for book-keeping purposes)
     *
     * @param self[in]        Pointer to this worker
     * @param message[in/out] As input, this determines which message to wait
     *                        for. Note that this is a pointer to a pointer.
     *                        The returned message, once used, needs to be
     *                        freed with pdFree.
     * @return 0 on success and a non-zero error code
     */
    u8 (*waitMessage)(struct _ocrWorker_t *self,
                      struct _ocrPolicyMsg_t **message);
} ocrWorkerFcts_t;

typedef struct _ocrWorker_t {
    ocrFatGuid_t fguid;
    struct _ocrPolicyDomain_t *pd;
    ocrLocation_t location;
    ocrWorkerType_t type;
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcess_t *statProcess;
#endif

    ocrCompTarget_t **computes; /**< Compute node(s) associated with this worker */
    u64 computeCount;           /**< Number of compute node(s) associated */

    struct _ocrTask_t *curTask; /**< Currently executing task */
    
    ocrWorkerFcts_t fcts;
} ocrWorker_t;


/****************************************************/
/* OCR WORKER FACTORY                               */
/****************************************************/

typedef struct _ocrWorkerFactory_t {
    ocrWorker_t* (*instantiate) (struct _ocrWorkerFactory_t * factory,
                                  ocrParamList_t *perInstance);
    void (*initialize) (struct _ocrWorkerFactory_t * factory, struct _ocrWorker_t * worker, ocrParamList_t *perInstance);

    void (*destruct)(struct _ocrWorkerFactory_t * factory);
    ocrWorkerFcts_t workerFcts;
} ocrWorkerFactory_t;

void initializeWorkerOcr(ocrWorkerFactory_t * factory, ocrWorker_t * self, ocrParamList_t *perInstance);

#endif /* __OCR_WORKER_H__ */

