#
# Makefile for the OCR Runtime on the x86-linux platform
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

ifndef ARCH
ARCH=fsim-null-fsim-ce
else
ifneq (${ARCH}, fsim-null-fsim-ce)
$(error ARCH is set to ${ARCH} but expected fsim-null-fsim-ce)
endif
endif
#
# Tool-chain to be used for the build
#

RMD_INSTALL ?= ../../../ss/install

#DEFAULT_CONFIG = ce-default.cfg      #Switch to this configuration file to go back to a single 256K memory allocator.
DEFAULT_CONFIG = ce-multialloc-default.cfg


CC      := gcc
CFLAGS  := -fno-builtin -fno-stack-protector -Wno-unused-local-typedefs -I $(RMD_INSTALL)/include

RM      := rm
RMFLAGS := -rf

CP      := cp
MKDIR   := mkdir
LN      := ln

# Shared libraries specific builds
LDFLAGS :=

# CFLAGS_SHARED will be concatenated with any
# common CFLAGS options
CFLAGS_SHARED :=

# Static libraries specific builds
# Same as for CFLAGS_SHARED
CFLAGS_STATIC :=
AR := ar
ARFLAGS := cru

RANLIB := ranlib

# Library supported
SUPPORTS_SHARED=no
SUPPORTS_STATIC=yes

.PHONY: all
all: static

.PHONY: debug
debug: debug-static

include ../common.mk
