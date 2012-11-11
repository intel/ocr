OCR_SRC_DATABLOCK=datablock

noinst_LTLIBRARIES += libocr_datablock.la
libocr_la_LIBADD += libocr_datablock.la

libocr_datablock_la_SOURCES = \
$(OCR_SRC_DATABLOCK)/ocr-datablock.c
libocr_datablock_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_datablock_regular.la
libocr_la_LIBADD += libocr_datablock_regular.la

libocr_datablock_regular_la_SOURCES = \
$(OCR_SRC_DATABLOCK)/regular/regular.c
libocr_datablock_regular_la_CFLAGS = $(AM_CFLAGS)
