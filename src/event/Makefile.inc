
#
# OCR-HC scheduler implementation
#
noinst_LTLIBRARIES += libocr_event_hc.la
libocr_la_LIBADD += libocr_event_hc.la

libocr_event_hc_la_SOURCES = \
event/hc/hc-event.c

libocr_event_hc_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-FSIM scheduler implementation
#
#noinst_LTLIBRARIES += libocr_event_fsim.la
#libocr_la_LIBADD += libocr_event_fsim.la

#libocr_event_fsim_la_SOURCES = \
event/fsim/fsim-event.c

#libocr_event_fsim_la_CFLAGS = $(AM_CFLAGS)
