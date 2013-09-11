
noinst_LTLIBRARIES += libocr_mem_target_shared.la
libocr_la_LIBADD += libocr_mem_target_shared.la

libocr_mem_target_shared_la_SOURCES = \
mem-target/shared/shared-mem-target.c
libocr_mem_target_shared_la_CFLAGS = $(AM_CFLAGS)
