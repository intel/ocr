OCR_SRC_SCHEDULER=scheduler

#
# OCR-HC scheduler implementation
#
noinst_LTLIBRARIES += libocr_scheduler_hc.la
libocr_la_LIBADD += libocr_scheduler_hc.la

libocr_scheduler_hc_la_SOURCES = \
$(OCR_SRC_SCHEDULER)/hc/hc-scheduler.c

libocr_scheduler_hc_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-FSIM scheduler implementation
#
#noinst_LTLIBRARIES += libocr_scheduler_fsim.la
#libocr_la_LIBADD += libocr_scheduler_fsim.la

#libocr_scheduler_fsim_la_SOURCES = \
#$(OCR_SRC_SCHEDULER)/fsim/fsim-scheduler.c

#libocr_scheduler_fsim_la_CFLAGS = $(AM_CFLAGS)
