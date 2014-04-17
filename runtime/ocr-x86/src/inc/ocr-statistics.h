/**
 * @brief OCR interface to the statistics gathering framework
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifdef OCR_ENABLE_STATISTICS

#ifndef __OCR_STATISTICS_H__
#define __OCR_STATISTICS_H__

#include "ocr-sync.h"
#include "ocr-types.h"

/**
 * @defgroup ocrStats Statistics collection framework
 *
 * @brief Framework to collect runtime statistics in OCR. This is an optional
 * component
 *
 * The statistics collector is based on the following concepts:
 *     - ocrStatsProcess_t which are the 'processes' in terms of
 *       Lamport clocks. They are the things that 'tick'.
 *         - The following elements 'tick' in OCR:
 *             - Executors and allocators
 *             - EDTs, Events and Data-blocks
 *         - The ocrStatsProcess_t are not implementable (ie: they are provided by
 *           the OCR runtime and should not be tweaked)
 *     - ocrStatsMessage_t is the base class for messages exchanged between ocrStatsProcess_t.
 *       They contain the message sender and tick as well as any additional
 *       information for the message.
 *     - ocrStatsFilter_t are the information collectors and sit on top of the
 *       ocrStatsProcess_t. In other words, ocrStatsProcess_t forwards relevant messages
 *       it receives to the ocrStatsFilter_t after having modified the tick to be the greater
 *       of its own tick + 1 or the tick of the messagge it received (Lamport).
 *         - The ocrStatsFilter_t are implementable and may choose to ignore messages
 *           and/or process them differently. They are the first line in determining
 *           which information should be collected
 *     - ocrStatsAgg_t will aggregate the information from various ocrStatsFilter_t. This
 *       is not implementable
 *
 * The following events will trigger tick updates:
 *     - EDT1 acquires DB1                        EDT1 <-> DB1  (STATS_DB_ACQ)
 *     - EDT1 'acquires' Evt1 (pure control dep)  EDT1 <-  Evt1 (STATS_EVT_ACQ)
 *     - EDT1 starts on Exe1                      EDT1 <-> Exe1 (STATS_EDT_START)
 *     - EDT1 creates XX (EDT/Evt/DB w/o acquire) EDT1  -> XX   (STATS_*_CREATE and possibly STATS_DEP_ADD)
 *     - EDT1 destroys/frees XX (EDT/Evt/DB)      EDT1  -> XX   (STATS_*_DESTROY)
 *     - EDT1 satisfies Evt1                      EDT1  -> Evt1 (STATS_DEP_SATISFY)
 *         If Evt1 has "sinks" (like EDT2)        Evt1  -> EDT2 (STATS_DEP_SATISFY/STATS_EDT_READY)
 *     - EDT1 releases DB1                        EDT1  -> DB1  (STATS_DB_REL)
 *     - EDT1 adds a dep between Evt1/DB1 & EDT2  EDT1  -> Evt1/DB1 (STATS_DEP_ADD)
 *         If Evt1/DB1 is satisfied               Evt1/DB1 -> EDT2  (STATS_DEP_SATISFY)
 *     - EDT1 ends                                EDT1 <-> Exe1 (STATS_EDT_END)
 *     - DB1 moves from Allocator1 to Allocator2  DB1 <-> All1 <-> All2 (STATS_DB_MOVE)
 *     - DB1 gets created on All1                 DB1 <-> All1  (STATS_DB_CREATE)
 *     - DB1 gets removed/freed on All1           DB1 <-> All1  (STATS_DB_DESTROY)
 *     - EDT1 accesses (r/w or r/o) DB1           EDT1 <-> DB1 (local) (STATS_DB_ACCESS)
 *         This is a local ticker that is synched on acquire/release/free since we do not (cannot)
 *         model overlapping accesses to the same DB
 * @{
 */


/**
 * @brief Enumeration defining the type of events that can be handled
 *
 * @todo Having it here may not make it too easy to change (recompile
 * a lot) but on the other hand, if you are interested in other types
 * of events, a lot of the code will probably change... See if this can/should
 * be improved.
 * @todo Figure out if we need/want events on event satisfaction
 */
typedef enum {
    /* EDT related events */
    STATS_EVT_INVAL,   /**< An invalid event */
    STATS_EDT_CREATE,  /**< The EDT was first created */
    STATS_EDT_DESTROY, /**< The EDT was destroyed (rare) */
    STATS_EDT_READY,   /**< The EDT became ready to run (last dep satisfied)*/
    STATS_EDT_START,   /**< The EDT started execution */
    STATS_EDT_END,     /**< The EDT ended execution */

    /* DB related events */
    STATS_DB_CREATE,   /**< The DB was created */
    STATS_DB_DESTROY,  /**< The DB was destroyed/freed */
    STATS_DB_ACQ,      /**< The DB was acquired */
    STATS_DB_REL,      /**< The DB was releaased */
    STATS_DB_MOVE,     /**< The DB was moved */
    STATS_DB_ACCESS,   /**< The DB was accessed (read or write) */

    /* Event related events */
    STATS_EVT_CREATE,  /**< The Event was created */
    STATS_EVT_DESTROY, /**< The Event was destroyed */

    /* Dependence related events */
    STATS_DEP_SATISFY, /**< A dependence was satisfied */
    STATS_DEP_ADD,     /**< A dependence was added */
    STATS_EVT_MAX      /**< Marker for number of events */
} ocrStatsEvt_t;

/**
 * @brief Base class for messages sent between ocrStatsProcess_t
 *
 * This class can be extended to provide specific message implementations
 * @see ocr-stat-messages.h
 */
typedef struct _ocrStatsMessage_t {
    void (*create)(struct _ocrStatsMessage_t *self, ocrStatsEvt_t
                   type, u64 tick, ocrGuid_t src, ocrGuid_t dest,
                   void* configuration);

    void (*destruct)(struct _ocrStatsMessage_t *self);

    /**
     * @brief Dump the message to a string
     *
     * The function should allocate the string and return
     * a pointer to it. It will be freed by the caller.
     * The message should *not* include the type, tick, source
     * and destination which will be properly combined. The
     * returned string should only contain message specific
     * information
     *
     * @param self      Pointer to this message
     * @return String for the message
     */
    char* (*dump)(struct _ocrStatsMessage_t *self);

    // Data
    u64 tick;
    ocrGuid_t src, dest;
    ocrStatsEvt_t type;
    volatile u8 state; /**< Internal information used for sync messages:
                        * 0: delete on processing
                        * 1: keep after processing and set to 2
                        * 2: done with processing and tick updated
                        */
} ocrStatsMessage_t;

/**
 * @brief "Base" message creation function
 */
void ocrStatsMessageCreate(ocrStatsMessage_t *self, ocrStatsEvt_t type, u64 tick, ocrGuid_t src,
                           ocrGuid_t dest, void* configuration);

void ocrStatsMessageDestruct(ocrStatsMessage_t *self);

/**
 * @brief A "filter" on statistics being collected.
 *
 * This is a "base class" for all other filter interfaces which can be
 * implemented to provide just the statistics needed.
 *
 * Filters are organized in a tree with each filter having an
 * optional 'parent' filter. Periodically (depending on how the
 * filter is defined) or upon the destruction of the filter, it
 * will merge with its parent filter.
 *
 * The leave filters are attached to each process and other filters
 * aggregate the information from children filters.
 */
typedef struct _ocrStatsFilter_t {
    // TODO: Do we need a GUID for this, probably not
    void (*create)(struct _ocrStatsFilter_t *self, struct _ocrStatsFilter_t *parent,
                   void* configuration);

    void (*destruct)(struct _ocrStatsFilter_t *self);

    /**
     * @brief "Dump" the information in this filter.
     *
     * The function will allocate memory for the string 'out' returned
     * and return a non-null value if the function needs to be called again
     * to get additional output (to allow for the splitting up of
     * the dumped message)
     *
     * @param self           Pointer to this ocrStatsFilter
     * @param out            Returned pointer to the output string
     * @param chunk          ID of the chunk to dump. Always starts at 0 and
     *                       the caller can then follow the chunk IDs returned
     *                       by successive calls of the function
     * @param configuration  Optional configuration (implementation specific)
     *
     * @return 0 if nothing left to dump or a non-zero ID indicating the
     * next "chunk" ID to pass to the dump function to get the next part
     * of the output
     */
    u64 (*dump)(struct _ocrStatsFilter_t *self, char** out, u64 chunk,
               void* configuration);

    /**
     * @brief "Notify" the filter of a particular message.
     *
     * @param self      This filter
     * @param mess      The message received
     */
    void (*notify)(struct _ocrStatsFilter_t *self,
                   const ocrStatsMessage_t *mess);

    /**
     * @brief Merge information from one filter into that of another
     *
     * This is used when information from one filter is combined with that
     * of another. In particular, each EDT will keep track of the loads/stores
     * to a DB in its own filter which is merged back to the DB's global filter
     * when the EDT releases the DB. This is also used when children filter are
     * merged with their parent
     *
     * @param self      This filter
     * @param other     Filter to merge into this filter
     * @param toKill    If non-zero, this means the filter is going away after this merge
     *                  and the assumption is that the caller will not destroy the filter
     *                  leaving this responsability to the callee (this allows for better
     *                  parallelism). If 0, the filter will not be destroyed and it
     *                  can be assumed that on return, this call will have all the
     *                  information it requires to do the merge.
     */
    void (*merge)(struct _ocrStatsFilter_t *self, struct _ocrStatsFilter_t *other, u8 toKill);

    struct _ocrStatsFilter_t *parent;
} ocrStatsFilter_t;

/**
 * @brief "Base" filter creation function
 */
void ocrStatsFilterCreate(ocrStatsFilter_t *self, ocrStatsFilter_t *parent,
                          void* configuration);

void ocrStatsFilterDestruct(ocrStatsFilter_t *self);

char* ocrStatsFilterDump(u64 tick, ocrStatsEvt_t type,  ocrGuid_t src, ocrGuid_t dest);


/**
 * @brief A "process" in the Lamport clock sense of things.
 *
 * This class cannot be extended/implemented and contains
 * only data. Use the ocrStatsXXX functions to access
 *
 * @todo See about making the filters structure more space efficient
 * @warn There is an implicit assumption that 'tick' will only be touched by
 * one worker at a time (not race free and other considerations). In particular,
 * this means that a process 'receiving' messages should not tick on its own accord (ie:
 * actively send messages) and vice-versa (a process sending message should not be receiving
 * them as well). This is true currently because the only things that 'send messages' are
 * the running EDTs and they do not receive any messages (apart from sync messages
 * which they initiate).
 */
typedef struct _ocrStatsProcess_t {
    ocrGuid_t me;               /**< GUID of this process (GUID of EDT for example, ...) */
    ocrLock_t *processing;      /**< Lock will be head when the queue of messages is being processed */
    ocrQueue_t *messages;       /**< Messages queued up (in tick order) */
    u64 tick;                   /**< Tick right before processing messages[0] */
    ocrStatsFilter_t ***filters;/**< Filters "listening" to events sent to this process */
    u64 *filterCounts;          /**< Bits [0-31] contain number of filters, bits [32-63] contain allocated filters */
} ocrStatsProcess_t;

/**
 * @brief Create a ocrStatsProcess_t instance
 *
 * @param self  Instance to create/initialize
 * @param guid  GUID of the element (EDT, DB, ...) this is attached to
 */
void ocrStatsProcessCreate(ocrStatsProcess_t *self, ocrGuid_t guid);

/**
 * @brief Destroy a ocrStatsProcess_t instance
 *
 * @param self  Instance to destroy
 */
void ocrStatsProcessDestruct(ocrStatsProcess_t *self);

/**
 * @brief Register a filter to respond to the messages
 * indicated by the bit mask
 *
 * When a process receives a "message" (in Lamport terminology),
 * it will inform all the filters registered with the type of message
 * received and then free the message. Messages will always be passed in
 * order to the filters but there is no guarantee as to what order
 * the filters will receive the messages
 *
 * @param self      The ocrStatsProcess_t to register with
 * @param bitMask   Bitmask of ocrStatsEvt_t that filter responds to
 * @param filter    Filter to pass the message to
 *
 * @warn A filter should be registered to AT MOST one process
 * @warn Filter registration is not "thread-safe" so it should be done
 * only on process creation
 */
void ocrStatsProcessRegisterFilter(ocrStatsProcess_t *self, u64 bitMask,
                                   ocrStatsFilter_t *filter);

/**
 * @brief Send a one-way message to dst
 *
 * This call will:
 *     - increment src's clock by 1
 *     - Inform dst of the message and update its clock by
 *       taking the larger of the message's clock or dst's clock + 1
 *       We call this resulting clock 'tMess'
 *     - Set the message's clock to 'tMess' and pass the message to relevant
 *       filters
 *
 * @param src       ocrStatsProcess_t sending the message
 * @param dst       ocrStatsProcess_t reciving the message
 * @param msg       ocrStatsMessage_t sent
 */

void ocrStatsAsyncMessage(ocrStatsProcess_t *src, ocrStatsProcess_t *dst,
                          ocrStatsMessage_t *msg);

/**
 * @brief Send a two-way message to dst
 *
 * This call will:
 *     - increment src's clock by 1
 *     - Inform dst of the message and update its clock by
 *       taking the larger of the message's clock or dst's clock + 1
 *     - We call this resulting clock 'tMess'
 *     - Set the message's clock to 'tMess' and pass the message to the
 *       relevant filters.
 *     - Update the src's clock to match the maximum of 'tMess' and the
 *       current src's clock
 *
 * @param src       ocrStatsProcess_t sending the message
 * @param dst       ocrStatsProcess_t reciving the message
 * @param msg       ocrStatsMessage_t sent
 */
void ocrStatsSyncMessage(ocrStatsProcess_t *src, ocrStatsProcess_t *dst,
                         ocrStatsMessage_t *msg);

/**
 * @}
 */

#endif /* __OCR_STATISTICS_H__ */

#endif /* OCR_ENABLE_STATISTICS */
