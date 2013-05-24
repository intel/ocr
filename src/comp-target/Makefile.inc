OCR_SRC_COMP_TARGET=comp-target

noinst_LTLIBRARIES += libocr_comp_target.la
libocr_la_LIBADD += libocr_comp_target.la

libocr_comp_target_la_SOURCES = $(OCR_SRC_COMP_TARGET)/ocr-comp-target.c
libocr_comp_target_la_CFLAGS = $(AM_CFLAGS)

#
# hc comp-target implementation
#
noinst_LTLIBRARIES += libocr_comp_target_hc.la
libocr_la_LIBADD += libocr_comp_target_hc.la

libocr_comp_target_hc_la_SOURCES = $(OCR_SRC_COMP_TARGET)/ocr-comp-target-hc/ocr-comp-target-hc.c
libocr_comp_target_hc_la_CFLAGS = $(AM_CFLAGS)
