#! /usr/bin/env python

import sys, csv, string

if len(sys.argv) != 4:
    print "Usage: slice_speedup_by_array_size.py <input csv file> \
    <output csv file> <array size to extract>"
    exit()

array_size = int(sys.argv[3])

inFile = open(sys.argv[1], "rb")  #Open the input file
inReader = csv.reader(inFile)  #Grind it into CSV format

inputRow = inReader.next() #Grab the first line in the file \
#Which we'll discard, since it's just the header.

outFile = open(sys.argv[2], "wb")
outWriter = csv.writer(outFile)

outWriter.writerow(["chunk_size", "num_cores", "speedup"])
sequential = 0 # define this variable

for row in inReader:
    if int(row[0]) == array_size:
        if int(row[2]) == 1:  #This is the single-core time for this
                              #size
                              sequential = float(row[3])
                              print sequential
        speedup = sequential/float(row[3])
        outWriter.writerow([row[1], row[2], speedup])

