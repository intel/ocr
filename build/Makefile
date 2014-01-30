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
	@echo "   x86-pthread-x86     -- OCR for X86 Linux machines"
	@echo "   x86-pthread-fsim    -- OCR for Traleika Glacier FSIM but emulated on X86"
	@echo "   fsim-null-fsim-ce   -- CE parts of OCR for Traleika Glacier FSIM"
	@echo "   fsim-null-fsim-xe   -- XE parts of OCR for Traleika Glacier FSIM"
	@echo "   x86-builder-fsim-ce -- CE struct builder for Traleika Glacier FSIM on X86"
	@echo "   x86-builder-fsim-xe -- XE struct builder for Traleika Glacier FSIM on X86"
	@echo ""
	@echo "Valid targets are:"
	@echo ""
	@echo "   all          -- Optimized build (alias for static)"
	@echo "   static       -- Optimized static build (produces a .a)"
	@echo "   shared       -- Optimized shared build (produces a .so)"
	@echo "   debug-static -- Unoptimized static build"
	@echo "   debug-shared -- Unoptimized shared build"
	@echo "   install      -- Install built distribution"
	@echo "   uninstall    -- Remove installed distribution"
	@echo "   clean        -- Clean-up build objects, etc."
	@echo "   squeaky      -- Clean-up all architectures"
	@echo ""

.PHONY: all
all:
	@$(MAKE) -C $(ARCH) $@

.PHONY: static
static:
	@$(MAKE) -C $(ARCH) $@

.PHONY: shared
shared:
	@$(MAKE) -C $(ARCH) $@

.PHONY: debug-static
debug-static:
	@$(MAKE) -C $(ARCH) $@

.PHONY: debug-shared
debug-shared:
	@$(MAKE) -C $(ARCH) $@

.PHONY: install
install:
	@$(MAKE) -C $(ARCH) $@

.PHONY: uninstall
	@$(MAKE) -C $(ARCH) $@

.PHONY: clean
clean:
	@$(MAKE) -C $(ARCH) $@

.PHONY: squeaky
squeaky:
	ARCH=x86-pthread-x86 $(MAKE) -C x86-pthread-x86 clean
	ARCH=x86-pthread-fsim $(MAKE) -C x86-pthread-fsim clean
	ARCH=fsim-null-fsim-ce $(MAKE) -C fsim-null-fsim-ce clean
	ARCH=fsim-null-fsim-xe $(MAKE) -C fsim-null-fsim-xe clean
	ARCH=x86-builder-fsim-ce $(MAKE) -C x86-builder-fsim-ce clean
	ARCH=x86-builder-fsim-xe $(MAKE) -C x86-builder-fsim-xe clean
