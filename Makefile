#
# Makefile for the OCR Runtime
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#


.PHONY: default
default:
	@echo "Must specify an arch and a target, e.g.:"
	@echo ""
	@echo "   ARCH=x86-pthread-x86 make all"
	@echo ""
	@echo "Valid architectures are:"
	@echo ""
	@echo "   x86-pthread-x86    -- OCR for X86 Linux machines"
	@echo "   x86-pthread-fsim   -- OCR for Traleika Glacier FSIM but emulated on X86"
	@echo "   fsim-null-fsim-ce  -- CE parts of OCR for Traleika Glacier FSIM"
	@echo "   fsim-null-fsim-xe  -- XE parts of OCR for Traleika Glacier FSIM"
	@echo ""
	@echo "Valid targets are:"
	@echo ""
	@echo "   all          -- Optimized build (alias for static)"
	@echo "   static       -- Optimized static build (produces a .a)"
	@echo "   shared       -- Optimized shared build (produces a .so)"
	@echo "   debug-static -- Unoptimized static build"
	@echo "   debug-shared -- Unoptimized shared build"
	@echo "   install      -- Install built distribution"
	@echo "   uninstall    -- Remove built distribution"
	@echo "   clean        -- Clean-up build objects, etc."
	@echo "   squeaky      -- Clean-up all architectures"
	@echo ""

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

.PHONY: squeaky
squeaky:
	$(MAKE) -C build $@


