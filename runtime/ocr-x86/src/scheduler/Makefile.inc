
#
# OCR-HC scheduler implementation
#
noinst_LTLIBRARIES += libocr_scheduler_hc.la
libocr_la_LIBADD += libocr_scheduler_hc.la

libocr_scheduler_hc_la_SOURCES = \
scheduler/hc/hc-scheduler.c
#scheduler/hc/hc-placed-scheduler.c

libocr_scheduler_hc_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-FSIM scheduler implementation
#
#noinst_LTLIBRARIES += libocr_scheduler_fsim.la
#libocr_la_LIBADD += libocr_scheduler_fsim.la

#libocr_scheduler_fsim_la_SOURCES = \
#scheduler/fsim/fsim-scheduler.c

#libocr_scheduler_fsim_la_CFLAGS = $(AM_CFLAGS)
