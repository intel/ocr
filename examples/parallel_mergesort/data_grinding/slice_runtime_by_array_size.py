#! /usr/bin/env python

import sys, csv, string

if len(sys.argv) != 4:
    print "Usage: slice_runtime_by_array_size.py <input csv file> \
    <output csv file> <chunk size to extract>"
    exit()

array_size = int(sys.argv[3])

inFile = open(sys.argv[1], "rb")  #Open the input file
inReader = csv.reader(inFile)  #Grind it into CSV format

inputRow = inReader.next() #Grab the first line in the file \
#Which we'll discard, since it's just the header.

outFile = open(sys.argv[2], "wb")
outWriter = csv.writer(outFile)

outWriter.writerow(["chunk_size", "num_cores", "run_time"])

for row in inReader:
    if int(row[0]) == array_size:
        outWriter.writerow([row[1], row[2], row[3]])

