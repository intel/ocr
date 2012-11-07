SUBDIRS = 

AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

noinst_LTLIBRARIES = libocr_executor_pthread.la

libocr_executor_pthread_la_SOURCES = ocr-executor-pthread.c

libocr_executor_pthread_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
