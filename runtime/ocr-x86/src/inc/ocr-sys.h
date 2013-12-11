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
/* SYSTEM FUNCTIONS                                 */
/****************************************************/

// TODO (Bala/Ivan): We need to have a bringup type of function
// We have this in the driver but we need to figure out what
// to pass to it. It should not be a file but rather
// an address of some kind of binary representation of
// how to bring up the machine. Also, we need to decide
// how we distribute the bring up (ie: is only one of these
// functions called (probably not) or are there multiple and
// how do we sync all this up


/**
 * @brief Instructs the runtime to tear itself down
 *
 * The policy-domain passed as argument is the policy-domain
 * initiating the shutdown.
 *
 * This call will call 'stop' and 'finish' on all policy domains
 * and shut the runtiem down
 *
 * @param[in] pd              Policy domain initiating the shutdown
 */
void (*teardown)(struct _ocrPolicyDomain_t *pd);


/**
 * @brief Returns the current environment (policy domain, current EDT and
 * an initialized message)
 *
 * This function allows you to select what you want returned by
 * passing NULL for what you do not want
 *
 * @param pd[out]             Will return a pointer to the current policy
 *                            domain. The resulting pointer should *not*
 *                            be freed. If NULL, will not return anything.
 *                            Note that if *pd is non NULL on input, the call
 *                            will consider that to be the PD to use (saves
 *                            on overhead). If *pd is NULL on return, no policy
 *                            domain is currently active (boot-up or shut-down)
 * @param edt[out]            Will return a pointer to the current EDT
 *                            The resulting pointer should *not* be
 *                            freed. If NULL, will not return anything. If
 *                            *edt is NULL on return, no EDT is currently
 *                            executing (runtime code)
 * @param msg[out]            Will initialize the ocrPolicyMsg_t pointed
 *                            to by msg. If NULL, no initialization will
 *                            occur. Note that no allocation of msg will
 *                            happen so it needs to already be either
 *                            allocated or on the stack
 */
void (*getCurrentEnv)(struct _ocrPolicyDomain_t **pd, struct _ocrTask_t **edt,
                      struct _ocrPolicyMsg_t *msg);

// TODO: Do we need this
extern struct _ocrPolicyDomain_t * (*getMasterPD)();


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

    void (*start)(struct _ocrSys_t* self);

    void (*stop)(struct _ocrSys_t* self);

    void (*finish)(struct _ocrSys_t* self);

    
    /**
     * @brief Prints a message
     *
     * @param self          This system descriptor
     * @param str           String to print
     * @param length        Number of characters to print
     *
     * @warning This function is called from the policy domain directly
     * and is not meant to be called directly from the rest of
     * the runtime code
     */
    void (*print)(struct _ocrSys_t *self, const char* str, u64 length);

    /**
     * @brief Writes the byte array 'str' to a "file"
     * identified by 'id'
     *
     * This call will return the number of characters written
     * @param self          This system descriptor
     * @param str           Characters to write out
     * @param length        Number of characters to write out
     * @param id            Implementation specific ID
     * @return              Number of characters actually written
     *
     * @warning This function is called from the policy domain directly
     * and is not meant to be called directly from the rest of
     * the runtime code
     */
    u64 (*write)(struct _ocrSys_t *self, const char* str, u64 length, u64 id);

    /**
     * @brief Reads bytes into the byte array 'str'
     * from a "file" identified by 'id'
     *
     * This call will return the number of characters read
     *
     * @param self          This system descriptor
     * @param str           Buffer to write to
     * @param length        Size of the buffer (maximum)
     * @param id            Implementation specific ID
     * @return              Number of characters actually read
     * 
     * @warning This function is called from the policy domain directly
     * and is not meant to be called directly from the rest of
     * the runtime code
     */
    u64 (*read)(struct _ocrSys_t *self, char *str, u64 length, u64 id);
    
    /**
     * @brief Perform a memory fence
     *
     * @param self          This system descriptor
     * @todo Do we want to differentiate different types
     * of fences?
     */
    void (*fence)(struct _ocrSys_t *self);

    void (*memCopy)(struct _ocrSys_t *self, void* destination, void* source,
                    u64 size, bool isBackground);
    
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

    /**
     * @brief Convenience function that basically implements a simple
     * lock
     *
     * This will usually be a wrapper around cmpswap32. This function
     * will block until the lock can be acquired
     *
     * @param self      This system descriptor
     * @param lock      Pointer to a 32 bit value
     */
    void (*lock32)(struct _ocrSys_t *self, u32* lock);

    /**
     * @brief Convenience function to implement a simple
     * unlock
     *
     * @param self      This system descriptor
     * @param lock      Pointer to a 32 bit value
     */
    void (*unlock32)(struct _ocrSys_t *self, u32* lock);

    /**
     * @brief Convenience function to implement a simple
     * trylock
     *
     * @param self      This system descriptor
     * @param lock      Pointer to a 32 bit value
     * @return 0 if the lock has been acquired and a non-zero
     * value if it cannot be acquired
     */
    u8 (*trylock32)(struct _ocrSys_t *self, u32* lock);
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
