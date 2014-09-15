
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __SCHEDULER_BLOCKING_SUPPORT_H__
#define __SCHEDULER_BLOCKING_SUPPORT_H__

#include "ocr-config.h"
#ifdef ENABLE_SCHEDULER_BLOCKING_SUPPORT

struct _ocrWorker_t;

u8 handleWorkerNotProgressing(struct _ocrWorker_t * worker);

#endif /* ENABLE_SCHEDULER_BLOCKING_SUPPORT */
#endif /* __SCHEDULER_BLOCKING_SUPPORT_H__ */



