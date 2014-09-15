#
# Makefile for the OCR Runtime on the x86-linux platform
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

ifndef ARCH
OCR_TYPE=tg-null-tg-xe
else
ifneq (${OCR_TYPE}, tg-null-tg-xe)
$(error OCR_TYPE is set to ${OCR_TYPE} but expected tg-null-tg-xe)
endif
endif
#
# Tool-chain to be used for the build
#

DEFAULT_CONFIG = xe-default.cfg

TG_INSTALL ?= ../../../ss/install

PREFIX  := ${TG_INSTALL}/bin/rmd-linux-elf-
CC      := $(PREFIX)clang
CFLAGS  := -frmd-extensions -fno-builtin -Wno-typedef-redefinition -I ${TG_INSTALL}/include

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
CFLAGS_STATIC := $(CFLAGS)
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

include ../common.mk
