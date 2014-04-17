#! /usr/bin/env python

import sys, csv, string

if len(sys.argv) != 3:
    print "Usage: parse_pthread_speedup.py <input csv file> \
    <output csv file>"
    exit()

inFile = open(sys.argv[1], "rb")  #Open the input file
inReader = csv.reader(inFile)  #Grind it into CSV format

inputRow = inReader.next() #Grab the first line in the file \
#Which we'll discard, since it's just the header.

outFile = open(sys.argv[2], "wb")
outWriter = csv.writer(outFile)

outWriter.writerow(["array_size", "num_cores", "speedup"])
sequential = 0 # define this variable

for row in inReader:
    
    if int(row[1]) == 1:  #This is the single-core time for this
        #size
        sequential = float(row[2])
        print sequential
    speedup = sequential/float(row[2])
    outWriter.writerow([row[0], row[1], speedup])

