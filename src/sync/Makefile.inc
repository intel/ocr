OCR_SRC_SYNC=sync

noinst_LTLIBRARIES += libocr_sync.la
libocr_la_LIBADD += libocr_sync.la

libocr_sync_la_SOURCES = \
$(OCR_SRC_SYNC)/ocr-sync.c

libocr_sync_la_CFLAGS = $(AM_CFLAGS)

#TODO this should be conditionally built
noinst_LTLIBRARIES += libocr_sync_x86.la
libocr_la_LIBADD += libocr_sync_x86.la

libocr_sync_x86_la_SOURCES = \
$(OCR_SRC_SYNC)/x86/x86.c
