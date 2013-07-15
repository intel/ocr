/**
 * @brief Very simple GUID implementation that basically considers
 * pointers to be GUIDs
 *
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-11-13
 * @warning GUIDs should *not* in general, be considered analogous to
 * pointers. This is just a sample implementation for illustration purposes
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

#ifndef __OCR_GUIDPROVIDER_PTR_H__
#define __OCR_GUIDPROVIDER_PTR_H__

#include "ocr-types.h"
#include "ocr-guid.h"

/**
 * @brief Trivial GUID provider
 * @warning This will always restore OCR_GUID_NONE for the type.
 * In other words, it never stores the type
 */
typedef struct {
    ocrGuidProvider_t base;
} ocrGuidProviderPtr_t;

typedef struct {
    ocrGuidProviderFactory_t base;
} ocrGuidProviderFactoryPtr_t;

ocrGuidProviderFactory_t* newGuidProviderFactoryPtr(ocrParamList_t *typeArg);

#define __GUID_END_MARKER__
#include "ocr-guid-end.h"
#undef __GUID_END_MARKER__

#endif /* __OCR_GUIDPROVIDER_PTR_H__ */
