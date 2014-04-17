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

outWriter.writerow(["chunk_size", "num_cores", "speedup"])
sequential = {} # define this variable

for row in inReader:
    if int(row[1]) == chunk_size:
        if int(row[2]) == 1:  #This is the single-core time for this
                              #size, so record it.  Counts on the
                              #results being ordered so that we see
                              #each single-core time before any
                              #multi-core time for that chunk size
        
                              sequential[int(row[0])] = float(row[3])
                              print sequential
        speedup = sequential[int(row[0])]/float(row[3])
        outWriter.writerow([row[0], row[2], speedup])

