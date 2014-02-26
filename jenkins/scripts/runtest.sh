#!/bin/bash

# Arguments in non '_params' case:
# <ARCH to use> <directory to execute in> <golden file> <commandline to run>
if [ $1 == "_params" ]; then
    exit 0
else
    export OCR_INSTALL=${JJOB_START_HOME}/ocr/install/$1/
    export LD_LIBRARY_PATH=${OCR_INSTALL}/lib/
    cd $2
    TMPOUT=`mktemp --tmpdir=.`
    echo "Using '$TMPOUT' as an output file"
    GOLDEN=$3
    shift 3
    $@ > $TMPOUT
    RETURNCODE=$?
    if [ $RETURNCODE -eq 0 ]; then
        echo "Test ran successfully. Verifying output '${TMPOUT}' with '$GOLDEN'"
        diff -q ${TMPOUT} $GOLDEN
        if [ $? -eq 0 ]; then
            echo "Outputs match -- SUCCESS"
        else
            echo "Verification failed -- FAILURE"
            RETURNCODE=1
        fi
    else
        echo "Test failed -- FAILURE"
    fi
    rm ${TMPOUT}
    exit ${RETURNCODE}
fi
