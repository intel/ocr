# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.

# Get absolute paths. This is useful since we go in
# subfolders to do make and that screws up the relative
# paths
WORKLOAD_SRC          := $(shell cd "${WORKLOAD_SRC}" && pwd)
WORKLOAD_BUILD_ROOT   := $(shell mkdir -p "${WORKLOAD_BUILD_ROOT}" && cd "${WORKLOAD_BUILD_ROOT}" && pwd)
WORKLOAD_INSTALL_ROOT := $(shell mkdir -p "${WORKLOAD_INSTALL_ROOT}" && cd "${WORKLOAD_INSTALL_ROOT}" && pwd)
OCR_SRC               := $(shell cd "${OCR_SRC}" && pwd)
OCR_INSTALL_ROOT      := $(shell mkdir -p "${OCR_INSTALL_ROOT}" && cd "${OCR_INSTALL_ROOT}" && pwd)
OCR_BUILD_ROOT        := $(shell cd "${OCR_BUILD_ROOT}" && pwd)

OCR_INSTALL      := ${OCR_INSTALL_ROOT}/${OCR_TARGET}
OCR_BUILD        := ${OCR_BUILD_ROOT}/${OCR_TARGET}

WORKLOAD_INSTALL := ${WORKLOAD_INSTALL_ROOT}/${ARCH}
WORKLOAD_BUILD    := ${WORKLOAD_BUILD_ROOT}/${ARCH}

CC	= gcc
CFLAGS_BASE = -g -Werror -I. -I${OCR_INSTALL}/include


LDFLAGS_BASE = -L ${OCR_INSTALL}/lib -locr

CFLAGS  := $(CFLAGS_BASE) $(CFLAGS)
LDFLAGS := $(LDFLAGS_BASE) $(LDFLAGS)

STRIP   = strip

CUT	= cut
GREP	= grep
RM	= rm
CP      = cp
MKDIR   = mkdir

OBJS := $(notdir $(SRCS:.c=.o))
OBJS := $(notdir $(OBJS:.cpp=.o))
OBJS := $(addprefix $(WORKLOAD_BUILD)/,$(OBJS))

VALIDGOALS := install run default clean

.PHONY: install
.ONESHELL:
install: default | $(WORKLOAD_INSTALL)
	@echo "Installing from $(WORKLOAD_BUILD) into $(WORKLOAD_INSTALL)..."
	@echo ""
	@echo "####"
	@echo "To run, set OCR_INSTALL to ${OCR_INSTALL} and export it; set "\
	"LD_LIBRARY_PATH to ${OCR_INSTALL}/lib and export it; set OCR_CONFIG to "\
	"${OCR_INSTALL}/config/default.cfg (default one) and export it"
	@echo "####"
	@$(RM) -f $(WORKLOAD_INSTALL)/$(TARGET)
	@$(CP) $(WORKLOAD_BUILD)/$(TARGET) $(WORKLOAD_INSTALL)/

.PHONY: run
.ONESHELL:
run:    WORKLOAD_ARGS := $(filter-out $(VALIDGOALS),$(MAKECMDGOALS))
run:    WORKLOAD_EXEC ?= $(WORKLOAD_INSTALL)
run:    OCR_CONFIG ?= $(OCR_INSTALL)/config/default.cfg
run:    install
	cd $(WORKLOAD_EXEC)
	LD_LIBRARY_PATH=$(OCR_INSTALL)/lib OCR_CONFIG=$(OCR_CONFIG) bash -c './$(TARGET) $(WORKLOAD_ARGS)'

.PHONY: default
default: $(WORKLOAD_BUILD)/$(TARGET)

# Require libocr.a as this means all include files are up-to-date
# The .so dependence is so that we can have the include files in place
$(WORKLOAD_BUILD)/%.o: $(WORKLOAD_SRC)/%.c $(OCR_INSTALL)/lib/libocr.so | $(WORKLOAD_BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(WORKLOAD_BUILD)/%.o: $(WORKLOAD_SRC)/%.cpp $(OCR_INSTALL)/lib/libocr.so | $(WORKLOAD_BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(WORKLOAD_BUILD)/$(TARGET).dbg: $(OBJS) $(OCR_INSTALL)/lib/libocr.so | $(WORKLOAD_BUILD)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(WORKLOAD_BUILD)/$(TARGET): $(WORKLOAD_BUILD)/$(TARGET).dbg | $(WORKLOAD_BUILD)
	$(CP) $(WORKLOAD_BUILD)/$(TARGET).dbg $(WORKLOAD_BUILD)/$(TARGET)
	$(STRIP) $(WORKLOAD_BUILD)/$(TARGET)

# libocr.so
$(OCR_INSTALL)/lib/libocr.so: $(OCR_BUILD)/Makefile
	@echo "Building libocr.so..."
	OCR_INSTALL=$(OCR_INSTALL) OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD) $(TARGETS) install

.PHONY: clean
clean:
	-$(RM) -rf $(WORKLOAD_BUILD)
	-$(RM) -rf $(WORKLOAD_INSTALL)

$(WORKLOAD_BUILD):
	mkdir -p $(WORKLOAD_BUILD)

$(WORKLOAD_INSTALL):
	mkdir -p $(WORKLOAD_INSTALL)

# Prevent args from being interpreted as targets
%:
	@echo -n ""
