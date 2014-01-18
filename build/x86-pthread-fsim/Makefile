#
# Makefile for the OCR Runtime on the x86-linux platform
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

#
# Tool-chain to be used for the build
#

CC      := gcc
CFLAGS  :=  

AR      := ar
ARFLAGS := cru

RANLIB  := ranlib

RM      := rm
RMFLAGS := -rf

CP      := cp

# Enable this if you want valgrind for allocators
CFLAGS += -DENABLE_VALGRIND

include ../common.mk
