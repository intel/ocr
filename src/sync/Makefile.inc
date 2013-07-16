
libocr_sync_la_CFLAGS = $(AM_CFLAGS)

#TODO this should be conditionally built
noinst_LTLIBRARIES += libocr_sync_x86.la
libocr_la_LIBADD += libocr_sync_x86.la

libocr_sync_x86_la_SOURCES = \
sync/x86/x86-sync.c
