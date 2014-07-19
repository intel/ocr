#!/bin/bash

mkdir -p ${OCR_BUILD_ROOT}/$1
cd ${OCR_BUILD_ROOT}/$1
make all install
RETURN_CODE=$?

if [ $RETURN_CODE -eq 0 ]; then
    echo "**** Build SUCCESS ****"
else
    echo "**** Build FAILURE ****"
fi

exit $RETURN_CODE
