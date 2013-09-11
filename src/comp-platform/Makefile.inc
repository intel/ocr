
noinst_LTLIBRARIES += libocr_comp_platform.la
libocr_la_LIBADD += libocr_comp_platform.la

libocr_comp_platform_la_SOURCES = comp-platform/ocr-comp-platform.c
libocr_comp_platform_la_CFLAGS = $(AM_CFLAGS)

#
# Pthread comp-platform implementation
#
noinst_LTLIBRARIES += libocr_comp_platform_pthread.la
libocr_la_LIBADD += libocr_comp_platform_pthread.la -lpthread

libocr_comp_platform_pthread_la_SOURCES = comp-platform/pthread/pthread-comp-platform.c
libocr_comp_platform_pthread_la_CFLAGS = $(AM_CFLAGS)
