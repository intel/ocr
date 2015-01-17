found_run := $(strip $(foreach count, $(shell seq -s ' ' $(words $(MAKECMDGOALS))), $(if $(findstring run,$(word $(count), $(MAKECMDGOALS))),$(count))))
ifneq (,$(found_run))
  # use the rest as arguments for "run" (only if there are some)
  ifneq ($(found_run), $(words $(MAKECMDGOALS)))
    $(info Extracting WORKLOAD_ARGS from the command-line)
    WORKLOAD_ARGS := $(wordlist $(shell dc -e '$(found_run) 1 + p'), $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))
    $(eval WORKLOAD_UNIQ_ARGS := $(shell echo "${WORKLOAD_ARGS}" | tr '[:blank:]' '\n' | sort -u | tr '\n' ' '))
    $(eval .PHONY:${WORKLOAD_UNIQ_ARGS})
    $(eval ${WORKLOAD_UNIQ_ARGS}:;@:)
  endif
  $(info WORKLOAD_ARGS used: '$(WORKLOAD_ARGS)')
endif



# Extract input files and regular arguments
ifneq (${WORKLOAD_INPUT_FILE_IDX}, 0)
  WORKLOAD_INPUTS := $(wordlist ${WORKLOAD_INPUT_FILE_IDX}, $(words ${WORKLOAD_ARGS}), ${WORKLOAD_ARGS})
  WORKLOAD_ARGS   := $(wordlist 1, $(shell dc -e '${WORKLOAD_INPUT_FILE_IDX} 1 - p'), ${WORKLOAD_ARGS})
endif
