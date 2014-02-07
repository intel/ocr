
rootdir=../../..

AUX_DIST = \
	$(ac_aux_dir)/config.guess		\
	$(ac_aux_dir)/config.sub		\
	$(ac_aux_dir)/install-sh		\
	$(ac_aux_dir)/ltmain.sh			\
	$(ac_aux_dir)/missing			\
	$(ac_aux_dir)/depcomp

library_includedir=$(includedir)
library_include_HEADERS = $(rootdir)/inc/ocr-db.h $(rootdir)/inc/ocr-edt.h $(rootdir)/inc/ocr-tuning.h \
$(rootdir)/inc/ocr-types.h $(rootdir)/inc/ocr.h $(rootdir)/inc/compat.h $(rootdir)/inc/ocr-stathooks.h

# Distribute runtime interface headers - set by configure
if INCLUDE_RUNTIME_ITF_HEADERS
library_include_HEADERS += $(rootdir)/inc/ocr-runtime-itf.h
endif

# Distribute OCR library interface headers - set by configure
if INCLUDE_OCR_LIBRARY_ITF_HEADERS
library_include_HEADERS += $(rootdir)/inc/ocr-lib.h
endif

configdir = $(prefix)/config
config_DATA = $(rootdir)/machine-configs/default.cfg

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 \
	configure  \
	$(AUX_DIST)

# Includes folder we need when compiling ocr. top_srcdir is where this Makefile.am is placed
OCR_INCLUDES_DIR = \
-I$(top_srcdir)/..  \
-I$(top_srcdir)/../../../inc \
-I$(top_srcdir)/../../../src -I$(top_srcdir)/../../../src/inc -I$(top_srcdir)/../../../src/inc/hc -I$(top_srcdir)/../../../src/inc/utils

# cflags: important to define that otherwise we inherit default values too
CFLAGS = -Wall -g -O0

# Debug flags

# Enable naming of EDTs
# This uses the name of the function to name the EDT templates and therefore
# the EDTs.
# NOTE: The application must also have this flag defined
# If this is note the case, an ASSERT will happen
CFLAGS += -DOCR_ENABLE_EDT_NAMING

# Debugging support
# Enable debug
CFLAGS += -DOCR_DEBUG
# Define level
CFLAGS += -DOCR_DEBUG_LVL=DEBUG_LVL_WARN
# Define which modules you want for debugging
# You can optionally define an individual debuging level by
# defining DEBUG_LVL_XXX like OCR_DEBUG_LEVEL. If not defined,
# the default will be used
CFLAGS += -DOCR_DEBUG_ALLOCATOR #-DDEBUG_LVL_ALLOCATOR=DEBUG_LVL_VVERB
CFLAGS += -DOCR_DEBUG_COMP_PLATFORM
CFLAGS += -DOCR_DEBUG_COMP_TARGET
CFLAGS += -DOCR_DEBUG_DATABLOCK
CFLAGS += -DOCR_DEBUG_EVENT
CFLAGS += -DOCR_DEBUG_GUID
CFLAGS += -DOCR_DEBUG_INIPARSING
CFLAGS += -DOCR_DEBUG_MACHINE
CFLAGS += -DOCR_DEBUG_MEM_PLATFORM
CFLAGS += -DOCR_DEBUG_MEM_TARGET
CFLAGS += -DOCR_DEBUG_POLICY
CFLAGS += -DOCR_DEBUG_SCHEDULER
CFLAGS += -DOCR_DEBUG_STATS #-DDEBUG_LVL_STATS=DEBUG_LVL_VVERB
CFLAGS += -DOCR_DEBUG_SYNC
CFLAGS += -DOCR_DEBUG_TASK
CFLAGS += -DOCR_DEBUG_WORKER
CFLAGS += -DOCR_DEBUG_WORKPILE

# Enable statistics with this
#CFLAGS += -DOCR_ENABLE_STATISTICS
# This adds statistics for LD/STs and requires compiling
# the user code with the OCRStats LLVM pass
#CFLAGS += -DOCR_ENABLE_PROFILING_STATISTICS

# additional cflags for ocr modules
OCR_CFLAGS =

# Standard am options all the submodules can grab
OCR_AM_CFLAGS = $(OCR_INCLUDES_DIR) $(OCR_CFLAGS)

# Add warning treated as errors for standard ocr modules build
# For modules that do need to bypass this, just use the
# OCR_AM_CFLAGS variable
AM_CFLAGS = $(OCR_AM_CFLAGS) -Werror

lib_LTLIBRARIES = libocr.la

# These are populated in sub-folder's Makefile.inc
noinst_LTLIBRARIES =
libocr_la_LIBADD =

# build the ocr runtime library
libocr_la_SOURCES =
libocr_la_CFLAGS = $(AM_CFLAGS)

#
## Allocator
#

noinst_LTLIBRARIES += libocr_allocator_tlsf.la
libocr_la_LIBADD += libocr_allocator_tlsf.la

libocr_allocator_tlsf_la_SOURCES = \
../../../src/allocator/tlsf/tlsf-allocator.c

# bypass -Werror from top-level makefile to pass 
# compilation on 32 bits systems
libocr_allocator_tlsf_la_CFLAGS = $(OCR_AM_CFLAGS)

#
## API
#

noinst_LTLIBRARIES += libocr_api.la
libocr_la_LIBADD += libocr_api.la

libocr_api_la_SOURCES = \
../../../src/api/ocr.c \
../../../src/api/ocr-db.c \
../../../src/api/ocr-edt.c \
../../../src/api/ocr-rt-itf.c \
../../../src/api/ocr-lib.c

libocr_api_la_CFLAGS = $(AM_CFLAGS)

#
## COMM-PLATFORM
#

noinst_LTLIBRARIES += libocr_comm_platform_null.la
libocr_la_LIBADD += libocr_comm_platform_null.la

libocr_comm_platform_null_la_SOURCES = \
../../../src/comm-platform/null/null-comm-platform.c

libocr_comm_platform_null_la_CFLAGS = $(AM_CFLAGS)

#
## COMP-PLATFORM PTHREAD
#

noinst_LTLIBRARIES += libocr_comp_platform_pthread.la
libocr_la_LIBADD += libocr_comp_platform_pthread.la -lpthread

libocr_comp_platform_pthread_la_SOURCES = \
../../../src/comp-platform/pthread/pthread-comp-platform.c \
../../../src/comp-platform/pthread/thread-binding.c

libocr_comp_platform_pthread_la_CFLAGS = $(AM_CFLAGS)

#
## COMP-TARGET HC
#

noinst_LTLIBRARIES += libocr_comp_target_hc.la
libocr_la_LIBADD += libocr_comp_target_hc.la

libocr_comp_target_hc_la_SOURCES = \
../../../src/comp-target/passthrough/passthrough-comp-target.c

libocr_comp_target_hc_la_CFLAGS = $(AM_CFLAGS)

#
## DATABLOCK
#

noinst_LTLIBRARIES += libocr_datablock_regular.la
libocr_la_LIBADD += libocr_datablock_regular.la

libocr_datablock_regular_la_SOURCES = \
../../../src/datablock/regular/regular-datablock.c

libocr_datablock_regular_la_CFLAGS = $(AM_CFLAGS)

#
## DRIVER
#

noinst_LTLIBRARIES += libocr_driver.la
libocr_la_LIBADD += libocr_driver.la

libocr_driver_la_SOURCES = \
../../../src/driver/ocr-driver.c

libocr_driver_la_CFLAGS = $(AM_CFLAGS) $(INIPARSER_CFLAGS)

#
## SCHEDULER HC
#

noinst_LTLIBRARIES += libocr_event_hc.la
libocr_la_LIBADD += libocr_event_hc.la

libocr_event_hc_la_SOURCES = \
../../../src/event/hc/hc-event.c

libocr_event_hc_la_CFLAGS = $(AM_CFLAGS)

#
## EXTERNAL
#

noinst_LTLIBRARIES += libocr_external.la
libocr_la_LIBADD += libocr_external.la

libocr_external_la_SOURCES = \
../../../src/external/dictionary.c \
../../../src/external/iniparser.c

libocr_external_la_CFLAGS = $(AM_CFLAGS)

#
## GUID
#

noinst_LTLIBRARIES += libocr_guid_ptr.la
libocr_la_LIBADD += libocr_guid_ptr.la

libocr_guid_ptr_la_SOURCES = \
../../../src/guid/ptr/ptr-guid.c

#
## MACHINE-DESCRIPTION
#

noinst_LTLIBRARIES += libocr_machine_description.la
libocr_la_LIBADD += libocr_machine_description.la $(INIPARSER_LIBS)

libocr_machine_description_la_SOURCES = \
../../../src/machine-description/ocr-machine-description.c

libocr_machine_description_la_CFLAGS =  $(INIPARSER_CFLAGS) $(AM_CFLAGS)

#
## MEM-PLATFORM
#

noinst_LTLIBRARIES += libocr_mem_platform_malloc.la
libocr_la_LIBADD += libocr_mem_platform_malloc.la

libocr_mem_platform_malloc_la_SOURCES = \
../../../src/mem-platform/malloc/malloc-mem-platform.c

libocr_mem_platform_malloc_la_CFLAGS = $(AM_CFLAGS)

#
## MEM-TARGET
#

noinst_LTLIBRARIES += libocr_mem_target_shared.la
libocr_la_LIBADD += libocr_mem_target_shared.la

libocr_mem_target_shared_la_SOURCES = \
../../../src/mem-target/shared/shared-mem-target.c

libocr_mem_target_shared_la_CFLAGS = $(AM_CFLAGS)

#
## POLICY-DOMAIN
#

noinst_LTLIBRARIES += libocr_policy_domain.la
libocr_la_LIBADD += libocr_policy_domain.la

libocr_policy_domain_la_SOURCES = ../../../src/policy-domain/ocr-policy-domain.c
libocr_policy_domain_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_policy_domain_hc.la
libocr_la_LIBADD += libocr_policy_domain_hc.la

libocr_policy_domain_hc_la_SOURCES = \
../../../src/policy-domain/hc/hc-policy.c

libocr_policy_domain_hc_la_CFLAGS = $(AM_CFLAGS)

#
## SCHEDULER HC
#

noinst_LTLIBRARIES += libocr_scheduler_hc.la
libocr_la_LIBADD += libocr_scheduler_hc.la

libocr_scheduler_hc_la_SOURCES = \
../../../src/scheduler/hc/hc-scheduler.c

libocr_scheduler_hc_la_CFLAGS = $(AM_CFLAGS)

#
## STATISTICS
#

noinst_LTLIBRARIES += libocr_statistics.la
libocr_la_LIBADD += libocr_statistics.la

libocr_statistics_la_SOURCES = \
../../../src/statistics/ocr-statistics.c \
../../../src/statistics/internal.c \
../../../src/statistics/ocr-statistics-callbacks.c \
../../../src/statistics/stats-llvm-callback.c

libocr_statistics_la_CFLAGS =  $(AM_CFLAGS)

# Default statistics provider
noinst_LTLIBRARIES += libocr_statistics_default.la
libocr_la_LIBADD += libocr_statistics_default.la

libocr_statistics_default_la_SOURCES = \
../../../src/statistics/default/default-statistics.c

libocr_statistics_default_la_CFLAGS = $(AM_CFLAGS)

# Filters
noinst_LTLIBRARIES += libocr_statistics_filters.la
libocr_la_LIBADD += libocr_statistics_filters.la

libocr_statistics_filters_la_SOURCES = \
../../../src/statistics/filters/filter-dump.c       \
../../../src/statistics/filters/filter-trivial.c

libocr_statistics_filters_la_CFLAGS = $(AM_CFLAGS)

# Messages
noinst_LTLIBRARIES += libocr_statistics_messages.la
libocr_la_LIBADD += libocr_statistics_messages.la

libocr_statistics_messages_la_SOURCES = \
../../../src/statistics/messages/message-trivial.c

libocr_statistics_messages_la_CFLAGS = $(AM_CFLAGS)

#
## SYSBOOT
#

noinst_LTLIBRARIES += libocr_sysboot.la
libocr_la_LIBADD += libocr_sysboot.la

libocr_sysboot_la_SOURCES = ../../../src/sysboot/linux-sysboot.c
libocr_sysboot_la_CFLAGS = $(AM_CFLAGS)

#
## TASKS HC
#

noinst_LTLIBRARIES += libocr_task_hc.la
libocr_la_LIBADD += libocr_task_hc.la

libocr_task_hc_la_SOURCES = \
../../../src/task/hc/hc-task.c

libocr_task_hc_la_CFLAGS = $(AM_CFLAGS)

#
## UTILS
#

noinst_LTLIBRARIES += libocr_utils.la
libocr_la_LIBADD += libocr_utils.la -lelf

libocr_utils_la_SOURCES = \
../../../src/utils/deque.c \
../../../src/utils/elf-utils.c \
../../../src/utils/ocr-utils.c \
../../../src/utils/rangeTracker.c


libocr_utils_la_CFLAGS = $(AM_CFLAGS)

#
## WORKER HC
#

noinst_LTLIBRARIES += libocr_worker_hc.la
libocr_la_LIBADD += libocr_worker_hc.la

libocr_worker_hc_la_SOURCES = ../../../src/worker/hc/hc-worker.c
libocr_worker_hc_la_CFLAGS = $(AM_CFLAGS)

#
## WORKPILE HC
#

noinst_LTLIBRARIES += libocr_workpile_hc.la
libocr_la_LIBADD += libocr_workpile_hc.la

libocr_workpile_hc_la_SOURCES = \
../../../src/workpile/hc/hc-workpile.c

libocr_workpile_hc_la_CFLAGS = $(AM_CFLAGS)
