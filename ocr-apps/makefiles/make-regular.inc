# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.

# Get absolute paths. This is useful since we go in
# subfolders to do make and that screws up the relative
# paths
ifeq ($(RUN_JENKINS), run)
OCR_SRC               :=
OCR_BUILD_ROOT        :=
OCR_BUILD             :=
WORKLOAD_BUILD_ROOT   :=
WORKLOAD_BUILD        :=
APPS_ROOT             :=
WORKLOAD_SRC          :=
else
OCR_SRC               := $(shell cd "${OCR_SRC}" && pwd)
OCR_BUILD_ROOT        := $(shell cd "${OCR_BUILD_ROOT}" && pwd)
OCR_BUILD             := ${OCR_BUILD_ROOT}/${OCR_TARGET}
WORKLOAD_BUILD_ROOT   := $(shell mkdir -p "${WORKLOAD_BUILD_ROOT}" && cd "${WORKLOAD_BUILD_ROOT}" && pwd)
WORKLOAD_BUILD        := ${WORKLOAD_BUILD_ROOT}/${OCR_TYPE}
APPS_ROOT             := $(shell cd "${APPS_ROOT}" && pwd)
WORKLOAD_SRC          := $(shell cd "${WORKLOAD_SRC}" && pwd)
endif

# Tool to use as a wrapper to run
RUN_TOOL              ?=

APPS_LIBS_ROOT        := $(shell cd "${APPS_LIBS_ROOT}" && pwd)

HOSTNAME              := $(shell hostname)

OCR_INSTALL_ROOT := $(shell mkdir -p "${OCR_INSTALL_ROOT}" && cd "${OCR_INSTALL_ROOT}" && pwd)
OCR_INSTALL      := ${OCR_INSTALL_ROOT}/${OCR_TARGET}

WORKLOAD_INSTALL_ROOT := $(shell mkdir -p "${WORKLOAD_INSTALL_ROOT}" && cd "${WORKLOAD_INSTALL_ROOT}" && pwd)
WORKLOAD_INSTALL      := ${WORKLOAD_INSTALL_ROOT}/${OCR_TYPE}

REQ_LIBS ?=

CC	= gcc
CFLAGS_BASE =  -Werror -I. -I${OCR_INSTALL}/include -I $(APPS_LIBS_ROOT)/include


LDFLAGS_BASE = -L ${OCR_INSTALL}/lib -L $(APPS_LIBS_ROOT)/lib -locr

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

FULL_REQ_LIBS := $(addprefix $(APPS_ROOT)/libs/src/,$(REQ_LIBS))

.PHONY: install
.ONESHELL:
install: | $(WORKLOAD_INSTALL)
	@echo "Installing from $(WORKLOAD_BUILD) into $(WORKLOAD_INSTALL)..."
	@echo ""
	@echo "####"
	@echo "To run, use 'OCR_TYPE=${OCR_TYPE} make run <args>' or, manually (in bash):"
	@echo "export OCR_INSTALL=${OCR_INSTALL} && export LD_LIBRARY_PATH=${OCR_INSTALL}/lib "\
	"&& export OCR_CONFIG=${OCR_INSTALL}/config/default.cfg (default one) and run "\
	"the executable $(TARGET) in $(WORKLOAD_INSTALL)"
	@echo "####"
	@$(RM) -f $(WORKLOAD_INSTALL)/$(TARGET)
	@$(CP) $(WORKLOAD_BUILD)/$(TARGET) $(WORKLOAD_INSTALL)/

ifeq (yes, $(DEBUGMAKE))
.PHONY: debug_pre_prerun
debug_pre_prerun:
	@echo "Initial arguments are '${WORKLOAD_ARGS}' and inputs are '${WORKLOAD_INPUTS}'"

prerun: debug_pre_prerun

debug_post_prerun: prerun
	@echo "Post prerun arguments are '${WORKLOAD_ARGS}' and inputs are '${WORKLOAD_INPUTS}'"
else
debug_post_prerun: prerun

endif

.PHONY: run_internal
.ONESHELL:
run_internal: WORKLOAD_EXEC ?= $(WORKLOAD_INSTALL)
run_internal: OCR_CONFIG ?= $(OCR_INSTALL)/config/default.cfg
ifeq ($(RUN_JENKINS), run)
run_internal: debug_post_prerun
else
run_internal: debug_post_prerun install
endif
ifeq ($(HOSTNAME), bar1)
	$(error Please refrain from running jobs on bar1!)
endif
	cd $(WORKLOAD_EXEC)
	LD_LIBRARY_PATH=$(OCR_INSTALL)/lib:$(APPS_LIBS_ROOT) OCR_CONFIG=$(OCR_CONFIG) bash -c '$(RUN_TOOL) ./$(TARGET) $(WORKLOAD_ARGS) $(WORKLOAD_INPUTS)'

postrun: run_internal

.PHONY: run
run: postrun

.PHONY: default
default: reqlibs $(WORKLOAD_BUILD)/$(TARGET)

# Builds any libraries required by the workload
.PHONY: parallel-reqlibs
parallel-reqlibs: $(FULL_REQ_LIBS)

.PHONY: $(FULL_REQ_LIBS)
$(FULL_REQ_LIBS):
	ARCH=x86 APPS_ROOT=$(APPS_ROOT) APPS_LIBS_ROOT=$(APPS_LIBS_ROOT) \
	$(MAKE) -C $@ install

.PHONY: reqlibs
reqlibs: parallel-reqlibs

# Require libocr as this means all include files are up-to-date
$(WORKLOAD_BUILD)/%.o: $(WORKLOAD_SRC)/%.c | libocr $(WORKLOAD_BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(WORKLOAD_BUILD)/%.o: $(WORKLOAD_SRC)/%.cpp | libocr $(WORKLOAD_BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(WORKLOAD_BUILD)/$(TARGET).dbg: $(OBJS) | libocr $(WORKLOAD_BUILD)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(WORKLOAD_BUILD)/$(TARGET): $(WORKLOAD_BUILD)/$(TARGET).dbg | $(WORKLOAD_BUILD)
	$(CP) $(WORKLOAD_BUILD)/$(TARGET).dbg $(WORKLOAD_BUILD)/$(TARGET)

# libocr
# Make this a .PHONY target so that we check if any of the
# OCR source files have changed
.PHONY: libocr

ifeq ($(RUN_JENKINS), run)
libocr: ;
else ifeq ($(RUN_JENKINS), build)
# If we are running in Jenkins, we do not need to recheck if the file exists
libocr: $(OCR_INSTALL)/lib/libocr.so
$(OCR_INSTALL)/lib/libocr.so: $(OCR_BUILD)/Makefile
	@echo "Building libocr.so..."
	OCR_TYPE=$(OCR_TYPE) OCR_INSTALL=$(OCR_INSTALL) OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD) $(TARGETS) install
else
libocr: $(OCR_BUILD)/Makefile
	@echo "Checking on libocr.so..."
	OCR_TYPE=$(OCR_TYPE) OCR_INSTALL=$(OCR_INSTALL) OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD) $(TARGETS) install
endif

.PHONY: clean
clean:
	-$(RM) -rf $(WORKLOAD_BUILD)

.PHONY: uninstall
uninstall:
	-$(RM) -rf $(WORKLOAD_INSTALL)

$(WORKLOAD_BUILD):
	mkdir -p $(WORKLOAD_BUILD)

$(WORKLOAD_INSTALL):
	mkdir -p $(WORKLOAD_INSTALL)
