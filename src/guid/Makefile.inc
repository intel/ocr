OCR_SRC_GUID=guid

noinst_LTLIBRARIES += libocr_guid.la
libocr_la_LIBADD += libocr_guid.la

libocr_guid_la_SOURCES = \
$(OCR_SRC_GUID)/ocr-function-id.c \
$(OCR_SRC_GUID)/ocr-guid.c

libocr_guid_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_guid_ptr.la
libocr_la_LIBADD += libocr_guid_ptr.la

libocr_guid_ptr_la_SOURCES = \
$(OCR_SRC_GUID)/ptr/ptr.c
