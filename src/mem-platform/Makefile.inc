OCR_SRC_MEM_PLATFORM=mem-platform

noinst_LTLIBRARIES += libocr_mem_platform.la
libocr_la_LIBADD += libocr_mem_platform.la

libocr_mem_platform_la_SOURCES =
libocr_mem_platform_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_mem_platform_malloc.la
libocr_la_LIBADD += libocr_mem_platform_malloc.la

libocr_mem_platform_malloc_la_SOURCES = \
$(OCR_SRC_MEM_PLATFORM)/malloc/malloc-mem-platform.c
libocr_mem_platform_malloc_la_CFLAGS = $(AM_CFLAGS)
