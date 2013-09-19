/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef FSIM_H_
#define FSIM_H_

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#if 0
#include "hc.h"
#include "ocr-runtime.h"
#include "deque.h"

/******************************************************/
/* OCR-FSIM POLICY DOMAIN                             */
/******************************************************/

typedef struct _ocrPolicyDomainFsim_t {
  ocrPolicyDomain_t base;
  ocrTaskFactory_t * taskMessageFactory;
} ocrPolicyDomainFsim_t;

typedef struct {
    ocrWorkerHc_t hcBase;
    pthread_cond_t isRunningCond;
    pthread_mutex_t isRunningMutex;
} ocrWorkerFsimXE_t;

typedef struct ocrWorkpileFsimMessage_t {
    ocrWorkpile_t base;
    semiConcDeque_t * deque;
} ocrWorkpileFsimMessage_t;


/******************************************************/
/* OCR-FSIM SCHEDULERS                                */
/******************************************************/

typedef struct  {
    ocrSchedulerFactory_t base;
} ocrSchedulerFactoryFsimXE_t;

typedef struct  {
    ocrSchedulerFactory_t base;
} ocrSchedulerFactoryFsimCE_t;

typedef struct {
    ocrSchedulerHc_t scheduler;
} ocrSchedulerFsimXE_t;

typedef struct {
    ocrSchedulerHc_t scheduler;
    int in_message_popping_mode;
} ocrSchedulerFsimCE_t;


/******************************************************/
/* OCR-FSIM WORKERS                                   */
/******************************************************/

typedef struct  {
    ocrWorkerFactory_t base;
} ocrWorkerFactoryFsimXE_t;

typedef struct  {
    ocrWorkerFactory_t base;
} ocrWorkerFactoryFsimCE_t;

typedef struct _paramListWorkerFsimInst_t {
  paramListWorkerInst_t base;
  int workerId;
} paramListWorkerFsimInst_t;



/******************************************************/
/* OCR-FSIM Workpile                                  */
/******************************************************/

typedef struct {
  ocrWorkpileFactory_t base;
} ocrWorkpileFactoryFsimMessage_t;


/******************************************************/
/* OCR-FSIM Task Factory                              */
/******************************************************/

typedef struct {
    ocrTaskFactory_t base_factory;
} ocrTaskFactoryFsim_t;

ocrTaskFactory_t* newTaskFactoryFsim(ocrParamList_t* perType);
void destructTaskFactoryFsim ( ocrTaskFactory_t* base );

typedef struct {
    ocrTaskFactoryHc_t base_factory;
} ocrTaskFactoryFsimMessage_t;

ocrTaskFactory_t* newTaskFactoryFsimMessage(ocrParamList_t* perType);
void destructTaskFactoryFsimMessage ( ocrTaskFactory_t* base );

typedef struct fsim_message_interface_struct {
    int (*is_message) ( struct fsim_message_interface_struct *);
} fsim_message_interface_t;

typedef struct {
    ocrTaskHc_t base;
    fsim_message_interface_t message_interface;
} ocrTaskFsimBase_t;

typedef struct {
    ocrTaskFsimBase_t fsimBase;
} ocrTaskFsim_t;

typedef enum message_type_enum { PICK_MY_WORK_UP, GIVE_ME_WORK } message_type;

typedef struct {
    ocrTaskFsimBase_t fsimBase;
    message_type type;
    ocrGuid_t from_worker_guid;
} ocrTaskFsimMessage_t;

typedef ocrEventHc_t fsim_event_t;

#endif

#endif /* FSIM_H_ */
