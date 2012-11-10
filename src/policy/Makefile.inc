OCR_SRC_POLICY=policy

noinst_LTLIBRARIES += libocr_policy.la
libocr_la_LIBADD += libocr_policy.la

libocr_policy_la_SOURCES = $(OCR_SRC_POLICY)/ocr-policy.c
libocr_policy_la_CFLAGS = $(AM_CFLAGS)
