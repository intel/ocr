#
# OCR top level directory
#
OCRDIR := ../../runtime/ocr-x86

#
# Object & dependence file subdirectory
#
OBJDIR := objs

#
# Generate a list of all source files and the respective objects
#
SRCS   := $(shell find $(OCRDIR)/src -name '*.[csS]' -print)
OBJS   := $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))

#
# Generate a source search path
#
VPATH  := $(shell find $(OCRDIR)/src -type d -print)

#
# Global CFLAGS to be passed into all architecture builds
#
CFLAGS := -I . -I $(OCRDIR)/inc -I $(OCRDIR)/src -I $(OCRDIR)/src/inc -DELS_USER_SIZE=0

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
all: $(OBJS)
	@echo "$(ARCH): ALL RULE"

debug: $(OBJS)
	@echo "$(ARCH): DEBUG RULE"

clean:
	-$(RM) $(RMFLAGS) $(OBJDIR)/*

