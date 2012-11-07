SUBDIRS = 

AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

noinst_LTLIBRARIES = libocr_guid.la

libocr_guid_la_SOURCES = ocr-function-id.c ocr-guid.c

libocr_guid_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
