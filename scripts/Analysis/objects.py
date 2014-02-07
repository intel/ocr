#
# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.
#

import base_objects

class SimpleFilter(object):
    """A simple message filter. A filter processes messages sent to a LamportProcess.
    It should be used as a base class for any LamportProcess that needs to respond to
    messages that are dealt with by this filter. The following rules should be
    respected for filters:
        - All variables and methods should be class variables/methods

        - Non private variables and methods (not starting with _) will
          be inherited by any of the LamportProcess that use this filter as
          instance variable/method
        - Private variables/methods (starting with _) will be true class
          variables/methods and can be used to deal with state that needs to be
          maintained across LamportProcesses"""
    _classVariable = 42
    processVariable = 43

    @classmethod
    def respondsTo(cls, self, kind):
        """Return true or false if the filter responds to
        a particular kind of message. Kind is of the format 'type.subtype'
        """
        return True

    @classmethod
    def notify(cls, self, message):
        """Process the message received. The allMessages variable
        for the LamportProcess will already been updated by the time
        this function is called
        """
        pass

    @classmethod
    def dump(cls, self, **kwargs):
        """Prints out information collected by this filter"""
        # Dump all messages in this simple case
        print "--- Messages dump for %r ---" % (self)
        for msgList in self.allMessages:
            for msg in msgList:
                print "\t%r" % (msg)

class SimpleLamportProcess(base_objects.LamportProcess, SimpleFilter):
    pass # This will be the class properly
