OCR_SRC_DRIVER=driver

noinst_LTLIBRARIES += libocr_driver.la
libocr_la_LIBADD += libocr_driver.la

libocr_driver_la_SOURCES = \
$(OCR_SRC_DRIVER)/ocr-config.c \
$(OCR_SRC_DRIVER)/ocr-driver.c \
$(OCR_SRC_DRIVER)/ocr-runtime-model.c \
$(OCR_SRC_DRIVER)/ocr-models/ocr-model-flat.c \
$(OCR_SRC_DRIVER)/ocr-models/ocr-model-fsim.c  \
$(OCR_SRC_DRIVER)/ocr-models/ocr-model-thor.c
libocr_driver_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-HC driver implementation
#
noinst_LTLIBRARIES += libocr_driver_hc.la
libocr_la_LIBADD += libocr_driver_hc.la

libocr_driver_hc_la_SOURCES = $(OCR_SRC_DRIVER)/ocr-driver-hc/ocr-driver-hc.c
libocr_driver_hc_la_CFLAGS = $(AM_CFLAGS)
