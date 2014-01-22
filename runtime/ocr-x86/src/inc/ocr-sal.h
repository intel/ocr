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
#include "utils/ocr-utils.h"

/****************************************************/
/* CONVENIENCE FUNCTIONS                            */
/****************************************************/

// The following functions are generic and implemented
// using a call to the policy domain but they are
// common enough to merit their own simplified
// wrapper
// For example, sal_exit will send a message to
// the policy domain of type PD_MSG_SYS_EXIT which
// may eventually call into ocrSal_t (exit)

/**
 * @brief Exit OCR
 *
 * This brings down the runtime cleanly
 */
void sal_exit(u64 errorCode);

/**
 * @brief Emergency abort.
 */
void sal_abort();

/**
 * @brief Will cause an abort if the condition is false.
 *
 * @note Use the ASSERT macro (in debug.h) which will call this
 * if debug mode is enabled
 */
void sal_assert(bool cond, const char* file, u64 line);

/**
 * @brief Causes the formatted string to be printed out
 * to a console output
 *
 * This operation may be a no-op on some platforms
 */
int sal_printf(const char * fmt, ...);

/**
 * @brief Causes the formatted string to be printed out
 * to a memory buffer
 *
 * This operation may be a no-op on some platforms
 */
int sal_snprintf(char * buf, int size, const char * fmt, ...);

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
extern void (*teardown)(struct _ocrPolicyDomain_t *pd);


/**
 * @brief Returns the current environment (policy domain, current EDT and
 * current worker)
 *
 * This function allows you to select what you want returned by
 * passing NULL for what you do not want. For all these parameters, if
 * *parameter is non NULL, the function may consider that to be the
 * current value of the parameter. If the parameter is NULL, no
 * value will be returned (ie: the caller does not care about the policy-domain
 * or worker or edt or msg). None of the returned pointers should be freed.
 *
 * @param pd[out]             Will return a pointer to the current policy
 *                            domain.
 *                            be freed. If NULL, will not return anything.
 *                            If *pd is NULL on return, no policy domain is
 *                            active (boot-up)
 * @param worker[out]         Will return a pointer to the current worker
 *                            If NULL, will not return anything. If *worker is
 *                            NULL on return, the worker has not started yet.
 * @param edt[out]            Will return a pointer to the current EDT
 *                            If NULL, will not return anything. If
 *                            *edt is NULL on return, no EDT is currently
 *                            executing (runtime code)
 * @param msg[out]            If NULL, will not return anything. On return, initializes
 *                            the message passed in to a value that is initialized
 *                            for communication if needed.
 *                            The message should be used at most once.
 */
extern void (*getCurrentEnv)(struct _ocrPolicyDomain_t **pd, struct _ocrWorker_t **worker,
                             struct _ocrTask_t **edt, struct _ocrPolicyMsg_t *msg);

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/
typedef struct {
    ocrParamList_t base;
} paramListSalFact_t;

typedef struct {
    ocrParamList_t base;
} paramListSalInst_t;


/****************************************************/
/* OCR SYSTEM                                       */
/****************************************************/

// TODO: In the future, maybe think about exposing a vector of
// capabilities so that the PD can check what the system
// provides and act accordingly

struct _ocrSal_t;

typedef struct _ocrSalFcts_t {
    /**
     * @brief Destructor equivalent
     *
     * Cleans up the system description and associated
     * metadata
     *
     * @param self          Pointer to this system descriptor
     */
    void (*destruct)(struct _ocrSal_t* self);

    void (*begin)(struct _ocrSal_t* self, struct _ocrPolicyDomain_t *pd);
    
    void (*start)(struct _ocrSal_t* self, struct _ocrPolicyDomain_t *pd);

    void (*stop)(struct _ocrSal_t* self);

    void (*finish)(struct _ocrSal_t* self);


    /**
     * @brief Exits the runtime cleanly
     *
     * @param self          This system descriptor
     * @param errorCode     Code to return
     */
    void (*exit)(struct _ocrSal_t *self, u64 errorCode);

    /**
     * @brief Aborts the runtime (emergency)
     *
     * @param self          This system descriptor
     */
    void (*abort)(struct _ocrSal_t *self);
    
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
    void (*print)(struct _ocrSal_t *self, const char* str, u64 length);

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
     * @todo Not used/implemented
     */
    u64 (*write)(struct _ocrSal_t *self, const char* str, u64 length, u64 id);

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
     * @todo Not used/implemented
     */
    u64 (*read)(struct _ocrSal_t *self, char *str, u64 length, u64 id);
} ocrSalFcts_t;

/**
 * @brief The very low-level system software
 *
 */
typedef struct _ocrSal_t {
    struct _ocrPolicyDomain_t *pd;
    ocrSalFcts_t fcts;
} ocrSal_t;

/****************************************************/
/* OCR SYSTEM FACTORY                               */
/****************************************************/

/**
 * @brief Factory for system descriptors
 */
typedef struct _ocrSalFactory_t {
    /**
     * @brief Destroy the factory freeing up any
     * memory associated with it
     *
     * @param self          This factory
     */
    void (*destruct)(struct _ocrSalFactory_t *self);


    ocrSal_t* (*instantiate)(struct _ocrSalFactory_t* factory, ocrParamList_t *perInstance);

    ocrSalFcts_t salFcts;
} ocrSalFactory_t;

#endif /* __OCR_SAL_H__ */
