
noinst_LTLIBRARIES += libocr_statistics.la
libocr_la_LIBADD += libocr_statistics.la

libocr_statistics_la_SOURCES = \
statistics/ocr-statistics.c \
statistics/internal.c \
statistics/stats-llvm-callback.c

libocr_statistics_la_CFLAGS =  $(AM_CFLAGS)

# Default statistics provider
noinst_LTLIBRARIES += libocr_statistics_default.la
libocr_la_LIBADD += libocr_statistics_default.la

libocr_statistics_default_la_SOURCES = \
statistics/default/default-statistics.c

libocr_statistics_default_la_CFLAGS = $(AM_CFLAGS)

# Filters
noinst_LTLIBRARIES += libocr_statistics_filters.la
libocr_la_LIBADD += libocr_statistics_filters.la

libocr_statistics_filters_la_SOURCES = \
statistics/filters/filter-dump.c       \
statistics/filters/filter-trivial.c

libocr_statistics_filters_la_CFLAGS = $(AM_CFLAGS)

# Messages
noinst_LTLIBRARIES += libocr_statistics_messages.la
libocr_la_LIBADD += libocr_statistics_messages.la

libocr_statistics_messages_la_SOURCES = \
statistics/messages/message-trivial.c

libocr_statistics_messages_la_CFLAGS = $(AM_CFLAGS)
