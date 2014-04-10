#!/usr/bin/python

# Modified in 2014 by Romain Cledat (now at Intel). The original
# license (BSD) is below. This file is also subject to the license
# aggrement located in the file LICENSE and cannot be distributed
# without it. This notice cannot be removed or modified

# Copyright (c) 2011, Romain Cledat
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Georgia Institute of Technology nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL ROMAIN CLEDAT BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import sys, getopt, re, math, copy, os, fnmatch
from operator import itemgetter

### Globals
funcNames = []
topLevelsPerThread = []
totalDict = {}
currentThread = -1

def getTopLevelFunctions(id, name = None):
    global topLevelsPerThread
    global currentThread
    assert(currentThread == -1 or (currentThread >= 0 and currentThread < len(topLevelsPerThread)))
    if currentThread == -1:
        curDict = totalDict
    else:
        curDict = topLevelsPerThread[currentThread]
    if id in curDict:
        assert(name is None or curDict[id].name == name)
        return curDict[id]
    else:
        curDict[id] = FuncEntry(id, name)
        return curDict[id]

def flattenList(iterable):
    it = iter(iterable)
    for e in it:
        if isinstance(e, (list, tuple)):
            for f in flattenList(e):
                yield f
        else:
            yield e

class ChildEntry:
    "Represents a child of a top-level function being tracked by the profiler"
    def __init__(self, tlParent, tlChild, count, totalTime, totalTimeSq, totalTimeInChildren, totalTimeInChildrenSq):
        """Creating an entry will make the child (tlChild) aware of the parent"""
        self.parent = tlParent
        self.tlEntry = tlChild

        tlChild.addParent(tlParent)

        # Statistics on this entry
        self.count = count
        self.totalTime = totalTime
        self.totalTimeSq = totalTimeSq
        self.totalTimeInChildren = totalTimeInChildren
        self.totalTimeInChildrenSq = totalTimeInChildrenSq

        # Calculated statistics
        self.totalTimeSelf = 0.0
        self.avg = 0.0
        self.stdDev = 0.0

    def __getitem__(self, key):
        return getattr(self, key)

    def calculateStatistics(self):
        """Calculates useful statistics"""

        self.totalTimeSelf = self.totalTime - self.totalTimeInChildren
        self.avg = self.totalTime/self.count
        try:
            self.stdDev = math.sqrt(self.totalTimeSq/self.count - self.avg*self.avg)
        except ValueError:
            self.stdDev = 0.0 # Small value


    def merge(self, other):
        assert(self.parent == other.parent and self.tlEntry == other.tlEntry)
        self.count += other.count
        self.totalTime += other.totalTime
        self.totalTimeSq += other.totalTimeSq
        self.totalTimeInChildren += other.totalTimeInChildren
        self.totalTimeInChildrenSq += other.totalTimeInChildrenSq


class FuncEntry:
    "Represents a top-level function that is being tracked by the profiler"

    def __init__(self, id, name, *other):
        self.id = id
        self.name = name
        self.parentEntries = { } # Will contain parentId : FuncEntry where FuncEntry is the entry for this function in the parent parentId
        self.childrenEntries = { }

        # Statistics on this entry
        if len(other) == 3:
            self.count = other[0]
            self.totalTime = other[1]
            self.totalTimeSq = other[2]
            self.isInit = True
        elif len(other) == 0:
            self.count = 0
            self.totalTime = 0
            self.totalTimeSq = 0
            self.isInit = False
        else:
            assert(False)

        # Calculated statistics
        self.totalTimeSelf = 0.0
        self.totalTimeInChildren = 0.0
        self.avg = 0.0
        self.stdDev = 0.0
        self.rank = 0

    def isInitialized(self):
        return self.isInit

    def set(self, count, totalTime, totalTimeSq):
        assert(not self.isInit)
        self.count = count
        self.totalTime = totalTime
        self.totalTimeSq = totalTimeSq
        self.isInit = True

    def __getitem__(self, key):
        return getattr(self, key)

    def __repr__(self):
        return "%s [%d]" % (self.name, self.rank)

    def __cmp__(self, other):
        if self.id < other.id:
            return -1
        elif self.id == other.id:
            return 0
        return 1

    def addParent(self, tlParent):
        """Adds a parent."""
        if tlParent.id in self.parentEntries:
            assert(self.parentEntries[tlParent.id] is tlParent)
        else:
            self.parentEntries[tlParent.id] = tlParent

    def addChild(self, childEntry):
        """Adds a child."""
        if childEntry.tlEntry.id in self.childrenEntries:
            assert(self.childrenEntries[childEntry.tlEntry.id] is childEntry)
        else:
            self.childrenEntries[childEntry.tlEntry.id] = childEntry

    def calculateStatistics(self):
        """Calculates useful statistics (like time spent in children etc...)"""
        assert(self.isInit)
        for k,child in self.childrenEntries.iteritems():
            self.totalTimeInChildren += child.totalTime
            child.calculateStatistics()

        self.totalTimeSelf = self.totalTime - self.totalTimeInChildren
        self.avg = self.totalTime/self.count
        try:
            self.stdDev = math.sqrt(self.totalTimeSq/self.count - self.avg*self.avg)
        except ValueError:
            self.stdDev = 0.0

    def merge(self, otherEntry):
        """Merges information about the otherEntry with this one, summing
        all statistics in particular"""

        assert(self.id == otherEntry.id and self.name == otherEntry.name)

        self.count += otherEntry.count
        self.totalTime += otherEntry.totalTime
        self.totalTimeSq += otherEntry.totalTimeSq

        for k,child in otherEntry.childrenEntries.iteritems():
            if k in self.childrenEntries:
                self.childrenEntries[k].merge(child)
            else:
                # Create a childEntry which will take care of linking
                # everything correctly
                self.childrenEntries[k] = ChildEntry(self,
                    getTopLevelFunctions(k, child.tlEntry.name), child.count, child.totalTime,
                    child.totalTimeSq, child.totalTimeInChildren,
                    child.totalTimeInChildrenSq)

### Main Program ###

class Usage:
    def __init__(self, msg):
        self.msg = msg


def main(argv=None):
    global lastId
    global funcNamesToId
    global currentThread
    global topLevelsPerThread
    global totalDict
    if argv is None:
        argv = sys.argv

    lineEntry_re = re.compile(r"ENTRY ([0-9]+):([0-9]+) = count\(([0-9]+)\), sum\(([0-9.]+)\), sumSq\(([0-9.]+)\)(?:(?:$)|(?:, sumChild\(([0-9.]+)\), sumSqChild\(([0-9.]+)\)$))")
    lineDef_re = re.compile(r"DEF ([A-Za-z_0-9]+) ([0-9]+)")
    baseFile="profiler_"
    threads=range(0,4)

    try:
        try:
            opts, args = getopt.getopt(argv[1:], "hb:t:", ["help", "base-file=", "thread="])
        except getopt.error, err:
            raise Usage(err)
        for o, a in opts:
            if o in ("-h", "--help"):
                raise Usage(\
"""
    -h,--help: Prints this message
    -b,--base-file: Specifies the base name of the profile files (defaults to profiler_)
    -t,--thread: Specifies the threads to use (format is start:end or comma separated list of threads or '*' (defaults to 0:4)
""")
            elif o in ("-b", "--base-file"):
                baseFile = a
            elif o in ("-t", "--thread"):
                if a == '*':
                    threads=None
                elif ':' in a:
                    start,end = re.split(":", a)
                    threads = range(int(start), int(end))
                else:
                    threads = re.split(",", a)
            else:
                raise Usage("Unhandled option")
        if args is not None and len(args) > 0:
            raise Usage("Extraneous arguments")
    except Usage, msg:
        print >>sys.stderr, msg.msg
        print >>sys.stderr, "For help use -h"
        return 2

    if threads is None:
        allNames = []
        # We are going to look for all the files starting with baseFile
        for file in os.listdir('.'):
            if fnmatch.fnmatch(file, '%s*' % (baseFile)):
                allNames.append(file)
        threads = range(0, len(allNames))
    else:
        allNames = ["%s%d" % (baseFile, i) for i in threads]

    t = 0
    for fileName in allNames:
        if t >= len(topLevelsPerThread):
            topLevelsPerThread.extend((t - len(topLevelsPerThread) + 1) * [{}])
        currentThread = t
        currentStack = []
        currentRealStack = []    # The "real" stack is the stack with all the functions
                                # including the ones which are "paused" and therefore
                                # do not have their children counted

        print "Processing file %s..." % (fileName)

        fileHandle = open(fileName, "r")

        for line in fileHandle:
            matchOb = lineDef_re.match(line)
            if matchOb is not None:
                funcName, funcId_s = matchOb.groups()
                funcId = int(funcId_s)
                if len(funcNames) <= funcId:
                    funcNames.extend(['']*(funcId-len(funcNames)+1))
                funcNames[funcId] = funcName
                continue
            # Here we should always have a match
            parentId_s, childId_s, count_s, sum_s, sumSq_s, sumChild_s, sumChildSq_s = lineEntry_re.match(line).groups()
            parentId = int(parentId_s)
            childId = int(childId_s)
            count = int(count_s)
            sum = float(sum_s)
            sumSq = float(sumSq_s)

            pFuncEntry = getTopLevelFunctions(parentId, funcNames[parentId]) # Makes sure that we know about this function
            if parentId == childId:
                # Self-node, we create a FuncEntry
                assert(sumChild_s is None and sumChildSq_s is None)
                pFuncEntry.set(count, sum, sumSq)
            else:
                # Here we have a childEntry
                assert(sumChild_s is not None and sumChildSq_s is not None)
                cFuncEntry = getTopLevelFunctions(childId, funcNames[childId])
                pFuncEntry.addChild(ChildEntry(pFuncEntry, cFuncEntry, count, sum, sumSq, float(sumChild_s), float(sumChildSq_s)))
        fileHandle.close()
        t = t + 1
    print "Done processing all files, creating global time structure..."
    currentThread = -1 # Signal that now we are working on the totalDict

    # We will now "merge" all the different times into one big structure
    for aDict in topLevelsPerThread:
        for key, val in aDict.iteritems():
            curEntry = getTopLevelFunctions(key, val.name)
            if curEntry.isInitialized():
                # The entry was already present and inited so we merge
                curEntry.merge(val)
            else:
                # Here, we will manually create it
                curEntry.set(val.count, val.totalTime, val.totalTimeSq)
                # Now create all the children entries
                for cKey, child in val.childrenEntries.iteritems():
                    tlChild = getTopLevelFunctions(cKey, child.tlEntry.name)
                    curEntry.addChild(ChildEntry(curEntry, tlChild, child.count, child.totalTime, child.totalTimeSq, child.totalTimeInChildren,
                        child.totalTimeInChildrenSq))

    print "Done creating global time structure, processing statistics..."

    for entry in flattenList([aDict.values() for aDict in topLevelsPerThread] + totalDict.values()):
        entry.calculateStatistics()

    print "Done processing statistics, sorting and printing..."
    for t in flattenList([threads, -1]):
        currentThread = t
        if currentThread >= 0:
            curDict = topLevelsPerThread[t]
        else:
            curDict = totalDict

        # Calculate total time
        totalMeasuredTime = 0.0
        for entry in curDict.itervalues():
            totalMeasuredTime += entry.totalTimeSelf
        # Sort by total time to assign a rank
        sortedByTotal = sorted(curDict.values(), key=itemgetter('totalTime'), reverse=True)

        for count,e in enumerate(sortedByTotal):
            e.rank = count+1 # It looks nicer if we start at 1

        # Now print
        if currentThread >= 0:
            print "#### Thread %d ####" % (currentThread)
        else:
            print "#### Overall ###"

        print "\tTotal measured time: %f" % totalMeasuredTime
        print "\t--- Flat profile ---"
        print "%Time        Cum ms             Self ms             Calls             Avg (Cum)            Std Dev (Cum)"
        sortedEntries = sorted(curDict.values(), key=itemgetter('totalTimeSelf'), reverse=True)
        for e in sortedEntries:
            print "%6.2f    %16.6f    %16.6f    %12d    %16.6f    %16.6f    %s [%d]" % (e.totalTimeSelf/totalMeasuredTime*100, e.totalTime,
                e.totalTimeSelf, e.count, e.avg, e.stdDev, e.name, e.rank)

        print "\n\n\t--- Call-graph profile ---"
        print "Index      %Time           Self             Descendant                 Called"
        for e in sortedByTotal:
            # First build a list of all the entries that refer to us in our parents
            myParentsChildrenEntry = []
            for pEntry in e.parentEntries.values():
                myParentsChildrenEntry.append(pEntry.childrenEntries[e.id])

            # Print all parents
            for myChildEntry in sorted(myParentsChildrenEntry, key=itemgetter("totalTime"), reverse=True):
                print " "*18, "%16.6f    %16.6f    %12d/%-12d    %s [%d]" % (myChildEntry.totalTimeSelf, myChildEntry.totalTimeInChildren, myChildEntry.count,
                    e.count, myChildEntry.parent.name, myChildEntry.parent.rank)

            # Print our entry
            print "[%3d]     %6.2f    %16.6f    %16.6f        %12d            %s [%d]" % (e.rank, e.totalTime/totalMeasuredTime*100, e.totalTimeSelf,
                e.totalTimeInChildren, e.count, e.name, e.rank)

            # Print all children
            for cEntry in sorted(e.childrenEntries.values(), key=itemgetter('totalTime'), reverse=True):
                print " "*18, "%16.6f    %16.6f    %12d/%-12d    %s [%d]" % (cEntry.totalTimeSelf, cEntry.totalTimeInChildren, cEntry.count,
                    cEntry.tlEntry.count, cEntry.tlEntry.name, cEntry.tlEntry.rank)
            # Done for this function
            print "-"*90

        print "\n"
    return 0

if __name__ == "__main__":
    sys.exit(main())
