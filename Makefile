#
# Makefile for the OCR Runtime
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#


.PHONY: default
default:
	@$(MAKE) -C build $@

.PHONY: all
all:
	@$(MAKE) -C build $@

.PHONY: static
static:
	@$(MAKE) -C build $@

.PHONY: shared
shared:
	@$(MAKE) -C build $@

.PHONY: debug-static
debug-static:
	@$(MAKE) -C build $@

.PHONY: debug-shared
debug-shared:
	@$(MAKE) -C build $@

.PHONY: install
install:
	@$(MAKE) -C build $@

.PHONY: uninstall
uninstall:
	@$(MAKE) -C build $@

.PHONY: clean
clean:
	@$(MAKE) -C build $@
	-@$(RM) $(RMFLAGS) TAGS

.PHONY: squeaky
squeaky:
	@$(MAKE) -C build $@
	-@$(RM) $(RMFLAGS) TAGS

.PHONY: tags
tags:
	@find src/ -type f -name '*.[ch]' | xargs etags -a
