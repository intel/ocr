#
# Makefile for the OCR Runtime
#
#                 INTEL CONFIDENTIAL
# Copyright (c) 2010-2013 Intel Corporation. All rights reserved.
#
# THIS PROGRAM IS FULLY PROTECTED BY COPYRIGHT LAWS AND IS
# CONSIDERED A TRADE SECRET BELONGING TO THE INTEL CORPORATION.
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
	@echo "   clean        -- Clean-up build objects, etc."
	@echo ""

all:
	@$(MAKE) -C $(ARCH) all

debug:
	@$(MAKE) -C $(ARCH) debug

clean:
	@$(MAKE) -C $(ARCH) clean
