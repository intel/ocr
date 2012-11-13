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

typedef enum {
    OCR_GUID_NONE = 0
} ocrGuidKind;

/**
 * @brief Provider for GUIDs for the system
 *
 * GUIDs should be unique and are used to
 * identify and locate objects (and their associated
 * metadata mostly). GUIDs serve as a level of indirection
 * to allow objects to move around in the system and
 * support different address spaces (in the future)
 */
typedef struct _ocrGuidProvider_t {
    /**
     * @brief Constructor equivalent
     *
     * @param self          Pointer to this GUID provider
     * @param config        And optional configuration (not currently used)
     */
    void (*create)(struct _ocrGuidProvider_t* self, void* config);

    /**
     * @brief Destructor equivalent
     *
     * This will free the GUID provider and any
     * memory that it uses
     *
     * @param self          Pointer to this GUID provider
     */
    void (*destruct)(struct _ocrGuidProvider_t* self);

    /**
     * @brief Returns a GUID for an object of type 'type'
     * and associates the value val.
     *
     * The GUID provider basically associates a value with the
     * GUID (can be a pointer to the metadata for example). This
     * creates an association
     *
     * @param self          Pointer to this GUID provider
     * @param guid          GUID returned
     * @param type          Type of the object that will be associated with the GUID
     * @param val           Value to be associated
     * @return 0 on success or an error code
     */
    u8 (*getGuid)(struct _ocrGuidProvider_t* self, ocrGuid_t* guid, ocrGuidKind type,
                  u64 val);

    /**
     * @brief Returns the associated value for the GUID 'guid'
     */
    u8 (*getVal)(struct _ocrGuidProvider_t* self, u64* val, ocrGuid_t guid);


} ocrGuidProvider_t;

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

#define UNINITIALIZED_GUID ((ocrGuid_t)-2)

#define ERROR_GUID ((ocrGuid_t)-1)

#endif /* __OCR_GUID__H_ */
