# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.

OBJS := $(addprefix $(WORKLOAD_BUILD)/,$(notdir $(SRCS:.c=.o)))

.ONESHELL:
install: default .FORCE
	@echo "Installing from $(WORKLOAD_BUILD) into $(WORKLOAD_INSTALL)..."
	@echo ""
	@echo "####"
	@echo "To run: ${RMD_INSTALL}/bin/fsim -s -c <machine-config> -c ${WORKLOAD_INSTALL}/config.cfg"
	@echo "####"
	@$(CP) $(WORKLOAD_BUILD)/$(TARGET) $(WORKLOAD_INSTALL)/
	@$(CP) $(WORKLOAD_BUILD)/$(TARGET).blob $(WORKLOAD_INSTALL)/
	@$(CP) $(WORKLOAD_SRC)/config.cfg $(WORKLOAD_INSTALL)/
	@$(CP) $(WORKLOAD_BUILD)/build.rmdkrnl/rmdkrnl $(WORKLOAD_INSTALL)/

default: $(WORKLOAD_BUILD)/$(TARGET) $(WORKLOAD_BUILD)/$(TARGET).blob $(WORKLOAD_BUILD)/rmdkrnl

# Require libocr.a as this means all include files are up-to-date
$(WORKLOAD_BUILD)/%.o: $(WORKLOAD_SRC)/%.c $(OCR_INSTALL_XE)/lib/libocr.a
	$(CC) $(CFLAGS) -fno-builtin -c -o $@ $<
#	$(CC) $(CFLAGS) -fno-builtin -c -o $@.tmp $<
#	$(OBJCOPY) -R .data_local -R .rodata_local -R .bss_local -R .data_block -R .rodata_block -R .bss_block $@.tmp $@
#	$(RM) $@.tmp

$(WORKLOAD_BUILD)/$(TARGET).dbg: $(OBJS) $(OCR_INSTALL_XE)/lib/libocr.a
	$(LD) $(LDFLAGS) -o $@ $(OBJS) --whole-archive $(OCR_INSTALL_XE)/lib/libocr.a

$(WORKLOAD_BUILD)/$(TARGET): $(WORKLOAD_BUILD)/$(TARGET).dbg
	$(CP) $(WORKLOAD_BUILD)/$(TARGET).dbg $(WORKLOAD_BUILD)/$(TARGET)
	$(STRIP) $(WORKLOAD_BUILD)/$(TARGET)

# Build the final blob
# Note the order of the dependence. IMPORTANT!! Do not randomly change
$(WORKLOAD_BUILD)/$(TARGET).blob: $(OCR_SRC)/scripts/Blob/aggregate_binary_files.sh \
$(WORKLOAD_BUILD)/$(TARGET).blob.ce $(WORKLOAD_BUILD)/$(TARGET).blob.xe \
$(WORKLOAD_BUILD)/$(TARGET).blob.arg
	$^ $@

# Build the CE blob
$(WORKLOAD_BUILD)/$(TARGET).blob.ce: $(OCR_INSTALL_CE)/config/default.cfg \
$(WORKLOAD_BUILD)/rmdkrnl.dbg $(OCR_INSTALL_ROOT)/$(CE_BUILDER)/bin/builder.exe
	RMD_INSTALL=$(RMD_INSTALL) WORKLOAD_BUILD=$(WORKLOAD_BUILD) STRUCT_OUT=$(WORKLOAD_BUILD)/$(TARGET).blob.ce \
$(OCR_INSTALL_ROOT)/$(CE_BUILDER)/bin/builder.exe -ocr:cfg $(OCR_INSTALL_CE)/config/default.cfg

$(OCR_INSTALL_CE)/config/default.cfg: $(OCR_INSTALL_CE)/lib/libocr.a

# Build the XE blob
$(WORKLOAD_BUILD)/$(TARGET).blob.xe: $(OCR_INSTALL_XE)/config/default.cfg \
$(WORKLOAD_BUILD)/$(TARGET).dbg $(OCR_INSTALL_ROOT)/$(XE_BUILDER)/bin/builder.exe
	RMD_INSTALL=$(RMD_INSTALL) WORKLOAD_BUILD=$(WORKLOAD_BUILD) STRUCT_OUT=$(WORKLOAD_BUILD)/$(TARGET).blob.xe \
	ARGS_OUT=$(WORKLOAD_BUILD)/$(TARGET).blob.arg APP_FILE=$(WORKLOAD_BUILD)/$(TARGET).dbg \
	$(OCR_INSTALL_ROOT)/$(XE_BUILDER)/bin/builder.exe -ocr:cfg $(OCR_INSTALL_XE)/config/default.cfg

$(OCR_INSTALL_XE)/config/default.cfg: $(OCR_INSTALL_XE)/lib/libocr.a

# The arg blob is build automatically but just say how
$(WORKLOAD_BUILD)/$(TARGET).blob.arg: $(WORKLOAD_BUILD)/$(TARGET).blob.xe

# Local version of the RMDKRNL (built with the CE's libocr)
# We build locally so we can run without impacting RMD_INSTALL
.ONESHELL:
$(WORKLOAD_BUILD)/rmdkrnl.dbg: $(OCR_INSTALL_CE)/lib/libocr.a .FORCE
	@echo "Building RMDKRNL from $(RMD_SRC)..."
	-@$(RM) -rf $(WORKLOAD_BUILD)/build.rmdkrnl
	@$(MKDIR) -p $(WORKLOAD_BUILD)/build.rmdkrnl
	@$(CP) $(RMD_SRC)/build/build.rmdkrnl/make-me.make $(WORKLOAD_BUILD)/build.rmdkrnl
	OCR_INSTALL=$(OCR_INSTALL_CE) RMD_SRC=$(RMD_SRC) \
	RMD_INSTALL=$(RMD_INSTALL) \
	$(MAKE) -C $(WORKLOAD_BUILD)/build.rmdkrnl -f make-me.make $(TARGETS)

$(WORKLOAD_BUILD)/rmdkrnl: $(WORKLOAD_BUILD)/rmdkrnl.dbg

# XE libocr.a
$(OCR_INSTALL_XE)/lib/libocr.a: $(OCR_BUILD_XE)/Makefile .FORCE
	@echo "Building XE libocr.a..."
	OCR_INSTALL=$(OCR_INSTALL_XE) OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD_XE) $(TARGETS) install

# XE builder
$(OCR_INSTALL_ROOT)/$(XE_BUILDER)/bin/builder.exe: $(OCR_BUILD_ROOT)/$(XE_BUILDER)/Makefile .FORCE
	@echo "Building XE builder..."
	OCR_INSTALL=$(OCR_INSTALL_ROOT)/$(XE_BUILDER) \
	OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD_ROOT)/$(XE_BUILDER) $(TARGETS) install

# CE libocr.a
$(OCR_INSTALL_CE)/lib/libocr.a: $(OCR_BUILD_CE)/Makefile .FORCE
	@echo "Building CE libocr.a..."
	OCR_INSTALL=$(OCR_INSTALL_CE) OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD_CE) $(TARGETS) install

# CE builder
$(OCR_INSTALL_ROOT)/$(CE_BUILDER)/bin/builder.exe: $(OCR_BUILD_ROOT)/$(CE_BUILDER)/Makefile .FORCE
	@echo "Building CE builder..."
	OCR_INSTALL=$(OCR_INSTALL_ROOT)/$(CE_BUILDER) \
	OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD_ROOT)/$(CE_BUILDER) $(TARGETS) install

clean:
	-$(RM) -rf $(WORKLOAD_BUILD)
	-$(RM) -rf $(WORKLOAD_INSTALL)

.PHONY:
.FORCE:
