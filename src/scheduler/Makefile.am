SUBDIRS = ocr-scheduler-hc

AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

noinst_LTLIBRARIES = libocr_scheduler.la

libocr_scheduler_la_SOURCES = ocr-scheduler.c

libocr_scheduler_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
