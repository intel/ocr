#! /usr/bin/python

# Script to parse a directory of output files from Vtune into a single data
# file that can be graphed easily.

import glob, sys, os, csv, numpy

if len(sys.argv) != 3:
    print "Usage: parse_vtune_sweep.py <directory of vtune results> <output file>"
    exit()

resultsFiles = os.path.join(sys.argv[1], "*.csv")
#print resultsFiles


# Make two passes over the files.  The first constructs the list of functions
# that vtune found as hotspots in each of the files, as well as lists of the
# array size, chunk size, and number of cores used in the test for each file
# The second pass goes in and fills in the times that Vtune found for each
# function

# Pass #1:
arraySizeList = []
chunkSizeList = []
numProcessorsList = []
functionsList = []

for fileName in glob.iglob(resultsFiles):
#    print fileName
    fileNameParts = fileName.split("_")  # break the name into parts so we can
    #extract the array size, chunk size, and number of processors
    
#    print fileNameParts
    fileNameLength = len(fileNameParts)
    arraySize = fileNameParts[fileNameLength - 3]  # count from the back in case
    # the directory name contains hyphens

    chunkSize = fileNameParts[fileNameLength - 2]

    numProcessors = fileNameParts[fileNameLength -1]

    numProcessors = numProcessors.split(".")
    numProcessors = numProcessors[0] # Trim off the .csv at the end of the
    #filename

#   print arraySize
#   print chunkSize
#   print numProcessors
    
    arraySizeList.append(arraySize)
    chunkSizeList.append(chunkSize)
    numProcessorsList.append(numProcessors)
    # ok, now parse the file to extract the functions and time in function
    
    csvFile = open(fileName, 'rb') # open the file
    reader = csv.reader(csvFile) # parse the CSV
    reader.next()  # Skip the header row
    for row in reader:  # Iterate through the hotspot functions
        if functionsList.count(row[0]) == 0 and row[2] > 0:
            # The function isn't one we've seen before and it has a non-zero
            # run time, so add it to the list of functions we care about
            functionsList.append(row[0])
        
# Construct the array of run times, filling in with zeroes
valueArray = [[0 for x in range(0, len(functionsList))] for x in \
              range(0, len(arraySizeList))]

# Now, make another pass over the files, extracting the per-function run times
fileIndex = 0

for fileName in glob.iglob(resultsFiles):
 #   print fileName
    csvFile = open(fileName, 'rb') # open the file
    reader = csv.reader(csvFile) # parse the CSV
    reader.next()  # Skip the header row
    for row in reader:  # Iterate through the hotspot functions
        if row[2] > 0: # This function had non-zero time in this test
            functionIndex = functionsList.index(row[0])
    #        print functionIndex
    #       print fileIndex
            valueArray[fileIndex][functionIndex] = row[2]
    fileIndex = fileIndex + 1
        
#print valueArray

#print arraySizeList
#print chunkSizeList
#print numProcessorsList
#print functionsList

#The base valueArray will have way too many functions in it, so trim
#to the top 10 by average percentage of run time.
#print valueArray

totalTime = []
# compute the total time spent in each test
for valueRow in valueArray:
    sum = 0
    for value in valueRow:
        sum = sum + float(value)
    totalTime.append(sum)
#print "fish"
#print totalTime

#Scale the time spent in each function to its fraction of that tests'
#execution time

for testNum in range(0, len(valueArray)):
    for funcNum in range(0, len(valueArray[testNum])):
        valueArray[testNum][funcNum] = float(valueArray[testNum][funcNum]) \
        / totalTime[testNum]

fracSum = []
# Now, sum the execution time fractions of each function
for funcNum in range(0, len(functionsList)):
    funcSum = 0
    for testNum in range(0, len(valueArray)):
        funcSum = funcSum + valueArray[testNum][funcNum]
    fracSum.append(funcSum)

#print fracSum

# Compute the rank of the functions by sum of fractions
fracSumArray = numpy.array(fracSum)
temp = fracSumArray.argsort()
ranks = temp.argsort()

#print ranks


# Ok, we've parsed the data.  Now output it in an easy-to-graph format
outFileName = sys.argv[2]
outFile = open(outFileName, 'wb')
writer = csv.writer(outFile)

outRow = ['Array.Size', 'Chunk.Size', 'Num.Processors', 'Function', 'Run.Time']
writer.writerow(outRow)

for test in range(0, len(arraySizeList)):
    for function in range(0, len(functionsList)):
        if(ranks[function] > len(ranks) - 10):
            #this function is in the top 10 
            outRow = [arraySizeList[test], chunkSizeList[test], numProcessorsList[test], functionsList[function], valueArray[test][function]]
            writer.writerow(outRow)

print "done"
