
noinst_LTLIBRARIES += libocr_utils.la
libocr_la_LIBADD += libocr_utils.la -lelf

libocr_utils_la_SOURCES = \
utils/ocr-utils.c \
utils/rangeTracker.c \
utils/elf-utils.c

libocr_utils_la_CFLAGS = $(AM_CFLAGS)
