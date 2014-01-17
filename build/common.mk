#
# OCR top level directory
#
OCRDIR := ../..
OCRLIB := libocr.a

#
# Object & dependence file subdirectory
#
OBJDIR := objs

#
# Generate a list of all source files and the respective objects
#
SRCS   := $(shell find -L $(OCRDIR)/src -name '*.[csS]' -print)
OBJS   := $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))

#
# Generate a source search path
#
VPATH  := $(shell find -L $(OCRDIR)/src -type d -print)

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
CFLAGS += -DOCR_DEBUG_LVL=DEBUG_LVL_VVERB
# Define which modules you want for debugging
# You can optionally define an individual debuging level by
# defining DEBUG_LVL_XXX like OCR_DEBUG_LEVEL. If not defined,                                                                                                                                                                        # the default will be used
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
CFLAGS += -DOCR_DEBUG_SYNC
CFLAGS += -DOCR_DEBUG_TASK
CFLAGS += -DOCR_DEBUG_WORKER
CFLAGS += -DOCR_DEBUG_WORKPILE


#
# Global CFLAGS to be passed into all architecture builds
# concatenated with the architecture-specific CFLAGS at the end

CFLAGS := -g -Wall -I . -I $(OCRDIR)/inc -I $(OCRDIR)/src -I $(OCRDIR)/src/inc -DELS_USER_SIZE=0 $(CFLAGS)

#
# Objects build rules
#
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(OBJDIR)/%.o: %.S
	$(CC) $(CFLAGS) -MMD -c $< -o $@

#
# Include auto-generated dependence files
#
-include $(wildcard $(OBJDIR)/*.d)

#
# Build targets
#
.PHONY: all
all: CFLAGS := -O2 $(CFLAGS)
all: default

.PHONY: debug
debug: CFLAGS := -O0 $(CFLAGS)
debug: default

.PHONY: default
default: $(OBJS)
	$(AR) $(ARFLAGS) $(OCRLIB) $^
	$(RANLIB) $(OCRLIB)

.PHONY: install
install: default
	$(CP) $(OCRLIB) $(OCRDIR)/install/$(ARCH)/lib
	$(CP) -r $(OCRDIR)/inc/* $(OCRDIR)/install/$(ARCH)/include
	$(CP) -r $(OCRDIR)/machine-configs/default.cfg $(OCRDIR)/install/$(ARCH)/config

.PHONY:clean
clean:
	-$(RM) $(RMFLAGS) $(OBJDIR)/*
	-$(RM) $(RMFLAGS) $(OCRDIR)/install/$(ARCH)/lib/*
	-$(RM) $(RMFLAGS) $(OCRDIR)/install/$(ARCH)/include/*
	-$(RM) $(RMFLAGS) $(OCRDIR)/install/$(ARCH)/config/default.cfg
