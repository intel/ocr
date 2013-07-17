#
# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.
#

import base_objects, objects, config
import sys, getopt
import cmd
import logging

class InteractiveLoop(cmd.Cmd):
    """Command loop for interactive usage of the analysis scripts"""
    prompt = '# '
    intro = 'Read trace-file, entering the interactive shell'

    def __init__(self, driver):
        import readline
        super(InteractiveLoop, self).__init__(self)
        readline.set_completer_delims(" \t\n")
        self.driver = driver

    def do_dump(self, line):
        pass

    def complete_dump(self, line):
        pass

    def do_EOF(self, line):
        return True

class DriverResource(object):
    def __enter__(self):
        class Driver(object):
            def __init__(self, traceFileName, outFileName):
                self.myLogger = logging.getLogger('driver')
                self.traceFileName = None
                self.outFileName = None
                self.traceFile = None
                self.outFile = None
                self.processByGuid = dict()

            def setTraceFileInput(self, traceFileName):
                self.myLogger.info("Opening trace file %s", traceFileName)
                self.traceFileName = traceFileName
                if self.traceFile:
                    self.traceFile.close()
                self.traceFile = open(self.traceFileName, "r")

            def setFileOutput(self, outFileName):
                self.myLogger.info("Opening output file %s", outFileName)
                self.outFileName = outFileName
                if self.outFile:
                    self.outFile.close()
                self.outFile = open(self.outFileName, "w")

            def getOrCreateProcessForGuid(self, guid, objType):
                """Returns the object associated with the GUID or creates one"""
                val = self.processByGuid.get(guid, None)
                if val is None:
                    self.myLogger.info("Creating a process for GUID %d of type %d",
                                       guid, objType)
                val = config.gProcessesToCreate[objType](guid)
                self.processByGuid[guid] = val
                return val

            def readTrace(self):
                """Reads (or re-reads) the trace file from the beginning. This
                does not reset any internal data structures used to keep track
                of what is read
                """
                self.traceFile.seek(0) # Make sure we are back at the beginning
                self.myLogger.info("Reading trace file '%s'", self.traceFileName)
                for line in self.traceFile:
                    msg = Message.newMessage(self, line)
                    msg.transport()

            def dumpAll(self):
                self.myLogger.info("Dumping all information")
                for k,v in self.processByGuid.iteritems():
                    print "\n--- PROCESS %d ---\n" % (k)
                    v.dump()
                    print "\n------------------\n"

            def cleanup(self):
                if self.traceFile:
                    self.traceFile.close()
                if self.outFile:
                    self.outFile.close()

        self.internalObject = Driver()
        return self.internalObject

    def __exit__(self, exceptionType, exceptionValue, traceback):
        self.internalObject.cleanup()
        return False


class Usage:
    def __init__(self, msg):
        self.msg = msg

def main(argv=None):

    myLogger = logging.getLogger("main")
    inputFileName = ""
    outputFileName = ""
    cmdFileName = ""
    doInteractive = True # Defaults to interactive except if a cmd file is provided
    if argv is None:
        argv = sys.argv

    try:
        try:
            opts, args = getopt.getopt(argv[1:], "hvi:o:", ["help",
                                                            "version",
                                                            "input=",
                                                            "output="])
        except getopt.error, err:
            raise Usage(err)
        for o, a in opts:
            if o in ("-h", "--help"):
                raise Usage(\
"""
    -h,--help:    Prints this message
    -v,--version: Version information
    -i,--input:   Path to the file containing the trace to be analyzed
    -c,--command: Path to the file containing the list of interactive
                  commands to run (not yet implemented). If not specified,
                  the script will enter interactive mode
    -o,--output:  Path to the output file. If not specified, the output
                  file will be the same as the input file with '_out' appended
""")
            elif o in ("-v", "--version"):
                raise Usage("""Version 0.1 (Alpha quality)""")
            elif o in ("-i", "--input"):
                inputFileName = a
            elif o in ("-o", "--output"):
                outputFileName = a
            elif o in ("-c", "--command"):
                cmdFileName = a
            else:
                raise Usage("Unhandled option: %s" % (o))
            if args is not None and len(args) > 0:
                raise Usage("Extraneous arguments: " + ' '.join(args))

            if len(inputFileName) == 0:
                raise Usage("Input file not specified")
            if len(outputFileName) == 0:
                outputFileName = inputFileName + "_out"
            if len(cmdFileName) > 0:
                doInteractive = False

            myLogger.info("Starting analysis with trace file '%s', out file '%s' and interactive %d",
                          inputFileName, outputFileName, doInteractive)

            with DriverResource() as driver:
                myLogger.debug("Initialized driver")
                driver.setTraceFileInput(inputFileName)
                driver.setFileOutput(outputFileName)
                if doInteractive:
                    # Start interactive loop
                    myLogger.info("--- Entering interactive loop")
                    InteractiveLoop(driver).cmdloop()
                    myLogger.info("--- Done with interactive loop")
                else:
                    # Not implemented yet
                    assert(0)

            # End of with

        except Usage, msg:
            print >> sys.stderr, msg.msg
            print >> sys.stderr, "For help, use -h"
            return 2

        # Here, all arguments have been parsed and we can start
        # reading the file and parsing the commands



if __name__ == '__main__':
    sys.exit(main())
