
#
# NULL comm-platform implementation
#
noinst_LTLIBRARIES += libocr_comm_platform_null.la
libocr_la_LIBADD += libocr_comm_platform_null.la

libocr_comm_platform_null_la_SOURCES = comm-platform/null/null-comm-platform.c

libocr_comm_platform_null_la_CFLAGS = $(AM_CFLAGS)
