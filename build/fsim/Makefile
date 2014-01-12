#
# Makefile for the OCR Runtime on the x86-linux platform
#
# For OCR licensing terms, see ../runtime/LICENSE.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

#
# Tool-chain to be used for the build
#

PREFIX  := ../../../ss/bin/bin/rmd-linux-elf-
CC      := $(PREFIX)clang
CFLAGS  := -frmd-extensions -fno-builtin

AR      := $(PREFIX)ar
ARFLAGS := cru

RANLIB  := $(PREFIX)ranlib

RM      := rm
RMFLAGS := -rf


include ../common.mk
