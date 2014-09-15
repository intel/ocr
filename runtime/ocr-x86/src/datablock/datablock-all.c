/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "datablock/datablock-all.h"
#include "debug.h"

const char * dataBlock_types [] = {
    "Regular",
    "Lockable",
    "Placed",
    NULL
};

ocrDataBlockFactory_t* newDataBlockFactory(dataBlockType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_DATABLOCK_REGULAR
    case dataBlockRegular_id:
        return newDataBlockFactoryRegular(typeArg, (u32)type);
        break;
#endif
#ifdef ENABLE_DATABLOCK_LOCKABLE
    case dataBlockLockable_id:
        return newDataBlockFactoryLockable(typeArg, (u32)type);
        break;
#endif
#ifdef ENABLE_DATABLOCK_PLACED
    case dataBlockPlaced_id:
//        return newDataBlockFactoryPlaced(typeArg, (u32)type);
//        break;
#endif
    default:
        ASSERT(0);
    }
    return NULL;
}