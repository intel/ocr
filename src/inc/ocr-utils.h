/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_UTILS_H__
#define __OCR_UTILS_H__

#include "ocr-types.h"

// Forward declaration
struct _ocrPolicyDomain_t;

/******************************************************/
/* PARAMETER LISTS                                    */
/******************************************************/

/**
 * @brief Parameter list used to create factories and or instances (through the
 * factories)
 *
 * This struct is meant to be "extended" with the parameters required for a
 * particular function. This type is used in newXXXFactory() functions as well
 * as the instantiate() functions in factories and allows us to have a single API
 * for all instantiate functions thereby making it easy to instantiate multiple
 * types of objects programatically
 */
typedef struct _ocrParamList_t {
    u64 size;                   /**< Size of this parameter list (in bytes) */
    struct _ocrPolicyDomain_t *policy;  /**< Policy domain for this factory/instance */
    char* misc;                 /**< Miscellaneous arguments (NULL terminated string) */
} ocrParamList_t;

#define ALLOC_PARAM_LIST(result, type)                  \
    do {                                                \
        result = (ocrParamList_t *) calloc(1, sizeof(type));          \
        ocrParamList_t *_t = (ocrParamList_t*)result;   \
        _t->size = (u64)sizeof(type);                   \
    } while(0);

#define INIT_PARAM_LIST(var, type)                      \
    do {                                                \
    ocrParamList_t *_t = (ocrParamList_t*)&var;         \
    _t->size = sizeof(type);                            \
    } while(0);

/******************************************************/
/*  ABORT / EXIT OCR                                  */
/******************************************************/

void ocr_abort();

void ocr_exit();


/******************************************************/
/* BITVECTOR OPERATIONS                               */
/******************************************************/

/**
 * @brief Bit operations used to manipulate bit
 * vectors. Currently used in the regular
 * implementation of data-blocks
 */


/**
 * @brief Finds the position of the MSB that
 * is 1
 *
 * @param val  Value to look at
 * @return  MSB set to 1 (from 0 to 15)
 */
u32 fls16(u16 val);

/**
 * @brief Finds the position of the MSB that
 * is 1
 *
 * @param val  Value to look at
 * @return  MSB set to 1 (from 0 to 31)
 */
u32 fls32(u32 val);

/**
 * @brief Finds the position of the MSB that
 * is 1
 *
 * @param val  Value to look at
 * @return  MSB set to 1 (from 0 to 63)
 */
u32 fls64(u64 val);

/**
 * @brief Convenient structure to keep track
 * of GUIDs in a way that is indexable.
 *
 * This is basically a 64 entry vector (at most)
 * with an associated bit vector keeping track of
 * available slots (to reuse them)
 * @todo Extend to have a different number than 64 entries
 */
typedef struct _ocrGuidTracker_t {
    u64 slotsStatus; /**< Bit vector. A 0 indicates the slot is *used* (1 = available slot) */
    ocrGuid_t slots[64]; /**< Slots */
} ocrGuidTracker_t;

/**
 * @brief Initialize an ocrGuidTracker
 *
 * @param self              GUID tracker to initialize
 */
void ocrGuidTrackerInit(ocrGuidTracker_t *self);

/**
 * @brief Adds a GUID to an ocrGuidTracker
 *
 * @param self              GUID tracker to use
 * @param toTrack           GUID to add to the tracker
 * @return ID for the slot used in the tracker. Used for ocrGuidTrackerRemove.
 *         Returns 64 if no slot is found (failure)
 *
 * @warning This method is not thread safe. Use your own locking mechanism around
 * these calls
 */
u32 ocrGuidTrackerTrack(ocrGuidTracker_t *self, ocrGuid_t toTrack);

/**
 * @brief Removes a GUID from an ocrGuidTracker
 *
 * @param self              GUID tracker to use
 * @param toTrack           GUID to remove from the tracker (used to verify that ID is correct)
 * @param id                ID of the GUID to remove (returned by ocrGuidTrackerTrack)
 * @return True on success and false on failure
 *
 * @warning This method is not thread safe
 */
bool ocrGuidTrackerRemove(ocrGuidTracker_t *self, ocrGuid_t toTrack, u32 id);

/**
 * @brief Finds the "next" used slot in the ocrGuidTracker, returns
 * it and marks the slot as unused
 *
 * This function is useful to iterate over all the GUIDs that are stored
 * in the tracker
 *
 * @param self              ocrGuidTracker used
 * @return The slot ID of the next valid GUID or 64 if none are found
 */
u32 ocrGuidTrackerIterateAndClear(ocrGuidTracker_t *self);

/**
 * @brief Searches for the GUID toFind in the ocrGuidTracker
 *
 * @param self              GUID tracker to use
 * @param toFind            GUID to find
 * @return ID for the slot in the tracker or 64 if the GUID is not found
 */
u32 ocrGuidTrackerFind(ocrGuidTracker_t *self, ocrGuid_t toFind);

typedef struct ocrPlaceTrackerStruct_t {
    u64 existInPlaces;
} ocrPlaceTracker_t;

void ocrPlaceTrackerAllocate ( ocrPlaceTracker_t** toFill );
void ocrPlaceTrackerInsert ( ocrPlaceTracker_t* self, unsigned char currPlaceID );
void ocrPlaceTrackerRemove ( ocrPlaceTracker_t* self, unsigned char currPlaceID );
void ocrPlaceTrackerInit ( ocrPlaceTracker_t* self );


#endif /* __OCR_UTILS_H__ */
