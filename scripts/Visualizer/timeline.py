import sys
import re
import subprocess
import os
from operator import itemgetter

LOG_FILE_PATH = './filtered.txt'
#TYPE_STRINGS = ('EVT(INFO)', 'EDT(INFO)')
HTML_FILE_NAME = 'timeline.html'

START_TIME_INDEX = 9
END_TIME_INDEX = 11
FCT_NAME_INDEX = 7
WORKER_ID_INDEX = 2
NUM_TICKS = 15

#========= Write EDT EXECTUION events to xml file ==========
def postProcessData(inString, outFile, offSet, lastLineFlag):
    words = inString.split()
    workerID = words[WORKER_ID_INDEX][WORKER_ID_INDEX:]
    fctName = words[FCT_NAME_INDEX]
    startTime = (int(words[START_TIME_INDEX]) - offSet)/1000 #Ns to Ms
    endTime = (int(words[END_TIME_INDEX]) - offSet)/1000 #Ns to Ms

    if lastLineFlag == True:
        outFile.write('\t\t[\'Worker: ' + str(workerID) + '\', \'' + str(fctName) + '\', new Date(0,0,0,0,0,0,' + str(startTime) + '), new Date(0,0,0,0,0,0,' + str(endTime) + ') ]]);\n')
    else:
        outFile.write('\t\t[\'Worker: ' + str(workerID) + '\', \'' + str(fctName) + '\', new Date(0,0,0,0,0,0,' + str(startTime) + '), new Date(0,0,0,0,0,0,' + str(endTime) + ') ],\n')

#========== Strip Un-needed events from OCR debug log ========
def runShellStrip():
    subprocess.call(['./stripExcessEvents.sh'])
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
def appendPostHTML(outFile, zoom):

    outFile.write('\tdashboard.bind(control, chart)\n')
    outFile.write('\tdashboard.draw(data)\n')
    outFile.write('}\n')
    outFile.write('</script>\n')
    outFile.write('</head>\n')
    outFile.write('<body style="font-family: Arial;border: 0 none;">\n')
    outFile.write('<div id="dashboard" style="width: 1800px; overflow:scroll;"></div>\n')
    outFile.write('<div id="chart" style="width:1800px; height: ' + str(zoom) + 'px;"></div>\n')
    outFile.write('<div id="control" style="width: 1670px; height: 200px; left: 138px"></div>\n')
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

#======== cleanup temporary files =========
def cleanup():
    os.remove('log.txt')
    os.remove('filtered.txt')
    return 0
#=========MAIN==========

def main():

    #Get rid of events we are not interested in seeing
    runShellStrip()

    outFile = open(HTML_FILE_NAME, 'a')

    #Begin processing log file
    with open (LOG_FILE_PATH, 'r') as inFile:

        lines = inFile.readlines()
        sortedLines = sortAscendingStartTime(lines)

        #Grab various statistcs
        exeTime = getExecutionTime(sortedLines)
        uniqWorkers = getUniqueWorkers(sortedLines)
        numThreads = len(uniqWorkers)
        sortedWrkrs = sortByWorker(sortedLines, uniqWorkers)
        numEDTs = len(sortedLines)

        #Used to set zoom bar position
        zoom = 75 + (numThreads*40)

        if zoom > 715:
            #715 is height of 16 rows.  Past 16 rows the overflow scroll will take over.
            #increase this value if you want more rows to be displayed in single frame
            zoom = 715

        #Arrange data
        firstLine = sortedLines[0].split()
        timeOffset = int(firstLine[START_TIME_INDEX])
        lastLine = sortedLines[-1]
        totalTime = int(lastLine.split()[END_TIME_INDEX]) - timeOffset

        appendPreHTML(outFile, exeTime, numThreads, numEDTs, totalTime, zoom)

        #Format Data to be written as HTML
        firstLine = sortedLines[0].split()
        timeOffset = int(firstLine[START_TIME_INDEX])
        lastLine = sortedWrkrs[-1]
        totalTime = int(lastLine.split()[END_TIME_INDEX]) - timeOffset

        for line in sortedWrkrs:
            lastLineFlag = False
            if line is lastLine:
                lastLineFlag = True
            postProcessData(line, outFile, timeOffset, lastLineFlag)

    appendPostHTML(outFile, zoom)
    cleanup()
    sys.exit(0)

main();

