OCR_SRC_LOW_WORKERS=low-workers

noinst_LTLIBRARIES += libocr_low_workers.la
libocr_la_LIBADD += libocr_low_workers.la

libocr_low_workers_la_SOURCES = $(OCR_SRC_LOW_WORKERS)/ocr-low-workers.c
libocr_low_workers_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-HC low-worker implementation
#
noinst_LTLIBRARIES += libocr_low_workers_hc.la
libocr_la_LIBADD += libocr_low_workers_hc.la

libocr_low_workers_hc_la_SOURCES = $(OCR_SRC_LOW_WORKERS)/ocr-low-workers-hc/ocr-low-workers-hc.c
libocr_low_workers_hc_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-FSIM low-worker implementation
#
noinst_LTLIBRARIES += libocr_low_workers_fsim.la
libocr_la_LIBADD += libocr_low_workers_fsim.la

libocr_low_workers_fsim_la_SOURCES = $(OCR_SRC_LOW_WORKERS)/ocr-low-workers-fsim/ocr-low-workers-fsim.c
libocr_low_workers_fsim_la_CFLAGS = $(AM_CFLAGS)

