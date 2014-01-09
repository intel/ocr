
noinst_LTLIBRARIES += libocr_sal.la
libocr_la_LIBADD += libocr_sal.la

libocr_sal_la_SOURCES = sal/ocr-sal.c
libocr_sal_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_sal_linux.la
libocr_la_LIBADD += libocr_sal_linux.la

libocr_sal_linux_la_SOURCES = \
sal/linux/sal-linux.c

# bypass -Werror from top-level makefile to pass 
# compilation on 32 bits systems
libocr_sal_linux_la_CFLAGS = $(OCR_AM_CFLAGS)
