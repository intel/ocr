
noinst_LTLIBRARIES += libocr_sysboot.la
libocr_la_LIBADD += libocr_sysboot.la

libocr_sysboot_la_SOURCES = sysboot/linux-sysboot.c
libocr_sysboot_la_CFLAGS = $(AM_CFLAGS)
