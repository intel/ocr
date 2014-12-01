/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "policy-domain/policy-domain-all.h"
#include "debug.h"

#define DEBUG_TYPE POLICY

const char * policyDomain_types [] = {
    "HC",
    "HCDist",
    "XE",
    "CE",
    NULL
};

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

u8 ocrPolicyMsgGetMsgHeaderSize(ocrPolicyMsg_t *msg, u32 *headerSize) {
    *headerSize = 0;
#define PD_MSG msg
    // To make it clean and easy to extend, it
    // requires two switch statements
    switch(msg->type & PD_MSG_TYPE_ONLY) {
#define PER_TYPE(type)                          \
        case type:                              \
            *headerSize = PD_MSG_SIZE_FULL(type); \
            break;
#include "ocr-policy-msg-list.h"
#undef PER_TYPE
    default:
        DPRINTF(DEBUG_LVL_WARN, "Type 0x%x not handled in getMsgSize!!\n", msg->type & PD_MSG_TYPE_ONLY);
        ASSERT(0);
    }
    return 0;
}


u8 ocrPolicyMsgGetMsgSize(ocrPolicyMsg_t *msg, u64 *fullSize,
                          u64 *marshalledSize, u32 mode) {
    *fullSize = 0;
    *marshalledSize = 0;
    u8 flags = mode & MARSHALL_FLAGS;

#define PD_MSG msg
    // To make it clean and easy to extend, it
    // requires two switch statements
    switch(msg->type & PD_MSG_TYPE_ONLY) {
#define PER_TYPE(type)                          \
        case type:                              \
            *fullSize = PD_MSG_SIZE_FULL(type); \
            break;
#include "ocr-policy-msg-list.h"
#undef PER_TYPE
    default:
#ifdef ENABLE_POLICY_DOMAIN_CE
        // Special case for CEs
        msg->type = PD_MSG_REQUEST | PD_CE_CE_MESSAGE | PD_MSG_MGT_SHUTDOWN;
        *fullSize = PD_MSG_SIZE_FULL(PD_MSG_MGT_SHUTDOWN);
        // This can typically happen during shutdown when comms is closed, to be fixed in #134
        return 0;
#else
        DPRINTF(DEBUG_LVL_WARN, "Error: Message type 0x%x not handled in getMsgSize\n", msg->type & PD_MSG_TYPE_ONLY);
        ASSERT(false);
#endif
    }

    msg->size = *fullSize;

    switch(msg->type & PD_MSG_TYPE_ONLY) {
    case PD_MSG_WORK_CREATE:
#define PD_TYPE PD_MSG_WORK_CREATE
        *marshalledSize = PD_MSG_FIELD(paramv)?sizeof(u64)*PD_MSG_FIELD(paramc):0ULL;
        break;
#undef PD_TYPE

    case PD_MSG_EDTTEMP_CREATE:
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
#ifdef OCR_ENABLE_EDT_NAMING
        *marshalledSize = PD_MSG_FIELD(funcNameLen)*sizeof(char);
        if(*marshalledSize) {
            *marshalledSize += 1*sizeof(char); // Null terminating character
        }
#endif
        break;
#undef PD_TYPE

    case PD_MSG_COMM_TAKE:
#define PD_TYPE PD_MSG_COMM_TAKE
        *marshalledSize = sizeof(ocrFatGuid_t)*PD_MSG_FIELD(guidCount);
        break;
#undef PD_TYPE

    case PD_MSG_COMM_GIVE:
#define PD_TYPE PD_MSG_COMM_GIVE
        *marshalledSize = sizeof(ocrFatGuid_t)*PD_MSG_FIELD(guidCount);
        break;
#undef PD_TYPE

    case PD_MSG_GUID_METADATA_CLONE:
#define PD_TYPE PD_MSG_GUID_METADATA_CLONE
        *marshalledSize = PD_MSG_FIELD(size);
        // Make sure that if we have a size, we have a PTR
        ASSERT((*marshalledSize == 0) || (PD_MSG_FIELD(guid.metaDataPtr) != NULL));
        break;
#undef PD_TYPE

    case PD_MSG_DB_ACQUIRE:
        if(flags & MARSHALL_DBPTR) {
#define PD_TYPE PD_MSG_DB_ACQUIRE
            *marshalledSize = PD_MSG_FIELD(size);
#undef PD_TYPE
        }
        break;

    case PD_MSG_DB_RELEASE:
        if(flags & MARSHALL_DBPTR) {
#define PD_TYPE PD_MSG_DB_RELEASE
            *marshalledSize = PD_MSG_FIELD(size);
#undef PD_TYPE
        }
        break;

    default:
        // Nothing to do
        ;
    }

    *fullSize += *marshalledSize;
    DPRINTF(DEBUG_LVL_VVERB, "Msg 0x%lx of type 0x%x: fullSize %ld; addlSize %ld\n",
            msg, msg->type & PD_MSG_TYPE_ONLY, *fullSize, *marshalledSize);
#undef PD_MSG
    return 0;
}

u8 ocrPolicyMsgMarshallMsg(ocrPolicyMsg_t* msg, u8* buffer, u32 mode) {

    u8* startPtr = NULL;
    u8* curPtr = NULL;
    ocrPolicyMsg_t* outputMsg = NULL;
    u8 flags = mode & MARSHALL_FLAGS;
    mode &= MARSHALL_TYPE;
    u8 isAddl = (mode == MARSHALL_ADDL);
    u8 fixupPtrs = (mode != MARSHALL_DUPLICATE);

    switch(mode) {
    case MARSHALL_FULL_COPY:
    case MARSHALL_DUPLICATE:
        // Copy msg into the buffer for the common part
        hal_memCopy(buffer, msg, msg->size, false);
        startPtr = buffer;
        curPtr = buffer + msg->size;
        outputMsg = (ocrPolicyMsg_t*)buffer;
        break;
    case MARSHALL_APPEND:
        ASSERT((u64)buffer == (u64)msg);
        startPtr = (u8*)(msg);
        curPtr = startPtr + msg->size;
        ASSERT(msg->size); // Make sure the message is not of zero size
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
        // If paramc is greater than zero but paramv is NULL, do not serialize.
        // Implementation: So far, we do not pass depv directly as part of this message
        // If we do, we need to implement marshalling for that as well
        ASSERT(PD_MSG_FIELD(depv) == NULL);
        // First copy things over
        if (PD_MSG_FIELD(paramv) != NULL) {
            u64 s = sizeof(u64)*PD_MSG_FIELD(paramc);
            // There are params and they are already serialized.
            // Can happen when sending a response and the request already had
            // marshalled data
            if(PD_MSG_FIELD(paramv) != (void*)curPtr) {
                hal_memCopy(curPtr, PD_MSG_FIELD(paramv), s, false);
            }
            // Now fixup the pointer
            if(fixupPtrs) {
                DPRINTF(DEBUG_LVL_VVERB, "Converting paramv (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD(paramv), ((u64)(curPtr - startPtr)<<1) + isAddl);
                PD_MSG_FIELD(paramv) = (void*)((((u64)(curPtr - startPtr))<<1) + isAddl);
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "Copying paramv (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD(paramv), curPtr);
                PD_MSG_FIELD(paramv) = (void*)curPtr;
            }
            // Finally move the curPtr for the next object (none as of now)
            curPtr += s;
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_EDTTEMP_CREATE: {
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
        // First copy things over
#ifdef OCR_ENABLE_EDT_NAMING
        u64 s = sizeof(char)*(PD_MSG_FIELD(funcNameLen)+1);
        if(s) {
            hal_memCopy(curPtr, PD_MSG_FIELD(funcName), s, false);
            // Now fixup the pointer
            if(fixupPtrs) {
                DPRINTF(DEBUG_LVL_VVERB, "Converting funcName (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD(funcName), ((u64)(curPtr - startPtr)<<1) + isAddl);
                PD_MSG_FIELD(funcName) = (char*)(((u64)(curPtr - startPtr)<<1) + isAddl);
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "Copying funcName (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD(funcName), curPtr);
                PD_MSG_FIELD(funcName) = (char*)curPtr;
            }
            // Finally move the curPtr for the next object (none as of now)
            curPtr += s;
        }
#endif
        break;
#undef PD_TYPE
    }

    case PD_MSG_COMM_TAKE: {
#define PD_TYPE PD_MSG_COMM_TAKE
        // First copy things over
        u64 s = sizeof(ocrFatGuid_t)*PD_MSG_FIELD(guidCount);
        if(s) {
            hal_memCopy(curPtr, PD_MSG_FIELD(guids), s, false);
            // Now fixup the pointer
            if(fixupPtrs) {
                DPRINTF(DEBUG_LVL_VVERB, "Converting guids (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD(guids), ((u64)(curPtr - startPtr)<<1) + isAddl);
                PD_MSG_FIELD(guids) = (ocrFatGuid_t*)(((u64)(curPtr - startPtr)<<1) + isAddl);
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "Copying guids (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD(guids), curPtr);
                PD_MSG_FIELD(guids) = (ocrFatGuid_t*)curPtr;
            }
            // Finally move the curPtr for the next object (none as of now)
            curPtr += s;
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_COMM_GIVE: {
#define PD_TYPE PD_MSG_COMM_GIVE
        // First copy things over
        u64 s = sizeof(ocrFatGuid_t)*PD_MSG_FIELD(guidCount);
        if(s) {
            hal_memCopy(curPtr, PD_MSG_FIELD(guids), s, false);
            // Now fixup the pointer
            if(fixupPtrs) {
                DPRINTF(DEBUG_LVL_VVERB, "Converting guids (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD(guids), ((u64)(curPtr - startPtr)<<1) + isAddl);
                PD_MSG_FIELD(guids) = (ocrFatGuid_t*)(((u64)(curPtr - startPtr)<<1) + isAddl);
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "Copying guids (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD(guids), curPtr);
                PD_MSG_FIELD(guids) = (ocrFatGuid_t*)curPtr;
            }
            // Finally move the curPtr for the next object (none as of now)
            curPtr += s;
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_GUID_METADATA_CLONE: {
#define PD_TYPE PD_MSG_GUID_METADATA_CLONE
        // First copy things over
        u64 s = PD_MSG_FIELD(size);
        if(s) {
            hal_memCopy(curPtr, PD_MSG_FIELD(guid.metaDataPtr), s, false);
            // Now fixup the pointer
            if(fixupPtrs) {
                DPRINTF(DEBUG_LVL_VVERB, "Converting metadata clone (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD(guid.metaDataPtr), ((u64)(curPtr - startPtr)<<1) + isAddl);
                PD_MSG_FIELD(guid.metaDataPtr) = (void*)(((u64)(curPtr - startPtr)<<1) + isAddl);
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "Copying metadata clone (0x%lx) to 0x%lx\n",
                        (u64)PD_MSG_FIELD(guid.metaDataPtr), curPtr);
                PD_MSG_FIELD(guid.metaDataPtr) = (void*)curPtr;
            }
            // Finally move the curPtr for the next object (none as of now)
            curPtr += s;
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_DB_ACQUIRE: {
        if(flags & MARSHALL_DBPTR) {
#define PD_TYPE PD_MSG_DB_ACQUIRE
            // First copy things over
            u64 s = PD_MSG_FIELD(size);
            if(s) {
                hal_memCopy(curPtr, PD_MSG_FIELD(ptr), s, false);
                // Now fixup the pointer
                if(fixupPtrs) {
                    DPRINTF(DEBUG_LVL_VVERB, "Converting DB acquire ptr (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD(ptr), ((u64)(curPtr - startPtr)<<1) + isAddl);
                    PD_MSG_FIELD(ptr) = (void*)(((u64)(curPtr - startPtr)<<1) + isAddl);
                } else {
                    DPRINTF(DEBUG_LVL_VVERB, "Copying DB acquire ptr (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD(ptr), curPtr);
                    PD_MSG_FIELD(ptr) = (void*)curPtr;
                }
                // Finally move the curPtr for the next object (none as of now)
                curPtr += s;
            }
#undef PD_TYPE
        }
        break;

    }

    case PD_MSG_DB_RELEASE: {
        if(flags & MARSHALL_DBPTR) {
#define PD_TYPE PD_MSG_DB_RELEASE
            // First copy things over
            u64 s = PD_MSG_FIELD(size);
            if(s) {
                hal_memCopy(curPtr, PD_MSG_FIELD(ptr), s, false);
                // Now fixup the pointer
                if(fixupPtrs) {
                    DPRINTF(DEBUG_LVL_VVERB, "Converting DB release ptr (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD(ptr), ((u64)(curPtr - startPtr)<<1) + isAddl);
                    PD_MSG_FIELD(ptr) = (void*)(((u64)(curPtr - startPtr)<<1) + isAddl);
                } else {
                    DPRINTF(DEBUG_LVL_VVERB, "Copying DB release ptr (0x%lx) to 0x%lx\n",
                            (u64)PD_MSG_FIELD(ptr), curPtr);
                    PD_MSG_FIELD(ptr) = (void*)curPtr;
                }
                // Finally move the curPtr for the next object (none as of now)
                curPtr += s;
            }
#undef PD_TYPE
        }
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
        outputMsg->size = (u64)(curPtr) - (u64)(startPtr);
    }

    DPRINTF(DEBUG_LVL_VVERB, "Size of message set to 0x%lu\n", outputMsg->size);
    return 0;
}

u8 ocrPolicyMsgUnMarshallMsg(u8* mainBuffer, u8* addlBuffer,
                             ocrPolicyMsg_t* msg, u32 mode) {
    u8* localMainPtr = (u8*)msg;
    u8* localAddlPtr = NULL;

    u64 fullSize=0, marshalledSize=0;

    u8 flags = mode & MARSHALL_FLAGS;
    mode &= MARSHALL_TYPE;

    switch(mode) {
    case MARSHALL_FULL_COPY:
        // We copy mainBuffer into msg
        // Do this before getting the size because we need to
        // copy any additional parts in mainBuffer if present
        DPRINTF(DEBUG_LVL_VVERB, "Unmarshall full-copy: 0x%lx -> 0x%lx of size %ld\n",
                (u64)mainBuffer, (u64)msg, ((ocrPolicyMsg_t*)mainBuffer)->size);
        hal_memCopy(msg, mainBuffer, ((ocrPolicyMsg_t*)mainBuffer)->size, false);

        ocrPolicyMsgGetMsgSize(msg, &fullSize, &marshalledSize, mode);
        // This resets the size of msg to be the proper "common" size
        // excluding any additional stuff so we can take the localAddlPtr info
        localAddlPtr = (u8*)msg + ((ocrPolicyMsg_t*)msg)->size;
        break;

    case MARSHALL_APPEND:
        ASSERT((u64)msg == (u64)mainBuffer);
        ocrPolicyMsgGetMsgSize(msg, &fullSize, &marshalledSize, mode);
        // This resets the size of msg to be the proper "common" size
        // excluding any additional stuff so we can take the localAddlPtr info
        localAddlPtr = (u8*)msg + ((ocrPolicyMsg_t*)msg)->size;
        break;

    case MARSHALL_ADDL: {
        // Here, we have to copy mainBuffer into msg if needed
        // and also create a new chunk to hold additional information
        u64 origSize = ((ocrPolicyMsg_t*)mainBuffer)->size;
        ocrPolicyMsgGetMsgSize((ocrPolicyMsg_t*)mainBuffer, &fullSize, &marshalledSize, mode);
        // At this point, we have mainBuffer->size properly set to just
        // the common part even if before it may have been bigger (for example
        // if it was marshalled as APPEND)
        if((u64)msg != (u64)mainBuffer) {
            // Therefore, we always only copy the useful part here and exclude all
            // "pointed-to" data
            hal_memCopy(msg, mainBuffer, ((ocrPolicyMsg_t*)mainBuffer)->size, false);
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
            if(origSize != msg->size) {
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
        if ((PD_MSG_FIELD(paramc) > 0) && (PD_MSG_FIELD(paramv) != NULL)) {
            u64 t = (u64)(PD_MSG_FIELD(paramv));
            PD_MSG_FIELD(paramv) = (void*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
            DPRINTF(DEBUG_LVL_VVERB, "Converted field paramv from 0x%lx to 0x%lx\n",
                    t, (u64)PD_MSG_FIELD(paramv));
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_EDTTEMP_CREATE: {
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
#ifdef OCR_ENABLE_EDT_NAMING
        if(PD_MSG_FIELD(funcNameLen) > 0) {
            u64 t = (u64)(PD_MSG_FIELD(funcName));
            PD_MSG_FIELD(funcName) = (char*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
            DPRINTF(DEBUG_LVL_VVERB, "Converted field funcName from 0x%lx to 0x%lx\n",
                    t, (u64)PD_MSG_FIELD(funcName));
        }
#endif
        break;
#undef PD_TYPE
    }

    case PD_MSG_COMM_TAKE: {
#define PD_TYPE PD_MSG_COMM_TAKE
        if(PD_MSG_FIELD(guidCount) > 0) {
            u64 t = (u64)(PD_MSG_FIELD(guids));
            PD_MSG_FIELD(guids) = (ocrFatGuid_t*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
            DPRINTF(DEBUG_LVL_VVERB, "Converted field guids from 0x%lx to 0x%lx\n",
                    t, (u64)PD_MSG_FIELD(guids));
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_COMM_GIVE: {
#define PD_TYPE PD_MSG_COMM_GIVE
        if(PD_MSG_FIELD(guidCount) > 0) {
            u64 t = (u64)(PD_MSG_FIELD(guids));
            PD_MSG_FIELD(guids) = (ocrFatGuid_t*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
            DPRINTF(DEBUG_LVL_VVERB, "Converted field guids from 0x%lx to 0x%lx\n",
                    t, (u64)PD_MSG_FIELD(guids));
        }
        break;
#undef PD_TYPE
    }

    case PD_MSG_GUID_METADATA_CLONE: {
#define PD_TYPE PD_MSG_GUID_METADATA_CLONE
        if(PD_MSG_FIELD(size) > 0) {
            u64 t = (u64)(PD_MSG_FIELD(guid.metaDataPtr));
            PD_MSG_FIELD(guid.metaDataPtr) = (void*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
            DPRINTF(DEBUG_LVL_VVERB, "Converted metadata ptr from 0x%lx to 0x%lx\n",
                    t, (u64)PD_MSG_FIELD(guid.metaDataPtr));
        }
        break;
#undef PD_TYPE
    }

    // Temporary hack: in TG, there's no marshalling of DB messages' pointers
    case PD_MSG_DB_ACQUIRE: {
        if(flags & MARSHALL_DBPTR) {
#define PD_TYPE PD_MSG_DB_ACQUIRE
            if(PD_MSG_FIELD(size) > 0) {
                u64 t = (u64)(PD_MSG_FIELD(ptr));
                PD_MSG_FIELD(ptr) = (void*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
                DPRINTF(DEBUG_LVL_VVERB, "Converted DB acquire ptr from 0x%lx to 0x%lx\n",
                        t, (u64)PD_MSG_FIELD(ptr));
            }
#undef PD_TYPE
        }
        break;
    }

    case PD_MSG_DB_RELEASE: {
        if(flags & MARSHALL_DBPTR) {
#define PD_TYPE PD_MSG_DB_RELEASE
            if(PD_MSG_FIELD(size) > 0) {
                u64 t = (u64)(PD_MSG_FIELD(ptr));
                PD_MSG_FIELD(ptr) = (void*)((t&1?localAddlPtr:localMainPtr) + (t>>1));
                DPRINTF(DEBUG_LVL_VVERB, "Converted DB release ptr from 0x%lx to 0x%lx\n",
                        t, (u64)PD_MSG_FIELD(ptr));
            }
#undef PD_TYPE
        }
        break;
    }

    default:
        // Nothing to do
        ;
    }
#undef PD_MSG
    // Fixup the size in the case of APPEND and FULL_COPY
    if(mode == MARSHALL_APPEND || mode == MARSHALL_FULL_COPY) {
        msg->size = fullSize;
    }
    DPRINTF(DEBUG_LVL_VVERB, "Done unmarshalling and have size of message %ld\n", msg->size);
    return 0;
}
