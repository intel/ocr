#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage <sub-directory> <arch>"
    exit 1
fi

WORKLOAD_INSTALL=${WORKLOAD_INSTALL_ROOT}/$2

echo "Running kernel '$1' for architecture '$2'"
rm -f ${WORKLOAD_INSTALL}/runlog
RUN_JENKINS=run make -f ${WORKLOAD_SRC}/Makefile.$2 run 2>&1 1> ${WORKLOAD_INSTALL}/runlog
RETURN_CODE=$?

if [ $RETURN_CODE -eq 0 ]; then
    echo "**** Run SUCCESS ****"
else
    echo "**** Run FAILURE ****"
fi

exit $RETURN_CODE
