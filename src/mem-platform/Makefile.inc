
noinst_LTLIBRARIES += libocr_mem_platform_malloc.la
libocr_la_LIBADD += libocr_mem_platform_malloc.la

libocr_mem_platform_malloc_la_SOURCES = \
mem-platform/malloc/malloc-mem-platform.c
libocr_mem_platform_malloc_la_CFLAGS = $(AM_CFLAGS)
