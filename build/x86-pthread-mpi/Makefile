#
# Makefile for the OCR Runtime on the x86-linux platform
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

ifndef OCR_TYPE
OCR_TYPE=x86-pthread-mpi
else
ifneq (${OCR_TYPE}, x86-pthread-mpi)
$(error OCR_TYPE is set to ${OCR_TYPE} but expected x86-pthread-mpi)
endif
endif

DEFAULT_CONFIG=mach-hc-dist-mpi-clone-8w-lockable.cfg

#
# Tool-chain to be used for the build
#

CC      := mpicc
CFLAGS  := -DINIT_DEQUE_CAPACITY=2048

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
#CFLAGS += -I/usr/include -DENABLE_VALGRIND

# if MPI_ROOT is set, use the specified mpi install
ifdef MPI_ROOT
# Name of the MPI shared library to link with
ifndef MPI_LIB
MPI_LIB = mpi
endif
CFLAGS += -I${MPI_ROOT}/include
LDFLAGS += -L${MPI_ROOT}/lib -l${MPI_LIB}
endif

.PHONY: all
all: static shared

.PHONY: debug
debug: debug-static debug-shared

include ../common.mk
