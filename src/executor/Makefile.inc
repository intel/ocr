OCR_SRC_EXECUTOR=executor

noinst_LTLIBRARIES += libocr_executor.la
libocr_la_LIBADD += libocr_executor.la

libocr_executor_la_SOURCES = $(OCR_SRC_EXECUTOR)/ocr-executor.c
libocr_executor_la_CFLAGS = $(AM_CFLAGS)

#
# Pthread executor implementation
#
noinst_LTLIBRARIES += libocr_executor_pthread.la
libocr_la_LIBADD += libocr_executor_pthread.la -lpthread

libocr_executor_pthread_la_SOURCES = $(OCR_SRC_EXECUTOR)/ocr-executor-pthread/ocr-executor-pthread.c
libocr_executor_pthread_la_CFLAGS = $(AM_CFLAGS)
