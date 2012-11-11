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

// Default kinds of ocr modules
extern ocr_executor_kind ocr_executor_default_kind;
extern ocr_worker_kind ocr_worker_default_kind;
extern ocr_scheduler_kind ocr_scheduler_default_kind;
extern ocr_policy_kind ocr_policy_default_kind;
extern ocr_workpile_kind ocr_workpile_default_kind;
extern ocrAllocatorKind ocrAllocatorDefaultKind;
extern ocrLowMemoryKind ocrLowMemoryDefaultKind;
extern ocrDataBlockKind ocrDataBlockDefaultKind;
extern ocrLockKind ocrLockDefaultKind;

// Default values to configure ocr
extern u32 ocr_config_default_nb_hardware_threads;

#endif /* __OCR_CONFIG_H__ */
