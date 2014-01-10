#
# OCR top level directory
#
OCRDIR := ../../runtime/ocr-x86

#
# Cook a list of all source files and their objects
#
SRCS   := $(shell find $(OCRDIR)/src -name '*.[csS]' -print)
OBJS   := $(addsuffix .o, $(basename $(notdir $(SRCS))))

#
# Cook a source search path -- all subdirs in the OCR tree
#
VPATH  := $(shell find $(OCRDIR)/src -type d -print)

#
# Global CFLAGS to be passed into all architecture builds
#
CFLAGS := -I . -I $(OCRDIR)/inc -I $(OCRDIR)/src -I $(OCRDIR)/src/inc -DELS_USER_SIZE=0
