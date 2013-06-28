OCR_SRC_EVENT=event

#
# OCR-HC scheduler implementation
#
noinst_LTLIBRARIES += libocr_event_hc.la
libocr_la_LIBADD += libocr_event_hc.la

libocr_event_hc_la_SOURCES = \
$(OCR_SRC_EVENT)/hc/hc-event.c

libocr_even_hc_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-FSIM scheduler implementation
#
#noinst_LTLIBRARIES += libocr_event_fsim.la
#libocr_la_LIBADD += libocr_event_fsim.la

#libocr_event_fsim_la_SOURCES = \
#$(OCR_SRC_EVENT)/fsim/fsim-event.c

#libocr_event_fsim_la_CFLAGS = $(AM_CFLAGS)
