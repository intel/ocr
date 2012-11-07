SUBDIRS = ocr-low-workers-hc

AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

noinst_LTLIBRARIES = libocr_low_workers.la

libocr_low_workers_la_SOURCES = ocr-low-workers.c

libocr_low_workers_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
