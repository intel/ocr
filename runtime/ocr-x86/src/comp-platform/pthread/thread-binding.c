/*
 * This file is part of the Habanero-C implementation and
 * distributed under the Modified BSD License.
 * See LICENSE for details.
 *
 */
#include "ocr-config.h"
#ifdef ENABLE_COMP_PLATFORM_PTHREAD

#include "debug.h"
#include "ocr-types.h"

#define _GNU_SOURCE
#define __USE_GNU
#include <unistd.h>
#include <sched.h>
#include <errno.h>

#define DEBUG_TYPE COMP_PLATFORM
/* Platform specific thread binding implementations */

#ifdef __linux
static void bindThreadWithMask(u32 * mask, u32 lg) {
    u32 i = 0;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    /* Copy the mask from the int array to the cpuset */
    for (i=0; i<lg; i++) {
        CPU_SET(mask[i], &cpuset);
    }

    /* Set affinity */
    int res = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
    if (res != 0) {
        if (errno == ESRCH) {
            DPRINTF(DEBUG_LVL_WARN, "bindThread: ESRCH: Process not found!\n");
        }
        if (errno == EINVAL) {
            DPRINTF(DEBUG_LVL_WARN, "bindThread: EINVAL: CPU mask does not contain any actual physical processor\n");
        }
        if (errno == EFAULT) {
            DPRINTF(DEBUG_LVL_WARN, "bindThread: EFAULT: memory address was invalid\n");
        }
        if (errno == EPERM) {
            DPRINTF(DEBUG_LVL_WARN, "bindThread: EPERM: process does not have appropriate privileges\n");
        }
    }
}
#else
static void bindThreadWithMask(u32 * mask, u32 lg) {
    DPRINTF(DEBUG_LVL_WARN, "bindThread: No thread binding support for this platform\n");
    ASSERT(0);
}
#endif /* __linux */

/** Thread binding api to bind a worker thread using a particular binding strategy **/
void bindThread(u32 mask) {
    bindThreadWithMask(&mask, 1);
}
#endif /* ENABLE_COMP_PLATFORM_PTHREAD */
