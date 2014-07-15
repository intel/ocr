# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.

OBJS := $(notdir $(SRCS:.c=.o))
OBJS := $(notdir $(OBJS:.cpp=.o))
OBJS := $(addprefix $(WORKLOAD_BUILD)/,$(OBJS))

.ONESHELL:
install: default
	@echo "Installing from $(WORKLOAD_BUILD) into $(WORKLOAD_INSTALL)..."
	@echo ""
	@echo "####"
	@echo "To run, set OCR_INSTALL to ${OCR_INSTALL} and export it; set "\
	"LD_LIBRARY_PATH to ${OCR_INSTALL}/lib and export it; set OCR_CONFIG to "\
	"${OCR_INSTALL}/config/default.cfg (default one) and export it"
	@echo "####"
	@$(CP) -pf $(WORKLOAD_BUILD)/$(TARGET) $(WORKLOAD_INSTALL)/

run:	TMPDIR := $(shell mktemp -d)
run:
	$(CP) $(WORKLOAD_INSTALL)/$(TARGET) $(TMPDIR)/$(TARGET)
	LD_LIBRARY_PATH=$(OCR_INSTALL)/lib OCR_CONFIG=${OCR_INSTALL}/config/default.cfg bash -c '$(TMPDIR)/$(TARGET) "$(filter-out $@,$(MAKECMDGOALS))"'
	$(RM) -rf $(TMPDIR)

default: $(WORKLOAD_BUILD)/$(TARGET)

# Require libocr.a as this means all include files are up-to-date
# The .so dependence is so that we can have the include files in place
$(WORKLOAD_BUILD)/%.o: $(WORKLOAD_SRC)/%.c $(OCR_INSTALL)/lib/libocr.so
	$(CC) $(CFLAGS) -c -o $@ $<

$(WORKLOAD_BUILD)/%.o: $(WORKLOAD_SRC)/%.cpp $(OCR_INSTALL)/lib/libocr.so
	$(CC) $(CFLAGS) -c -o $@ $<

$(WORKLOAD_BUILD)/$(TARGET).dbg: $(OBJS) $(OCR_INSTALL)/lib/libocr.so
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(WORKLOAD_BUILD)/$(TARGET): $(WORKLOAD_BUILD)/$(TARGET).dbg
	$(CP) $(WORKLOAD_BUILD)/$(TARGET).dbg $(WORKLOAD_BUILD)/$(TARGET)
	$(STRIP) $(WORKLOAD_BUILD)/$(TARGET)

# libocr.so
$(OCR_INSTALL)/lib/libocr.so: $(OCR_BUILD)/Makefile
	@echo "Building libocr.so..."
	OCR_INSTALL=$(OCR_INSTALL) OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD) $(TARGETS) install

clean:
	-$(RM) -rf $(WORKLOAD_BUILD)
	-$(RM) -rf $(WORKLOAD_INSTALL)

.PHONY: clean run install

# Prevent args from being interpreted as targets
%:
	@echo -n ""
