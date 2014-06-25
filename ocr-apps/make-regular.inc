# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.

OBJS := $(addprefix $(WORKLOAD_BUILD)/,$(notdir $(SRCS:.c=.o)))

.ONESHELL:
install: default .FORCE
	@echo "Installing from $(WORKLOAD_BUILD) into $(WORKLOAD_INSTALL)..."
	@echo ""
	@echo "####"
	@echo "To run, set OCR_INSTALL to ${OCR_INSTALL} and OCR_CONFIG to ${OCR_INSTALL}/config/default.cfg (default one)"
	@echo "####"
	@$(CP) $(WORKLOAD_BUILD)/$(TARGET) $(WORKLOAD_INSTALL)/
	@$(CP) $(WORKLOAD_SRC)/config.cfg $(WORKLOAD_INSTALL)/

default: $(WORKLOAD_BUILD)/$(TARGET)

# Require libocr.a as this means all include files are up-to-date
# The .so dependence is so that we can have the include files in place
$(WORKLOAD_BUILD)/%.o: $(WORKLOAD_SRC)/%.c $(OCR_INSTALL)/lib/libocr.so
	$(CC) $(CFLAGS) -c -o $@ $<

$(WORKLOAD_BUILD)/$(TARGET).dbg: $(OBJS) $(OCR_INSTALL)/lib/libocr.so
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(WORKLOAD_BUILD)/$(TARGET): $(WORKLOAD_BUILD)/$(TARGET).dbg
	$(CP) $(WORKLOAD_BUILD)/$(TARGET).dbg $(WORKLOAD_BUILD)/$(TARGET)
	$(STRIP) $(WORKLOAD_BUILD)/$(TARGET)

# libocr.so
$(OCR_INSTALL)/lib/libocr.so: $(OCR_BUILD)/Makefile .FORCE
	@echo "Building libocr.so..."
	OCR_INSTALL=$(OCR_INSTALL) OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD) $(TARGETS) install

clean:
	-$(RM) -rf $(WORKLOAD_BUILD)
	-$(RM) -rf $(WORKLOAD_INSTALL)

.PHONY:
.FORCE:


