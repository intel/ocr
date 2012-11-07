/**
 * @brief OCR internal interface for the description of the emulated
 * machine
 * @authors Todd A. Anderson, Intel Corporation
 * @date 2012-09-21
 * Copyright (c) 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/


#ifndef __OCR_MACHINE_H__
#define __OCR_MACHINE_H__

#include "ocr-types.h"
#include "ocr-config.h"

// ===============================================================================

/**
   @brief Models a processor.  There is some flexibiliy here.  You could model a
          4 core machine as 4 processors or 1 processor with 4 threads.
 **/
typedef struct {
    u32 numThreads;
    u32 id;
    u32 procIndex;
} Processor;

/**
   @brief This will probably be an abstract class that is the base for modeling
          a memory region.
 **/
typedef struct {
    char *name;
    u32   mrIndex;
    void *address;  // is valid only if is_cache is false
    int   is_cache;
} MemoryRegion;

/**
   @brief Models the cost for a processor to access a given memory region.
 **/
typedef struct {
    int    valid;
    double timeCost;
} CostDescriptor;

int CostDescriptor_equal(const CostDescriptor *left, const CostDescriptor *right);
int CostDescriptor_less (const CostDescriptor *left, const CostDescriptor *right);

typedef struct {
    u32 num_processors;
    u32 num_memory_regions;
    Processor ** machineProcessors;
    MemoryRegion ** machineMemoryRegions;
    CostDescriptor ** costMatrix;
} MachineDescription;

/**
   @brief Get the number of processors in the platform.
 **/
u32 MachineDescription_getNumProcessors(MachineDescription *this);

/**
   @brief Get the number of distinct memory regions in the platform.
 **/
u32 MachineDescription_getNumMemoryRegions(MachineDescription *this);

/**
   @brief Get information about one of the processors.
 **/
Processor * MachineDescription_getProcessor(MachineDescription *this, u32 processor_index);

/**
   @brief Get information about one of the memory regions.
 **/
MemoryRegion * MachineDescription_getMemoryRegion(MachineDescription *this, u32 memory_region_index);

/**
   @brief Get the cost for the given processor to access the given memory.
 **/
CostDescriptor MachineDescription_getProcessorMemoryCost(MachineDescription *this, u32 processor_index, u32 memory_region_index);

/**
   @brief Get the set of processor indices which have equal access costs to the
          specified memory region as the specified processor.
 **/
u32 * MachineDescription_whoElseShares(MachineDescription *this, u32 processor_index, u32 memory_region_index, u32 *num_results);

/**
   @brief Get the set of processor indices which have the lowest access cost to the
          specified memory region.
 **/
u32 * MachineDescription_closestProcessorsToMemory(MachineDescription *this, u32 memory_region_index, u32 *num_results);

/**
   @brief Get the set of memory region indices which have the lowest access cost
          from the specified processor.
 **/
u32 * MachineDescription_closestMemoryRegionsToProcessor(MachineDescription *this, u32 processor_index, u32 *num_results);

/**
   @brief Get the full M x N matrix of costs for each of the M processors to
          access each of the N memory regions.
 **/
CostDescriptor ** MachineDescription_getFullCostMatrix(MachineDescription *this);

int MachineDescription_loadFromPDL(MachineDescription *this, const char *pdlFilename);

/**
   @brief Get a pointer to the currently loaded machine description.
 **/
MachineDescription * getMachineDescription();

/**
   @brief Fill in the machine description with Univ of Vienna PDL
          in XML.
 **/
void setMachineDescriptionFromPDL(const char *pdlFilename);

#endif /* __OCR_MACHINE_H__ */
