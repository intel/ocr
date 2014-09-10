#!/bin/bash

if [ $# -ne 4 ]
then
    echo "Usage: ${0} <tile width> <tile height> <string1 file> <string2 file>"
    exit 1
fi

# Determine the size of first file
SIZE1=`stat -c %s ${3}`
SIZE2=`stat -c %s ${4}`
echo $SIZE1 $SIZE2

# Concatenate both into a single file
cat ${3} ${4} > smithwaterman_output.bin
