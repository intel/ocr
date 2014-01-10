
noinst_LTLIBRARIES += libocr_sal.la
libocr_la_LIBADD += libocr_sal.la

libocr_sal_la_SOURCES = sal/ocr-sal.c
libocr_sal_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_sal_linux.la
libocr_la_LIBADD += libocr_sal_linux.la

libocr_sal_linux_la_SOURCES = sal/linux/linux-sal.c

libocr_sal_linux_la_CFLAGS = $(AM_CFLAGS)
