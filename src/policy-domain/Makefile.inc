OCR_SRC_POLICY_DOMAIN=policy-domain

noinst_LTLIBRARIES += libocr_policy_domain.la
libocr_la_LIBADD += libocr_policy_domain.la

libocr_policy_domain_la_SOURCES = $(OCR_SRC_POLICY_DOMAIN)/ocr-policy-domain.c
libocr_policy_domain_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_policy_domain_hc.la
libocr_la_LIBADD += libocr_policy_domain_hc.la

libocr_policy_domain_hc_la_SOURCES = \ 
$(OCR_SRC_POLICY_DOMAIN)/hc/hc-policy.c
#$(OCR_SRC_POLICY_DOMAIN)/hc/hc-placed-policy.c
libocr_policy_domain_hc_la_CFLAGS = $(AM_CFLAGS)

#noinst_LTLIBRARIES += libocr_policy_domain_fsim.la
#libocr_la_LIBADD += libocr_policy_domain_fsim.la

#libocr_policy_domain_fsim_la_SOURCES = $(OCR_SRC_POLICY_DOMAIN)/fsim/fsim-policy.c
#libocr_policy_domain_fsim_la_CFLAGS = $(AM_CFLAGS)
