SUBDIRS = ocr-driver-hc

AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

noinst_LTLIBRARIES = libocr_driver.la

libocr_driver_la_SOURCES = ocr-driver.c ocr-factory.c ocr-runtime-model.c

libocr_driver_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
