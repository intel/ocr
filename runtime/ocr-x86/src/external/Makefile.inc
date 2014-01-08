
noinst_LTLIBRARIES += libocr_external.la
libocr_la_LIBADD += libocr_external.la

libocr_external_la_SOURCES = \
external/dictionary.c \
external/iniparser.c

libocr_external_la_CFLAGS = $(AM_CFLAGS)
