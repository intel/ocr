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

RMD_INSTALL ?= ../../../ss/install

CC      := gcc
CFLAGS  := -fno-builtin -fno-stack-protector -Wno-unused-local-typedefs -I ${RMD_INSTALL}/include

RM      := rm
RMFLAGS := -rf

CP      := cp
MKDIR   := mkdir
LN      := ln

EXEFLAGS := -lelf -lpthread

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

CFLAGS_EXEC :=

RANLIB := ranlib

# Library supported
SUPPORTS_SHARED=no
SUPPORTS_STATIC=no
SUPPORTS_EXEC=yes


# For Valgrind, uncomment this line
#CFLAGS += -I/usr/include -DENABLE_VALGRIND


.PHONY: all
all: exec

.PHONY: debug
debug: debug-exec

include ../common.mk
