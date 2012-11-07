SUBDIRS = allocator driver executor guid low-memory low-workers machine policy scheduler utils
# disable sync because its implemented in cpp
#SUBDIRS = sync
 
AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

lib_LTLIBRARIES = libocr.la

# build the ocr runtime library
libocr_la_SOURCES = ocr-runtime.c ocr-edt.c
libocr_la_CFLAGS = $(AM_CFLAGS)
libocr_la_LIBADD = \
driver/libocr_driver.la \
executor/libocr_executor.la \
guid/libocr_guid.la \
low-workers/libocr_low_workers.la \
policy/libocr_policy.la \
scheduler/libocr_scheduler.la \
executor/ocr-executor-pthread/libocr_executor_pthread.la \
driver/ocr-driver-hc/libocr_driver_hc.la \
low-workers/ocr-low-workers-hc/libocr_low_workers_hc.la \
scheduler/ocr-scheduler-hc/libocr_scheduler_hc.la \
-lpthread


MAINTAINERCLEANFILES = Makefile.in
