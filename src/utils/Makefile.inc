
OCR_SRC_UTILS=utils

noinst_LTLIBRARIES += libocr_utils.la
libocr_la_LIBADD += libocr_utils.la

libocr_utils_la_SOURCES = \
$(OCR_SRC_UTILS)/ocr-utils.c

libocr_utils_la_CFLAGS = $(AM_CFLAGS)
