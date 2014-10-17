ifeq (run,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run" (only if there are some)
  ifneq (1, $(words $(MAKECMDGOALS)))
    WORKLOAD_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
    $(eval WORKLOAD_UNIQ_ARGS := $(shell echo "${WORKLOAD_ARGS}" | tr '[:blank:]' '\n' | sort -u | tr '\n' ' '))
    $(eval .PHONY:${WORKLOAD_UNIQ_ARGS})
    $(eval ${WORKLOAD_UNIQ_ARGS}:;@:)
  endif
endif

# Extract input files and regular arguments
ifneq (${WORKLOAD_INPUT_FILE_IDX}, 0)
  WORKLOAD_INPUTS := $(wordlist ${WORKLOAD_INPUT_FILE_IDX}, $(words ${WORKLOAD_ARGS}), ${WORKLOAD_ARGS})
  WORKLOAD_ARGS   := $(wordlist 1, $(shell dc -e '${WORKLOAD_INPUT_FILE_IDX} 1 - p'), ${WORKLOAD_ARGS})
endif
