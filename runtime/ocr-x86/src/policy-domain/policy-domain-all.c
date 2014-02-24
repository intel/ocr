/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "policy-domain/policy-domain-all.h"
#include "debug.h"

#define DEBUG_TYPE POLICY

// Everything in the marshalling code will be
// aligned with this alignment. This has to be the LCM of
// all required alignments. 8 is usually a safe value
// TODO: It needs to be consistent across PDs
// TODO: Do we make this configurable in ocr-config.h?
// NOTE: MUST BE POWER OF 2
#define MAX_ALIGN 8

const char * policyDomain_types [] = {
    "HC",
    "HCDist",
    "XE",
    "CE",
    NULL
};

void initializePolicyMessage(ocrPolicyMsg_t* msg, u64 bufferSize) {
    msg->bufferSize = bufferSize;
    //TODO-225: Shouldn't be mandatory
    // ASSERT(bufferSize >= sizeof(ocrPolicyMsg_t));
    msg->usefulSize = 0;
}

ocrPolicyDomainFactory_t * newPolicyDomainFactory(policyDomainType_t type, ocrParamList_t *perType) {
    switch(type) {
#ifdef ENABLE_POLICY_DOMAIN_HC
    case policyDomainHc_id:
        return newPolicyDomainFactoryHc(perType);
#endif
#ifdef ENABLE_POLICY_DOMAIN_HC_DIST
    case policyDomainHcDist_id:
        return newPolicyDomainFactoryHcDist(perType);
#endif
#ifdef ENABLE_POLICY_DOMAIN_XE
    case policyDomainXe_id:
        return newPolicyDomainFactoryXe(perType);
#endif
#ifdef ENABLE_POLICY_DOMAIN_CE
    case policyDomainCe_id:
        return newPolicyDomainFactoryCe(perType);
#endif
    default:
        ASSERT(0);
    }
    return NULL;
}

void initializePolicyDomainOcr(ocrPolicyDomainFactory_t * factory, ocrPolicyDomain_t * self, ocrParamList_t *perInstance) {
    self->fcts = factory->policyDomainFcts;

    self->myLocation = ((paramListPolicyDomainInst_t*)perInstance)->location;

    self->schedulers = NULL;
    self->allocators = NULL;
    self->commApis = NULL;
    self->shutdownCode = 0;
}

u64 ocrPolicyMsgGetMsgBaseSize(ocrPolicyMsg_t *msg, bool isIn) {
    u64 baseSize = 0;
#define PD_MSG msg
    // To make it clean and easy to extend, it
    // requires two switch statements
    switch(msg->type & PD_MSG_TYPE_ONLY) {
#define PER_TYPE(type)                                  \
    case type:                                          \
        if(isIn) {                                      \
            baseSize = _PD_MSG_SIZE_IN(type);                          \
        } else {                                        \
            baseSize = _PD_MSG_SIZE_OUT(type);         \
        }                                               \
        break;
#include "ocr-policy-msg-list.h"
#undef PER_TYPE
    default:
#ifdef ENABLE_POLICY_DOMAIN_CE
        // Special case for CEs
        msg->type = PD_MSG_REQUEST | PD_CE_CE_MESSAGE | PD_MSG_MGT_SHUTDOWN;
        baseSize = _PD_MSG_SIZE_IN(PD_MSG_MGT_SHUTDOWN);
        // This can typically happen during shutdown when comms is closed, to be fixed in #134
        return 0;
#else
        DPRINTF(DEBUG_LVL_WARN, "Error: Message type 0x%x not handled in getMsgSize\n", msg->type & PD_MSG_TYPE_ONLY);
        ASSERT(false);
#endif
    }
    // Align baseSize
    baseSize = (baseSize + MAX_ALIGN - 1)&(~(MAX_ALIGN-1));
#undef PD_MSG
    return baseSize;
}

u8 ocrPolicyMsgGetMsgSize(ocrPolicyMsg_t *msg, u64 *baseSize,
                          u64 *marshalledSize, u32 mode) {
    *baseSize = 0;
    *marshalledSize = 0;
    u8 flags = mode & MARSHALL_FLAGS;
    ASSERT(((msg->type & (PD_MSG_REQUEST | PD_MSG_RESPONSE)) != (PD_MSG_REQUEST | PD_MSG_RESPONSE)) &&
           ((msg->type & PD_MSG_REQUEST) || (msg->type & PD_MSG_RESPONSE)));

    u8 isIn = (msg->type & PD_MSG_REQUEST) != 0ULL;

    *baseSize = ocrPolicyMsgGetMsgBaseSize(msg, isIn);

#define PD_MSG msg
    switch(msg->type & PD_MSG_TYPE_ONLY) {
    case PD_MSG_WORK_CREATE:
#define PD_TYPE PD_MSG_WORK_CREATE
        if(isIn) {
            ASSERT(MAX_ALIGN % sizeof(u64) == 0);
            *marshalledSize = PD_MSG_FIELD_I(paramv)?sizeof(u64)*PD_MSG_FIELD_IO(paramc):0ULL;
        }
        break;
#undef PD_TYPE

    case PD_MSG_EDTTEMP_CREATE:
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
#ifdef OCR_ENABLE_EDT_NAMING
        if(isIn) {
            *marshalledSize = PD_MSG_FIELD_I(funcNameLen)*sizeof(char);
            if(*marshalledSize) {
                *marshalledSize += 1*sizeof(char); // Null terminating character
            }
        }
#endif
        break;
#undef PD_TYPE

    case PD_MSG_COMM_TAKE:
#define PD_TYPE PD_MSG_COMM_TAKE
        if(isIn) {
            if(PD_MSG_FIELD_IO(guids) != NULL && PD_MSG_FIELD_IO(guids[0].guid) != NULL_GUID) {
                // Specific GUIDs have been requested so we need
                // to marshall those. Otherwise, we do not need to marshall
                // anything
                *marshalledSize = sizeof(ocrFatGuid_t)*PD_MSG_FIELD_IO(guidCount);
            }
        } else {
            *marshalledSize = sizeof(ocrFatGuid_t)*PD_MSG_FIELD_IO(guidCount);
        }
        break;
#undef PD_TYPE

    case PD_MSG_COMM_GIVE:
#define PD_TYPE PD_MSG_COMM_GIVE
        *marshalledSize = sizeof(ocrFatGuid_t)*PD_MSG_FIELD_IO(guidCount);
        break;
#undef PD_TYPE

    case PD_MSG_GUID_METADATA_CLONE:
#define PD_TYPE PD_MSG_GUID_METADATA_CLONE
        if(!isIn) {
            *marshalledSize = PD_MSG_FIELD_O(size);
            // Make sure that if we have a size, we have a PTR
            //TODO commented because it bombs when I try to get the size of an out which
            //is an in. (need a real api parameter)
            //ASSERT((*marshalledSize == 0) || (PD_MSG_FIELD_IO(guid.metaDataPtr) != NULL));
        }
        break;
#undef PD_TYPE

    case PD_MSG_DB_ACQUIRE:
        if((flags & MARSHALL_DBPTR) && (!isIn)) {
#define PD_TYPE PD_MSG_DB_ACQUIRE
            *marshalledSize = PD_MSG_FIELD_O(size);
#undef PD_TYPE
        }
        break;

    case PD_MSG_DB_RELEASE:
        if((flags & MARSHALL_DBPTR) && (isIn)) {
#define PD_TYPE PD_MSG_DB_RELEASE
            *marshalledSize = PD_MSG_FIELD_I(size);
#undef PD_TYPE
        }
        break;

    default:
        // Nothing to do
        ;
    }

    // Align marshalled size
    // This is not stricly required as what matters is really baseSize
    *marshalledSize = (*marshalledSize + MAX_ALIGN - 1)&(~(MAX_ALIGN-1));

    DPRINTF(DEBUG_LVL_VVERB, "Msg 0x%lx of type 0x%x: baseSize %ld; addlSize %ld\n",
            msg, msg->type & PD_MSG_TYPE_ONLY, *baseSize, *marshalledSize);
#undef PD_MSG
    return 0;
}

u8 ocrPolicyMsgMarshallMsg(ocrPolicyMsg_t* msg, u64 baseSize, u8* buffer, u32 mode) {

    u8* startPtr = NULL;
    u8* curPtr = NULL;
    ocrPolicyMsg_t* outputMsg = NULL;
    u8 flags = mode & MARSHALL_FLAGS;
    mode &= MARSHALL_TYPE;
    u8 isAddl = (mode == MARSHALL_ADDL);
    u8 fixupPtrs = (mode != MARSHALL_DUPLICATE);
    u8 isIn = (msg->type & PD_MSG_REQUEST) != 0ULL;

    if(baseSize % MAX_ALIGN != 0) {
        u64 t = baseSize;
        baseSize = (baseSize + MAX_ALIGN -1)&(~(MAX_ALIGN-1));
        DPRINTF(DEBUG_LVL_WARN, "Adjusted base size in ocrPolicyMsgMarshallMsg to be %d aligned (from %u to %u)\n",
                MAX_ALIGN, t, baseSize);
    }

    ASSERT(((msg->type & (PD_MSG_REQUEST | PD_MSG_RESPONSE)) != (PD_MSG_REQUEST | PD_MSG_RESPONSE)) &&
           ((msg->type & PD_MSG_REQUEST) || (msg->type & PD_MSG_RESPONSE)));

    // The usefulSize is set by the marshalling code so unless the message
    // has been already marshalled, it is zero.
    // Hence, first thing is to set the msg's usefulSize field.
    if(msg->usefulSize == 0) {
        ASSERT((((u8*)msg) == buffer) ? (baseSize <= msg->bufferSize) : true);
        msg->usefulSize = baseSize;
    }

    switch(mode) {
    case MARSHALL_FULL_COPY:
    case MARSHALL_DUPLICATE:
    {
        ocrPolicyMsg_t * bufferMsg = (ocrPolicyMsg_t*) buffer;
        u32 bufBSize = bufferMsg->bufferSize;
        u32 bufUSize = bufferMsg->usefulSize;
        // Copy msg into the buffer for the common part
        hal_memCopy(buffer, msg, baseSize, false);
        bufferMsg->bufferSize = bufBSize;
        bufferMsg->usefulSize = bufUSize;
        startPtr = buffer;
        curPtr = buffer + baseSize;
        ASSERT(((ocrPolicyMsg_t*)buffer)->bufferSize >= baseSize);
        outputMsg = (ocrPolicyMsg_t*)buffer;
        break;
    }
    case MARSHALL_APPEND:
        ASSERT((u64)buffer == (u64)msg);
        startPtr = (u8*)(msg);
        curPtr = startPtr + baseSize;
        ASSERT(msg->bufferSize >= baseSize); // Make sure the message is not of zero size
        outputMsg = msg;
        break;
    case MARSHALL_ADDL:
        startPtr = buffer;
        curPtr = buffer;
        outputMsg = msg;
        break;
    default:
        ASSERT(0);
    }

    DPRINTF(DEBUG_LVL_VERB, "Got message 0x%lx to marshall into 0x%lx mode %d: "
            "startPtr: 0x%lx, curPtr: 0x%lx, outputMsg: 0x%lx\n",
            (u64)msg, (u64)buffer, mode, (u64)startPtr, (u64)curPtr, (u64)outputMsg);

    // At this point, we can replace all "pointers" inside
    // outputMsg with stuff that we put @ curPtr

#define PD_MSG outputMsg
    switch(outputMsg->type & PD_MSG_TYPE_ONLY) {
    case PD_MSG_WORK_CREATE: {
#define PD_TYPE PD_MSG_WORK_CREATE
        // Implementation: So far, we do not pass depv directly as part of this message
        // If we do, we need to implement marshalling for that as well
        ASSERT(PD_MSG_FIELD_I(depv) == NULL);
        if(isIn) {
            // Catch misues for paramc
            ASSERT(((PD_MSG_FIELD_IO(paramc) != 0) && (PD_MSG_FIELD_I(paramv) != NULL))
                   || ((PD_MSG_FIELD_IO(paramc) == 0) && (PD_MSG_FIELD_I(paramv) == NULL)));

            // First copy things over
            u64 s = sizeof(u64)*PD_MSG_FIELD_IO(paramc);
            if(s) {
                hal_memCopy(curPtr, PD_MSG_FIELD_I(paramv), s, false);

                // Now fixup the pointer
                if(fixupPtrs) {
                    DPRINTF(DEBUG_LVL_VVERB, "Converting paramv (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD_I(paramv), ((u64)(curPtr - startPtr)<<1) + isAddl);
                    PD_MSG_FIELD_I(paramv) = (void*)((((u64)(curPtr - startPtr))<<1) + isAddl);
                } else {
                    DPRINTF(DEBUG_LVL_VVERB, "Copying paramv (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD_I(paramv), curPtr);
                    PD_MSG_FIELD_I(paramv) = (void*)curPtr;
                }
                // Finally move the curPtr for the next object (none as of now)
                curPtr += s;
            } else {
                PD_MSG_FIELD_I(paramv) = NULL;
            }
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_EDTTEMP_CREATE: {
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
#ifdef OCR_ENABLE_EDT_NAMING
        if(isIn) {
            // First copy things over
            u64 s = sizeof(char)*(PD_MSG_FIELD_I(funcNameLen)+1);
            if(s) {
                hal_memCopy(curPtr, PD_MSG_FIELD_I(funcName), s, false);
                // Now fixup the pointer
                if(fixupPtrs) {
                    DPRINTF(DEBUG_LVL_VVERB, "Converting funcName (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD_I(funcName), ((u64)(curPtr - startPtr)<<1) + isAddl);
                    PD_MSG_FIELD_I(funcName) = (char*)(((u64)(curPtr - startPtr)<<1) + isAddl);
                } else {
                    DPRINTF(DEBUG_LVL_VVERB, "Copying funcName (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD_I(funcName), curPtr);
                    PD_MSG_FIELD_I(funcName) = (char*)curPtr;
                }
                // Finally move the curPtr for the next object (none as of now)
                curPtr += s;
            } else {
                PD_MSG_FIELD_I(funcName) = NULL;
            }
        }
#endif
        break;
#undef PD_TYPE
    }

    case PD_MSG_COMM_TAKE: {
#define PD_TYPE PD_MSG_COMM_TAKE
        // First copy things over
        u64 s = sizeof(ocrFatGuid_t)*PD_MSG_FIELD_IO(guidCount);
        if(isIn) {
            if(PD_MSG_FIELD_IO(guids) != NULL &&
               PD_MSG_FIELD_IO(guids[0].guid) != NULL_GUID && s != 0) {
                hal_memCopy(curPtr, PD_MSG_FIELD_IO(guids), s, false);
                // Now fixup the pointer
                if(fixupPtrs) {
                    DPRINTF(DEBUG_LVL_VVERB, "Converting guids (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD_IO(guids), ((u64)(curPtr - startPtr)<<1) + isAddl);
                    PD_MSG_FIELD_IO(guids) = (ocrFatGuid_t*)(((u64)(curPtr - startPtr)<<1) + isAddl);
                } else {
                    DPRINTF(DEBUG_LVL_VVERB, "Copying guids (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD_IO(guids), curPtr);
                    PD_MSG_FIELD_IO(guids) = (ocrFatGuid_t*)curPtr;
                }
                // Finally move the curPtr for the next object (none as of now)
                curPtr += s;
            } else {
                PD_MSG_FIELD_IO(guids) = NULL;
            }
        } else {
            if(s) {
                hal_memCopy(curPtr, PD_MSG_FIELD_IO(guids), s, false);
                // Now fixup the pointer
                if(fixupPtrs) {
                    DPRINTF(DEBUG_LVL_VVERB, "Converting guids (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD_IO(guids), ((u64)(curPtr - startPtr)<<1) + isAddl);
                    PD_MSG_FIELD_IO(guids) = (ocrFatGuid_t*)(((u64)(curPtr - startPtr)<<1) + isAddl);
                } else {
                    DPRINTF(DEBUG_LVL_VVERB, "Copying guids (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD_IO(guids), curPtr);
                    PD_MSG_FIELD_IO(guids) = (ocrFatGuid_t*)curPtr;
                }
                // Finally move the curPtr for the next object (none as of now)
                curPtr += s;
            } else {
                PD_MSG_FIELD_IO(guids) = NULL;
            }
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_COMM_GIVE: {
#define PD_TYPE PD_MSG_COMM_GIVE
        // First copy things over
        u64 s = sizeof(ocrFatGuid_t)*PD_MSG_FIELD_IO(guidCount);
        if(s) {
            hal_memCopy(curPtr, PD_MSG_FIELD_IO(guids), s, false);
            // Now fixup the pointer
            if(fixupPtrs) {
                DPRINTF(DEBUG_LVL_VVERB, "Converting guids (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD_IO(guids), ((u64)(curPtr - startPtr)<<1) + isAddl);
                PD_MSG_FIELD_IO(guids) = (ocrFatGuid_t*)(((u64)(curPtr - startPtr)<<1) + isAddl);
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "Copying guids (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD_IO(guids), curPtr);
                PD_MSG_FIELD_IO(guids) = (ocrFatGuid_t*)curPtr;
            }
            // Finally move the curPtr for the next object (none as of now)
            curPtr += s;
        } else {
            PD_MSG_FIELD_IO(guids) = NULL;
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_GUID_METADATA_CLONE: {
#define PD_TYPE PD_MSG_GUID_METADATA_CLONE
        if(!isIn) {
            // First copy things over
            u64 s = PD_MSG_FIELD_O(size);
            if(s) {
                hal_memCopy(curPtr, PD_MSG_FIELD_IO(guid.metaDataPtr), s, false);
                // Now fixup the pointer
                if(fixupPtrs) {
                    DPRINTF(DEBUG_LVL_VVERB, "Converting metadata clone (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD_IO(guid.metaDataPtr), ((u64)(curPtr - startPtr)<<1) + isAddl);
                    PD_MSG_FIELD_IO(guid.metaDataPtr) = (void*)(((u64)(curPtr - startPtr)<<1) + isAddl);
                } else {
                    DPRINTF(DEBUG_LVL_VVERB, "Copying metadata clone (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD_IO(guid.metaDataPtr), curPtr);
                    PD_MSG_FIELD_IO(guid.metaDataPtr) = (void*)curPtr;
                }
                // Finally move the curPtr for the next object (none as of now)
                curPtr += s;
            } else {
                PD_MSG_FIELD_IO(guid.metaDataPtr) = NULL;
            }
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_DB_ACQUIRE: {
#define PD_TYPE PD_MSG_DB_ACQUIRE
        // Set to zero if not outgoing message because in that case
        // we don't care about marshalling. Also makes sure we only
        // read the size field if valid
        u64 s = (!isIn)?(PD_MSG_FIELD_O(size)):0ULL;
        if((flags & MARSHALL_DBPTR) && s) {
            hal_memCopy(curPtr, PD_MSG_FIELD_O(ptr), s, false);
            // Now fixup the pointer
            if(fixupPtrs) {
                DPRINTF(DEBUG_LVL_VVERB, "Converting DB acquire ptr (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD_O(ptr), ((u64)(curPtr - startPtr)<<1) + isAddl);
                PD_MSG_FIELD_O(ptr) = (void*)(((u64)(curPtr - startPtr)<<1) + isAddl);
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "Copying DB acquire ptr (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD_O(ptr), curPtr);
                PD_MSG_FIELD_O(ptr) = (void*)curPtr;
            }
            // Finally move the curPtr for the next object (none as of now)
            curPtr += s;
        }
#undef PD_TYPE
        break;
    }

    case PD_MSG_DB_RELEASE: {
#define PD_TYPE PD_MSG_DB_RELEASE
        u64 s = isIn?PD_MSG_FIELD_I(size):0ULL;
        if((flags & MARSHALL_DBPTR) && s) {
            hal_memCopy(curPtr, PD_MSG_FIELD_I(ptr), s, false);
            // Now fixup the pointer
            if(fixupPtrs) {
                DPRINTF(DEBUG_LVL_VVERB, "Converting DB release ptr (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD_I(ptr), ((u64)(curPtr - startPtr)<<1) + isAddl);
                PD_MSG_FIELD_I(ptr) = (void*)(((u64)(curPtr - startPtr)<<1) + isAddl);
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "Copying DB release ptr (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD_I(ptr), curPtr);
                PD_MSG_FIELD_I(ptr) = (void*)curPtr;
            }
            // Finally move the curPtr for the next object (none as of now)
            curPtr += s;
        }
#undef PD_TYPE
        break;
    }

    default:
        // Nothing to do
        ;
    }
#undef PD_MSG

    // Update the size of the output message if needed
    if(mode == MARSHALL_FULL_COPY || mode == MARSHALL_DUPLICATE
       || mode == MARSHALL_APPEND) {
        outputMsg->usefulSize = (u64)(curPtr) - (u64)(startPtr);
        // Align things properly to stay clean
        outputMsg->usefulSize = (outputMsg->usefulSize + MAX_ALIGN - 1) & (~(MAX_ALIGN-1));
    } else {
        outputMsg->usefulSize = baseSize;
    }

    DPRINTF(DEBUG_LVL_VVERB, "Useful size of message set to %u\n", outputMsg->usefulSize);
    return 0;
}

u8 ocrPolicyMsgUnMarshallMsg(u8* mainBuffer, u8* addlBuffer,
                             ocrPolicyMsg_t* msg, u32 mode) {
    u8* localMainPtr = (u8*)msg;
    u8* localAddlPtr = NULL;

    u64 baseSize=0, marshalledSize=0;
    u8 isIn = (((ocrPolicyMsg_t*)mainBuffer)->type & PD_MSG_REQUEST) != 0ULL;

    ASSERT(((((ocrPolicyMsg_t*)mainBuffer)->type & (PD_MSG_REQUEST | PD_MSG_RESPONSE)) != (PD_MSG_REQUEST | PD_MSG_RESPONSE)) &&
           ((((ocrPolicyMsg_t*)mainBuffer)->type & PD_MSG_REQUEST) ||
            (((ocrPolicyMsg_t*)mainBuffer)->type & PD_MSG_RESPONSE)));

    u8 flags = mode & MARSHALL_FLAGS;
    mode &= MARSHALL_TYPE;

    switch(mode) {
    case MARSHALL_FULL_COPY:
        ocrPolicyMsgGetMsgSize((ocrPolicyMsg_t*)mainBuffer, &baseSize, &marshalledSize, mode | flags);
        ASSERT(((ocrPolicyMsg_t*)mainBuffer)->usefulSize <= baseSize + marshalledSize);
        ASSERT(((ocrPolicyMsg_t*)mainBuffer)->usefulSize >= baseSize);

        DPRINTF(DEBUG_LVL_VVERB, "Unmarshall full-copy: 0x%lx -> 0x%lx of useful size %ld\n",
                (u64)mainBuffer, (u64)msg, ((ocrPolicyMsg_t*)mainBuffer)->usefulSize);
        hal_memCopy(msg, mainBuffer, ((ocrPolicyMsg_t*)mainBuffer)->usefulSize, false);

        // We set localAddlPtr to whatever may have been marshalled in mainBuffer
        localAddlPtr = (u8*)msg + baseSize;
        break;

    case MARSHALL_APPEND:
        // Same as above except that msg and mainBuffer are one and the same
        ASSERT((u64)msg == (u64)mainBuffer);
        ocrPolicyMsgGetMsgSize(msg, &baseSize, &marshalledSize, mode | flags);
        ASSERT(msg->usefulSize <= baseSize + marshalledSize);
        ASSERT(msg->usefulSize >= baseSize);
        localAddlPtr = (u8*)msg + baseSize;
        break;

    case MARSHALL_ADDL: {
        // Here, we have to copy mainBuffer into msg if needed
        // and also create a new chunk to hold additional information
        u64 origSize = ((ocrPolicyMsg_t*)mainBuffer)->usefulSize;
        ocrPolicyMsgGetMsgSize((ocrPolicyMsg_t*)mainBuffer, &baseSize, &marshalledSize, mode | flags);
        if((u64)msg != (u64)mainBuffer) {
            // Only copy the base part (this may be smaller than origSize if
            // the marshalling was done in APPEND mode)
            hal_memCopy(msg, mainBuffer, baseSize, false);
        }
        if(marshalledSize != 0) {
            // We now create a new chunk of memory locally to hold
            // what we need to un-marshall
            ocrPolicyDomain_t *pd = NULL;
            getCurrentEnv(&pd, NULL, NULL, NULL);
            localAddlPtr = (u8*)pd->fcts.pdMalloc(pd, marshalledSize);
            ASSERT(localAddlPtr);

            // Check if the marshalled information was appended to
            // the mainBuffer
            if(origSize != baseSize) {
                ASSERT(addlBuffer == NULL); // We can't have both
                hal_memCopy(localAddlPtr, mainBuffer + origSize, marshalledSize, false);
            } else {
                ASSERT(addlBuffer != NULL);
                // Will be copied later
            }
        }
        break;
    }

    default:
        ASSERT(0);
    } // End of switch

    if(addlBuffer != NULL) {
        ASSERT(localAddlPtr != NULL && marshalledSize != 0);
        hal_memCopy(localAddlPtr, addlBuffer, marshalledSize, false);
    }
    DPRINTF(DEBUG_LVL_VERB, "Unmarshalling message with mainAddr 0x%lx and addlAddr 0x%lx\n",
            (u64)localMainPtr, (u64)localAddlPtr);

    // At this point, we go over the pointers that we care about and
    // fix them up
#define PD_MSG msg
    switch(msg->type & PD_MSG_TYPE_ONLY) {
    case PD_MSG_WORK_CREATE: {
#define PD_TYPE PD_MSG_WORK_CREATE
        if(isIn) {
            if(PD_MSG_FIELD_IO(paramc) > 0) {
                u64 t = (u64)(PD_MSG_FIELD_I(paramv));
                PD_MSG_FIELD_I(paramv) = (void*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
                DPRINTF(DEBUG_LVL_VVERB, "Converted field paramv from 0x%lx to 0x%lx\n",
                        t, (u64)PD_MSG_FIELD_I(paramv));
            }
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_EDTTEMP_CREATE: {
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
#ifdef OCR_ENABLE_EDT_NAMING
        if(isIn) {
            if(PD_MSG_FIELD_I(funcNameLen) > 0) {
                u64 t = (u64)(PD_MSG_FIELD_I(funcName));
                PD_MSG_FIELD_I(funcName) = (char*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
                DPRINTF(DEBUG_LVL_VVERB, "Converted field funcName from 0x%lx to 0x%lx\n",
                        t, (u64)PD_MSG_FIELD_I(funcName));
            }
        }
#endif
        break;
#undef PD_TYPE
    }

    case PD_MSG_COMM_TAKE: {
#define PD_TYPE PD_MSG_COMM_TAKE
        if(isIn) {
            if(PD_MSG_FIELD_IO(guidCount) > 0) {
                u64 t = (u64)(PD_MSG_FIELD_IO(guids));
                if(t) {
                    PD_MSG_FIELD_IO(guids) = (ocrFatGuid_t*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
                    DPRINTF(DEBUG_LVL_VVERB, "Converted field guids from 0x%lx to 0x%lx\n",
                            t, (u64)PD_MSG_FIELD_IO(guids));
                }
            }
        } else {
            if(PD_MSG_FIELD_IO(guidCount) > 0) {
                u64 t = (u64)(PD_MSG_FIELD_IO(guids));
                ASSERT(t);
                PD_MSG_FIELD_IO(guids) = (ocrFatGuid_t*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
                DPRINTF(DEBUG_LVL_VVERB, "Converted field guids from 0x%lx to 0x%lx\n",
                        t, (u64)PD_MSG_FIELD_IO(guids));
            }
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_COMM_GIVE: {
#define PD_TYPE PD_MSG_COMM_GIVE
        if(PD_MSG_FIELD_IO(guidCount) > 0) {
            u64 t = (u64)(PD_MSG_FIELD_IO(guids));
            PD_MSG_FIELD_IO(guids) = (ocrFatGuid_t*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
            DPRINTF(DEBUG_LVL_VVERB, "Converted field guids from 0x%lx to 0x%lx\n",
                    t, (u64)PD_MSG_FIELD_IO(guids));
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_GUID_METADATA_CLONE: {
#define PD_TYPE PD_MSG_GUID_METADATA_CLONE
        if(!isIn) {
            ASSERT(PD_MSG_FIELD_O(size) > 0);
            u64 t = (u64)(PD_MSG_FIELD_IO(guid.metaDataPtr));
            PD_MSG_FIELD_IO(guid.metaDataPtr) = (void*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
            DPRINTF(DEBUG_LVL_VVERB, "Converted metadata ptr from 0x%lx to 0x%lx\n",
                    t, (u64)PD_MSG_FIELD_IO(guid.metaDataPtr));
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_DB_ACQUIRE: {
        if((flags & MARSHALL_DBPTR) && (!isIn)) {
#define PD_TYPE PD_MSG_DB_ACQUIRE
            ASSERT(PD_MSG_FIELD_O(size) > 0);
            u64 t = (u64)(PD_MSG_FIELD_O(ptr));
            PD_MSG_FIELD_O(ptr) = (void*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
            DPRINTF(DEBUG_LVL_VVERB, "Converted DB acquire ptr from 0x%lx to 0x%lx\n",
                    t, (u64)PD_MSG_FIELD_O(ptr));
#undef PD_TYPE
        }
        break;
    }

    case PD_MSG_DB_RELEASE: {
#define PD_TYPE PD_MSG_DB_RELEASE
        // The size can be zero if the release didn't require a writeback
        if((flags & MARSHALL_DBPTR) && (isIn) && (PD_MSG_FIELD_I(size) > 0)) {
            u64 t = (u64)(PD_MSG_FIELD_I(ptr));
            PD_MSG_FIELD_I(ptr) = (void*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
            DPRINTF(DEBUG_LVL_VVERB, "Converted DB release ptr from 0x%lx to 0x%lx\n",
                    t, (u64)PD_MSG_FIELD_I(ptr));
        }
        break;
#undef PD_TYPE
    }

    default:
        // Nothing to do
        ;
    }
#undef PD_MSG

    // Invalidate all ocrFatGuid_t pointers
    //TODO-225: There's the issue of understanding when a 'foreign' metadataPtr should be
    // nullify and when it was actually marshalled as part of the payload.

    // We only invalidate if crossing address-spaces
    // Some messages treat I/O GUIDs a little differently:
    //  - PD_MSG_GUID_METADATA_CLONE uses the metaDataPtr as a pointer to inside the message
    //    (it is marshalled)
    //  - PD_MSG_GUID_CREATE can have a non null metaDataPtr on input when requesting a GUID
    //    for a known metadata
    // Do the in/out ones
    if(flags & MARSHALL_NSADDR) {
        if ((msg->type & PD_MSG_TYPE_ONLY) != PD_MSG_GUID_METADATA_CLONE) {
            u32 count = PD_MSG_FG_IO_COUNT_ONLY_GET(msg->type);

            // Nullify foreign GUID's metadataPtr, resolve local ones that are NULL
            DPRINTF(DEBUG_LVL_VVERB, "Invalidating %d ocrFatGuid_t I/O pointers for 0x%lx starting at 0x%lx\n",
                    count, msg, (&(msg->args)));
            // Process InOut GUIDS
            ocrFatGuid_t *guids = (ocrFatGuid_t*)(&(msg->args));
            ocrPolicyDomain_t *pd = NULL;
            getCurrentEnv(&pd, NULL, NULL, NULL);
            while(count) {
                if (guids->guid != NULL_GUID) {
                    // Determine if GUID is local
                    //TODO-225: what we should really do is compare the PD location and the guid's one
                    // but the overhead going through the current api sounds unreasonnable.
                    // For now rely on the provider not knowing the GUID. Note there's a potential race
                    // here with code in hc-dist-policy that may be setting
                    u64 val;
                    pd->guidProviders[0]->fcts.getVal(pd->guidProviders[0], guids->guid, &val, NULL);
                    if (val == 0) {
                        guids->metaDataPtr = NULL;
                    } else {
                        // else we know the GUID
                        // local GUID, check if it is known
                        guids->metaDataPtr = (void *) val;
                    }
                } else {
                    if((msg->type & PD_MSG_REQUEST) &&
                       ((msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_GUID_CREATE)) {
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_CREATE
                        if(PD_MSG_FIELD_I(size) != 0) {
                            ASSERT(guids->metaDataPtr == NULL);
                        }
#undef PD_MSG
#undef PD_TYPE
                    } else {
                        ASSERT(guids->metaDataPtr == NULL);
                    }
                }
                ++guids;
                --count;
            }

            // Now do the in or out
            if(isIn) {
                count = PD_MSG_FG_I_COUNT_ONLY_GET(msg->type);
                DPRINTF(DEBUG_LVL_VVERB, "Invalidating %d ocrFatGuid_t I pointers\n", count);
            } else {
                count = PD_MSG_FG_O_COUNT_ONLY_GET(msg->type);
                DPRINTF(DEBUG_LVL_VVERB, "Invalidating %d ocrFatGuid_t O pointers\n", count);
            }
            // TODO: I really don't like this...
            switch(msg->type & PD_MSG_TYPE_ONLY) {
#define PER_TYPE(type)                                                  \
                case type:                                              \
                    guids = (ocrFatGuid_t*)(&(_PD_MSG_INOUT_STRUCT(msg, type))); \
                    break;
#include "ocr-policy-msg-list.h"
#undef PER_TYPE
            default:
                ASSERT(0);
            }

            while(count) {
                if (guids->guid != NULL_GUID) {
                    // Determine if GUID is local
                    //TODO-225: what we should really do is compare the PD location and the guid's one
                    // but the overhead going through the current api sounds unreasonnable.
                    // For now rely on the provider not knowing the GUID. Note there's a potential race
                    // here with code in hc-dist-policy that may be setting
                    u64 val;
                    pd->guidProviders[0]->fcts.getVal(pd->guidProviders[0], guids->guid, &val, NULL);
                    if (val == 0) {
                        guids->metaDataPtr = NULL;
                    } else {
                        // else we know the GUID
                        // local GUID, check if it is known
                        guids->metaDataPtr = (void *) val;
                    }
                } else {
                    ASSERT(guids->metaDataPtr == NULL);
                }
                ++guids;
                --count;
            }
        } // end metaDataPtr setup
    } // End MARSHALL_NSADDR

    // Set the size properly
    if(mode == MARSHALL_APPEND || mode == MARSHALL_FULL_COPY) {
        msg->usefulSize = baseSize + marshalledSize;
    } else {
        msg->usefulSize = baseSize;
    }
    DPRINTF(DEBUG_LVL_VVERB, "Done unmarshalling and have size of message %ld\n", msg->usefulSize);
    return 0;
}

