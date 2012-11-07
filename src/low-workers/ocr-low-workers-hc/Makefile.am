SUBDIRS = 

AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

noinst_LTLIBRARIES = libocr_low_workers_hc.la

libocr_low_workers_hc_la_SOURCES = ocr-low-workers-hc.c

libocr_low_workers_hc_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
