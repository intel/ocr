# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.


# Get absolute paths. This is useful since we go in
# subfolders to do make and that screws up the relative
# paths
ifeq ($(RUN_JENKINS), run)
TG_SRC                :=
OCR_SRC               :=
OCR_BUILD_ROOT        :=
OCR_BUILD_XE          :=
OCR_BUILD_CE          :=
WORKLOAD_BUILD_ROOT   :=
WORKLOAD_BUILD        :=
APPS_LIBS_ROOT        :=
WORKLOAD_SRC          :=
else
TG_SRC                := $(shell cd "${TG_SRC}" && pwd)
OCR_SRC               := $(shell cd "${OCR_SRC}" && pwd)
OCR_BUILD_ROOT        := $(shell cd "${OCR_BUILD_ROOT}" && pwd)
OCR_BUILD_XE          := ${OCR_BUILD_ROOT}/${XE_TARGET}
OCR_BUILD_CE          := ${OCR_BUILD_ROOT}/${CE_TARGET}
WORKLOAD_BUILD_ROOT   := $(shell mkdir -p "${WORKLOAD_BUILD_ROOT}" && cd "${WORKLOAD_BUILD_ROOT}" && pwd)
WORKLOAD_BUILD        := ${WORKLOAD_BUILD_ROOT}/${OCR_TYPE}
APPS_LIBS_ROOT        := $(shell cd "${APPS_LIBS_ROOT}" && pwd)
WORKLOAD_SRC          := $(shell cd "${WORKLOAD_SRC}" && pwd)
endif

APPS_ROOT             := $(shell cd "${APPS_ROOT}" && pwd)

HOSTNAME              := $(shell hostname)

TG_INSTALL            := $(shell cd "${TG_INSTALL}" && pwd)

OCR_INSTALL_ROOT := $(shell mkdir -p ${OCR_INSTALL_ROOT} && cd "${OCR_INSTALL_ROOT}" && pwd)
OCR_INSTALL_XE   := ${OCR_INSTALL_ROOT}/${XE_TARGET}
OCR_INSTALL_CE   := ${OCR_INSTALL_ROOT}/${CE_TARGET}

FIX_CE_CONFIG    ?= ${OCR_SRC}/scripts/Configs/ce_config_fix.py
OCR_CE_CONFIG    ?= $(OCR_INSTALL_CE)/config/default.cfg
OCR_XE_CONFIG    ?= $(OCR_INSTALL_XE)/config/default.cfg

WORKLOAD_INSTALL_ROOT := $(shell mkdir -p "${WORKLOAD_INSTALL_ROOT}" && cd "${WORKLOAD_INSTALL_ROOT}" && pwd)
WORKLOAD_INSTALL      := ${WORKLOAD_INSTALL_ROOT}/${OCR_TYPE}

REQ_LIBS         ?=

PREFIX           := ${TG_INSTALL}/bin/rmd-linux-elf

AS	= $(PREFIX)-as
ASFLAGS	=

AR	= $(PREFIX)-ar
ARFLAGS	= rcs

CC	= $(PREFIX)-clang
CFLAGS_BASE	= -frmd-extensions -I $(OCR_INSTALL_XE)/include -DTG_ARCH -I $(APPS_LIBS_ROOT)/include -I $(TG_INSTALL)/include

OBJCOPY	= $(PREFIX)-objcopy
STRIP	= $(PREFIX)-strip

LD	= $(PREFIX)-ld
LDFLAGS_BASE	= -L $(APPS_LIBS_ROOT)/lib -L $(TG_INSTALL)/ld-scripts -T fsim-xe-icache.ld -static -Map=$(WORKLOAD_BUILD)/out.map

CUT	= cut
GREP	= grep
SED     = sed
RM	= rm
CP      = cp
MKDIR   = mkdir


CFLAGS  := $(CFLAGS_BASE) $(CFLAGS)
LDFLAGS := $(LDFLAGS_BASE) $(LDFLAGS)

OBJS := $(addprefix $(WORKLOAD_BUILD)/,$(notdir $(SRCS:.c=.o)))

FULL_REQ_LIBS := $(addprefix $(APPS_ROOT)/libs/src/,$(REQ_LIBS))

ifeq ($(RUN_TORQUE), yes)
RUN_CMD := $(TG_INSTALL)/bin/fsim-scripts/fsim-wrapper.sh
MACHINE_CONFIG :=
else
RUN_CMD := $(TG_INSTALL)/bin/fsim
MACHINE_CONFIG ?= $(TG_INSTALL)/fsim-configs/localhost.cfg
MACHINE_CONFIG := -c $(MACHINE_CONFIG)
endif

ifeq ($(RUN_JENKINS), run)
RUN_CMD := $(TG_INSTALL)/bin/fsim-scripts/fsim-torque.sh
MACHINE_CONFIG :=
endif

.PHONY: install
.ONESHELL:
ifeq ($(RUN_JENKINS), run)
install: | ${WORKLOAD_INSTALL}
else
install: $(WORKLOAD_SRC)/config.cfg | ${WORKLOAD_INSTALL}
endif
	@echo "Installing from $(WORKLOAD_BUILD) into $(WORKLOAD_INSTALL)..."
	@echo ""
	@echo "####"
	@echo "To run, use 'OCR_TYPE=${OCR_TYPE} make run <args>' or run manually:"
	@echo "(from $(WORKLOAD_INSTALL)): ${RUN_CMD} -s ${MACHINE_CONFIG} -c ${WORKLOAD_INSTALL}/config.cfg"
	@echo "####"
	@$(RM) -f $(WORKLOAD_INSTALL)/$(TARGET)* $(WORKLOAD_INSTALL)/config.cfg $(WORKLOAD_INSTALL)/rmdkrnl
	@$(CP) $(WORKLOAD_BUILD)/$(TARGET) $(WORKLOAD_INSTALL)/
	@$(CP) $(WORKLOAD_BUILD)/$(TARGET).blob $(WORKLOAD_INSTALL)/
	@$(CP) $(WORKLOAD_SRC)/config.cfg $(WORKLOAD_INSTALL)/
	@$(CP) $(WORKLOAD_BUILD)/build.rmdkrnl/rmdkrnl $(WORKLOAD_INSTALL)/

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
.SHELLFLAGS=-ec
run_internal: WORKLOAD_EXEC ?= $(WORKLOAD_INSTALL)
ifeq ($(RUN_JENKINS), run)
run_internal:
else
run_internal: install
endif
ifeq ($(HOSTNAME), bar1)
	$(error Please refrain from running jobs on bar1!)
endif
	@$(MKDIR) -p $(WORKLOAD_EXEC)/logs
	@$(RM) -rf $(WORKLOAD_EXEC)/logs/*
	@$(RM) -f $(WORKLOAD_EXEC)/console.out
	cd $(WORKLOAD_EXEC)
	WORKLOAD_EXEC=$(WORKLOAD_EXEC) WORKLOAD_INSTALL=$(WORKLOAD_INSTALL) $(RUN_CMD) -s $(MACHINE_CONFIG) -c $(WORKLOAD_INSTALL)/config.cfg
	@$(GREP) -H "CONSOLE: XE[0-7]" logs/*brd*.chp*.unt*.blk*.CE.00 | $(SED) 's/.*\.blk\([0-9]\+\)\.CE\.00:CONSOLE:/BLK\1/' > console.out

postrun: run_internal
postrun: WORKLOAD_OUTS := `find -L $(WORKLOAD_EXEC)/logs/* -name '*.app_output' -print`

.PHONY: run
run: postrun

.PHONY: default
default: reqlibs $(WORKLOAD_BUILD)/$(TARGET) $(WORKLOAD_BUILD)/$(TARGET).blob \
  $(WORKLOAD_BUILD)/build.rmdkrnl/rmdkrnl

# Builds any libraries required by the workload
.PHONY: parallel-reqlibs
parallel-reqlibs: $(FULL_REQ_LIBS)

.PHONY: $(FULL_REQ_LIBS)
$(FULL_REQ_LIBS):
	ARCH=tg APPS_ROOT=$(APPS_ROOT) APPS_LIBS_ROOT=$(APPS_LIBS_ROOT) \
	$(MAKE) -C $@ install

.PHONY: reqlibs
reqlibs: parallel-reqlibs

# Require libocrxe as this means all include files are up-to-date
$(WORKLOAD_BUILD)/%.o: $(WORKLOAD_SRC)/%.c | libocrxe $(WORKLOAD_BUILD)
	$(CC) $(CFLAGS) -fno-builtin -c -o $@ $<
#	$(CC) $(CFLAGS) -fno-builtin -c -o $@.tmp $<
#	$(OBJCOPY) -R .data_local -R .rodata_local -R .bss_local -R .data_block -R .rodata_block -R .bss_block $@.tmp $@
#	$(RM) $@.tmp

# Two pre-requisites here that seem to be the same but serve different purposes:
#  - libocr.a: Execute this rule if libocr.a changes due to being rebuilt
#  - libocrxe: This rule will ALWAYS execute (it is a PHONY target) and it will
#              check if libocr.a needs to be rebuilt. If we listed this as a
#              regular pre-requisite, we would *always* rebuild the blob (PHONY rules are infectious)
# Other rules for the blobs follow similar ideas
$(WORKLOAD_BUILD)/$(TARGET).dbg: $(OBJS) $(OCR_INSTALL_XE)/lib/libocr.a | libocrxe $(WORKLOAD_BUILD)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) --whole-archive $(OCR_INSTALL_XE)/lib/libocr.a

$(WORKLOAD_BUILD)/$(TARGET): $(WORKLOAD_BUILD)/$(TARGET).dbg | $(WORKLOAD_BUILD)
	$(CP) $(WORKLOAD_BUILD)/$(TARGET).dbg $(WORKLOAD_BUILD)/$(TARGET)
	$(STRIP) $(WORKLOAD_BUILD)/$(TARGET)

# Build the final blob
# Note the order of the dependence. IMPORTANT!! Do not randomly change
$(WORKLOAD_BUILD)/$(TARGET).blob: $(OCR_SRC)/scripts/Blob/aggregate_binary_files.sh \
$(WORKLOAD_BUILD)/$(TARGET).blob.ce $(WORKLOAD_BUILD)/$(TARGET).blob.xe \
$(WORKLOAD_BUILD)/$(TARGET).blob.arg | $(WORKLOAD_BUILD)
	$^ $(WORKLOAD_INPUTS) $@

# Build the CE blob
$(WORKLOAD_BUILD)/$(TARGET).blob.ce: $(OCR_CE_CONFIG) $(WORKLOAD_SRC)/config.cfg \
$(OCR_INSTALL_ROOT)/$(CE_BUILDER)/bin/builder.exe \
$(WORKLOAD_BUILD)/build.rmdkrnl/rmdkrnl.dbg | rmdkrnl.dbg builderce $(WORKLOAD_BUILD)
	@echo "#### BUILDING CE BLOB ####"
	$(FIX_CE_CONFIG) --fsimcfg $(WORKLOAD_SRC)/config.cfg --ocrcfg $(OCR_CE_CONFIG)
	TG_INSTALL=$(TG_INSTALL) WORKLOAD_BUILD=$(WORKLOAD_BUILD) STRUCT_OUT=$(WORKLOAD_BUILD)/$(TARGET).blob.ce \
$(OCR_INSTALL_ROOT)/$(CE_BUILDER)/bin/builder.exe -ocr:cfg $(OCR_CE_CONFIG)

$(OCR_CE_CONFIG): | rmdkrnl

# Build the XE blob
$(WORKLOAD_BUILD)/$(TARGET).blob.xe: debug_post_prerun $(OCR_INSTALL_XE)/config/default.cfg \
$(WORKLOAD_BUILD)/$(TARGET).dbg $(OCR_INSTALL_ROOT)/$(XE_BUILDER)/bin/builder.exe | builderxe $(WORKLOAD_BUILD)
	@echo "#### BUILDING XE BLOB ####"
	TG_INSTALL=$(TG_INSTALL) WORKLOAD_BUILD=$(WORKLOAD_BUILD) STRUCT_OUT=$(WORKLOAD_BUILD)/$(TARGET).blob.xe \
	ARGS_OUT=$(WORKLOAD_BUILD)/$(TARGET).blob.arg APP_FILE=$(WORKLOAD_BUILD)/$(TARGET).dbg \
	$(OCR_INSTALL_ROOT)/$(XE_BUILDER)/bin/builder.exe -ocr:cfg $(OCR_XE_CONFIG) $(WORKLOAD_ARGS)

$(OCR_XE_CONFIG): | libocrxe

# The arg blob is build automatically by the XE builder
$(WORKLOAD_BUILD)/$(TARGET).blob.arg: $(WORKLOAD_BUILD)/$(TARGET).blob.xe | $(WORKLOAD_BUILD)

# XE libocr
# Phony target to check on build everytime
.PHONY: libocrxe

ifeq ($(RUN_JENKINS), run)
# If running Jenkins, on TG, we have to be agressive and assume all is OK
# With run, we do not even attempt to build more stuff
libocrxe: ;
$(OCR_INSTALL_XE)/lib/libocr.a: ;
else ifeq ($(RUN_JENKINS), build)
# This set of rules will:
#  - check if libocr.a has been modified
#  - if it has, it will rebuild it
#  - these rules do *not* check if a source modification happened in OCR's tree
libocrxe: $(OCR_INSTALL_XE)/lib/libocr.a
$(OCR_INSTALL_XE)/lib/libocr.a: $(OCR_BUILD_XE)/Makefile
	@echo "#### BUILDING XE libocr.a ####"
	OCR_TYPE=$(XE_TARGET) OCR_INSTALL=$(OCR_INSTALL_XE) OCR_SRC=$(OCR_SRC) \
	$(MAKE) -C $(OCR_BUILD_XE) $(TARGETS) install
else
# This set of rules is the safest (will catch all changes)
# It will *always* run Make on libocrxe which will check for
# any modification to OCR's source
libocrxe: $(OCR_BUILD_XE)/Makefile
	@echo "#### CHECKING XE libocr.a ####"
	OCR_TYPE=$(XE_TARGET) OCR_INSTALL=$(OCR_INSTALL_XE) OCR_SRC=$(OCR_SRC) \
	$(MAKE) -C $(OCR_BUILD_XE) $(TARGETS) install
$(OCR_INSTALL_XE)/lib/libocr.a: | libocrxe
endif

# Note: builderxe, rmdkrnl, builderce all follow the same idea as libocrxe

# XE builder
# Phony target to check on build everytime
.PHONY: builderxe

ifeq ($(RUN_JENKINS), run)
# If running Jenkins, no need to recheck for source change
builderxe: ;
$(OCR_INSTALL_ROOT)/$(XE_BUILDER)/bin/builder.exe: ;
else ifeq ($(RUN_JENKINS), build)

builderxe: $(OCR_INSTALL_ROOT)/$(XE_BUILDER)/bin/builder.exe
$(OCR_INSTALL_ROOT)/$(XE_BUILDER)/bin/builder.exe: $(OCR_BUILD_ROOT)/$(XE_BUILDER)/Makefile
	@echo "#### BUILDING XE BUILDER ####"
	OCR_TYPE=$(XE_BUILDER) OCR_INSTALL=$(OCR_INSTALL_ROOT)/$(XE_BUILDER) \
	OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD_ROOT)/$(XE_BUILDER) $(TARGETS) install
else

builderxe: $(OCR_BUILD_ROOT)/$(XE_BUILDER)/Makefile
	@echo "#### CHECKING XE BUILDER ####"
	OCR_TYPE=$(XE_BUILDER) OCR_INSTALL=$(OCR_INSTALL_ROOT)/$(XE_BUILDER) \
	OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD_ROOT)/$(XE_BUILDER) $(TARGETS) install
$(OCR_INSTALL_ROOT)/$(XE_BUILDER)/bin/builder.exe: | builderxe
endif

# Local version of the RMDKRNL (built with the CE's libocr)
# We build locally so we can run without impacting TG_INSTALL
# This will internally build libocrce.a if needed (ie: we don't build it
# explicitly here)
.PHONY: rmdkrnl.dbg

ifeq ($(RUN_JENKINS), run)
rmdkrnl.dbg: ;
$(WORKLOAD_BUILD)/build.rmdkrnl/rmdkrnl.dbg: ;
else ifeq ($(RUN_JENKINS), build)

rmdkrnl.dbg: $(WORKLOAD_BUILD)/build.rmdkrnl/rmdkrnl
.ONESHELL:
$(WORKLOAD_BUILD)/build.rmdkrnl/rmdkrnl.dbg: $(TG_SRC)/build/build.rmdkrnl/make-me.make \
| $(WORKLOAD_BUILD)/build.rmdkrnl
	@echo "#### BUILDING RMDKRNL FROM $(TG_SRC) ####"
	OCR_SRC=$(OCR_SRC) OCR_TARGET=$(CE_TARGET) OCR_INSTALL=$(OCR_INSTALL_CE) OCR_BUILD=$(OCR_BUILD_CE) \
	TG_SRC=$(TG_SRC) TG_INSTALL=$(TG_INSTALL) OCRAPPS_RUN_JENKINS=$(RUN_JENKINS) \
	$(MAKE) -C $(WORKLOAD_BUILD)/build.rmdkrnl -f make-me.make $(TARGETS)
else

.ONESHELL:
rmdkrnl.dbg: $(TG_SRC)/build/build.rmdkrnl/make-me.make | $(WORKLOAD_BUILD)/build.rmdkrnl
	@echo "#### CHECKING RMDKRNL FROM $(TG_SRC) ####"
	OCR_SRC=$(OCR_SRC) OCR_TARGET=$(CE_TARGET) OCR_INSTALL=$(OCR_INSTALL_CE) OCR_BUILD=$(OCR_BUILD_CE) \
	TG_SRC=$(TG_SRC) TG_INSTALL=$(TG_INSTALL) OCRAPPS_RUN_JENKINS=$(RUN_JENKINS) \
	$(MAKE) -C $(WORKLOAD_BUILD)/build.rmdkrnl -f make-me.make $(TARGETS)
$(WORKLOAD_BUILD)/build.rmdkrnl/rmdkrnl.dbg: | rmdkrnl.dbg
endif

.PHONY: rmdkrnl
rmdkrnl: rmdkrnl.dbg

$(WORKLOAD_BUILD)/build.rmdkrnl/rmdkrnl: $(WORKLOAD_BUILD)/build.rmdkrnl/rmdkrnl.dbg

# CE builder
.PHONY: builderce

ifeq ($(RUN_JENKINS), run)
builderce: ;
$(OCR_INSTALL_ROOT)/$(CE_BUILDER)/bin/builder.exe: ;

else ifeq ($(RUN_JENKINS), build)
builderce: $(OCR_INSTALL_ROOT)/$(CE_BUILDER)/bin/builder.exe
$(OCR_INSTALL_ROOT)/$(CE_BUILDER)/bin/builder.exe: $(OCR_BUILD_ROOT)/$(CE_BUILDER)/Makefile
	@echo "#### BUILDING CE BUILDER ####"
	OCR_TYPE=$(CE_BUILDER) OCR_INSTALL=$(OCR_INSTALL_ROOT)/$(CE_BUILDER) \
	OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD_ROOT)/$(CE_BUILDER) $(TARGETS) install
else

builderce: $(OCR_BUILD_ROOT)/$(CE_BUILDER)/Makefile
	@echo "#### CHECKING CE BUILDER ####"
	OCR_TYPE=$(CE_BUILDER) OCR_INSTALL=$(OCR_INSTALL_ROOT)/$(CE_BUILDER) \
	OCR_SRC=$(OCR_SRC) $(MAKE) -C $(OCR_BUILD_ROOT)/$(CE_BUILDER) $(TARGETS) install
$(OCR_INSTALL_ROOT)/$(CE_BUILDER)/bin/builder.exe: | builderce
endif

.PHONY: clean
clean:
	-$(RM) -rf $(WORKLOAD_BUILD)

.PHONY: uninstall
uninstall:
	-$(RM) -rf $(WORKLOAD_INSTALL)

$(WORKLOAD_BUILD):
	mkdir -p $(WORKLOAD_BUILD)

$(WORKLOAD_BUILD)/build.rmdkrnl: $(TG_SRC)/build/build.rmdkrnl/make-me.make | $(WORKLOAD_BUILD)
	mkdir -p $(WORKLOAD_BUILD)/build.rmdkrnl
	-@$(RM) -rf $(WORKLOAD_BUILD)/build.rmdkrnl/*
	@$(CP) $(TG_SRC)/build/build.rmdkrnl/make-me.make $(WORKLOAD_BUILD)/build.rmdkrnl

$(WORKLOAD_INSTALL):
	mkdir -p $(WORKLOAD_INSTALL)
