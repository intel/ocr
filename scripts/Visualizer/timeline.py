import sys
import re
import subprocess
import os
from operator import itemgetter

START_TIME_INDEX = 9
END_TIME_INDEX = 11
FCT_NAME_INDEX = 7
WORKER_ID_INDEX = 2
NUM_TICKS = 15
PD_ID_INDEX = 1

MAX_EDTS_PER_PAGE = 1000

#========= Write EDT EXECTUION events to html file ==========
def postProcessData(inString, outFile, offSet, lastLineFlag):
    words = inString.split()
    workerID = words[WORKER_ID_INDEX][WORKER_ID_INDEX:]
    fctName = words[FCT_NAME_INDEX]
    startTime = (int(words[START_TIME_INDEX]) - offSet)/1000 #Ns to Ms
    endTime = (int(words[END_TIME_INDEX]) - offSet)/1000 #Ns to Ms
    whichNode = words[PD_ID_INDEX][1:]

    if lastLineFlag == True:
        outFile.write('\t\t[\'Worker: ' + str(workerID) + '\', \'' + str(fctName) + ' on ' + str(whichNode) + '\', new Date(0,0,0,0,0,0,' + str(startTime) + '), new Date(0,0,0,0,0,0,' + str(endTime) + ') ]]);\n')
    else:
        outFile.write('\t\t[\'Worker: ' + str(workerID) + '\', \'' + str(fctName) + ' on ' + str(whichNode) + '\', new Date(0,0,0,0,0,0,' + str(startTime) + '), new Date(0,0,0,0,0,0,' + str(endTime) + ') ],\n')

#========== Strip Un-needed events from OCR debug log ========
def runShellStrip(dbgLog):
    os.system("egrep -w \'EDT\\(INFO\\)|EVT\\(INFO\\)\' " + str(dbgLog) + " | egrep -w \'FctName\' > filtered.txt")
    return 0

#========== Handles variable execution times and writes time periods ========
def writeTimeTicks(outFile, totalTime):
    timeIncr = totalTime/NUM_TICKS
    curTime = timeIncr
    outFile.write('\t\t\t\t\t\tticks: [\n')
    for i in xrange(0, (NUM_TICKS - 1)):
        outFile.write('\t\t\t\t\t\tnew Date(0,0,0,0,0,0,' + str(curTime) + '),\n')
        curTime = curTime + timeIncr
    outFile.write('\t\t\t\t\t\tnew Date(0,0,0,0,0,0,' + str(curTime) + ') \n')
    outFile.write('\t\t\t\t\t\t]')


#========== Write Common open HTML tags to output file ========
def appendPreHTML(outFile, exeTime, numThreads, numEDTs, totalTime, zoom):
    outFile.write('<html>\n')
    outFile.write('<head>\n')
    outFile.write('<p>Total (CPU) execution time: ' + str(exeTime) + ' milliseconds</p>\n')
    outFile.write('<p>Total number of executing workers (threads): ' + str(numThreads) + '</p>\n')
    outFile.write('<p>Total number of executed tasks: ' + str(numEDTs) + '</p>\n')
    outFile.write('<p><b>NOTE: </b>The recorded times do not accurately reflect execution time.  This visualization relies on OCR logging, which requires a large amount of I/O operations during the application\'s runtime.</p>\n')
    outFile.write('<script type="text/javascript" src="https://www.google.com/jsapi"></script>\n')
    outFile.write('<script type="text/javascript">\n')
    outFile.write('\tgoogle.load(\'visualization\', \'1\', {packages: [\'controls\']});\n')
    outFile.write('\tgoogle.setOnLoadCallback(drawVisualization);\n')
    outFile.write('\tfunction drawVisualization() {\n')
    outFile.write('\t\tvar dashboard = new google.visualization.Dashboard(\n')
    outFile.write('\t\tdocument.getElementById(\'dashboard\'));\n')
    outFile.write('\t\tvar control = new google.visualization.ControlWrapper({\n')
    outFile.write('\t\t\tcontrolType: \'ChartRangeFilter\',\n')
    outFile.write('\t\t\tcontainerId: \'control\',\n')
    outFile.write('\t\t\toptions: {\n')
    outFile.write('\t\t\t\tfilterColumnIndex: 3,\n')
    outFile.write('\t\t\t\tui: {\n')
    outFile.write('\t\t\t\tchartType: \'LineChart\',\n')
    outFile.write('\t\t\t\tchartOptions: {\n')
    outFile.write('\t\t\t\t\theight: 50, \n')
    outFile.write('\t\t\t\t\tchartArea: {\n')
    outFile.write('\t\t\t\t\t\twidth: \'100%\',\n')
    outFile.write('\t\t\t\t\t},\n')
    outFile.write('\t\t\t\t\thAxis: {\n')
    outFile.write('\t\t\t\t\t\tbaselineColor: \'none\',\n')

    #Ready for deployment but not yet needed.
    #writeTimeTicks(outFile, totalTime)

    outFile.write('\t\t\t\t\t}\n')
    outFile.write('\t\t\t\t},\n')
    outFile.write('\t\t\t\tchartView: {\n')
    outFile.write('\t\t\t\t\t\'columns\': [2,3]\n')
    outFile.write('\t\t\t\t}\n')
    outFile.write('\t\t\t}\n')
    outFile.write('\t\t},\n')
    outFile.write('\t});\n')
    outFile.write('\tvar chart = new google.visualization.ChartWrapper({\n')
    outFile.write('\t\t\'chartType\': \'Timeline\',\n')
    outFile.write('\t\t\t\'containerId\': \'chart\',\n')
    outFile.write('\t\t\t\'options\': {\n')
    outFile.write('\t\t\t\t\'width\': 1800,\n')
    outFile.write('\t\t\t\t\'height\': ' + str(zoom) +',\n')
    outFile.write('\t\t\t\t\'chartArea\': {\n')
    outFile.write('\t\t\t\t\twidth: \'100%\',\n')
    outFile.write('\t\t\t\t\theight: \'80%\'\n')
    outFile.write('\t\t\t\t},\n')
    outFile.write('\t\t\t\t\'backgroundColor\': \'#ffd\'\n')
    outFile.write('\t\t\t},\n')
    outFile.write('\t\t\t\'view\': {\n')
    outFile.write('\t\t\t\'columns\': [0, 1, 2, 3]\n')
    outFile.write('\t\t}\n')
    outFile.write('\t});\n')
    outFile.write('\tvar data = new google.visualization.DataTable();\n')

    outFile.write('\tdata.addColumn({ type: \'string\', id: \'Worker\' });\n')
    outFile.write('\tdata.addColumn({ type: \'string\', id: \'Fctn. Name\' });\n')
    outFile.write('\tdata.addColumn({ type: \'date\', id: \'Start\' });\n')
    outFile.write('\tdata.addColumn({ type: \'date\', id: \'End\' });\n')
    outFile.write('\tdata.addRows([\n')\

#========== Write Common closing HTML tags to output file ========
def appendPostHTML(outFile, zoom, flag, pageNum):

    outFile.write('\tdashboard.bind(control, chart)\n')
    outFile.write('\tdashboard.draw(data)\n')
    outFile.write('}\n')
    outFile.write('</script>\n')
    outFile.write('</head>\n')
    outFile.write('<body style="font-family: Arial;border: 0 none;">\n')
    outFile.write('<div id="dashboard" style="width: 1800px; overflow:scroll;"></div>\n')
    outFile.write('<div id="chart" style="width:1800px; height: ' + str(zoom) + 'px;"></div>\n')
    outFile.write('<div id="control" style="width: 1670px; height: 200px; left: 138px"></div>\n')

    nextPage = pageNum+1
    prevPage = pageNum-1

    if flag == "first":
    #Write 'next' button only
        outFile.write('<form action="./timeline' + str(nextPage) + '.html">\n')
        outFile.write('\t<input type="submit" value="Next">\n')
        outFile.write('</form>\n')

    if flag == "mid":
    #Write 'next' and 'previous' buttons
        outFile.write('<form action="./timeline' + str(prevPage) + '.html">\n')
        outFile.write('\t<input type="submit" value="Previous">\n')
        outFile.write('</form>\n')

        outFile.write('<form action="./timeline' + str(nextPage) + '.html">\n')
        outFile.write('\t<input type="submit" value="Next">\n')
        outFile.write('</form>\n')

    if flag == "last":
    #Write 'previous' button only
        outFile.write('<form action="./timeline' + str(prevPage) + '.html">\n')
        outFile.write('\t<input type="submit" value="Previous">\n')
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

#========= Sets zoom bar height to snap to timeline ==========
def setZoom(numThreads):
    zoom = 75 + (numThreads*40)
    if zoom > 715:
        #715 in the number of vertical pixels (from top of page to top of zoom bar)
        #at 16 rows. Inrease this value to show more records.
        zoom = 715
    return zoom

#========= Get a partition of records of size MAX_EDTS_PER_PAGE =========
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

#======== cleanup temporary files =========
def cleanup(dbgLog):
    #os.remove(dbgLog)
    os.remove('filtered.txt')
    return 0
#=========MAIN==========

def main():

    if len(sys.argv) != 2:
        print "Error.....  Usage: python timeline.py <inputFile>"
        sys.exit(0)
    dbgLog = sys.argv[1]

    #Get rid of events we are not interested in seeing
    runShellStrip(dbgLog)

    #Begin processing log file
    with open ('filtered.txt', 'r') as inFile:

        lines = inFile.readlines()
        sortedLines = sortAscendingStartTime(lines)

        #Grab various statistcs
        exeTime = getExecutionTime(sortedLines)
        uniqWorkers = getUniqueWorkers(sortedLines)
        numThreads = len(uniqWorkers)
        sortedWrkrs = sortByWorker(sortedLines, uniqWorkers)
        numEDTs = len(sortedLines)
        numPages = numEDTs/MAX_EDTS_PER_PAGE
        if numPages == 0:
            numPages = 1

        firstLine = sortedLines[0].split()
        timeOffset = int(firstLine[START_TIME_INDEX])
        lastLine = sortedLines[-1]
        totalTime = int(lastLine.split()[END_TIME_INDEX]) - timeOffset

        #Used to set zoom bar position
        zoom = setZoom(numThreads)

        #Format Data to be written as HTML
        firstLine = sortedLines[0].split()
        timeOffset = int(firstLine[START_TIME_INDEX])
        lastLine = sortedWrkrs[-1]
        totalTime = int(lastLine.split()[END_TIME_INDEX]) - timeOffset

        for i in xrange (1, numPages+1):
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
            localSortedWrkrs = sortByWorker(curPage, localUniqWrkrs)
            localLastLine = localSortedWrkrs[-1]

            pageName = "timeline" + str(i) + ".html"
            outFile = open(pageName, 'a')
            appendPreHTML(outFile, exeTime, numThreads,  numEDTs, totalTime, zoom)

            for line in localSortedWrkrs:
                lastLineFlag = False
                if line is localLastLine:
                    lastLineFlag = True
                postProcessData(line, outFile, timeOffset, lastLineFlag)
            appendPostHTML(outFile, zoom, flag, i)
            outFile.close()

    cleanup(dbgLog)
    sys.exit(0)

main();

