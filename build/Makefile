#
# Makefile for the OCR Runtime
#
# For OCR licensing terms, see top level LICENSE file.
#
# Author: Ivan Ganev <ivan.b.ganev@intel.com>
#

.PHONY: default
default:
	@echo "Must specify an OCR version/type to build, e.g.:"
	@echo ""
	@echo "   OCR_TYPE=x86-pthread-x86 make all"
	@echo ""
	@echo "Valid version are:"
	@echo ""
	@echo "   x86-pthread-x86     -- OCR for X86 Linux machines"
	@echo "   x86-pthread-tg      -- OCR for Traleika Glacier but emulated on X86"
	@echo "   tg-null-tg-ce       -- CE parts of OCR for Traleika Glacier"
	@echo "   tg-null-tg-xe       -- XE parts of OCR for Traleika Glacier"
	@echo "   x86-builder-tg-ce   -- CE struct builder for Traleika Glacier on X86"
	@echo "   x86-builder-tg-xe   -- XE struct builder for Traleika Glacier on X86"
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
	@$(MAKE) -C $(OCR_TYPE) $@

.PHONY: static
static:
	@$(MAKE) -C $(OCR_TYPE) $@

.PHONY: shared
shared:
	@$(MAKE) -C $(OCR_TYPE) $@

.PHONY: debug-static
debug-static:
	@$(MAKE) -C $(OCR_TYPE) $@

.PHONY: debug-shared
debug-shared:
	@$(MAKE) -C $(OCR_TYPE) $@

.PHONY: install
install:
	@$(MAKE) -C $(OCR_TYPE) $@

.PHONY: uninstall
uninstall:
	@$(MAKE) -C $(OCR_TYPE) $@

.PHONY: clean
clean:
	@$(MAKE) -C $(OCR_TYPE) $@

.PHONY: squeaky
squeaky:
	OCR_TYPE=x86-pthread-x86 $(MAKE) -C x86-pthread-x86 clean uninstall
	OCR_TYPE=x86-pthread-tg $(MAKE) -C x86-pthread-tg clean uninstall
	OCR_TYPE=tg-null-tg-ce $(MAKE) -C tg-null-tg-ce clean uninstall
	OCR_TYPE=tg-null-tg-xe $(MAKE) -C tg-null-tg-xe clean uninstall
	OCR_TYPE=x86-builder-tg-ce $(MAKE) -C x86-builder-tg-ce clean uninstall
	OCR_TYPE=x86-builder-tg-xe $(MAKE) -C x86-builder-tg-xe clean uninstall


