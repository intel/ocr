/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"
#include "ocr-worker.h"

ocrGuid_t getCurrentEDTFromWorker() {
    ocrPolicyCtx_t * ctx = getCurrentWorkerContext();
    ocrGuid_t workerGuid = ctx->sourceObj;
    ocrWorker_t *worker = NULL;
    deguidify(ctx->PD, workerGuid, (u64*)&(worker), NULL);
    return worker->fctPtrs->getCurrentEDT(worker);
}

void setCurrentEDTToWorker(ocrGuid_t edtGuid) {
    ocrPolicyCtx_t * ctx = getCurrentWorkerContext();
    ocrGuid_t workerGuid = ctx->sourceObj;
    ocrWorker_t *worker = NULL;
    deguidify(ctx->PD, workerGuid, (u64*)&(worker), NULL);
    worker->fctPtrs->setCurrentEDT(worker, edtGuid);
}

//void associate_comp_platform_and_worker(ocrWorker_t *worker) {}
