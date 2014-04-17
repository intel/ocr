
#
# hc comp-target implementation
#
noinst_LTLIBRARIES += libocr_comp_target_hc.la
libocr_la_LIBADD += libocr_comp_target_hc.la

libocr_comp_target_hc_la_SOURCES = comp-target/hc/hc-comp-target.c
libocr_comp_target_hc_la_CFLAGS = $(AM_CFLAGS)

#
# fsim comp-target implementation
#
noinst_LTLIBRARIES += libocr_comp_target_fsim.la
libocr_la_LIBADD += libocr_comp_target_fsim.la

libocr_comp_target_fsim_la_SOURCES = comp-target/fsim/fsim-comp-target.c
libocr_comp_target_fsim_la_CFLAGS = $(AM_CFLAGS)
