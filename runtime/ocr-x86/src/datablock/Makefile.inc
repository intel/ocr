
noinst_LTLIBRARIES += libocr_datablock_regular.la
libocr_la_LIBADD += libocr_datablock_regular.la

libocr_datablock_regular_la_SOURCES = \
datablock/regular/regular-datablock.c
libocr_datablock_regular_la_CFLAGS = $(AM_CFLAGS)
