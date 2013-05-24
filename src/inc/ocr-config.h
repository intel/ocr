/**
 * @brief Configuration for the OCR runtime
 *
 * This file describes the configuration options for OCR
 * to select the different implementations available
 * for the various modules
 *
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-09-21
 * Copyright (c) 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __OCR_CONFIG_H__
#define __OCR_CONFIG_H__

#include "ocr-executor.h"
#include "ocr-low-workers.h"
#include "ocr-scheduler.h"
#include "ocr-policy.h"
#include "ocr-workpile.h"
#include "ocr-guid.h"
#include "ocr-sync.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

// Default kinds of ocr modules
extern ocr_executor_kind ocr_executor_default_kind;
extern ocr_worker_kind ocr_worker_default_kind;
extern ocr_scheduler_kind ocr_scheduler_default_kind;
extern ocr_policy_kind ocr_policy_default_kind;
extern ocr_workpile_kind ocr_workpile_default_kind;
extern ocrAllocatorKind ocrAllocatorDefaultKind;
extern ocrMemPlatformKind ocrMemPlatformDefaultKind;
extern ocrDataBlockKind ocrDataBlockDefaultKind;
//extern ocrLockKind ocrLockDefaultKind;
extern ocrGuidProviderKind ocrGuidProviderDefaultKind;

// Default values to configure ocr
extern u32 ocr_config_default_nb_hardware_threads;

// XE kinds of ocr modules
extern ocr_executor_kind	ocr_executor_xe_kind;
extern ocr_worker_kind		ocr_worker_xe_kind;
extern ocr_scheduler_kind	ocr_scheduler_xe_kind;
extern ocr_policy_kind		ocr_policy_xe_kind;
extern ocr_workpile_kind	ocr_workpile_xe_kind;
extern ocrAllocatorKind		ocrAllocatorXEKind;
extern ocrMemPlatformKind		ocrMemPlatformXEKind;
extern ocrDataBlockKind		ocrDataBlockXEKind;
//extern ocrLockKind		ocrLockXEKind;
extern ocrGuidProviderKind	ocrGuidProviderXEKind;

// CE kinds of ocr modules
extern ocr_executor_kind	ocr_executor_ce_kind;
extern ocr_worker_kind		ocr_worker_ce_kind;
extern ocr_scheduler_kind	ocr_scheduler_ce_kind;
extern ocr_policy_kind		ocr_policy_ce_kind;
extern ocr_policy_kind		ocr_policy_ce_mastered_kind;
extern ocr_workpile_kind	ocr_workpile_ce_work_kind;
extern ocr_workpile_kind	ocr_workpile_ce_message_kind;
extern ocrAllocatorKind		ocrAllocatorCEKind;
extern ocrMemPlatformKind		ocrMemPlatformCEKind;
extern ocrDataBlockKind		ocrDataBlockCEKind;
//extern ocrLockKind		ocrLockCEKind;
extern ocrGuidProviderKind	ocrGuidProviderCEKind;

// Factories
ocrLockFactory_t        *GocrLockFactory;
ocrAtomic64Factory_t    *GocrAtomic64Factory;
ocrQueueFactory_t       *GocrQueueFactory;

#ifdef OCR_ENABLE_STATISTICS
// For now have a central statistics aggregator filter
ocrStatsFilter_t   *GocrFilterAggregator;

// HUGE HUGE HACK: it seems that currently we can run code outside of EDTs
// which is a big problem since then we don't have a source "process" for Lamport's
// clock. This should *NEVER* be the case and there should be no non-EDT code
// except for the bootstrap. For now, and for expediency, this is a hack where I have
// a fake Lamport process for the initial non-EDT code
ocrStatsProcess_t GfakeProcess;

#endif

#endif /* __OCR_CONFIG_H__ */
