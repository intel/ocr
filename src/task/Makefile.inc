
#
# OCR-HC scheduler implementation
#
noinst_LTLIBRARIES += libocr_task_hc.la
libocr_la_LIBADD += libocr_task_hc.la

libocr_task_hc_la_SOURCES = \
task/hc/hc-task.c

libocr_task_hc_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-FSIM scheduler implementation
#
noinst_LTLIBRARIES += libocr_task_fsim.la
libocr_la_LIBADD += libocr_task_fsim.la

libocr_task_fsim_la_SOURCES = \
task/fsim/fsim-task.c

libocr_task_fsim_la_CFLAGS = $(AM_CFLAGS)
