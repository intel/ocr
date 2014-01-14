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

#
# Global CFLAGS to be passed into all architecture builds
# concatenated with the architecture-specific CFLAGS at the end
#
CFLAGS := -g -I . -I $(OCRDIR)/inc -I $(OCRDIR)/src -I $(OCRDIR)/src/inc -DELS_USER_SIZE=0 $(CFLAGS)

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
