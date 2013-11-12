/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-runtime.h"

/**
   @brief Get @ offset in the currently running edt's local storage
   Note: not visible from the ocr user interface
 **/
ocrGuid_t ocrElsUserGet(u8 offset) {
    // User indexing start after runtime-reserved ELS slots
    offset = ELS_RUNTIME_SIZE + offset;
    ocrPolicyDomain_t * pd = getCurrentPD();
    ocrGuid_t edtGuid = getCurrentEDT();
    ocrTask_t * edt = NULL;
    deguidify(pd, edtGuid, (u64*)&(edt), NULL);
    return edt->els[offset];
}

/**
   @brief Set data @ offset in the currently running edt's local storage
   Note: not visible from the ocr user interface
 **/
void ocrElsUserSet(u8 offset, ocrGuid_t data) {
    // User indexing start after runtime-reserved ELS slots
    offset = ELS_RUNTIME_SIZE + offset;
    ocrPolicyDomain_t * pd = getCurrentPD();
    ocrGuid_t edtGuid = getCurrentEDT();
    ocrTask_t * edt = NULL;
    deguidify(pd, edtGuid, (u64*)&(edt), NULL);
    edt->els[offset] = data;
}

ocrGuid_t currentEdtUserGet() {
      ocrPolicyDomain_t * pd = getCurrentPD();
      if (pd != NULL) {
        return getCurrentEDT();
      }
      return NULL_GUID;
}

// exposed to runtime implementers as convenience
u64 ocrNbWorkers() {
  ocrPolicyDomain_t * pd = getCurrentPD();
  return pd->workerCount;
}

// exposed to runtime implementers as convenience
u64 ocrCurrentWorkerId() {
  ocrPolicyCtx_t * ctx = getCurrentWorkerContext();
  return ctx->sourceId;
}
