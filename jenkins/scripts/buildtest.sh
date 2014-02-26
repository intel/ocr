#!/bin/bash

# Arguments in non '_params' case:
# <ARCH to use> <directory to execute in> <command line to build>
if [ $1 == "_params" ]; then
    exit 0
else
    export OCR_INSTALL=${JJOB_START_HOME}/ocr/install/$1/
    export LD_LIBRARY_PATH=${OCR_INSTALL}/lib/
    cd $2
    shift 2
    $@
    RESCODE=$?
    if [ $RESCODE -eq 0 ]; then
        echo "Test built successfully -- SUCCESS"
        exit 0
    else
        echo "Test did not build -- FAILURE"
        exit $RESCODE
    fi
fi
