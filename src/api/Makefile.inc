
noinst_LTLIBRARIES += libocr_api.la
libocr_la_LIBADD += libocr_api.la

libocr_api_la_SOURCES = \
api/ocr.c \
api/ocr-db.c \
api/ocr-edt.c \
api/ocr-lib.c

libocr_api_la_CFLAGS = $(AM_CFLAGS)
