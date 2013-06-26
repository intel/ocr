OCR_SRC_EDT=edt

#
# OCR-HC scheduler implementation
#
noinst_LTLIBRARIES += libocr_edt_hc.la
libocr_la_LIBADD += libocr_edt_hc.la

libocr_edt_hc_la_SOURCES = \
$(OCR_SRC_EDT)/ocr-edt-hc/ocr-edf-hc.c

libocr_edt_hc_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-FSIM scheduler implementation
#
#noinst_LTLIBRARIES += libocr_edt_fsim.la
#libocr_la_LIBADD += libocr_edt_fsim.la

#libocr_edt_fsim_la_SOURCES = \
#$(OCR_SRC_EDT)/ocr-edt-fsim/ocr-task-event-fsim.c

#libocr_edt_fsim_la_CFLAGS = $(AM_CFLAGS)
