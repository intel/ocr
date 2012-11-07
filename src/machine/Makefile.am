SUBDIRS = 

AM_CFLAGS = $(XML_CFLAGS) -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

AM_LDFLAGS = $(XML_LIBS)

noinst_LTLIBRARIES = libocr_machine.la

libocr_machine_la_SOURCES = ocr-machine.c

libocr_machine_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
