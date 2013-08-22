SUBDIRS =

# Includes folder we need when compiling ocr
OCR_INCLUDES_DIR = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -I$(top_srcdir)/src/inc/fsim -I$(top_srcdir)/src/machine-description/iniparser/src

# cflags: important to define that otherwise we inherit default values too
CFLAGS = -Wall -g -O0
#CFLAGS +=-DOCR_ENABLE_STATISTICS

# Debug flags

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
CFLAGS += -DOCR_DEBUG_STATS -DDEBUG_LVL_STATS=DEBUG_LVL_VVERB
CFLAGS += -DOCR_DEBUG_SYNC
CFLAGS += -DOCR_DEBUG_TASK
CFLAGS += -DOCR_DEBUG_WORKER
CFLAGS += -DOCR_DEBUG_WORKPILE

# Enable statistics with this
CFLAGS += -DOCR_ENABLE_STATISTICS

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

include allocator/Makefile.inc
include api/Makefile.inc
include comp-platform/Makefile.inc
include comp-target/Makefile.inc
include datablock/Makefile.inc
include driver/Makefile.inc
include event/Makefile.inc
include guid/Makefile.inc
include machine-description/Makefile.inc
include mem-platform/Makefile.inc
include mem-target/Makefile.inc
include policy-domain/Makefile.inc
include scheduler/Makefile.inc
include statistics/Makefile.inc
include sync/Makefile.inc
include task/Makefile.inc
include utils/Makefile.inc
include worker/Makefile.inc
include workpile/Makefile.inc

# build the ocr runtime library
libocr_la_SOURCES =
libocr_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
