OCR_SRC_MEM_TARGET=mem-target

noinst_LTLIBRARIES += libocr_mem_target.la
libocr_la_LIBADD += libocr_mem_target.la

libocr_mem_target_la_SOURCES = \
$(OCR_SRC_MEM_TARGET)/ocr-mem-target.c

libocr_mem_target_la_CFLAGS = $(AM_CFLAGS)
