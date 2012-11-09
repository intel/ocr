SUBDIRS = allocator driver executor guid low-memory low-workers machine policy scheduler utils datablock
# disable sync because its implemented in cpp
#SUBDIRS = sync

AM_CFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(top_srcdir)/src/inc/hc -Werror -Wall -g

lib_LTLIBRARIES = libocr.la

# build the ocr runtime library
libocr_la_SOURCES = ocr-runtime.c ocr-edt.c
libocr_la_CFLAGS = $(AM_CFLAGS)
libocr_la_LIBADD = \
allocator/libocr_allocator.la \
allocator/tlsf/libocr_allocator_tlsf.la \
datablock/libocr_datablock.la \
datablock/regular/libocr_datablock_regular.la \
driver/libocr_driver.la \
driver/ocr-driver-hc/libocr_driver_hc.la \
executor/libocr_executor.la \
executor/ocr-executor-pthread/libocr_executor_pthread.la \
guid/libocr_guid.la \
low-memory/libocr_low_memory.la \
low-memory/malloc/libocr_low_memory_malloc.la \
low-workers/libocr_low_workers.la \
low-workers/ocr-low-workers-hc/libocr_low_workers_hc.la \
policy/libocr_policy.la \
scheduler/libocr_scheduler.la \
scheduler/ocr-scheduler-hc/libocr_scheduler_hc.la \
utils/libocr_utils.la \
-lpthread


MAINTAINERCLEANFILES = Makefile.in
