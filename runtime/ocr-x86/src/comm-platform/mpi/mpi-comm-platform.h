/**
 * @brief MPI communication platform
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */
#ifndef __MPI_COMM_PLATFORM_H__
#define __MPI_COMM_PLATFORM_H__

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_MPI

#include "utils/ocr-utils.h"
#include "utils/list.h"
#include "ocr-comm-platform.h"

typedef struct {
    ocrCommPlatformFactory_t base;
} ocrCommPlatformFactoryMPI_t;

#define MPI_COMM_RL_MAX 3

typedef struct {
    ocrCommPlatform_t base;
    u64 msgId;
    linkedlist_t * incoming;
    linkedlist_t * outgoing;
    iterator_t * incomingIt;
    iterator_t * outgoingIt;
    u64 maxMsgSize;
    volatile int rl;
    volatile int rl_completed [MPI_COMM_RL_MAX+1];
} ocrCommPlatformMPI_t;

typedef struct {
    paramListCommPlatformInst_t base;
} paramListCommPlatformMPI_t;

extern ocrCommPlatformFactory_t* newCommPlatformFactoryMPI(ocrParamList_t *perType);

void libdepInitMPI(ocrParamList_t *perType);

#endif /* ENABLE_COMM_PLATFORM_MPI */
#endif /* __MPI_COMM_PLATFORM_H__ */
