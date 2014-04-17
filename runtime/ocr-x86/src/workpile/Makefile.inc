
noinst_LTLIBRARIES += libocr_workpile.la
libocr_la_LIBADD += libocr_workpile.la

libocr_workpile_la_SOURCES = \
workpile/ocr-workpile.c

libocr_workpile_la_CFLAGS = $(AM_CFLAGS)


#
# OCR-HC workpile implementation
#
noinst_LTLIBRARIES += libocr_workpile_hc.la
libocr_la_LIBADD += libocr_workpile_hc.la

libocr_workpile_hc_la_SOURCES = \
workpile/hc/hc-workpile.c \
workpile/hc/deque.c

libocr_workpile_hc_la_CFLAGS = $(AM_CFLAGS)#

# OCR-FSIM workpile implementation
#
noinst_LTLIBRARIES += libocr_workpile_fsim.la
libocr_la_LIBADD += libocr_workpile_fsim.la

libocr_workpile_fsim_la_SOURCES = \
workpile/fsim/fsim-workpile.c

libocr_workpile_fsim_la_CFLAGS = $(AM_CFLAGS)
