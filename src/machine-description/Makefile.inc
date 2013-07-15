OCR_SRC_MACHINE_DESCRIPTION=machine-description

noinst_LTLIBRARIES += libocr_machine_description.la
libocr_la_LIBADD += libocr_machine_description.la $(INIPARSER_LIBS)

libocr_machine_description_la_SOURCES = \
$(OCR_SRC_MACHINE_DESCRIPTION)/ocr-machine-description.c

libocr_machine_description_la_CFLAGS =  $(INIPARSER_CFLAGS) $(AM_CFLAGS)

# install config files

libocr_machine_description_ladir = $(prefix)/config

libocr_machine_description_la_DATA = $(OCR_SRC_MACHINE_DESCRIPTION)/default.cfg

 
