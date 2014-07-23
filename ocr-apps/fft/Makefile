# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.

MAKE := make

ifndef ARCH
  $(error Need to specify ARCH!)
endif

all:
	@$(MAKE) -f Makefile.$(ARCH) $@

debug:
	@$(MAKE) -f Makefile.$(ARCH) $@

install:
	@$(MAKE) -f Makefile.$(ARCH) $@

run:
	@$(MAKE) -f Makefile.$(ARCH) $@

clean:
	@$(MAKE) -f Makefile.$(ARCH) $@
