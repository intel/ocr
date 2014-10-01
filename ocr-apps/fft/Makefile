# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.

MAKE := make

ifndef OCR_TYPE
  $(error Need to specify OCR_TYPE!)
endif

ifeq (run,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run" (only if there are some)
  ifneq (1, $(words $(MAKECMDGOALS)))
    ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
    $(eval UNIQ_ARGS := $(shell echo "${ARGS}" | tr '[:blank:]' '\n' | sort -u | tr '\n' ' '))
    $(eval .PHONY:${UNIQ_ARGS})
    $(eval ${UNIQ_ARGS}:;@:)
  endif
  ARGS := run $(ARGS)
else
  ARGS := $(MAKECMDGOALS)
endif

%::
	@$(MAKE) -f Makefile.$(OCR_TYPE) $(ARGS)
