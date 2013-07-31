
noinst_LTLIBRARIES += libocr_statistics.la
libocr_la_LIBADD += libocr_statistics.la

libocr_statistics_la_SOURCES = \
statistics/ocr-statistics.c
#statistics/stats-llvm-callback.c

libocr_statistics_la_CFLAGS =  $(AM_CFLAGS)

# Filters
noinst_LTLIBRARIES += libocr_statistics_filters.la
libocr_la_LIBADD += libocr_statistics_filters.la

libocr_statistics_filters_la_SOURCES = \
statistics/filters/filter-dump.c       \
statistics/filters/filter-trivial.c

# Messages
noinst_LTLIBRARIES += libocr_statistics_messages.la
libocr_la_LIBADD += libocr_statistics_messages.la

libocr_statistics_messages_la_SOURCES = \
statistics/messages/message-trivial.c
