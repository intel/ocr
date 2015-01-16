import sys
import re
import subprocess
import os
import time
import math
from operator import itemgetter

#Line split indices for each line from logs
START_TIME_INDEX = 9
END_TIME_INDEX = 11
FCT_NAME_INDEX = 7
WORKER_ID_INDEX = 2
PD_ID_INDEX = 1

#JS dependence hosted on xstack webserver
JAVASCRIPT_URL = "http://xstack.exascale-tech.com/public/vis/dist/vis.js"
#CSS dependence hosted on xstack webserver
CSS_URL = "http://xstack.exascale-tech.com/public/vis/dist/vis.css"
#For min value helper functions
HUGE_CONSTANT = sys.maxint
#Num. EDTs per html page to maintain fluidity
MAX_EDTS_PER_PAGE = 2000

#Naming constants for output files
STRIPPED_RECORDS = 'filtered.txt'
HTML_FILE_NAME = 'timeline'

#========= Write EDT EXECTUION events to html file ==========
def postProcessData(inString, outFile, offSet, lastLineFlag, counter, color):
    words = inString.split()
    workerID = words[WORKER_ID_INDEX][WORKER_ID_INDEX:]
    fctName = words[FCT_NAME_INDEX]
    startTime = (int(words[START_TIME_INDEX]) - offSet)/1000 #Ns to Ms
    endTime = (int(words[END_TIME_INDEX]) - offSet)/1000 #Ns to Ms
    whichNode = words[PD_ID_INDEX][1:]

    if lastLineFlag == True:
        outFile.write('\t\t{id: ' + str(counter) + ', group: ' + '\'' + str(workerID) + '\'' + ', content: \'' + str(fctName) + '\', start: new Date(' + str(startTime) + '), end: new Date(' +  str(endTime) + '), style: "background-color: ' + str(color) + ';"}\n')
        outFile.write('\t]);\n\n')
    else:
        outFile.write('\t\t{id: ' + str(counter) + ', group: ' + '\'' + str(workerID) + '\'' + ', content: \'' + str(fctName) + '\', start: new Date(' + str(startTime) + '), end: new Date(' +  str(endTime) + '), style: "background-color: ' + str(color) + ';"},\n')

#========== Strip Un-needed events from OCR debug log ========
def runShellStrip(dbgLog):
    os.system("egrep -w \'EDT\\(INFO\\)|EVT\\(INFO\\)\' " + str(dbgLog) + " | egrep -w \'FctName\' > " + STRIPPED_RECORDS)
    return 0

#========== Write Common open HTML tags to output file ========
def appendPreHTML(outFile, exeTime, numThreads, numEDTs, totalTime):
    outFile.write('<!DOCTYPE HTML>\n')
    outFile.write('<html>\n')
    outFile.write('<head>\n')
    outFile.write('<p>Total (CPU) execution time: ' + str(exeTime) + ' milliseconds</p>\n')
    outFile.write('<p>Total number of executing workers (threads): ' + str(numThreads) + '</p>\n')
    outFile.write('<p>Total number of executed tasks: ' + str(numEDTs) + '</p>\n')
    outFile.write('<BR><BR>\n')
    outFile.write('<p><b>&nbsp;&nbsp;Workers</b></p>\n')
    outFile.write('\t<style>\n')
    outFile.write('\t\tbody, html {\n')
    outFile.write('\t\t\tfont-family: arial, sans-serif;\n')
    outFile.write('\t\t\tfont-size: 11pt;\n')
    outFile.write('\t\t}\n\n')
    outFile.write('\t\t#visualization {\n')
    outFile.write('\t\t\tbox-sizing: border-box;\n')
    outFile.write('\t\t\twidth: 100%;\n')
    outFile.write('\t\t\theight: 300px;\n')
    outFile.write('\t\t}\n')
    outFile.write('\t</style>\n\n')
    outFile.write('\t<script src="' + JAVASCRIPT_URL + '"></script>\n')
    outFile.write('\t<link href="' + CSS_URL + '" rel="stylesheet" type="text/css" />\n')
    outFile.write('</head>\n')
    outFile.write('<body>\n')
    outFile.write('<div id="visualization"></div>\n')
    outFile.write('<script>\n')
    outFile.write('\tvar groups = new vis.DataSet([\n')


#========== Write HTML Data for worker (thread) groups ==========
def appendWorkerIdHTML(outFile, uniqueWorkers):
    uniqWrkrs = list(uniqueWorkers)
    for i in xrange(1, len(uniqWrkrs)+1):
        curID = uniqWrkrs[i-1]
        if i == len(uniqWrkrs):
            outFile.write('\t\t{id: ' + '\'' + str(curID) + '\'' + ', content: \'' + str(curID) + '\', value: ' + str(i) + '}\n')
            outFile.write('\t]);\n\n')
            break
        else:
            outFile.write('\t\t{id: ' +  '\'' + str(curID) + '\'' + ', content: \'' + str(curID) + '\', value: ' + str(i) + '},\n')

    outFile.write('\tvar items = new vis.DataSet([\n')


#========== Write Common closing HTML tags to output file ========
def appendPostHTML(outFile, flag, pageNum, start, end, timeOffset, numThreads):
    modStart = math.ceil((int(start) - timeOffset) / float(1000))
    modEnd = math.ceil((int(end) - timeOffset) / float(1000))

    outFile.write('\tvar container = document.getElementById(\'visualization\');\n')
    outFile.write('\tvar options = {\n')
    outFile.write('\t\tgroupOrder: function (a, b) {\n')
    outFile.write('\t\t\treturn a.value - b.value;\n')
    outFile.write('\t\t},\n')
    outFile.write('\t\tstack: false,\n')
    outFile.write('\t\tstart: new Date(' + str(modStart) + '),\n')
    outFile.write('\t\tmin: new Date(' + str(modStart) + '),\n')
    outFile.write('\t\tmax: new Date(' + str(modEnd) + '),\n')
    outFile.write('\t\tzoomMax: ' + str(modEnd) + ',\n')
    outFile.write('\t\teditable: true,\n')
    outFile.write('\t\tshowMajorLabels: false\n')
    outFile.write('\t};\n\n')
    outFile.write('\tvar timeline = new vis.Timeline(container);\n')
    outFile.write('\ttimeline.setOptions(options);\n')
    outFile.write('\ttimeline.setGroups(groups);\n')
    outFile.write('\ttimeline.setItems(items);\n')
    outFile.write('</script>\n')

    nextPage = pageNum+1
    prevPage = pageNum-1
    numPixelsNext = 184 + 42*(numThreads+3)
    numPixelsPrev = 184 + 42*(numThreads+4)
    if flag == "first":
    #Write 'next' button only
        outFile.write('<div>\n')
        outFile.write('\t<form action="./' + HTML_FILE_NAME + str(nextPage) + '.html" style="position: absolute; top: ' + str(numPixelsNext) + 'px;">\n')
        outFile.write('\t\t<input type="submit" value="Next">\n')
        outFile.write('</div>\n')
        outFile.write('</form>\n')

    if flag == "mid":
    #Write 'next' and 'previous' buttons
        outFile.write('<div>\n')
        outFile.write('\t<form action="./' + HTML_FILE_NAME + str(nextPage) + '.html" style="position: absolute; top: ' + str(numPixelsNext) + 'px;">\n')
        outFile.write('\t\t<input type="submit" value="Next">\n')
        outFile.write('</div>\n')
        outFile.write('</form>\n')


        outFile.write('<div>\n')
        outFile.write('\t<form action="./'+ HTML_FILE_NAME + str(prevPage) + '.html" style="position: absolute; top: ' + str(numPixelsPrev) + 'px;">\n')
        outFile.write('\t\t<input type="submit" value="Previous">\n')
        outFile.write('</div>\n')
        outFile.write('</form>\n')

    if flag == "last":
    #Write 'previous' button only
        outFile.write('<div>\n')
        outFile.write('\t<form action="./'+ HTML_FILE_NAME + str(prevPage) + '.html" style="position: absolute; top: ' + str(numPixelsPrev) + 'px;">\n')
        outFile.write('\t\t<input type="submit" value="Previous">\n')
        outFile.write('</div>\n')
        outFile.write('</form>\n')


    outFile.write('</body>\n')
    outFile.write('</html>\n')


#======== Compute and return total executin time ==========
def getExecutionTime(logFile):
    firstLine = logFile[0].split()
    lastLine = logFile[-1].split()
    offset = int(firstLine[START_TIME_INDEX])
    totalTime = (int(lastLine[END_TIME_INDEX]) - offset) / 1000 #Ns to Ms
    return totalTime

#======== Gathers unique EDT names to be mapped to color ======
def getUniqueFctNames(logFile):
    splitLog = []
    names = []
    for line in logFile:
        splitLog.append(line.split())

    for element in splitLog:
        names.append(element[FCT_NAME_INDEX])

    uniqNames = (set(names))
    return uniqNames


#======== Get the number of unique threads =========
def getUniqueWorkers(logFile):
    splitLog = []
    workers = []
    for line in logFile:
        splitLog.append(line.split())

    for element in splitLog:
        workers.append(element[WORKER_ID_INDEX][WORKER_ID_INDEX:])

    uniqWorkers = (set(workers))
    return uniqWorkers

#=========Account for (possible) out of order log writes by sorting=====
def sortAscendingStartTime(logFile):
    splitLog = []
    sortedList = []
    for line in logFile:
        splitLog.append(line.split())

    for element in splitLog:
        element[START_TIME_INDEX] = int(element[START_TIME_INDEX])

    tempSorted = sorted(splitLog, key=itemgetter(START_TIME_INDEX))

    for element in tempSorted:
        element[START_TIME_INDEX] = str(element[START_TIME_INDEX])

    for element in tempSorted:
        sortedList.append((' '.join(element))+'\n')

    return sortedList

#========== Sorts by worker to keep timeline format constant ===========
def sortByWorker(sortedLog, uniqWorkers):
    sortedWrkrs = []
    for worker in uniqWorkers:
        for line in sortedLog:
            lineSplit = line.split()
            if str(lineSplit[WORKER_ID_INDEX][WORKER_ID_INDEX:]) == str(worker):
                sortedWrkrs.append(line)

    return sortedWrkrs

#========== Subdivide data to accomodate multiple pages ==============
def getSub(log, flag, pageNum):

    if flag == "single":
        startIdx = 0
        endIdx = len(log)
    elif flag == "first":
        startIdx = 0
        endIdx = (pageNum*MAX_EDTS_PER_PAGE)
    elif flag == "mid":
        startIdx = ((pageNum-1)*MAX_EDTS_PER_PAGE)
        endIdx = ((pageNum)*MAX_EDTS_PER_PAGE)
    elif flag == "last":
        startIdx = ((pageNum-1)*MAX_EDTS_PER_PAGE)
        endIdx = len(log)

    sub = []
    for i in xrange(startIdx, endIdx):
        sub.append(log[i])

    return sub


#======== get color per Unique Function ========
def getColor(fctName, colorMap):
    for i in range(len(colorMap)):
        if colorMap[i][0] == fctName:
            return colorMap[i][1]
    print 'error, no color value.'
    sys.exit(0)


#======= key-val pair up: Functions to aesthetically pleasing colors =======
def assignColors(uniqFunctions):
    #TODO Expand this color list.
    cssColors = ['Blue', 'Red', 'DarkSeaGreen', 'Coral', 'Yellow', 'DarkOrange', 'DarkOrchid', 'HotPink', 'Gold', 'PaleGreen', 'Tan', 'Thistle', 'Tomato', 'Cadet Blue', 'Salmon', 'MediumTurquoise', 'DeepPink', 'Indigo', 'Khaki', 'LightBlue', 'Maroon', 'LimeGreen', 'BurlyWood', 'Brown', 'Crimson', 'LawnGreen', 'LightCyan', 'MediumOrchid', 'Linen', 'LightCoral', 'MediumSpringGreen', 'LightSteelBlue', 'Blue', 'Red', 'DarkSeaGreen', 'Coral', 'Yellow', 'DarkOrange', 'DarkOrchid', 'HotPink', 'Gold', 'PaleGreen', 'Tan', 'Thistle', 'Tomato', 'Cadet Blue', 'Salmon', 'MediumTurquoise', 'DeepPink', 'Indigo', 'Khaki', 'LightBlue', 'Maroon', 'LimeGreen', 'BurlyWood', 'Brown', 'Crimson', 'LawnGreen', 'LightCyan', 'MediumOrchid', 'Linen', 'LightCoral', 'MediumSpringGreen', 'LightSteelBlue']
    fctList = list(uniqFunctions)
    tempColors = []
    for i in range(len(fctList)):
        tempColors.append(cssColors[i])

    mappedColors = zip(fctList, tempColors)
    return mappedColors

#======= Getter for Minumum start time among subset of EDTs ========
def getMinStart(data):
    minStart = HUGE_CONSTANT
    idx = 0
    for i in range(len(data)):
        curStart = int(data[i][0].split()[9])
        if curStart < minStart:
            minStart = curStart
            idx = i
    return [minStart, idx]

#======= Getter for greatest start time among subset of EDTs =======
def getMaxEnd(data):
    maxEnd = 0
    idx = 0
    for i in range(len(data)):
        curEnd = int(data[i][0].split()[11])
        if curEnd > maxEnd:
            maxEnd = curEnd
            idx = i
    return [maxEnd, idx]

#======= Pre file-writing check to account for EDTs running beyond a single page =======
def preCheckEdtLength(toHtml, exeTime, numThreads, numEDTs, totalTime, uniqWorkers, timeOffset):

#Hideously Gross Hack.  Cleanup later if needed
#The below code essential slices the the latter part of an edt extending beyond its intial
#page, and appends the excess to the following page.

    for i in range(len(toHtml)):
        for j in range(len(toHtml[i])):
            if j == len(toHtml[i])-1 and i <= len(toHtml)-2:
                prevEnd = getMaxEnd(toHtml[i])
                curStart = getMinStart(toHtml[i+1])
                if int(prevEnd[0]) > int(curStart[0]):
                    prevEndTime = prevEnd[0]
                    nextStartTime = curStart[0]
                    prevEndIdx = prevEnd[1]
                    nextStartIdx = curStart[1]

                    prevStart = toHtml[i][prevEndIdx][0].split()[9]
                    prevEnd = toHtml[i][prevEndIdx][0].split()[11]
                    nextStart = toHtml[i+1][nextStartIdx][0].split()[9]
                    nextEnd = toHtml[i+1][nextStartIdx][0].split()[11]

                    newPrevLine = ''
                    newNextLine = ''
                    for sub in range(len(toHtml[i][prevEndIdx][0].split())):
                        if sub == 9:
                            newPrevLine +=  str(prevStart) + ' '
                            newNextLine += str(nextStart) + ' '
                        elif sub == 11:
                            newPrevLine +=  str(nextStart) + ' '
                            newNextLine += str(prevEnd) + ' '
                        else:
                            newPrevLine += str(toHtml[i][prevEndIdx][0].split()[sub]) + ' '
                            newNextLine += str(toHtml[i][prevEndIdx][0].split()[sub]) + ' '

                    splitColor = toHtml[i][prevEndIdx][3]
                    toHtml[i][prevEndIdx] = [newPrevLine, toHtml[i][prevEndIdx][1], prevEndIdx, splitColor, toHtml[i][prevEndIdx][4]]
                    toHtml[i+1].append([newNextLine, toHtml[i][prevEndIdx][1], len(toHtml[i+1]), splitColor, toHtml[i+1][nextStartIdx][4]])

    for i in range(len(toHtml)):
        pageName = HTML_FILE_NAME + str(i+1) + ".html"
        outFile = open(pageName, 'a')
        appendPreHTML(outFile, exeTime, numThreads, numEDTs, totalTime)
        appendWorkerIdHTML(outFile, uniqWorkers)

        for j in range(len(toHtml[i])):
            lastLineFlag = False
            if j == len(toHtml[i]) - 1:
                lastLineFlag = True
            postProcessData(toHtml[i][j][0], outFile, timeOffset, lastLineFlag, toHtml[i][j][2], toHtml[i][j][3])
        startWindow = getMinStart(toHtml[i])
        endWindow = getMaxEnd(toHtml[i])
        appendPostHTML(outFile, toHtml[i][j][4], i+1, startWindow[0], endWindow[0], timeOffset, numThreads)
        outFile.close()

#======== cleanup temporary files =========
def cleanup(dbgLog):
    #os.remove(dbgLog)
    os.remove(STRIPPED_RECORDS)


#=========MAIN==========

def main():

    if len(sys.argv) != 2:
        print "Error.....  Usage: python timeline.py <inputFile>"
        sys.exit(0)
    dbgLog = sys.argv[1]

    #Get rid of events we are not interested in seeing
    runShellStrip(dbgLog)
    #Begin processing log file
    with open (STRIPPED_RECORDS, 'r') as inFile:

        lines = inFile.readlines()
        if len(lines) == 0:
            print "Error: No EDT Names found in debug log.  Be sure proper flags are set and OCR has been rebuilt"
            sys.exit(0)

        sortedLines = sortAscendingStartTime(lines)

        #Grab various statistcs/Arrange Log for postprocess
        exeTime = getExecutionTime(sortedLines)
        uniqWorkers = getUniqueWorkers(sortedLines)
        uniqNames = getUniqueFctNames(sortedLines)
        mappedColors = assignColors(uniqNames)
        numThreads = len(uniqWorkers)
        sortedWrkrs = sortByWorker(sortedLines, uniqWorkers)
        numEDTs = len(sortedLines)

        numPages = numEDTs/MAX_EDTS_PER_PAGE
        if numPages == 0:
            numPages = 1

        firstLine = sortedLines[0].split()
        timeOffset = int(firstLine[START_TIME_INDEX])
        lastLine = sortedWrkrs[-1]
        totalTime = int(sortedLines[-1].split()[END_TIME_INDEX]) - timeOffset

        toHtml = []
        #Subdivide EDT records into multiple pages if need be
        for i in xrange(1, numPages+1):
            if numPages == 1:
                flag = "single"
            elif i == 1:
                flag = "first"
            elif i == numPages:
                flag = "last"
            else:
                flag = "mid"

            curPage = getSub(sortedLines, flag, i)
            localUniqWrkrs = getUniqueWorkers(curPage)
            localUniqNames = getUniqueFctNames(curPage)
            localSortedWrkrs = sortByWorker(curPage, localUniqWrkrs)
            localTimeSorted = sortAscendingStartTime(curPage)
            localTotalTime = int(localTimeSorted[-1].split()[END_TIME_INDEX]) - timeOffset
            localLastLine = localSortedWrkrs[-1]

            counter = 0

            localDataSet = []
            for line in localSortedWrkrs:
                lastLineFlag = False
                curFctName = line.split()[FCT_NAME_INDEX]
                curColor = getColor(curFctName, mappedColors)
                if line is localLastLine:
                    lastLineFlag = True
                localDataSet.append([line, lastLineFlag, counter, curColor, flag])
                counter += 1

            toHtml.append(localDataSet)

        #Check for overlap, write to file
        preCheckEdtLength(toHtml, exeTime, numThreads, numEDTs, totalTime, uniqWorkers, timeOffset)

    cleanup(dbgLog)
    inFile.close()
    sys.exit(0)

main();


