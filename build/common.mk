#
# OCR top level directory
#
OCR_SRC ?= ../..
OCR_INSTALL_ROOT ?= ../../install

OCR_BUILD := .

#
# Make sure we have absolute paths
#
OCR_SRC := $(shell cd "${OCR_SRC}" && pwd)
OCR_INSTALL_ROOT := $(shell mkdir -p "${OCR_INSTALL_ROOT}" && cd "${OCR_INSTALL_ROOT}" && pwd)
OCR_BUILD := $(shell cd "${OCR_BUILD}" && pwd)

OCR_INSTALL := $(OCR_INSTALL_ROOT)/$(OCR_TYPE)

#
# Object & dependence file subdirectory
#
OBJDIR := $(OCR_BUILD)/objs

#
# Default machine configuration
#
DEFAULT_CONFIG ?= mach-hc-4w.cfg

####################################################
# User Configurable settings
####################################################

# Number of elements in EDT local storage
# CFLAGS += -DELS_USER_SIZE=0

# Valgrind compatibility for internal allocators
# (x86 only, requires valgrind-devel & inclusion of
# /usr/include in search path)
# CFLAGS += -DENABLE_VALGRIND

# Static size for deques used to contain EDTs
CFLAGS += -DINIT_DEQUE_CAPACITY=2048

# Maximum number of function nesting for runtime profiler
# CFLAGS += -DMAX_PROFILER_LEVEL=512

# Maximum number of characters handled by a single PRINTF
# CFLAGS += -DPRINTF_MAX=1024

# Maximum number of blocks of 64 slots that an EDT
# can have IFF it needs to acquire the same DB on
# multiple slots
# CFLAGS += -DOCR_MAX_MULTI_SLOT=1

# Enables the collection of runtime profiling (only x86)
# CFLAGS += -DOCR_RUNTIME_PROFILER

# Enables naming of EDTs for easier debugging
# CFLAGS += -DOCR_ENABLE_EDT_NAMING

# Enables the collection of EDT R/W statistics
# (only on x86, requires OCR_ENABLE_EDT_NAMING)
# CFLAGS += -DOCR_ENABLE_EDT_PROFILING

# Enables data collection for execution timeline visualizer
# (x86 only.  Requires -DOCR_ENABLE_EDT_NAMING and DEBUG_LVL_INFO)
# CFLAGS += -DOCR_ENABLE_VISUALIZER -DOCR_ENABLE_EDT_NAMING

####################################################
# Experimental flags
####################################################

# Enable naming of EDTs
# This uses the name of the function to name the EDT templates and therefore
# the EDTs.
# NOTE: The application must also have this flag defined
# If this is note the case, an ASSERT will happen
# CFLAGS += -DOCR_ENABLE_EDT_NAMING

# Enable profiling data to be used by runtime. This requires EDT_NAMING
#CFLAGS += -DOCR_ENABLE_EDT_PROFILING -DOCR_ENABLE_EDT_NAMING

####################################################
# Debug flags
####################################################

# Debugging support
# Enable debug
CFLAGS += -DOCR_DEBUG
# Define level
 CFLAGS += -DOCR_DEBUG_LVL=DEBUG_LVL_WARN
#CFLAGS += -DOCR_DEBUG_LVL=DEBUG_LVL_INFO
# CFLAGS += -DOCR_DEBUG_LVL=DEBUG_LVL_VERB
# CFLAGS += -DOCR_DEBUG_LVL=DEBUG_LVL_VVERB

# Define which modules you want for debugging
# You can optionally define an individual debuging level by
# defining DEBUG_LVL_XXX like OCR_DEBUG_LEVEL. If not defined,
# the default will be used
CFLAGS += -DOCR_DEBUG_API
CFLAGS += -DOCR_DEBUG_ALLOCATOR -DDEBUG_LVL_ALLOCATOR=DEBUG_LVL_WARN
CFLAGS += -DOCR_DEBUG_COMP_PLATFORM
CFLAGS += -DOCR_DEBUG_COMM_PLATFORM
CFLAGS += -DOCR_DEBUG_COMM_API
CFLAGS += -DOCR_DEBUG_COMP_TARGET
CFLAGS += -DOCR_DEBUG_DATABLOCK
CFLAGS += -DOCR_DEBUG_EVENT
CFLAGS += -DOCR_DEBUG_GUID
CFLAGS += -DOCR_DEBUG_INIPARSING
CFLAGS += -DOCR_DEBUG_MACHINE
CFLAGS += -DOCR_DEBUG_MEM_PLATFORM
CFLAGS += -DOCR_DEBUG_MEM_TARGET
CFLAGS += -DOCR_DEBUG_POLICY
CFLAGS += -DOCR_DEBUG_SCHEDULER
CFLAGS += -DOCR_DEBUG_STATS #-DDEBUG_LVL_STATS=DEBUG_LVL_VVERB
CFLAGS += -DOCR_DEBUG_TASK
CFLAGS += -DOCR_DEBUG_UTIL -DDEBUG_LVL_UTIL=DEBUG_LVL_WARN
CFLAGS += -DOCR_DEBUG_WORKER
CFLAGS += -DOCR_DEBUG_WORKPILE

#
# Global CFLAGS to be passed into all architecture builds
# concatenated with the architecture-specific CFLAGS at the end

CFLAGS := -g -Wall $(CFLAGS) $(CFLAGS_USER)

################################################################
# END OF USER CONFIGURABLE OPTIONS                             #
################################################################

#
# Generate a list of all source files and the respective objects
#
SRCS   := $(shell find -L $(OCR_SRC)/src -name '*.[csS]' -print)

#
# Generate a source search path
#
VPATH  := $(shell find -L $(OCR_SRC)/src -type d -print)

ifneq (,$(findstring OCR_RUNTIME_PROFILER,$(CFLAGS)))
SRCSORIG = $(SRCS)
SRCS += $(OCR_BUILD)/src/profilerAutoGen.c
PROFILER_FILE=$(OCR_BUILD)/src/profilerAutoGen.c
CFLAGS += -I $(OCR_BUILD)/src
VPATH += $(OCR_BUILD)/src
else
PROFILER_FILE=
endif

OBJS_STATIC   := $(addprefix $(OBJDIR)/static/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))
OBJS_SHARED   := $(addprefix $(OBJDIR)/shared/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))
OBJS_EXEC     := $(addprefix $(OBJDIR)/exec/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))


# Update include paths
CFLAGS := -I . -I $(OCR_SRC)/inc -I $(OCR_SRC)/src -I $(OCR_SRC)/src/inc $(CFLAGS)

# Static library name (only set if not set in OCR_TYPE specific file)
ifeq (${SUPPORTS_STATIC}, yes)
OCRSTATIC ?= libocr.a
OCRSTATIC := $(OCR_BUILD)/$(OCRSTATIC)
CFLAGS_STATIC ?=
CFLAGS_STATIC := ${CFLAGS} ${CFLAGS_STATIC}
endif
# Shared library name (only set if not set in OCR_TYPE specific file)
ifeq (${SUPPORTS_SHARED}, yes)
CFLAGS_SHARED ?=
CFLAGS_SHARED := ${CFLAGS} ${CFLAGS_SHARED}
OCRSHARED ?= libocr.so
OCRSHARED := $(OCR_BUILD)/$(OCRSHARED)
endif
# Executable name (only set if not set in OCR_TYPE specific file)
ifeq (${SUPPORTS_EXEC}, yes)
CFLAGS_EXEC ?=
CFLAGS_EXEC := ${CFLAGS} ${CFLAGS_EXEC}
OCREXEC ?= builder.exe
OCREXEC := $(OCR_BUILD)/$(OCREXEC)
endif


#
# Build targets
#

.PHONY: static
static: CFLAGS_STATIC += -O2
static: supports-static info-static $(OCRSTATIC)

.PHONY: shared
shared: CFLAGS_SHARED += -O2
shared: supports-shared info-shared $(OCRSHARED)

.PHONY: exec
exec: CFLAGS_EXEC += -O2
exec: supports-exec info-exec $(OCREXEC)

.PHONY: debug-static
debug-static: CFLAGS_STATIC += -O0
debug-static: supports-static info-static $(OCRSTATIC)

.PHONY: debug-shared
debug-shared: CFLAGS_SHARED += -O0
debug-shared: supports-shared info-shared $(OCRSHARED)

.PHONY: debug-exec
debug-exec: CFLAGS_EXEC += -O0
debug-exec: supports-exec info-exec $(OCREXEC)

# This is for the profiler-generated file
$(OCR_BUILD)/src:
	@$(MKDIR) -p $(OCR_BUILD)/src

# Static target

.PHONY: supports-static
supports-static:
ifneq (${SUPPORTS_STATIC}, yes)
	$(error OCR type ${OCR_TYPE} does not support static library building)
endif

${OBJDIR}/static:
	@$(MKDIR) -p $(OBJDIR)/static


.PHONY: info-static
info-static:
	@printf "\033[32m>>>> Compile command for .c files is\033[1;30m '$(CC) $(CFLAGS_STATIC) -MMD -c <src> -o <obj>'\033[0m\n"
	@printf "\033[32m>>>> Building a static library with\033[1;30m '$(AR) $(ARFLAGS)'\033[0m\n"

$(OCRSTATIC): $(OBJS_STATIC)
	@echo "Linking static library ${OCRSTATIC}"
	@$(AR) $(ARFLAGS) $(OCRSTATIC) $^
	@$(RANLIB) $(OCRSTATIC)


# Shared target
.PHONY: supports-shared
supports-shared:
ifneq (${SUPPORTS_SHARED}, yes)
	$(error OCR type ${OCR_TYPE} does not support shared library building)
endif

${OBJDIR}/shared:
	@$(MKDIR) -p ${OBJDIR}/shared

.PHONY: info-shared
info-shared:
	@printf "\033[32m>>>> Compile command for .c files is\033[1;30m '$(CC) $(CFLAGS_SHARED) -MMD -c <src> -o <obj>'\033[0m\n"
	@printf "\033[32m>>>> Building a shared library with\033[1;30m '$(CC) $(LDFLAGS)'\033[0m\n"

$(OCRSHARED): $(OBJS_SHARED)
	@echo "Linking shared library ${OCRSHARED}"
	@$(CC) $(LDFLAGS) -o $(OCRSHARED) $^

# Exec target

.PHONY: supports-exec
supports-exec:
ifneq (${SUPPORTS_EXEC}, yes)
	$(error OCR type ${OCR_TYPE} does not support executable binary building)
endif

${OBJDIR}/exec:
	@$(MKDIR) -p $(OBJDIR)/exec


.PHONY: info-exec
info-exec:
	@printf "\033[32m>>>> Compile command for .c files is\033[1;30m '$(CC) $(CFLAGS_EXEC) -MMD -c <src> -o <obj>'\033[0m\n"

$(OCREXEC): $(OBJS_EXEC)
	@echo "Linking executable binary ${OCREXEC}"
	@$(CC) -o $(OCREXEC) $^ $(EXEFLAGS)


#
# Objects build rules
#

$(PROFILER_FILE): $(SRCSORIG) | $(OCR_BUILD)/src
	@echo "Generating profile file..."
	@$(OCR_SRC)/scripts/Profiler/generateProfilerFile.py $(OCR_SRC)/src $(OCR_BUILD)/src/profilerAutoGen h,c .git profiler
	@echo "\tDone."

$(OBJDIR)/static/%.o: %.c Makefile ../common.mk $(PROFILER_FILE) | $(OBJDIR)/static
	@echo "Compiling $<"
	@$(CC) $(CFLAGS_STATIC) -MMD -c $< -o $@

$(OBJDIR)/shared/%.o: %.c Makefile ../common.mk $(PROFILER_FILE) | $(OBJDIR)/shared
	@echo "Compiling $<"
	@$(CC) $(CFLAGS_SHARED) -MMD -c $< -o $@

$(OBJDIR)/exec/%.o: %.c Makefile ../common.mk | $(OBJDIR)/exec
	@echo "Compiling $<"
	@$(CC) $(CFLAGS_EXEC) -MMD -c $< -o $@

$(OBJDIR)/static/%.o: %.S Makefile ../common.mk | $(OBJDIR)/static
	@echo "Assembling $<"
	@$(CC) $(CFLAGS_STATIC) -MMD -c $< -o $@

$(OBJDIR)/shared/%.o: %.S Makefile ../common.mk | $(OBJDIR)/shared
	@echo "Assembling $<"
	@$(CC) $(CFLAGS_SHARED) -MMD -c $< -o $@

#
# Include auto-generated dependence files
#
ifeq (${SUPPORTS_STATIC}, yes)
-include $(wildcard $(OBJDIR)/static/*.d)
endif
ifeq (${SUPPORTS_SHARED}, yes)
-include $(wildcard $(OBJDIR)/shared/*.d)
endif
ifeq (${SUPPORTS_EXEC}, yes)
-include $(wildcard $(OBJDIR)/exec/*.d)
endif

# Install
INSTALL_TARGETS :=
INSTALL_LIBS    :=
INSTALL_EXES    := $(OCRRUNNER)
ifeq (${SUPPORTS_STATIC}, yes)
INSTALL_TARGETS += static
INSTALL_LIBS += $(OCRSTATIC)
endif
ifeq (${SUPPORTS_SHARED}, yes)
INSTALL_TARGETS += shared
INSTALL_LIBS += $(OCRSHARED)
endif
ifeq (${SUPPORTS_EXEC}, yes)
INSTALL_TARGETS += exec
INSTALL_EXES += $(OCREXEC)
endif

MACHINE_CONFIGS   := $(notdir $(wildcard $(OCR_SRC)/machine-configs/$(OCR_TYPE)/*))
INC_FILES         := $(addprefix extensions/, $(notdir $(wildcard $(OCR_SRC)/inc/extensions/*))) \
                     $(notdir $(wildcard $(OCR_SRC)/inc/*))

INSTALLED_LIBS    := $(addprefix $(OCR_INSTALL)/lib/, $(notdir $(INSTALL_LIBS)))
BASE_LIBS         := $(firstword $(dir $(INSTALL_LIBS)))
INSTALLED_EXES    := $(addprefix $(OCR_INSTALL)/bin/, $(notdir $(INSTALL_EXES)))
BASE_EXES         := $(firstword $(dir $(INSTALL_EXES)))
INSTALLED_CONFIGS := $(addprefix $(OCR_INSTALL)/config/, $(MACHINE_CONFIGS))
INSTALLED_INCS    := $(addprefix $(OCR_INSTALL)/include/, $(INC_FILES))

$(OCR_INSTALL)/lib/%: $(BASE_LIBS)% | $(OCR_INSTALL)/lib
	@$(RM) -f $@
	@$(CP) $< $@

$(OCR_INSTALL)/bin/%: $(BASE_EXES)% | $(OCR_INSTALL)/bin
	@$(RM) -f $@
	@$(CP) $< $@

$(OCR_INSTALL)/config/%: $(OCR_SRC)/machine-configs/$(OCR_TYPE)/% | $(OCR_INSTALL)/config
	@$(RM) -f $@
	@$(CP) $< $@

$(OCR_INSTALL)/include/%: $(OCR_SRC)/inc/% | $(OCR_INSTALL)/include $(OCR_INSTALL)/include/extensions
	@$(RM) -f $@
	@$(CP) $< $@

$(OCR_INSTALL)/lib $(OCR_INSTALL)/bin $(OCR_INSTALL)/config $(OCR_INSTALL)/include \
$(OCR_INSTALL)/include/extensions :
	@$(MKDIR) -p $@

.PHONY: install
.ONESHELL:
install: ${INSTALL_TARGETS} ${INSTALLED_LIBS} ${INSTALLED_EXES} ${INSTALLED_CONFIGS} ${INSTALLED_INCS}
	@printf "\033[32m Installed '$(INSTALL_LIBS) $(INSTALL_EXES)' into '$(OCR_INSTALL)'\033[0m\n"
	@if [ -d $(OCR_SRC)/machine-configs/$(OCR_TYPE) ]; then \
		$(RM) -f $(OCR_INSTALL)/config/default.cfg; \
		$(LN) -s ./$(DEFAULT_CONFIG) $(OCR_INSTALL)/config/default.cfg; \
	fi

.PHONY: uninstall
.ONESHELL:
uninstall:
	-$(RM) $(RMFLAGS) $(OCR_INSTALL)/*

.PHONY:clean
clean:
	-$(RM) $(RMFLAGS) $(OBJDIR)/* $(OCRSHARED) $(OCRSTATIC) $(OCREXEC) src/*

