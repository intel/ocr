OCR_SRC_SCHEDULER=scheduler

noinst_LTLIBRARIES += libocr_scheduler.la
libocr_la_LIBADD += libocr_scheduler.la

libocr_scheduler_la_SOURCES = \
$(OCR_SRC_SCHEDULER)/ocr-scheduler.c \
$(OCR_SRC_SCHEDULER)/ocr-workpile.c
libocr_scheduler_la_CFLAGS = $(AM_CFLAGS)

#
# OCR-HC scheduler implementation
#
noinst_LTLIBRARIES += libocr_scheduler_hc.la
libocr_la_LIBADD += libocr_scheduler_hc.la

libocr_scheduler_hc_la_SOURCES = \
$(OCR_SRC_SCHEDULER)/ocr-scheduler-hc/ocr-executor-hc.c \ 
$(OCR_SRC_SCHEDULER)/ocr-scheduler-hc/ocr-scheduler-hc.c \
$(OCR_SRC_SCHEDULER)/ocr-scheduler-hc/ocr-edf-hc.c \
$(OCR_SRC_SCHEDULER)/ocr-scheduler-hc/ocr-workpile-hc.c \ 
$(OCR_SRC_SCHEDULER)/ocr-scheduler-hc/deque.c \
$(OCR_SRC_SCHEDULER)/ocr-scheduler-hc/ocr-policy-hc.c

libocr_scheduler_hc_la_CFLAGS = $(AM_CFLAGS)
