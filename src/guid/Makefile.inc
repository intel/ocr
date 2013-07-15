OCR_SRC_GUID=guid

noinst_LTLIBRARIES += libocr_guid_ptr.la
libocr_la_LIBADD += libocr_guid_ptr.la

libocr_guid_ptr_la_SOURCES = \
$(OCR_SRC_GUID)/ptr/ptr-guid.c
