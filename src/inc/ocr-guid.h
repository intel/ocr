/**
 * @brief OCR internal API to GUID management
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
 **/

#ifndef __OCR_GUID_H__
#define __OCR_GUID_H__

#include "ocr-types.h"
#include "ocr-utils.h"

/*! \brief Globally Unique Identifier utility class
 *
 *  This class provides interfaces to get GUIDs for OCR types, get OCR types from GUID
 *  and set static sentinel values for GUIDs
 */
/*! \brief Gets the GUID for an OCR entity instance
 *  \param[in] p OCR entity instance pointer
 *  \return GUID for OCR entity instance
 *  Our current implementation just wraps the pointer value to a primitive type
 */
ocrGuid_t guidify(void * p);

/*! \brief Gets an OCR entity instance pointer from a GUID
 *  \param[in] id GUID for OCR entity instance
 *  \return OCR entity instance pointer
 *  Our current implementation just wraps the pointer value to a primitive type
 */
void * deguidify(ocrGuid_t id);

/*! GUID for a NULL pointer */
extern ocrGuid_t NULL_GUID;

/*! GUID for a uninitialized pointer */
extern ocrGuid_t UNINITIALIZED_GUID;

/*! GUID for an error message */
extern ocrGuid_t ERROR_GUID;

#endif /* __OCR_GUID__H_ */
