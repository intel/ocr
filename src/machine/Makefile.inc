OCR_SRC_MACHINE=machine

noinst_LTLIBRARIES += libocr_machine.la
libocr_la_LIBADD += libocr_machine.la $(XML_LIBS)

libocr_machine_la_SOURCES = \
$(OCR_SRC_MACHINE)/ocr-machine.c

libocr_machine_la_CFLAGS =  $(XML_CFLAGS) $(AM_CFLAGS)
