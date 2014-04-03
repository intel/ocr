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


import os, sys, glob, re

def main(argv=None):
    if argv is None:
        argv = sys.argv

    if len(argv) < 3:
        print "Usage: %s rootDirectory outputFile ext1,ext2, [exclude dir1, exclude dir2, ]" % (argv[0])
        return 2

    topDir = argv[1]
    outputFile = argv[2]
    extensions = argv[3].split(',')
    listExcluded = argv[4:]
    curRoot = os.getcwd()

    allFunctions = set()
    allWarnings = set()

    profileLine_re = re.compile(r"\s*START_PROFILE\(([0-9A-Za-z_]+)\)");

    for root, subDirs, files in os.walk(topDir):
        print "Going down '%s' ..." % (root)
        # Remove all the excluded directories
        toDelete = []
        for i,v in enumerate(subDirs):
            if v in listExcluded:
                toDelete.append(i)
        decrement = 0
        for i in toDelete:
            del subDirs[i-decrement]
            decrement += 1

        print subDirs
        realRoot = os.path.abspath(root)
        # Process interesting files
        for extension in extensions:
            files = glob.glob(realRoot + '/*.' + extension)
            for file in files:
                print "\tProcessing %s" % (os.path.basename(file))
                fileHandle = open(file, 'r')
                for line in fileHandle:
                    matchOb = profileLine_re.match(line)
                    if matchOb is not None:
                        # We have a match
                        if matchOb.group(1) in allFunctions:
                            allWarnings.add(matchOb.group(1))
                        else:
                            allFunctions.add(matchOb.group(1))

                fileHandle.close()
    # We processed all the functions. We can start writing the output
    fileHandle = open(outputFile + '.h' , 'w')
    fileHandle.write("""#ifndef __OCR_PROFILERAUTOGEN_H__
#define __OCR_PROFILERAUTOGEN_H__

#ifndef OCR_RUNTIME_PROFILER
#error "OCR_RUNTIME_PROFILER should be defined if including the auto-generated profile file"
#endif
""")
#
#    for warning in allWarnings:
#        fileHandle.write('#warning "%s is defined multiple times"\n' % (warning))

    # Define the enum
    fileHandle.write('typedef enum {\n    ')
    counter = 0
    for function in allFunctions:
        fileHandle.write('%s = %d,\n    ' % (function, counter))
        counter += 1


    fileHandle.write('MAX_EVENTS = %d\n} profilerEvent_t;\n' % (counter))

    # Define the id to name mapping
    fileHandle.write('extern const char* _profilerEventNames[];\n\n#endif\n')

    # Now write the C file
    fileHandle.close()
    fileHandle = open(outputFile + '.c', 'w')
    fileHandle.write("#ifdef OCR_RUNTIME_PROFILER\n")
    fileHandle.write('#include "%s.h"\n' % (outputFile))
    fileHandle.write('const char* _profilerEventNames[] = {\n')

    isFirst = True
    for function in allFunctions:
        if isFirst:
            fileHandle.write('    "%s"' % (function))
            isFirst = False
        else:
            fileHandle.write(',\n    "%s"' % (function))

    fileHandle.write('\n};\n#endif')

    fileHandle.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
