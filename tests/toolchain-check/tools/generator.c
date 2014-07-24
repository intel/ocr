#include <ocr-types.h>
#include "inc/debug.h"
#include "inc/ocr-policy-domain.h"
#include "inc/ocr-allocator.h"
#include "inc/ocr-comm-platform.h"
#include "inc/ocr-comp-platform.h"
#include "inc/ocr-comp-target.h"
#include "inc/ocr-mem-platform.h"
#include "inc/ocr-mem-target.h"
#include "inc/ocr-workpile.h"

#include "data.h"

void __attribute__ ((noinline)) stop(void) {
}

main(){
ocrPolicyMsg_t* ptr1 = (ocrPolicyMsg_t *)data;
ocrAllocator_t* ptr2 = (ocrAllocator_t*)data;
ocrCommApi_t* ptr3 = (ocrCommApi_t*)data;
ocrCommPlatform_t* ptr4 = (ocrCommPlatform_t*)data;
ocrCompPlatform_t* ptr5 = (ocrCompPlatform_t*)data;
ocrCompTarget_t* ptr6 = (ocrCompTarget_t*)data;
ocrDataBlock_t* ptr7 = (ocrDataBlock_t*)data;
ocrEvent_t* ptr8 = (ocrEvent_t*)data;
ocrGuidProvider_t* ptr9 = (ocrGuidProvider_t*)data;
ocrMemPlatform_t* ptr10 = (ocrMemPlatform_t*)data;
ocrMemTarget_t* ptr11 = (ocrMemTarget_t*)data;
ocrPolicyDomain_t* ptr12 = (ocrPolicyDomain_t*)data;
ocrScheduler_t* ptr13 = (ocrScheduler_t*)data;
ocrWorker_t* ptr14 = (ocrWorker_t*)data;
ocrWorkpile_t* ptr15 = (ocrWorkpile_t*)data;

stop();
}
