#
# Makefile for the OCR Runtime on the x86-linux platform
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

ifndef ARCH
ARCH=x86-builder-fsim-ce
else
ifneq (${ARCH}, x86-builder-fsim-ce)
$(error ARCH is set to ${ARCH} but expected x86-builder-fsim-ce)
endif
endif
#
# Tool-chain to be used for the build
#

CC      := gcc
CFLAGS  := -fno-builtin

RM      := rm
RMFLAGS := -rf

CP      := cp
MKDIR   := mkdir
LN      := ln

# Shared libraries specific builds
LDFLAGS := -shared -fpic

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
SUPPORTS_SHARED=no
SUPPORTS_STATIC=yes


# For Valgrind, uncomment this line
CFLAGS += -I/usr/include -DENABLE_VALGRIND


include ../common.mk

