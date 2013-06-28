/**
 * @brief OCR data-blocks
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

#ifndef __DATABLOCK_ALL_H__
#define __DATABLOCK_ALL_H__

#include "ocr-datablock.h"

#include "debug.h"
#include "ocr-utils.h"

// TODO: Bala will modify this
typedef enum _dataBlockType_t {
    dataBlockRegular_id,
    dataBlockPlaced_id
} dataBlockType_t;

// Regular datablock
#include "datablock/regular/regular-datablock.h"

// TODO sagnak: delete non-existant headers
// Placed datablock
// #include "datablock/placed/placed-datablock.h"


ocrDataBlockFactory_t* newDataBlockFactory(dataBlockType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case dataBlockRegular_id:
        return newDataBlockFactoryRegular(typeArg);
        break;
// TODO sagnak: delete non-existant parts
//    case dataBlockPlaced_id:
//        return newDataBlockFactoryPlaced(typeArg);
//        break;
    default:
        assert(0);
    }
    return NULL;
}

#endif /* __DATABLOCK_ALL_H__ */
