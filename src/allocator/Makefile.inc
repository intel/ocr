OCR_SRC_ALLOCATOR=allocator

noinst_LTLIBRARIES += libocr_allocator.la
libocr_la_LIBADD += libocr_allocator.la

libocr_allocator_la_SOURCES = \
$(OCR_SRC_ALLOCATOR)/ocr-allocator.c
libocr_allocator_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_allocator_tlsf.la
libocr_la_LIBADD += libocr_allocator_tlsf.la

libocr_allocator_tlsf_la_SOURCES = \
$(OCR_SRC_ALLOCATOR)/tlsf/tlsf-allocator.c
libocr_allocator_tlsf_la_CFLAGS = $(AM_CFLAGS)
