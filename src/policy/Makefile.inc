OCR_SRC_POLICY=policy

noinst_LTLIBRARIES += libocr_policy.la
libocr_la_LIBADD += libocr_policy.la

libocr_policy_la_SOURCES = $(OCR_SRC_POLICY)/ocr-policy.c
libocr_policy_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_policy_hc.la
libocr_la_LIBADD += libocr_policy_hc.la

libocr_policy_hc_la_SOURCES = $(OCR_SRC_POLICY)/ocr-policy-hc/ocr-policy-hc.c
libocr_policy_hc_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_policy_fsim.la
libocr_la_LIBADD += libocr_policy_fsim.la

libocr_policy_fsim_la_SOURCES = $(OCR_SRC_POLICY)/ocr-policy-fsim/ocr-policy-fsim.c
libocr_policy_fsim_la_CFLAGS = $(AM_CFLAGS)
