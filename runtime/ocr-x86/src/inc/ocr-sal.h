/**
 * @brief OCR internal interface for system
 * primitives that OCR needs to function and that do not
 * really fit in the *-platform classes. 
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */



#ifndef __OCR_SAL_H__
#define __OCR_SAL_H__

#include "ocr-types.h"
#include "ocr-utils.h"

/****************************************************/
/* SYSTEM FUNCTIONS                                 */
/****************************************************/

struct _ocrPolicyDomain_t;
struct _ocrPolicyMsg_t;
struct _ocrTask_t;
struct _ocrWorker_t;

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
 * @param worker[out]         Will return a pointer to the current worker
 *                            The resulting pointer should *not* be freed. If NULL,
 *                            will not return anything. If *worker is NULL on
 *                            return, the worker has not started yet
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
void (*getCurrentEnv)(struct _ocrPolicyDomain_t **pd, struct _ocrWorker_t **worker,
                      struct _ocrTask_t **edt, struct _ocrPolicyMsg_t *msg);

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


#endif /* __OCR_SAL_H__ */
