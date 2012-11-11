OCR_SRC_LOW_MEMORY=low-memory

noinst_LTLIBRARIES += libocr_low_memory.la
libocr_la_LIBADD += libocr_low_memory.la

libocr_low_memory_la_SOURCES = \
$(OCR_SRC_LOW_MEMORY)/ocr-low-memory.c
libocr_low_memory_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_low_memory_malloc.la
libocr_la_LIBADD += libocr_low_memory_malloc.la

libocr_low_memory_malloc_la_SOURCES = \
$(OCR_SRC_LOW_MEMORY)/malloc/malloc.c
libocr_low_memory_malloc_la_CFLAGS = $(AM_CFLAGS)
