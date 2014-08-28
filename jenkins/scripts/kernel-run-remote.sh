#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage <sub-directory> <arch>"
    exit 1
fi

WORKLOAD_INSTALL=${WORKLOAD_INSTALL_ROOT}/$2

echo "Running kernel '$1' for architecture '$2'"
WORKLOAD_EXEC=${WORKLOAD_INSTALL}
mkdir -p ${WORKLOAD_EXEC}/logs
rm -rf ${WORKLOAD_EXEC}/logs/*
cd ${WORKLOAD_EXEC}
WORKLOAD_EXEC=${WORKLOAD_EXEC} ${TG_INSTALL}/bin/fsim-scripts/fsim-torque.sh -s -c ${WORKLOAD_INSTALL}/config.cfg
RETURN_CODE=$?

if [ $RETURN_CODE -eq 0 ]; then
    echo "**** Run SUCCESS ****"
else
    echo "**** Run FAILURE ****"
fi

exit $RETURN_CODE
