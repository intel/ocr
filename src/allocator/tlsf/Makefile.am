SUBDIRS =

AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -Werror -Wall -g

noinst_LTLIBRARIES = libocr_allocator_tlsf.la

libocr_allocator_tlsf_la_SOURCES = tlsf-allocator.c

libocr_allocator_tlsf_la_CFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in
