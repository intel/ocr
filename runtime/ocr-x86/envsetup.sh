#!/bin/bash

#
# DO NOT EXECUTE THIS SCRIPT -- SOURCE IT INTO YOUR SHELL, E.G.:
#
#   [ibganev@bar2 ocr-x86]$ pwd
#   /home/ibganev/xstack/ocr/runtime/ocr-x86
#   [ibganev@bar2 ocr-x86]$
#   [ibganev@bar2 ocr-x86]$ source envsetup.sh
#   PROJECT_ROOT=/home/ibganev/xstack/ocr/runtime/ocr-x86
#   OCR_INSTALL=/home/ibganev/xstack/ocr/runtime/ocr-x86/ocr-install
#   LD_LIBRARY_PATH=/home/ibganev/xstack/ocr/runtime/ocr-x86/ocr-install/lib
#   [ibganev@bar2 ocr-x86]$
#

if [ "x${PROJECT_ROOT}" = "x" ]; then
    export PROJECT_ROOT=${PWD}
fi

echo "PROJECT_ROOT=${PROJECT_ROOT}"

export OCR_INSTALL=${PROJECT_ROOT}/ocr-install
echo "OCR_INSTALL=${OCR_INSTALL}"

export LD_LIBRARY_PATH=${OCR_INSTALL}/lib
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
