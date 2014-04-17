#! /usr/bin/env python

import sys, csv, string

if len(sys.argv) != 4:
    print "Usage: slice_runtime_by_chunk_size.py <input csv file> \
    <output csv file> <chunk size to extract>"
    exit()

chunk_size = int(sys.argv[3])

inFile = open(sys.argv[1], "rb")  #Open the input file
inReader = csv.reader(inFile)  #Grind it into CSV format

inputRow = inReader.next() #Grab the first line in the file \
#Which we'll discard, since it's just the header.

outFile = open(sys.argv[2], "wb")
outWriter = csv.writer(outFile)

outWriter.writerow(["array_size", "num_cores", "run_time"])

for row in inReader:
    if int(row[1]) == chunk_size:
        elem = str(row[0])
        outWriter.writerow([elem, row[2], row[3]])

