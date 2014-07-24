#include <ocr-types.h>
#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-comm-platform.h"
#include "ocr-comp-platform.h"
#include "ocr-comp-target.h"
#include "ocr-allocator.h"
#include "ocr-mem-platform.h"
#include "ocr-mem-target.h"
#include "ocr-workpile.h"

ocrPolicyMsg_t _data_0x1001  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x1001={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.edt={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.ptr=0x4746454443424140,.size=0x4f4e4d4c4b4a4948,.affinity={.guid=0x5756555453525150,.metaDataPtr=0x5f5e5d5c5b5a5958},.properties=0x63626160,.dbType=0x67666564,.allocator=0x6b6a6968},
}};
ocrPolicyMsg_t _data_0x2001  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x2001={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.edt={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.properties=0x43424140},
}};
ocrPolicyMsg_t _data_0x3001  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x3001={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.edt={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.ptr=0x4746454443424140,.properties=0x4b4a4948},
}};
ocrPolicyMsg_t _data_0x4001  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x4001={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.edt={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.properties=0x43424140},
}};
ocrPolicyMsg_t _data_0x5001  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x5001={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.edt={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.properties=0x43424140},
}};
ocrPolicyMsg_t _data_0x1002  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x1002={.size=0x2726252423222120,.ptr=0x2f2e2d2c2b2a2928,.allocatingPD={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.allocator={.guid=0x4746454443424140,.metaDataPtr=0x4f4e4d4c4b4a4948},.type=0x53525150,.properties=0x57565554},
}};
ocrPolicyMsg_t _data_0x2002  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x2002={.ptr=0x2726252423222120,.allocatingPD={.guid=0x2f2e2d2c2b2a2928,.metaDataPtr=0x3736353433323130},.allocator={.guid=0x3f3e3d3c3b3a3938,.metaDataPtr=0x4746454443424140},.type=0x4b4a4948,.properties=0x4f4e4d4c},
}};
ocrPolicyMsg_t _data_0x1004  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x1004={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.templateGuid={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.affinity={.guid=0x4746454443424140,.metaDataPtr=0x4f4e4d4c4b4a4948},.outputEvent={.guid=0x5756555453525150,.metaDataPtr=0x5f5e5d5c5b5a5958},.paramv=0x6766656463626160,.paramc=0x6b6a6968,.depc=0x6f6e6d6c,.properties=0x73727170,.workType=0x77767574},
}};
ocrPolicyMsg_t _data_0x2004  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x2004={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.properties=0x33323130},
}};
ocrPolicyMsg_t _data_0x3004  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x3004={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.properties=0x33323130},
}};
ocrPolicyMsg_t _data_0x1008  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x1008={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.funcPtr=0x3736353433323130,.paramc=0x3b3a3938,.depc=0x3f3e3d3c,.properties=0x43424140,.funcName=0x4f4e4d4c4b4a4948},
}};
ocrPolicyMsg_t _data_0x2008  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x2008={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.properties=0x33323130},
}};
ocrPolicyMsg_t _data_0x1010  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x1010={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.properties=0x33323130,.type=0x37363534},
}};
ocrPolicyMsg_t _data_0x2010  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x2010={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.properties=0x33323130},
}};
ocrPolicyMsg_t _data_0x3010  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x3010={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.data={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.properties=0x43424140},
}};
ocrPolicyMsg_t _data_0x1020  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x1020={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.size=0x3736353433323130,.kind=0x3b3a3938,.properties=0x3f3e3d3c},
}};
ocrPolicyMsg_t _data_0x2020  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x2020={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.kind=0x33323130,.properties=0x37363534},
}};
ocrPolicyMsg_t _data_0x3020  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x3020={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.properties=0x33323130},
}};
ocrPolicyMsg_t _data_0x1040  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x1040={.guids=0x2726252423222120,.extra=0x2f2e2d2c2b2a2928,.guidCount=0x33323130,.properties=0x37363534,.type=0x3b3a3938},
}};
ocrPolicyMsg_t _data_0x2040  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x2040={.guids=0x2726252423222120,.guidCount=0x2b2a2928,.properties=0x2f2e2d2c,.type=0x33323130},
}};
ocrPolicyMsg_t _data_0x1080  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x1080={.source={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.dest={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.slot=0x43424140,.properties=0x47464544},
}};
ocrPolicyMsg_t _data_0x2080  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x2080={.signaler={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.dest={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.slot=0x43424140,.properties=0x47464544},
}};
ocrPolicyMsg_t _data_0x3080  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x3080={.waiter={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.dest={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.slot=0x43424140,.properties=0x47464544},
}};
ocrPolicyMsg_t _data_0x4080  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x4080={.guid={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.payload={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.slot=0x43424140,.properties=0x47464544},
}};
ocrPolicyMsg_t _data_0x5080  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x5080={.signaler={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.dest={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.slot=0x43424140,.properties=0x47464544},
}};
ocrPolicyMsg_t _data_0x7080  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x7080={.edt={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.db={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.properties=0x43424140},
}};
ocrPolicyMsg_t _data_0x8080  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x8080={.edt={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.db={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.properties=0x43424140},
}};
ocrPolicyMsg_t _data_0x6080  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x6080={.waiter={.guid=0x2726252423222120,.metaDataPtr=0x2f2e2d2c2b2a2928},.dest={.guid=0x3736353433323130,.metaDataPtr=0x3f3e3d3c3b3a3938},.slot=0x43424140,.properties=0x47464544},
}};
ocrPolicyMsg_t _data_0x1100  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x1100={.buffer=0x2726252423222120,.length=0x2f2e2d2c2b2a2928,.properties=0x33323130},
}};
ocrPolicyMsg_t _data_0x2100  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x2100={.buffer=0x2726252423222120,.length=0x2f2e2d2c2b2a2928,.inputId=0x3736353433323130,.properties=0x3b3a3938},
}};
ocrPolicyMsg_t _data_0x3100  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x3100={.buffer=0x2726252423222120,.length=0x2f2e2d2c2b2a2928,.outputId=0x3736353433323130,.properties=0x3b3a3938},
}};
ocrPolicyMsg_t _data_0x4100  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x4100={.errorCode=0x2726252423222120,.properties=0x2b2a2928},
}};
ocrPolicyMsg_t _data_0x1200  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x1200={.properties=0x23222120},
}};
ocrPolicyMsg_t _data_0x2200  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x2200={.properties=0x23222120},
}};
ocrPolicyMsg_t _data_0x3200  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x3200={.neighbor=0x2726252423222120,.properties=0x2b2a2928},
}};
ocrPolicyMsg_t _data_0x4200  __attribute__((aligned (8)))= {.type=0x3020100,.size=0x7060504,.srcLocation=0xf0e0d0c0b0a0908,.destLocation=0x1716151413121110,.msgId=0x1f1e1d1c1b1a1918,.args={
._data_0x4200={.neighbor=0x2726252423222120,.properties=0x2b2a2928}
}};
ocrAllocator_t ptr2 __attribute__((aligned (8)))= {
.fguid={
.guid=0x706050403020100,
.metaDataPtr=0xf0e0d0c0b0a0908
},
.pd=0x1716151413121110,
.memories=0x1f1e1d1c1b1a1918,
.memoryCount=0x2726252423222120,
.fcts={
.destruct=0x2f2e2d2c2b2a2928,
.begin=0x3736353433323130,
.start=0x3f3e3d3c3b3a3938,
.stop=0x4746454443424140,
.finish=0x4f4e4d4c4b4a4948,
.allocate=0x5756555453525150,
.reallocate=0x6766656463626160
}
};
ocrCommApi_t ptr3 __attribute__((aligned (8)))= {
.pd=0x706050403020100,
.commPlatform=0xf0e0d0c0b0a0908,
.fcts={
.destruct=0x1716151413121110,
.begin=0x1f1e1d1c1b1a1918,
.start=0x2726252423222120,
.stop=0x2f2e2d2c2b2a2928,
.finish=0x3736353433323130,
.sendMessage=0x3f3e3d3c3b3a3938,
.pollMessage=0x4746454443424140,
.waitMessage=0x4f4e4d4c4b4a4948
}
};
ocrCommPlatform_t ptr4 __attribute__((aligned (8)))= {
.pd=0x706050403020100,
.location=0xf0e0d0c0b0a0908,
.fcts={
.destruct=0x1716151413121110,
.begin=0x1f1e1d1c1b1a1918,
.start=0x2726252423222120,
.stop=0x2f2e2d2c2b2a2928,
.finish=0x3736353433323130,
.setMaxExpectedMessageSize=0x3f3e3d3c3b3a3938,
.sendMessage=0x4746454443424140,
.pollMessage=0x4f4e4d4c4b4a4948,
.waitMessage=0x5756555453525150,
.destructMessage=0x5f5e5d5c5b5a5958,
.getMessageStatus=0x6766656463626160
}
};
ocrCompPlatform_t ptr5 __attribute__((aligned (8)))= {
.pd=0x706050403020100,
.worker=0xf0e0d0c0b0a0908,
.fcts={
.destruct=0x1716151413121110,
.begin=0x1f1e1d1c1b1a1918,
.start=0x2726252423222120,
.stop=0x2f2e2d2c2b2a2928,
.finish=0x3736353433323130,
.getThrottle=0x3f3e3d3c3b3a3938,
.setThrottle=0x4746454443424140,
.setCurrentEnv=0x4f4e4d4c4b4a4948
}
};
ocrCompTarget_t ptr6 __attribute__((aligned (8)))= {
.fguid={
.guid=0x706050403020100,
.metaDataPtr=0xf0e0d0c0b0a0908
},
.pd=0x1716151413121110,
.worker=0x1f1e1d1c1b1a1918,
.platforms=0x2726252423222120,
.platformCount=0x2f2e2d2c2b2a2928,
.fcts={
.destruct=0x3736353433323130,
.begin=0x3f3e3d3c3b3a3938,
.start=0x4746454443424140,
.stop=0x4f4e4d4c4b4a4948,
.finish=0x5756555453525150,
.getThrottle=0x5f5e5d5c5b5a5958,
.setThrottle=0x6766656463626160,
.setCurrentEnv=0x6f6e6d6c6b6a6968
}
};
ocrDataBlock_t ptr7 __attribute__((aligned (8)))= {
.guid=0x706050403020100,
.allocator=0xf0e0d0c0b0a0908,
.allocatingPD=0x1716151413121110,
.size=0x1f1e1d1c1b1a1918,
.ptr=0x2726252423222120,
.properties=0x2b2a2928,
.fctId=0x2f2e2d2c
};
ocrEvent_t ptr8 __attribute__((aligned (8)))= {
.guid=0x706050403020100,
.kind=0xb0a0908,
.fctId=0xf0e0d0c
};
ocrGuidProvider_t ptr9 __attribute__((aligned (8)))= {
.pd=0x706050403020100,
.id=0xb0a0908,
.fcts={
.destruct=0x1716151413121110,
.begin=0x1f1e1d1c1b1a1918,
.start=0x2726252423222120,
.stop=0x2f2e2d2c2b2a2928,
.finish=0x3736353433323130,
.getGuid=0x3f3e3d3c3b3a3938,
.createGuid=0x4746454443424140,
.getVal=0x4f4e4d4c4b4a4948,
.getKind=0x5756555453525150,
.releaseGuid=0x5f5e5d5c5b5a5958
}
};
ocrMemPlatform_t ptr10 __attribute__((aligned (8)))= {
.pd=0x706050403020100,
.size=0xf0e0d0c0b0a0908,
.startAddr=0x1716151413121110,
.endAddr=0x1f1e1d1c1b1a1918,
.fcts={
.destruct=0x2726252423222120,
.begin=0x2f2e2d2c2b2a2928,
.start=0x3736353433323130,
.stop=0x3f3e3d3c3b3a3938,
.finish=0x4746454443424140,
.getThrottle=0x4f4e4d4c4b4a4948,
.setThrottle=0x5756555453525150,
.getRange=0x5f5e5d5c5b5a5958,
.chunkAndTag=0x6766656463626160,
.tag=0x6f6e6d6c6b6a6968,
.queryTag=0x7776757473727170
}
};
ocrMemTarget_t ptr11 __attribute__((aligned (8)))= {
.fguid={
.guid=0x706050403020100,
.metaDataPtr=0xf0e0d0c0b0a0908
},
.pd=0x1716151413121110,
.size=0x1f1e1d1c1b1a1918,
.startAddr=0x2726252423222120,
.endAddr=0x2f2e2d2c2b2a2928,
.memories=0x3736353433323130,
.memoryCount=0x3f3e3d3c3b3a3938,
.fcts={
.destruct=0x4746454443424140,
.begin=0x4f4e4d4c4b4a4948,
.start=0x5756555453525150,
.stop=0x5f5e5d5c5b5a5958,
.finish=0x6766656463626160,
.getThrottle=0x6f6e6d6c6b6a6968,
.setThrottle=0x7776757473727170,
.getRange=0x7f7e7d7c7b7a7978,
.chunkAndTag=0x8786858483828180,
.tag=0x8f8e8d8c8b8a8988,
.queryTag=0x9796959493929190
}
};
ocrPolicyDomain_t ptr12 __attribute__((aligned (8)))= {
.fguid={
.guid=0x706050403020100,
.metaDataPtr=0xf0e0d0c0b0a0908
},
.schedulerCount=0x1716151413121110,
.allocatorCount=0x1f1e1d1c1b1a1918,
.workerCount=0x2726252423222120,
.commApiCount=0x2f2e2d2c2b2a2928,
.guidProviderCount=0x3736353433323130,
.taskFactoryCount=0x3f3e3d3c3b3a3938,
.taskTemplateFactoryCount=0x4746454443424140,
.dbFactoryCount=0x4f4e4d4c4b4a4948,
.eventFactoryCount=0x5756555453525150,
.schedulers=0x5f5e5d5c5b5a5958,
.allocators=0x6766656463626160,
.workers=0x6f6e6d6c6b6a6968,
.commApis=0x7776757473727170,
.taskFactories=0x7f7e7d7c7b7a7978,
.taskTemplateFactories=0x8786858483828180,
.dbFactories=0x8f8e8d8c8b8a8988,
.eventFactories=0x9796959493929190,
.guidProviders=0x9f9e9d9c9b9a9998,
.costFunction=0xa7a6a5a4a3a2a1a0,
.myLocation=0xafaeadacabaaa9a8,
.parentLocation=0xb7b6b5b4b3b2b1b0,
.neighbors=0xbfbebdbcbbbab9b8,
.neighborCount=0xc3c2c1c0,
.fcts={
.destruct=0xcfcecdcccbcac9c8,
.begin=0xd7d6d5d4d3d2d1d0,
.start=0xdfdedddcdbdad9d8,
.stop=0xe7e6e5e4e3e2e1e0,
.finish=0xefeeedecebeae9e8,
.processMessage=0xf7f6f5f4f3f2f1f0,
.sendMessage=0xfffefdfcfbfaf9f8,
.pollMessage=0x706050403020100,
.waitMessage=0xf0e0d0c0b0a0908,
.pdMalloc=0x1716151413121110,
.pdFree=0x1f1e1d1c1b1a1918
}
};
ocrScheduler_t ptr13 __attribute__((aligned (8)))= {
.fguid={
.guid=0x706050403020100,
.metaDataPtr=0xf0e0d0c0b0a0908
},
.pd=0x1716151413121110,
.workpiles=0x1f1e1d1c1b1a1918,
.workpileCount=0x2726252423222120,
.fcts={
.destruct=0x2f2e2d2c2b2a2928,
.begin=0x3736353433323130,
.start=0x3f3e3d3c3b3a3938,
.stop=0x4746454443424140,
.finish=0x4f4e4d4c4b4a4948,
.takeEdt=0x5756555453525150,
.giveEdt=0x5f5e5d5c5b5a5958,
.takeComm=0x6766656463626160,
.giveComm=0x6f6e6d6c6b6a6968
}
};
ocrWorker_t ptr14 __attribute__((aligned (8)))= {
.fguid={
.guid=0x706050403020100,
.metaDataPtr=0xf0e0d0c0b0a0908
},
.pd=0x1716151413121110,
.location=0x1f1e1d1c1b1a1918,
.type=0x23222120,
.computes=0x2f2e2d2c2b2a2928,
.computeCount=0x3736353433323130,
.curTask=0x3f3e3d3c3b3a3938,
.fcts={
.destruct=0x4746454443424140,
.begin=0x4f4e4d4c4b4a4948,
.start=0x5756555453525150,
.run=0x5f5e5d5c5b5a5958,
.finish=0x6766656463626160,
.stop=0x6f6e6d6c6b6a6968,
.isRunning=0x7776757473727170
}
};
ocrWorkpile_t ptr15 __attribute__((aligned (8)))= {
.fguid={
.guid=0x706050403020100,
.metaDataPtr=0xf0e0d0c0b0a0908
},
.pd=0x1716151413121110,
.fcts={
.destruct=0x1f1e1d1c1b1a1918,
.begin=0x2726252423222120,
.start=0x2f2e2d2c2b2a2928,
.stop=0x3736353433323130,
.finish=0x3f3e3d3c3b3a3938,
.pop=0x4746454443424140,
.push=0x4f4e4d4c4b4a4948
}
};
