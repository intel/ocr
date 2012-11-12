
SUBDIRS = src

AUX_DIST = \
	$(ac_aux_dir)/config.guess		\
	$(ac_aux_dir)/config.sub		\
	$(ac_aux_dir)/install-sh		\
	$(ac_aux_dir)/ltmain.sh			\
	$(ac_aux_dir)/missing			\
	$(ac_aux_dir)/depcomp

library_includedir=$(includedir)
library_include_HEADERS = inc/ocr-db.h inc/ocr-edt.h inc/ocr-tuning.h \
inc/ocr-types.h inc/ocr.h \
src/inc/ocr-config.h src/inc/ocr-runtime-def.h src/inc/ocr-executor.h src/inc/ocr-function-id.h \
src/inc/ocr-guid.h src/inc/ocr-low-workers.h src/inc/ocr-policy.h src/inc/ocr-machine.h \
src/inc/ocr-runtime.h src/inc/ocr-runtime-model.h src/inc/ocr-scheduler.h \
src/inc/ocr-task-event.h src/inc/ocr-utils.h src/inc/ocr-workpile.h 

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 \
	configure  \
	$(AUX_DIST)

