
SUBDIRS = machine-configs src

AUX_DIST = \
	$(ac_aux_dir)/config.guess		\
	$(ac_aux_dir)/config.sub		\
	$(ac_aux_dir)/install-sh		\
	$(ac_aux_dir)/ltmain.sh			\
	$(ac_aux_dir)/missing			\
	$(ac_aux_dir)/depcomp

library_includedir=$(includedir)
library_include_HEADERS = inc/ocr-db.h inc/ocr-edt.h inc/ocr-tuning.h \
inc/ocr-types.h inc/ocr.h inc/compat.h inc/ocr-stathooks.h

# Distribute runtime interface headers - set by configure
if INCLUDE_RUNTIME_ITF_HEADERS
library_include_HEADERS += inc/ocr-runtime-itf.h
endif

# Distribute OCR library interface headers - set by configure
if INCLUDE_OCR_LIBRARY_ITF_HEADERS
library_include_HEADERS += inc/ocr-lib.h
endif

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 \
	configure  \
	$(AUX_DIST)
