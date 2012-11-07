SUBDIRS = 

AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

noinst_LTLIBRARIES = libocr_scheduler_hc.la

libocr_scheduler_hc_la_SOURCES = ocr-scheduler-hc.c ocr-edf-hc.c deque.c

libocr_scheduler_hc_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
