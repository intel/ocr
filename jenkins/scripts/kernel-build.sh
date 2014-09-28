#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage <sub-directory> <arch>"
    exit 1
fi

echo "Building kernel '$1' for architecture '$2'"
RUN_JENKINS=yes make -f ${WORKLOAD_SRC}/Makefile.$2 all install
RETURN_CODE=$?

if [ $RETURN_CODE -eq 0 ]; then
    echo "**** Build SUCCESS ****"
else
    echo "**** Build FAILURE ****"
fi

exit $RETURN_CODE
