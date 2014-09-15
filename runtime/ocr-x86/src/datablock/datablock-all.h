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
#include "ocr-config.h"
#include "ocr-datablock.h"
#include "utils/ocr-utils.h"

typedef enum _dataBlockType_t {
    dataBlockRegular_id,
    dataBlockLockable_id,
    dataBlockPlaced_id,
    dataBlockMax_id
} dataBlockType_t;

extern const char * dataBlock_types[];

// Regular datablock
#include "datablock/regular/regular-datablock.h"
// Lockable datablock
#include "datablock/lockable/lockable-datablock.h"
// Placed datablock (not ported as of now)
// #include "datablock/placed/placed-datablock.h"


ocrDataBlockFactory_t* newDataBlockFactory(dataBlockType_t type, ocrParamList_t *typeArg);

#endif /* __DATABLOCK_ALL_H__ */
