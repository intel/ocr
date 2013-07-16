
noinst_LTLIBRARIES += libocr_worker.la
libocr_la_LIBADD += libocr_worker.la

libocr_worker_la_SOURCES = worker/ocr-worker.c
libocr_worker_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-HC worker implementation
#
noinst_LTLIBRARIES += libocr_worker_hc.la
libocr_la_LIBADD += libocr_worker_hc.la

libocr_worker_hc_la_SOURCES = worker/hc/hc-worker.c
libocr_worker_hc_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-FSIM worker implementation
#
#noinst_LTLIBRARIES += libocr_worker_fsim.la
#libocr_la_LIBADD += libocr_worker_fsim.la

#libocr_worker_fsim_la_SOURCES = worker/fsim/fsim-worker.c
#libocr_worker_fsim_la_CFLAGS = $(AM_CFLAGS)

