/**
 * @brief OCR internal interface for low-level system
 * primitives that OCR needs to function and that do not
 * really fit in the *-platform classes. 
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */



#ifndef __OCR_SYS_H__
#define __OCR_SYS_H__

#include "ocr-types.h"
#include "ocr-utils.h"

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/
typedef struct {
    ocrParamList_t base;
} paramListSysFact_t;

typedef struct {
    ocrParamList_t base;
} paramListSysInst_t;


/****************************************************/
/* OCR SYSTEM                                       */
/****************************************************/

// TODO: In the future, maybe think about exposing a vector of
// capabilities so that the PD can check what the system
// provides and act accordingly

struct _ocrSys_t;

typedef struct _ocrSysFcts_t {
    /**
     * @brief Destructor equivalent
     *
     * Cleans up the system description and associated
     * metadata
     *
     * @param self          Pointer to this system descriptor
     */
    void (*destruct)(struct _ocrSys_t* self);

    /**
     * @brief Compare and swap (64 bit)
     *
     * The semantics are as follows (all operations performed atomically):
     *     - if location is cmpValue, atomically replace with
     *       newValue and return cmpValue
     *     - if location is *not* cmpValue, return value at location
     *
     * @param self          This system descriptor
     * @param atomic        Pointer to the atomic value (location)
     * @param cmpValue      Expected value of the atomic
     * @param newValue      Value to set if the atomic has the expected value
     *
     * @return Old value of the atomic
     */
    u64 (*cmpswap64)(struct _ocrSys_t *self, u64* atomic, u64 cmpValue, u64 newValue);

    /**
     * @brief Atomic add (64 bit)
     *
     * The semantics are as follows (all operations performed atomically):
     *     - atomically increment location by addValue
     *     - return new value (after addition)
     *
     * @param self      This system descriptor
     * @param atomic    Pointer to the atomic value (location)
     * @param addValue  Value to add to location
     * @return New value of the location
     */
    u64 (*xadd64)(struct _ocrSys_t *self, u64* atomic, u64 addValue);

    /**
     * @brief Remote atomic add (64 bit)
     *
     * The semantics are as follows (all operations performed atomically):
     *     - atomically increment location by addValue
     *     - no value is returned (the increment will happen "at some
     *       point")
     *
     * @param self      This system descriptor
     * @param atomic    Pointer to the atomic value (location)
     * @param addValue  Value to add to location
     */
    void (*radd64)(struct _ocrSys_t *self, u64* atomic, u64 addValue);

    /**
     * @brief Compare and swap (64 bit)
     *
     * The semantics are as follows (all operations performed atomically):
     *     - if location is cmpValue, atomically replace with
     *       newValue and return cmpValue
     *     - if location is *not* cmpValue, return value at location
     *
     * @param self          This system descriptor
     * @param atomic        Pointer to the atomic value (location)
     * @param cmpValue      Expected value of the atomic
     * @param newValue      Value to set if the atomic has the expected value
     *
     * @return Old value of the atomic
     */
    u32 (*cmpswap32)(struct _ocrSys_t *self, u32* atomic, u32 cmpValue, u32 newValue);

    /**
     * @brief Atomic add (64 bit)
     *
     * The semantics are as follows (all operations performed atomically):
     *     - atomically increment location by addValue
     *     - return new value (after addition)
     *
     * @param self      This system descriptor
     * @param atomic    Pointer to the atomic value (location)
     * @param addValue  Value to add to location
     * @return New value of the location
     */
    u32 (*xadd32)(struct _ocrSys_t *self, u32* atomic, u32 addValue);

    /**
     * @brief Remote atomic add (32 bit)
     *
     * The semantics are as follows (all operations performed atomically):
     *     - atomically increment location by addValue
     *     - no value is returned (the increment will happen "at some
     *       point")
     *
     * @param self      This system descriptor
     * @param atomic    Pointer to the atomic value (location)
     * @param addValue  Value to add to location
     */
    void (*radd32)(struct _ocrSys_t *self, u32* atomic, u32 addValue);
} ocrSysFcts_t;

/**
 * @brief The very low-level system software
 *
 */
typedef struct _ocrSys_t {
    ocrSysFcts_t *fctPtrs;
} ocrSys_t;

/****************************************************/
/* OCR SYSTEM FACTORY                               */
/****************************************************/

/**
 * @brief Factory for system descriptors
 */
typedef struct _ocrSysFactory_t {
    /**
     * @brief Destroy the factory freeing up any
     * memory associated with it
     *
     * @param self          This factory
     */
    void (*destruct)(struct _ocrSysFactory_t *self);


    ocrSys_t* (*instantiate)(struct _ocrSysFactory_t* factory, ocrParamList_t *perInstance);

    ocrSysFcts_t sysFcts;
} ocrSysFactory_t;


#endif /* __OCR_SYS_H__ */
