/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_SAL_LINUX

#include "sal/linux/linux-sal.h"
#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-task.h"
#include "ocr-sal.h"
#include "ocr-sysboot.h"
#include "ocr-worker.h"

#include <stdlib.h>
#include <stdio.h>

void linuxSalTeardown(ocrPolicyDomain_t *pd) {
    // TODO: Implement
}

void linuxSalGetCurrentEnv(ocrPolicyDomain_t **pd, ocrWorker_t **worker,
                           ocrTask_t **edt, ocrPolicyMsg_t *msg) {
    // TODO: Implement
}

void (*teardown)(ocrPolicyDomain_t*) = &linuxSalTeardown;
void (*getCurrentEnv)(ocrPolicyDomain_t**, ocrWorker_t**,
                      ocrTask_t **, ocrPolicyMsg_t*) = &linuxSalGetCurrentEnv;

void linuxSalDestruct(ocrSal_t* self) {
    runtimeChunkFree((u64)self, NULL);
}

void linuxSalStart(ocrSal_t *self, ocrPolicyDomain_t *pd) {
    // No GUID for the system. Do we need one?
    self->pd = pd;
}

void linuxSalStop(ocrSal_t *self) {
    // Nothing to do
}

void linuxSalFinish(ocrSal_t *self) {
    // Nothing to do
}

void linuxSalExit(ocrSal_t *self, u64 errorCode) {
    exit(errorCode);
}

void linuxSalAbort(ocrSal_t *self) {
    abort();
}

void linuxSalPrint(ocrSal_t *self, const char* str, u64 length) {
    ASSERT(str[length] == '\0');
    printf("%s", str);
}

u64 linuxSalWrite(ocrSal_t *self, const char* str, u64 length, u64 id) {
    return 0;
}

u64 linuxSalRead(ocrSal_t *self, char *str, u64 length, u64 id) {
    return 0;
}

ocrSal_t * newSalLinux(ocrSalFactory_t * factory, ocrParamList_t* perInstance) {
    ocrSalLinux_t * sal = (ocrSalLInux_t*)runtimeChunkAlloc(sizeof(ocrSalLinux_t), NULL);

    sal->base.pd = NULL;
    sal->base.fcts = factory->salFcts;
        
    return (ocrSal_t*)sal;
}

/******************************************************/
/* OCR SAL LINUX FACTORY                              */
/******************************************************/
static void destructSalFactoryLinux(ocrSalFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrSalFactory_t *newSalFactoryLinux(ocrParamList_t *perType) {
    ocrSalFactory_t *base = (ocrSalFactory_t*)
        runtimeChunkAlloc(sizeof(ocrSalFactoryHc_t), NULL);
    base->instantiate = &newSalLinux;
    base->destruct = &destructSalFactoryLinux;
    base->salFcts.destruct = &linuxSalDestruct;
    base->salFcts.start = &linuxSalStart;
    base->salFcts.stop = &linuxSalStop;
    base->salFcts.finish = &linuxSalFinish;
    base->salFcts.exit = &linuxSalExit;
    base->salFcts.abort = &linuxSalAbort;
    base->salFcts.print = &linuxSalPrint;
    base->salFcts.write = &linuxSalWrite;
    base->salFcts.read = &linuxSalRead;

    return base;
}

#endif /* ENABLE_SAL_LINUX */
