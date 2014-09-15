#
# Makefile for the OCR Runtime on the x86-linux platform
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

ifndef ARCH
ARCH=x86-pthread-gasnet
else
ifneq (${ARCH}, x86-pthread-gasnet)
$(error ARCH is set to ${ARCH} but expected x86-pthread-gasnet)
endif
endif
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
SUPPORTS_SHARED=no
SUPPORTS_STATIC=yes

# For Valgrind, uncomment this line
#CFLAGS += -I/usr/include -DENABLE_VALGRIND

ifndef GASNET_ROOT
	$(error GASNET_ROOT not specified)
endif
ifndef GASNET_CONDUIT
	$(error GASNET_CONDUIT not specified)
endif
ifndef GASNET_TYPE
	$(error GASNET_TYPE not specified)
endif
# Used by ocrTests to link properly with gasnet
ifndef OCR_LDFLAGS
	$(error OCR_LDFLAGS not specified)
endif

CFLAGS += ${GASNET_CFLAGS} -I${GASNET_ROOT}/include -I${GASNET_ROOT}/include/${GASNET_CONDUIT}-conduit
# LIBS_STATIC += ${GASNET_ROOT}/lib/libgasnet-${GASNET_CONDUIT}-${GASNET_TYPE}.a

.PHONY: all
all: static

include ../common.mk

