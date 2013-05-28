OCR_SRC_MACHINE_DESCRIPTION=machine-description

noinst_LTLIBRARIES += libocr_machine_description.la
libocr_la_LIBADD += libocr_machine_description.la $(XML_LIBS)

libocr_machine_description_la_SOURCES = \
$(OCR_SRC_MACHINE_DESCRIPTION)/ocr-machine-description.c

libocr_machine_description_la_CFLAGS =  $(XML_CFLAGS) $(AM_CFLAGS)
