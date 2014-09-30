#
# Makefile for the OCR Runtime on the x86-linux platform
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

ifndef OCR_TYPE
OCR_TYPE=x86-pthread-x86
else
ifneq (${OCR_TYPE}, x86-pthread-x86)
$(error OCR_TYPE is set to ${OCR_TYPE} but expected x86-pthread-x86)
endif
endif

DEFAULT_CONFIG=mach-hc-4w.cfg

#
# Tool-chain to be used for the build
#

CC      := gcc
CFLAGS  :=

RM      := rm
RMFLAGS := -rf

CP      := cp
MKDIR   := mkdir
LN      := ln

# Shared libraries specific builds
LDFLAGS := -shared -fpic -lpthread

# CFLAGS_SHARED will be concatenated with any
# common CFLAGS options
CFLAGS_SHARED := -fpic

# Static libraries specific builds
# Same as for CFLAGS_SHARED
CFLAGS_STATIC :=
AR := ar
ARFLAGS := cru

RANLIB := ranlib

# Library supported
SUPPORTS_SHARED=yes
SUPPORTS_STATIC=yes
OCRRUNNER=ocrrun

# For Valgrind, uncomment this line
#CFLAGS += -I/usr/include -DENABLE_VALGRIND #-DELS_USER_SIZE=2

# For runtime-overhead profiling
#CFLAGS += -DOCR_RUNTIME_PROFILER -DPROFILE_KHZ=3400000

.PHONY: all
all: static shared

.PHONY: debug
debug: debug-static debug-shared

include ../common.mk
