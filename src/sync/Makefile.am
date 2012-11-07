SUBDIRS =

AM_CFLAGS = -g -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc

# headers to include when installing
pkginclude_HEADERS =

# Libraries to build
noinst_LTLIBRARIES = libocrsync.la

# files part of the library
libocrsync_la_SOURCES = ocr-sync-pthread/ocr-sync.c

# Add specific CFLAGS for building the runtime library
libocrsync_la_CFLAGS = $(AM_CFLAGS)

# Add specific LDFLAGS when linking the runtime library

# set library revision
libocrsync_la_LDFLAGS = -version-info 0:0:0

MAINTAINERCLEANFILES = Makefile.in
