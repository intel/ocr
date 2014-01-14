#
# Makefile for the OCR Runtime
#
# For OCR licensing terms, see ../runtime/LICENSE.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

.PHONY: default
default:
	@echo "Must specify an arch and a target, e.g.:"
	@echo ""
	@echo "   ARCH=x86-linux make all"
	@echo ""
	@echo "Valid architectures are:"
	@echo ""
	@echo "   x86-linux    -- OCR for X86 Linux machines"
	@echo "   fsim         -- OCR for Traleika Glacier FSIM"
	@echo ""
	@echo "Valid targets are:"
	@echo ""
	@echo "   all          -- Optimized build"
	@echo "   debug        -- Unoptimized build"
	@echo "   install      -- Install built distribution"
	@echo "   clean        -- Clean-up build objects, etc."
	@echo "   squeaky      -- Clean-up all architectures"
	@echo ""

.PHONY: all
all:
	@$(MAKE) -C build $@

.PHONY: debug
debug:
	@$(MAKE) -C build $@

.PHONY: install
install:
	@$(MAKE) -C build $@

.PHONY: clean
clean:
	@$(MAKE) -C build $@

.PHONY: squeaky
squeaky:
	@$(MAKE) -C build $@

