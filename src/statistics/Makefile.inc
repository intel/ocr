OCR_SRC_STATISTICS=statistics

noinst_LTLIBRARIES += libocr_statistics.la
libocr_la_LIBADD += libocr_statistics.la $(XML_LIBS)

libocr_statistics_la_SOURCES = \
$(OCR_SRC_STATISTICS)/ocr-statistics.c

libocr_statistics_la_CFLAGS =  $(XML_CFLAGS) $(AM_CFLAGS)
