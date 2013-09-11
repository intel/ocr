/**
 * @brief OCR data-blocks
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __DATABLOCK_ALL_H__
#define __DATABLOCK_ALL_H__

#include "debug.h"
#include "ocr-datablock.h"
#include "ocr-utils.h"

typedef enum _dataBlockType_t {
    dataBlockRegular_id,
    dataBlockPlaced_id,
    dataBlockMax_id
} dataBlockType_t;

const char * dataBlock_types [] = {
    "Regular",
    "Placed",
    NULL
};

// Regular datablock
#include "datablock/regular/regular-datablock.h"

// Placed datablock (not ported as of now)
// #include "datablock/placed/placed-datablock.h"


ocrDataBlockFactory_t* newDataBlockFactory(dataBlockType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case dataBlockRegular_id:
        return newDataBlockFactoryRegular(typeArg);
        break;
    case dataBlockPlaced_id:
//        return newDataBlockFactoryPlaced(typeArg);
//        break;
    default:
        ASSERT(0);
    }
    return NULL;
}

#endif /* __DATABLOCK_ALL_H__ */
