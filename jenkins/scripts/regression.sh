#!/bin/bash

if [ $1 == "_params" ]; then
    if [ $2 == "output" ]; then
        echo "${JJOB_PRIVATE_HOME}/ocr/tests/tests-log/TESTS-TestSuites.xml"
        exit 0
    fi
else
    # ARGS: ARCH CFG_FILE DB_IMPL
    ARCH=$1
    export OCR_INSTALL=${JJOB_SHARED_HOME}/ocr/install/${ARCH}/
    export PATH=${OCR_INSTALL}/bin:$PATH
    export LD_LIBRARY_PATH=${OCR_INSTALL}/lib:${LD_LIBRARY_PATH}

    CFG_FILE=$2;
    export OCR_CONFIG=${OCR_INSTALL}/config/${CFG_FILE}
    echo "regression.sh: Setting OCR_CONFIG to ${OCR_CONFIG}"

    DB_IMPL=$3;

    cd ${JJOB_PRIVATE_HOME}/ocr/tests/

    ./ocrTests -unstablefile unstable.${ARCH}-${DB_IMPL}
    exit $?
fi
