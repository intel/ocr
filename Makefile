#
# Makefile for the OCR Runtime
#
# For OCR licensing terms, see ../runtime/LICENSE.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

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

all:
	@$(MAKE) -C build $@

debug:
	@$(MAKE) -C build $@

install:
	@$(MAKE) -C build $@

clean:
	@$(MAKE) -C build $@

squeaky:
	@$(MAKE) -C build $@

