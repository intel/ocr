#
# OCR top level directory
#
OCRDIR ?= ../..

#
# Object & dependence file subdirectory
#
OBJDIR ?= objs

#
# Default machine configuration
#
DEFAULT_CONFIG ?= mach-hc-4w.cfg

# Debug flags

# Enable naming of EDTs
# This uses the name of the function to name the EDT templates and therefore
# the EDTs.
# NOTE: The application must also have this flag defined
# If this is note the case, an ASSERT will happen
#CFLAGS += -DOCR_ENABLE_EDT_NAMING

# Debugging support
# Enable debug
CFLAGS += -DOCR_DEBUG
# Define level
CFLAGS += -DOCR_DEBUG_LVL=DEBUG_LVL_WARN
# Define which modules you want for debugging
# You can optionally define an individual debuging level by
# defining DEBUG_LVL_XXX like OCR_DEBUG_LEVEL. If not defined,
# the default will be used
CFLAGS += -DOCR_DEBUG_ALLOCATOR #-DDEBUG_LVL_ALLOCATOR=DEBUG_LVL_VVERB
CFLAGS += -DOCR_DEBUG_COMP_PLATFORM
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
CFLAGS += -DOCR_DEBUG_UTIL
CFLAGS += -DOCR_DEBUG_WORKER
CFLAGS += -DOCR_DEBUG_WORKPILE


#
# Global CFLAGS to be passed into all architecture builds
# concatenated with the architecture-specific CFLAGS at the end

CFLAGS := -g -Wall -DELS_USER_SIZE=0 $(CFLAGS)

################################################################
# END OF USER CONFIGURABLE OPTIONS                             #
################################################################

#
# Generate a list of all source files and the respective objects
#
SRCS   := $(shell find -L $(OCRDIR)/src -name '*.[csS]' -print)
OBJS_STATIC   := $(addprefix $(OBJDIR)/static/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))
OBJS_SHARED   := $(addprefix $(OBJDIR)/shared/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))

#
# Generate a source search path
#
VPATH  := $(shell find -L $(OCRDIR)/src -type d -print)

# Update include paths
CFLAGS := -I . -I $(OCRDIR)/inc -I $(OCRDIR)/src -I $(OCRDIR)/src/inc $(CFLAGS)

# Static library name (only set if not set in ARCH specific file)
ifeq (${SUPPORTS_STATIC}, yes)
OCRSTATIC ?= libocr.a
CFLAGS_STATIC ?=
CFLAGS_STATIC := ${CFLAGS} ${CFLAGS_STATIC}
endif
# Shared library name (only set if not set in ARCH specific file)
ifeq (${SUPPORTS_SHARED}, yes)
CFLAGS_SHARED ?= 
CFLAGS_SHARED := ${CFLAGS} ${CFLAGS_SHARED}
OCRSHARED ?= libocr.so
endif


#
# Build targets
#
.PHONY: all
all: static

.PHONY: static
static: CFLAGS := -O2 $(CFLAGS)
static: supports-static info-static $(OCRSTATIC)

.PHONY: shared
shared: CFLAGS := -O2 $(CFLAGS)
shared: supports-shared info-shared $(OCRSHARED)

.PHONY: debug-static
debug-static: CFLAGS := -O0 $(CFLAGS)
debug-static: supports-static info-static $(OCRSTATIC)

.PHONY: debug-shared
debug-shared: CFLAGS := -O0 $(CFLAGS)
debug-shared: supports-shared info-shared $(OCRSHARED)

# Static target

.PHONY: supports-static
supports-static:
ifneq (${SUPPORTS_STATIC}, yes)
	$(error Architecture ${ARCH} does not support static library building)
endif

${OBJDIR}/static:
	@$(MKDIR) -p $(OBJDIR)/static


.PHONY: info-static
info-static:
	@echo -e "\e[32m>>>> Compile command for .c files is\e[1;30m '$(CC) $(CFLAGS_STATIC) -MMD -c <src> -o <obj>'\e[0m"
	@echo -e "\e[32m>>>> Building a static library with\e[1;30m '$(AR) $(ARFLAGS)'\e[0m"

$(OCRSTATIC): $(OBJS_STATIC)
	@echo "Linking static library ${OCRSTATIC}"
	@$(AR) $(ARFLAGS) $(OCRSTATIC) $^
	@$(RANLIB) $(OCRSTATIC)


# Shared target
.PHONY: supports-shared
supports-shared:
ifneq (${SUPPORTS_SHARED}, yes)
	$(error Architecture ${ARCH} does not support shared library building)
endif

${OBJDIR}/shared:
	@$(MKDIR) -p ${OBJDIR}/shared

.PHONY: info-shared
info-shared:
	@echo -e "\e[32m>>>> Compile command for .c files is\e[1;30m '$(CC) $(CFLAGS_SHARED) -MMD -c <src> -o <obj>'\e[0m"
	@echo -e "\e[32m>>>> Building a shared library with\e[1;30m '$(CC) $(LDFLAGS)'\e[0m"

$(OCRSHARED): $(OBJS_SHARED)
	@echo "Linking shared library ${OCRSHARED}"
	@$(CC) $(LDFLAGS) -o $(OCRSHARED) $^

#
# Objects build rules
#
$(OBJDIR)/static/%.o: %.c | $(OBJDIR)/static
	@echo "Compiling $<"
	@$(CC) $(CFLAGS_STATIC) -MMD -c $< -o $@

$(OBJDIR)/shared/%.o: %.c | $(OBJDIR)/shared
	@echo "Compiling $<"
	@$(CC) $(CFLAGS_SHARED) -MMD -c $< -o $@

$(OBJDIR)/static/%.o: %.S | $(OBJDIR)/static
	@echo "Assembling $<"
	@$(CC) $(CFLAGS_STATIC) -MMD -c $< -o $@

$(OBJDIR)/shared/%.o: %.S | $(OBJDIR)/shared
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

# Install
INSTALL_TARGETS :=
INSTALL_FILES := 
ifeq (${SUPPORTS_STATIC}, yes)
INSTALL_TARGETS += static
INSTALL_FILES += $(OCRSTATIC)
endif
ifeq (${SUPPORTS_SHARED}, yes)
INSTALL_TARGETS += shared
INSTALL_FILES += $(OCRSHARED)
endif

.PHONY: install
install: ${INSTALL_TARGETS}
	@echo -e "\e[32m Installing '$(INSTALL_FILES)' into '$(OCRDIR)/install/$(ARCH)'\e[0m"
	@$(CP) ${INSTALL_FILES} $(OCRDIR)/install/$(ARCH)/lib
	@$(CP) -r $(OCRDIR)/inc/* $(OCRDIR)/install/$(ARCH)/include
	@$(CP) -r $(OCRDIR)/machine-configs/* $(OCRDIR)/install/$(ARCH)/config
	-@$(LN) -fs ./$(DEFAULT_CONFIG) $(OCRDIR)/install/$(ARCH)/config/default.cfg

.PHONY: uninstall
uninstall: 
	-$(RM) $(RMFLAGS) $(OCRDIR)/install/$(ARCH)/lib/*
	-$(RM) $(RMFLAGS) $(OCRDIR)/install/$(ARCH)/include/*
	-$(RM) $(RMFLAGS) $(OCRDIR)/install/$(ARCH)/config/*

.PHONY:clean
clean:
	-$(RM) $(RMFLAGS) $(OBJDIR)/* $(OCRSHARED) $(OCRSTATIC)
