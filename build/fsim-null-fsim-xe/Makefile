#
# Makefile for the OCR Runtime on the x86-linux platform
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

ifndef ARCH
ARCH=fsim-null-fsim-xe
else
ifneq (${ARCH}, fsim-null-fsim-xe)
$(error ARCH is set to ${ARCH} but expected fsim-null-fsim-xe)
endif
endif
#
# Tool-chain to be used for the build
#

RMD_INSTALL ?= ../../../ss/install

PREFIX  := ${RMD_INSTALL}/bin/rmd-linux-elf-
CC      := $(PREFIX)clang
CFLAGS  := -frmd-extensions -fno-builtin -I ${RMD_INSTALL}/include

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
AR := $(PREFIX)ar
ARFLAGS := cru

RANLIB := $(PREFIX)ranlib

# Library supported
SUPPORTS_SHARED=no
SUPPORTS_STATIC=yes

.PHONY: all
all: static

.PHONY: debug
debug: debug-static

DEFAULT_CONFIG = fsim_xe.cfg

include ../common.mk
