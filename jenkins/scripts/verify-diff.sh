#!/bin/bash

# Verify if two files are the same
COMPLETE_OK=0
EXIT_CODE=0
TMP_OUTPUT=""
WARN_ERROR=0
PASSED_COUNT=1

function err_handler () {
    echo "Abnormal script termination"
    exit -1
}

# Make sure that this entire script executes fine
trap err_handler ERR

cd ${WORKLOAD_EXEC}

diff -b $1 $2
RESULT=$?
if [ $RESULT -eq "0" ]; then
    echo "Output verified"
    exit 0
fi
echo "Output incorrect"
exit 1

