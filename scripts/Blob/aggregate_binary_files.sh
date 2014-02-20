#!/bin/bash

if [ $# -ne 4 ]
then
echo "Usage: ${0} <CE struct file> <XE struct file> <Existing bin file> <Output bin file>" 
else

LEAF_CE_STRUCT=4096
XE_STRUCT=4096 

# This file aggregates multiple binary files resulting in the following layout:
# 1. Total size of "struct binaries" in ascii
# 2. CE Struct binary padded to 4K
# 3. XE Struct binary padded to 4K
# 4. Rest of original binary blob

# Total size in ascii goes first: 8 characters
let size=${LEAF_CE_STRUCT}+${XE_STRUCT}
size=`printf "%08d" $size`
echo -n $size > ${4}

# Copy the CE struct file
cat $1 >> $4

# Zero pad it
size=`stat -c%s $1`
let size=${LEAF_CE_STRUCT}-$size
dd if=/dev/zero ibs=1 count=$size status=none >> $4

# Do the same with XE struct file
cat $2 >> $4
size=`stat -c%s $1`
let size=${LEAF_CE_STRUCT}-$size
dd if=/dev/zero ibs=1 count=$size status=none >> $4

fi
