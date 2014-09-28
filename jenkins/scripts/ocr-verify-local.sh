#!/bin/bash

# Checks for WARN, ASSERT and FAILURE and PASSED
# For this to pass, it must have no ASSERT or FAILURE messages and have a certain number of PASSED
# This script will optionally fail on WARN

COMPLETE_OK=0
EXIT_CODE=0
TMP_OUTPUT=""
WARN_ERROR=0
PASSED_COUNT=1

while getopts "wc:" opt ; do
    case $opt in
        w)
            WARN_ERROR=1
            ;;
        c)
            PASSED_COUNT=$OPTARG
            ;;
        \?)
            ;;
    esac
done

function err_handler () {
    echo "Abnormal script termination"
    exit -1
}

# Make sure that this entire script executes fine
trap err_handler ERR

cd ${WORKLOAD_EXEC}

TMP_OUTPUT=runlog
COUNT=$(grep -c "(WARN)" ${TMP_OUTPUT}) || $(echo "")
if [ $COUNT -ne 0 ]; then
    if [ "$WARN_ERROR" -eq "1" ]; then
        echo "Found ${COUNT} instance(s) of WARN... this is an error!"
        exit 1
    else
        echo "Found ${COUNT} instance(s) of WARN... ignoring."
    fi
fi

COUNT=$(grep -c "ASSERT" ${TMP_OUTPUT}) || $(echo "")
if [ $COUNT -ne 0 ]; then
    echo "Found ${COUNT} instance(s) of ASSERT... this is an error!"
    exit 1
fi

COUNT=$(grep -c "FAILURE" ${TMP_OUTPUT}) || $(echo "")
if [ $COUNT -ne 0 ]; then
    echo "Found ${COUNT} instance(s) of FAILURE... this is an error!"
    exit 1
fi

COUNT=$(grep -c "PASSED" ${TMP_OUTPUT}) || $(echo "")
if [ $COUNT -ne $PASSED_COUNT ]; then
    echo "Found ${COUNT} PASSED but was expecting ${PASSED_COUNT}... this is an error!"
    exit 1
fi
echo "Output OK"
