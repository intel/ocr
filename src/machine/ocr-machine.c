/**
 * @brief Implementation of the machine description
 * @authors Todd A. Anderson, Intel Corporation
 * @date 2012-11-07
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

#include <stdio.h>
#include <ocr-machine.h>
#include <libxml/parser.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "ocr-macros.h"

MachineDescription * g_MachineDescription = NULL;

void Processor_ctor(Processor *this, u32 threads, u32 id, u32 procIndex) {
    this->numThreads = threads;
    this->id = id;
    this->procIndex = procIndex;
}

void MemoryRegion_ctor(MemoryRegion *this, char *name, u32 mrIndex, u64 size, int is_cache) {
    this->name = name;
    this->mrIndex = mrIndex;
    this->address = NULL;
    this->size = size;
    this->is_cache = is_cache;
}

// Returns false if the new cost was higher than some other path found, true otherwise.
int CostDescriptor_setTimeCost(CostDescriptor *this, double cost) {
    int res = 0;
    if(!this->valid || cost < this->timeCost) {
        this->timeCost = cost;
        res = 1;
    }
    this->valid = 1;
    return res;
}

int CostDescriptor_equal(const CostDescriptor *left, const CostDescriptor *right) {
    if ( !left->valid )  return 0;
    if ( !right->valid ) return 0;
    if ( left->timeCost == right->timeCost) return 1;
    return 0;
}

int CostDescriptor_less(const CostDescriptor *left, const CostDescriptor *right) {
    if ( left->valid ) {
        if ( right->valid ) {
            return left->timeCost < right->timeCost;
        } else {
            return 1;
        }
    } else {
        if ( right->valid ) {
            return 0;
        } else {
            return 1;
        }
    }
}


/**
 *    @brief Get the number of processors in the platform.
 *     **/
u32 MachineDescription_getNumProcessors(MachineDescription *this) {
    return this->num_processors;
}

/**
   @brief Get the number of hardware threads in the platform.
 **/
u32 MachineDescription_getNumHardwareThreads(MachineDescription *this) {
    u32 num_proc;
    u32 i;
    u32 num_hw = 0;

    num_proc = MachineDescription_getNumProcessors(this);
    for(i = 0; i < num_proc; ++i) {
        Processor *proc = MachineDescription_getProcessor(this, i);
        num_hw += proc->numThreads;
    }

    return num_hw;
}

/**
 *    @brief Get the number of distinct memory regions in the platform.
 *     **/
u32 MachineDescription_getNumMemoryRegions(MachineDescription *this) {
    return this->num_memory_regions;
}

/**
   @brief Get the total amount of DRAM in the system (in bytes).
 **/
u64 MachineDescription_getDramSize(MachineDescription *this) {
    u32 num_mr;
    u32 i;
    u64 main_memory_size = 0;

    num_mr = MachineDescription_getNumMemoryRegions(this);
    for(i = 0; i < num_mr; ++i) {
        MemoryRegion *mr = MachineDescription_getMemoryRegion(this, i);
        if(!mr->is_cache) {
            main_memory_size += mr->size;
        }
    }

    return main_memory_size;
}

unsigned getNumMasterMemoryRegions(xmlNodePtr master) {
    xmlNodePtr cur;
    unsigned res = 0;

    cur = master->xmlChildrenNode;
    while (cur != NULL) {
        //printf("%s\n",cur->name);
        if(!xmlStrcmp(cur->name, (const xmlChar *)"MemoryRegion")) {
            ++res;
        }
        cur = cur->next;
    }

    return res;
}

Processor * getProcessorFromId(MachineDescription *this, u32 id) {
    unsigned i;

    for(i = 0; i < MachineDescription_getNumProcessors(this); ++i) {
        if(this->machineProcessors[i]->id == id) {
            return this->machineProcessors[i];
        }
    }

    return NULL;
}

MemoryRegion * getMemoryRegionFromName(MachineDescription *this, const char *name) {
    unsigned i;

    for(i = 0; i < MachineDescription_getNumMemoryRegions(this); ++i) {
        if(strcmp(this->machineMemoryRegions[i]->name,name) == 0) {
            return this->machineMemoryRegions[i];
        }
    }

    return NULL;
}

xmlNodePtr getNodeByName(xmlNodePtr first_node, const char *name) {
    xmlNodePtr res;

    res = first_node;
    while(res != NULL) {
        if(!xmlStrcmp(res->name, (const xmlChar *)name)) {
            break;
        }
        res = res->next;
    }

    return res;
}

int get_mr_size(MachineDescription *this,
                xmlDocPtr doc,
                u64 *mr_size,
                xmlNodePtr mr_node,
                const char *descriptor_name) {
    xmlNodePtr MRDescriptor_node;
    xmlNodePtr Property_node;
    xmlNodePtr name_node, value_node;

    MRDescriptor_node = getNodeByName(mr_node->xmlChildrenNode, descriptor_name);
    if(MRDescriptor_node == NULL) {
        printf("Didn't find %s.\n",descriptor_name);
        return 0;
    }

    Property_node = getNodeByName(MRDescriptor_node->xmlChildrenNode, "Property");
    while(Property_node) {
        name_node = getNodeByName(Property_node->xmlChildrenNode, "name");
        if(name_node == NULL) {
            printf("Name not found in Property.\n");
            return 0;
        }

        if(!xmlStrcmp(xmlNodeListGetString(doc, name_node->xmlChildrenNode, 1), (const xmlChar *)"SIZE")) {
            const char *str;
            char *end;

            value_node = getNodeByName(Property_node->xmlChildrenNode, "value");
            if(value_node == NULL) {
                printf("Value not found in Property.\n");
                return 0;
            }

            str = (const char *)xmlNodeListGetString(doc, value_node->xmlChildrenNode, 1);
            *mr_size = strtoull(str, &end, 10);
            if(*mr_size == 0 && end == str) {
                printf("Size value was not a number.\n");
                return 0;
            }
            return 1;
        }

        Property_node = getNodeByName(Property_node->next, "Property");
    }

    printf("Didn't find Property.\n");
    return 0;
}

int get_access_time(MachineDescription *this,
                    xmlDocPtr doc,
                    double *access_time,
                    xmlNodePtr mr_node,
                    const char *descriptor_name) {
    xmlNodePtr MRDescriptor_node;
    xmlNodePtr Property_node;
    xmlNodePtr name_node, value_node;

    MRDescriptor_node = getNodeByName(mr_node->xmlChildrenNode, descriptor_name);
    if(MRDescriptor_node == NULL) {
        printf("Didn't find %s.\n",descriptor_name);
        return 0;
    }

    Property_node = getNodeByName(MRDescriptor_node->xmlChildrenNode, "Property");
    while(Property_node) {
        name_node = getNodeByName(Property_node->xmlChildrenNode, "name");
        if(name_node == NULL) {
            printf("Name not found in Property.\n");
            return 0;
        }

        if(!xmlStrcmp(xmlNodeListGetString(doc, name_node->xmlChildrenNode, 1), (const xmlChar *)"ACCESS_TIME")) {
            value_node = getNodeByName(Property_node->xmlChildrenNode, "value");
            if(value_node == NULL) {
                printf("Value not found in Property.\n");
                return 0;
            }

            *access_time = atof((const char *)xmlNodeListGetString(doc, value_node->xmlChildrenNode, 1));
            return 1;
        }

        Property_node = getNodeByName(Property_node->next, "Property");
    }

    printf("Didn't find Property.\n");
    return 0;
}

int MachineDescription_loadFromPDL(MachineDescription *this, const char *pdlFilename) {
    xmlDocPtr doc;
    xmlNodePtr cur, start;
    unsigned i, j;

    struct stat buf;
    int file_exists = stat(pdlFilename, &buf);
    if (file_exists != 0) {
        printf("Cannot find machine description file '%s'\n", pdlFilename);
        return 1;
    }

    doc = xmlParseFile(pdlFilename);
    if(doc == NULL) {
        printf("xmlParseFile failed.\n");
        return 1;
    }

    cur = xmlDocGetRootElement(doc);
    if(cur == NULL) {
        printf("Empty PDL.\n");
        xmlFreeDoc(doc);
        return 2;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *)"PlatformDescription")) {
        printf("Wrong PDL document type.\n");
        xmlFreeDoc(doc);
        return 3;
    }

    start = cur = cur->xmlChildrenNode;

    unsigned num_masters = 0, num_inter = 0, num_mr = 0;

    while (cur != NULL) {
        //printf("%s\n",cur->name);
        if(!xmlStrcmp(cur->name, (const xmlChar *)"Master")) {
            ++num_masters;
            num_mr += getNumMasterMemoryRegions(cur);
        } else if(!xmlStrcmp(cur->name, (const xmlChar *)"Interconnect")) {
            ++num_inter;
        } else if(!xmlStrcmp(cur->name, (const xmlChar *)"MemoryRegion")) {
            ++num_mr;
        } else {
            //printf("Unknown PDL child type %s\n",cur->name);
            //return 4;
        }
        cur = cur->next;
    }

//    printf("Procs: %d, Inter: %d, Memories: %d\n",num_masters, num_inter, num_mr);

    this->num_processors = num_masters;
    this->num_memory_regions = num_mr;

    // Setup the costMatrix with the right number of rows and columns.
    this->costMatrix = (CostDescriptor**)checked_malloc(this->costMatrix, num_masters * sizeof(CostDescriptor*));

    for(i = 0; i < num_masters; ++i) {
        this->costMatrix[i] = (CostDescriptor*)checked_malloc(this->costMatrix[i], num_mr * sizeof(CostDescriptor));
        memset(this->costMatrix[i],0,num_mr * sizeof(CostDescriptor));
    }

    this->machineProcessors    = (Processor**)   checked_malloc(this->machineProcessors, num_masters * sizeof(Processor*));;
    this->machineMemoryRegions = (MemoryRegion**)checked_malloc(this->machineMemoryRegions, num_mr      * sizeof(MemoryRegion*));;

    cur = start;

    unsigned master_index = 0, mr_index = 0;

    while (cur != NULL) {
        //printf("%s\n",cur->name);
        if(!xmlStrcmp(cur->name, (const xmlChar *)"Master")) {
            this->machineProcessors[master_index] = (Processor*)checked_malloc(this->machineProcessors[master_index], sizeof(Processor));
            Processor_ctor(this->machineProcessors[master_index],
                           atoi((const char *)xmlGetProp(cur,(const xmlChar *)"quantity")),
                           atoi((const char *)xmlGetProp(cur,(const xmlChar *)"id")),
                           master_index);
            ++master_index;

            xmlNodePtr mr_cur;
            mr_cur = cur->xmlChildrenNode;
            while (mr_cur != NULL) {
                if(!xmlStrcmp(mr_cur->name, (const xmlChar *)"MemoryRegion")) {
                    xmlChar * mrtype = NULL;
                    int is_cache = 0;
                    u64 mr_size;
                    int res;
                    char *mr_name = NULL;

                    this->machineMemoryRegions[mr_index] = (MemoryRegion*)checked_malloc(this->machineMemoryRegions[mr_index], sizeof(MemoryRegion));
                    mrtype = xmlGetProp(mr_cur,(const xmlChar *)"mrtype");
                    if(mrtype && !xmlStrcmp(mrtype,(const xmlChar *)"cache")) {
                        is_cache = 1;
                    } 

                    mr_name = strdup((const char *)xmlGetProp(mr_cur,(const xmlChar *)"id"));

                    res = get_mr_size(this, doc, &mr_size, mr_cur, "MRDescriptor");
                    if(!res) {
                        printf("Did not find size for memory region %s.\n",mr_name);
                        xmlFreeDoc(doc);
                        return 7;
                    }

                    MemoryRegion_ctor(this->machineMemoryRegions[mr_index],
                                      mr_name,
                                      mr_index,
                                      mr_size,
                                      is_cache);
                    ++mr_index;
                }
                mr_cur = mr_cur->next;
            }
        } else if(!xmlStrcmp(cur->name, (const xmlChar *)"MemoryRegion")) {
            xmlChar * mrtype = NULL;
            int is_cache = 0;
            u64 mr_size;
            int res;
            char *mr_name = NULL;

            this->machineMemoryRegions[mr_index] = (MemoryRegion*)checked_malloc(this->machineMemoryRegions[mr_index], sizeof(MemoryRegion));
            mrtype = xmlGetProp(cur,(const xmlChar *)"mrtype");
            if(mrtype && !xmlStrcmp(mrtype,(const xmlChar *)"cache")) {
                is_cache = 1;
            } 

            mr_name = strdup((const char *)xmlGetProp(cur,(const xmlChar *)"id"));

            res = get_mr_size(this, doc, &mr_size, cur, "MRDescriptor");
            if(!res) {
                printf("Did not find size for memory region %s.\n",mr_name);
                xmlFreeDoc(doc);
                return 7;
            }

            MemoryRegion_ctor(this->machineMemoryRegions[mr_index],
                              mr_name,
                              mr_index,
                              mr_size,
                              is_cache);
            ++mr_index;
        }
        cur = cur->next;
    }

#if 0
    printf("Processors\n");
    for(i = 0; i < num_masters; ++i) {
        printf("%d\n",this->machineProcessors[i]->id);
    }
    printf("Memory Regions\n");
    for(i = 0; i < num_mr; ++i) {
        printf("%s\n",this->machineMemoryRegions[i]->name);
    }
#endif

    cur = start;

    while (cur != NULL) {
        //printf("%s\n",cur->name);
        if(!xmlStrcmp(cur->name, (const xmlChar *)"Master")) {
            Processor *this_proc;

            this_proc = getProcessorFromId(this, atoi((const char *)xmlGetProp(cur,(const xmlChar *)"id")));
            if(this_proc == NULL) {
                printf("Processor not found from ID.\n");
                xmlFreeDoc(doc);
                return 5;
            }

            xmlNodePtr mr_cur;
            mr_cur = cur->xmlChildrenNode;
            while (mr_cur != NULL) {
                if(!xmlStrcmp(mr_cur->name, (const xmlChar *)"MemoryRegion")) {
                    MemoryRegion *this_mr;
                    this_mr = getMemoryRegionFromName(this, (const char *)xmlGetProp(mr_cur,(const xmlChar *)"id"));
                    if(this_mr == NULL) {
                        printf("MemoryRegion not found from name.\n");
                        xmlFreeDoc(doc);
                        return 6;
                    }
                    double mr_access_time;
                    int res = get_access_time(this, doc, &mr_access_time, mr_cur, "MRDescriptor");
                    if(!res) {
                        printf("Did not find access time for memory region.\n");
                        xmlFreeDoc(doc);
                        return 7;
                    }
                    CostDescriptor_setTimeCost(
                        &(this->costMatrix[this_proc->procIndex][this_mr->mrIndex]),
                        mr_access_time);
                }
                mr_cur = mr_cur->next;
            }
        } else if(!xmlStrcmp(cur->name, (const xmlChar *)"MemoryRegion")) {
            char * affinity, *affinity_save=NULL;

            MemoryRegion *this_mr;
            this_mr = getMemoryRegionFromName(this, (const char *)xmlGetProp(cur,(const xmlChar *)"id"));
            if(this_mr == NULL) {
                printf("MemoryRegion not found from name.\n");
                xmlFreeDoc(doc);
                return 6;
            }
            double mr_access_time;
            int res = get_access_time(this, doc, &mr_access_time, cur, "MRDescriptor");
            if(!res) {
                printf("Did not find access time for memory region.\n");
                xmlFreeDoc(doc);
                return 7;
            }

            affinity = (char *)xmlGetProp(cur, (const xmlChar *)"affinity");
            if(affinity) {
                char *token;

                token = strtok_r(affinity, " ", &affinity_save);
                while(token != NULL) {
                    unsigned to_proc_id;
                    Processor *this_proc;

                    to_proc_id = atoi((const char *)token);
                    this_proc = getProcessorFromId(this, to_proc_id);
                    if(this_proc == NULL) {
                        printf("Processor not found from id.\n");
                        xmlFreeDoc(doc);
                        return 6;
                    }
                    CostDescriptor_setTimeCost(
                        &(this->costMatrix[this_proc->procIndex][this_mr->mrIndex]),
                        mr_access_time);

                    token = strtok_r(NULL, " ", &affinity_save);
                }
            }
        } else if(!xmlStrcmp(cur->name, (const xmlChar *)"Interconnect")) {
            xmlNodePtr icdesc;
            char *from_opt = (char *)xmlGetProp(cur, (const xmlChar *)"from");
            char *to_opt   = (char *)xmlGetProp(cur, (const xmlChar *)"to");
            char *id_opt   = (char *)xmlGetProp(cur, (const xmlChar *)"mr_id");
            char *from_and_to;
            char *proc_token, *proc_token_save=NULL;

            if(from_opt == NULL) {
                printf("from attribute not found for Interconnect.\n");
                xmlFreeDoc(doc);
                return 8;
            }
            if(to_opt == NULL) {
                printf("to attribute not found for Interconnect.\n");
                xmlFreeDoc(doc);
                return 9;
            }
            if(id_opt == NULL) {
                printf("id attribute not found for Interconnect.\n");
                xmlFreeDoc(doc);
                return 10;
            }

            icdesc = getNodeByName(cur->xmlChildrenNode, "ICDescriptor");
            if(icdesc == NULL) {
                printf("ICDescriptor not found for Interconnect.\n");
                xmlFreeDoc(doc);
                return 11;
            }

            double mr_access_time;
            int res = get_access_time(this, doc, &mr_access_time, cur, "ICDescriptor");
            if(!res) {
                printf("Did not find access time for memory region.\n");
                xmlFreeDoc(doc);
                return 7;
            }

            from_and_to = (char*)checked_malloc(from_and_to, strlen(from_opt) + strlen(to_opt) + 2);
            strcpy(from_and_to,from_opt);
            strcat(from_and_to," ");
            strcat(from_and_to,to_opt);

            proc_token = strtok_r(from_and_to, " ", &proc_token_save);
            while(proc_token) {
                unsigned to_proc_id = atoi((const char *)proc_token);
                Processor *this_proc;
                char *mr_list, *mr_token, *mr_token_save=NULL;

                this_proc = getProcessorFromId(this, to_proc_id);
                if(this_proc == NULL) {
                    printf("Processor not found from ID.\n");
                    xmlFreeDoc(doc);
                    return 5;
                }

                mr_list = strdup(id_opt);

                mr_token = strtok_r(mr_list, " ", &mr_token_save);
                while(mr_token) {
                    MemoryRegion *this_mr;
                    this_mr = getMemoryRegionFromName(this, (const char *)mr_token);
                    if(this_mr == NULL) {
                        printf("MemoryRegion not found from name.\n");
                        xmlFreeDoc(doc);
                        return 6;
                    }
                    CostDescriptor_setTimeCost(
                        &(this->costMatrix[this_proc->procIndex][this_mr->mrIndex]),
                        mr_access_time);

                    mr_token = strtok_r(NULL, " ", &mr_token_save);
                }

                free(mr_list);

                proc_token = strtok_r(NULL, " ", &proc_token_save);
            }
        }

        cur = cur->next;
    }


    unsigned unknown_costs = 0;
    for(i = 0; i < MachineDescription_getNumProcessors(this); ++i) {
        for(j = 0; j < MachineDescription_getNumMemoryRegions(this); ++j) {
            if(!this->costMatrix[i][j].valid) {
#if 0
                printf("UnknownCost: Processor=%d, MemoryRegion=%s\n",
                    MachineDescription_getProcessor(this, i)->id,
                    MachineDescription_getMemoryRegion(this, j)->name);
#endif
                ++unknown_costs;
            }
        }
    }
    if(unknown_costs) {
        printf("Number of unknown costs is %d out of %d.\n",
            unknown_costs,
            MachineDescription_getNumProcessors(this) *
            MachineDescription_getNumMemoryRegions(this));
    }

    xmlFreeDoc(doc);
    return 0;
}

/**
 *    @brief Get information about one of the processors.
 *     **/
Processor * MachineDescription_getProcessor(MachineDescription *this, u32 processor_index) {
    return this->machineProcessors[processor_index];
}

/**
 *    @brief Get information about one of the memory regions.
 *     **/
MemoryRegion * MachineDescription_getMemoryRegion(MachineDescription *this, u32 memory_region_index) {
    return this->machineMemoryRegions[memory_region_index];
}

/**
 *    @brief Get the full M x N matrix of costs for each of the M processors to
 *              access each of the N memory regions.
 *               **/
CostDescriptor ** MachineDescription_getFullCostMatrix(MachineDescription *this) {
    return this->costMatrix;
}

void setMachineDescriptionFromPDL(const char *pdlFilename) {
    if(g_MachineDescription) {
        printf("Attempt to load the machine description twice.\n");
        exit(-1);
    } else {
        g_MachineDescription = (MachineDescription*)checked_malloc(g_MachineDescription, sizeof(MachineDescription));
        int ret = MachineDescription_loadFromPDL(g_MachineDescription,pdlFilename);
        if (ret != 0) {
            free(g_MachineDescription);
            g_MachineDescription = NULL;
        }
    }
}

MachineDescription * getMachineDescription() {
    return g_MachineDescription;
}

CostDescriptor MachineDescription_getProcessorMemoryCost(
               MachineDescription *this,
               u32 processor_index,
               u32 memory_region_index) {
    return this->costMatrix[processor_index][memory_region_index];
}

u32 * MachineDescription_whoElseShares(
               MachineDescription *this,
               u32 processor_index,
               u32 memory_region_index,
               u32 *num_results) {
    u32 *res=NULL;
    CostDescriptor curCD, thisCD;
    unsigned i;

    *num_results = 0;

    curCD = MachineDescription_getProcessorMemoryCost(this, processor_index, memory_region_index);

    for(i = 0; i < MachineDescription_getNumProcessors(this); ++i) {
        // skip the current processor
        if (i == processor_index) continue;

        thisCD = MachineDescription_getProcessorMemoryCost(this, i, memory_region_index);
        if( CostDescriptor_equal(&thisCD,&curCD )) {
            (*num_results)++;
        }
    }

    if(*num_results) {
        res = (u32 *)checked_malloc(res, *num_results * sizeof(u32));
        *num_results = 0;

        for(i = 0; i < MachineDescription_getNumProcessors(this); ++i) {
            // skip the current processor
            if (i == processor_index) continue;

            thisCD = MachineDescription_getProcessorMemoryCost(this, i, memory_region_index);
            if( CostDescriptor_equal(&thisCD,&curCD )) {
                res[*num_results] = i;
                (*num_results)++;
            }
        }
    }

    return res;
}

u32 * MachineDescription_closestProcessorsToMemory(
      MachineDescription *this,
      u32 memory_region_index,
      u32 *num_results) {

    u32 *res = NULL;
    CostDescriptor minCD = MachineDescription_getProcessorMemoryCost(this, 0, memory_region_index);
    *num_results = 1;

    unsigned i;
    for(i = 1; i < MachineDescription_getNumProcessors(this); ++i) {
        CostDescriptor curCD = MachineDescription_getProcessorMemoryCost(this, i, memory_region_index);

        if(CostDescriptor_less(&curCD,&minCD)) {
            minCD = curCD;
            *num_results = 1;
        } else if (CostDescriptor_equal(&curCD,&minCD)) {
            (*num_results)++;
        }
    }

    if ( !minCD.valid ) {
        *num_results = 0;
        return NULL;
    } else {
        res = (u32*)checked_malloc(res, *num_results * sizeof(u32));
        *num_results = 0;

        for(i = 0; i < MachineDescription_getNumProcessors(this); ++i) {
            CostDescriptor curCD = MachineDescription_getProcessorMemoryCost(this, i, memory_region_index);

            if(CostDescriptor_equal(&curCD,&minCD)) {
                res[*num_results] = i;
                (*num_results)++;
            }
        }
        return res;
    }
}

u32 * MachineDescription_closestMemoryRegionsToProcessor(
      MachineDescription *this,
      u32 processor_index,
      u32 *num_results) {

    u32 *res = NULL;
    CostDescriptor minCD = MachineDescription_getProcessorMemoryCost(this, processor_index, 0);
    *num_results = 1;

    unsigned i;
    for(i = 1; i < MachineDescription_getNumMemoryRegions(this); ++i) {
        CostDescriptor curCD = MachineDescription_getProcessorMemoryCost(this, processor_index, i);

        if(CostDescriptor_less(&curCD,&minCD)) {
            minCD = curCD;
            *num_results = 1;
        } else if (CostDescriptor_equal(&curCD,&minCD)) {
            (*num_results)++;
        }
    }

    if ( !minCD.valid ) {
        *num_results = 0;
        return NULL;
    } else {
        res = (u32*)checked_malloc(res, *num_results * sizeof(u32));
        *num_results = 0;

        for(i = 0; i < MachineDescription_getNumMemoryRegions(this); ++i) {
            CostDescriptor curCD = MachineDescription_getProcessorMemoryCost(this, processor_index, i);

            if(CostDescriptor_equal(&curCD,&minCD)) {
                res[*num_results] = i;
                (*num_results)++;
            }
        }
        return res;
    }
}
