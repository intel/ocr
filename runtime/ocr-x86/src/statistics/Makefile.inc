
noinst_LTLIBRARIES += libocr_statistics.la
libocr_la_LIBADD += libocr_statistics.la $(XML_LIBS)

libocr_statistics_la_SOURCES = \
statistics/ocr-statistics.c \
statistics/ocr-stat-user.c

libocr_statistics_la_CFLAGS =  $(XML_CFLAGS) $(AM_CFLAGS)
