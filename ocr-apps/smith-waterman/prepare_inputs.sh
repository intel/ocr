#!/bin/bash

if [ $# -ne 4 ]
then
    echo "Usage: ${0} <string1 file> <string2 file> <tile width> <tile height>"
    exit 0
fi

# Determine the size of first file
SIZE1=`stat -c %s ${1}`
SIZE2=`stat -c %s ${2}`
echo $SIZE1 $SIZE2

# Concatenate both into a single file
cat ${1} ${2} > smithwaterman_output.bin
