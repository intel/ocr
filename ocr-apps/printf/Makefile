#
#                 INTEL CONFIDENTIAL
# Copyright (c) 2010-2011 Intel Corporation. All rights reserved.
#
# THIS PROGRAM IS FULLY PROTECTED BY COPYRIGHT LAWS AND IS
# CONSIDERED A TRADE SECRET BELONGING TO THE INTEL CORPORATION.
#
# Author: Ivan B Ganev <ivan.b.ganev@intel.com>
#
#

MAKE := make

ifndef ARCH
  $(error Need to specify ARCH!)
endif

all:
	@$(MAKE) -f Makefile.$(ARCH) $@

debug:
	@$(MAKE) -f Makefile.$(ARCH) $@

clean:
	@$(MAKE) -f Makefile.$(ARCH) $@
