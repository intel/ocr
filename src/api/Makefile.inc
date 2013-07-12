
OCR_SRC_API=api

noinst_LTLIBRARIES += libocr_api.la
libocr_la_LIBADD += libocr_api.la

libocr_api_la_SOURCES = \
$(OCR_SRC_API)/ocr-db.c $(OCR_SRC_API)/ocr-edt.c

libocr_api_la_CFLAGS = $(AM_CFLAGS)
