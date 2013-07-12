OCR_SRC_DRIVER=driver

noinst_LTLIBRARIES += libocr_driver.la
libocr_la_LIBADD += libocr_driver.la

libocr_driver_la_SOURCES = \
$(OCR_SRC_DRIVER)/ocr-driver.c
libocr_driver_la_CFLAGS = $(AM_CFLAGS) $(INIPARSER_CFLAGS)
