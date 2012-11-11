SUBDIRS = 
 
AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

lib_LTLIBRARIES = libocr.la

# These are populated in sub-folder's Makefile.inc
noinst_LTLIBRARIES =
libocr_la_LIBADD =

include allocator/Makefile.inc
include driver/Makefile.inc
include executor/Makefile.inc
include guid/Makefile.inc
include low-workers/Makefile.inc
include machine/Makefile.inc
include policy/Makefile.inc
include scheduler/Makefile.inc
include utils/Makefile.inc
include workpile/Makefile.inc

# build the ocr runtime library
libocr_la_SOURCES = ocr-runtime.c ocr-edt.c
libocr_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
