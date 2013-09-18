/*
 * This file is part of the Habanero-C implementation and
 * distributed under the Modified BSD License.
 * See LICENSE for details.
 *
 */
//#include <thread_binding.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/** Platform specific thread binding implementations **/

#ifdef __linux

static void bind_thread_with_mask(int * mask, int lg) {
    int i = 0;
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
            printf("Warning: bind_thread: ESRCH: Process not found!\n");
        }
        if (errno == EINVAL) {
            printf("Warning: bind_thread: EINVAL: CPU mask does not contain any actual physical processor\n");
        }
        if (errno == EFAULT) {
            printf("Warning: bind_thread: EFAULT: memory address was invalid\n");
        }
        if (errno == EPERM) {
            printf("Warning: bind_thread: EPERM: process does not have appropriate privileges\n");
        }
    }
}

#else

static void bind_thread_with_mask(int * mask, int lg) {
    assert("Warning: bind_thread: No thread binding support for this platform\n");
}

#endif

/** Thread binding api to bind a worker thread using a particular binding strategy **/
void bind_thread(int mask) {
    bind_thread_with_mask(&mask, 1);
}
