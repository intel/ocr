#
# Makefile for the toolchain checker
#
# For OCR licensing terms, see top level LICENSE file.
#

CFLAGS     := -w -DOCR_DEBUG -DELS_USER_SIZE=0 -I../../inc -I../../src -I../../src/inc

CE_CC      := gcc
CE_CFLAGS  := $(CFLAGS) -fno-builtin -I ../../../ss/common/include -I../../build/fsim-null-fsim-ce

PREFIX  := ../../../ss/install/bin/rmd-linux-elf-
XE_CC      := $(PREFIX)clang
XE_CFLAGS  := $(CFLAGS) -frmd-extensions -fno-builtin -I ../../../ss/common/include -I../../build/fsim-null-fsim-xe

OBJDIR     := objs

.PHONY: all
all: check

check: obj-xe obj-ce
	od -x -v $(OBJDIR)/test.xe.data.o > $(OBJDIR)/xe
	od -x -v $(OBJDIR)/test.ce.data.o > $(OBJDIR)/ce
	diff $(OBJDIR)/xe $(OBJDIR)/ce

obj-ce:
	$(CE_CC) $(CE_CFLAGS) -c -g test.c -o $(OBJDIR)/test.ce.o
	objcopy -O binary -j .data $(OBJDIR)/test.ce.o $(OBJDIR)/test.ce.data.o

obj-xe:
	$(XE_CC) $(XE_CFLAGS) -c -g test.c -o $(OBJDIR)/test.xe.o
	$(PREFIX)objcopy -O binary -j .data_local $(OBJDIR)/test.xe.o $(OBJDIR)/test.xe.data.o

clean:
	rm -rf objs/*
