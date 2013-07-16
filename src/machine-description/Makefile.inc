
noinst_LTLIBRARIES += libocr_machine_description.la
libocr_la_LIBADD += libocr_machine_description.la $(INIPARSER_LIBS)

libocr_machine_description_la_SOURCES = \
machine-description/ocr-machine-description.c

libocr_machine_description_la_CFLAGS =  $(INIPARSER_CFLAGS) $(AM_CFLAGS)
