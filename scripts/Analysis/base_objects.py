#!/usr/bin/python

#
# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.
#


from itertools import chain
from blist import sorteddict
import logging

class LamportProcessCreator(type):
    """Meta-class creating LamportProcess based on filters.
    This will create a dictionary of methods that can be called
    on the LamportClass and properly direct them. It will also check
    for name conflicts and resolve them appropriately (hopefully)"""

    def __new__(cls, name, bases, attrs):
        myLogger = logging.getLogger('base.lamport-process')
        myLogger.info("Entering __new__ for LamportProcessCreator for class %s with bases %s", name, repr(bases))
        if name == 'LamportProcess':
            myLogger.debug("LamportProcess base class creation")
            return type.__new__(cls, name, bases, attrs)

        assert len([v for v in attrs.iterkeys() if not v.startswith('__')]) == 0
        assert len(bases) >= 1
        assert bases[0].__name__ == 'LamportProcess'

        allowedMethods = dict()
        allowedVariables = dict()

        # Check what methods and variables are in LamportProcess
        predefinedMethods = dict()
        predefinedVariables = []
        for attr in dir(LamportProcess):
            if attr.startswith('_'):
                continue
            obj = LamportProcess.__getattribute__(LamportProcess, attr)
            if callable(obj):
                # This is a method
                predefinedMethods[attr] = []
            else:
                # This is an attribute
                predefinedVariables.append(attr)
        # End for: We now have all the attributes from LamportProcess

        myLogger.debug("Got predefined methods: %s and predefined variables %s", repr(predefinedMethods.keys()), repr(predefinedVariables))
        # Go over all the other base classes and figure out which methods
        # Or attributes they define
        for base in bases[1:]:
            myLogger.debug("Processing base %s", base.__name__)
            for attr in dir(base):
                if attr.startswith('_'):
                    continue
                obj = base.__getattribute__(base, attr)
                if callable(obj):
                    if attr in allowedMethods:
                        if attr in predefinedMethods:
                            # This is *not* OK. We only allow class methods to overload the
                            # predefined ones
                            raise InvalidFilter("Filter method extending predefined one needs to be a class method for Filter %s: %s" %
                                                (base.__name__, attr))
                        else:
                            raise InvalidFilter("Filter method %s already exists (adding Filter %s)" % (attr, base.__name__))
                    else:
                        allowedMethods[attr] = obj
                elif type(obj) is classmethod:
                    if attr in predefinedMethods:
                        predefinedMethods[attr].append(base)
                    else:
                        raise InvalidFilter("Class method %d not allowed for Filter %s (not in predefined methods)" % (attr, base.__name__))
                else:
                    if attr in allowedVariables or attr in predefinedVariables:
                        raise InvalidFilter("Filter attribute %s already exists (adding Filter %s)" % (attr, base.__name__))
                    else:
                        allowedVariables[attr] = obj
            # End of attr: Got all attributes from base
        # End of base: Got all variables and methods from all bases

        # Now build the 'attrs' dictionary
        attrs = dict(chain(allowedMethods.iteritems(), allowedVariables.iteritems()))
        # We add the attributes about what to call for the common functions
        for k,v in predefinedMethods.iteritems():
            if len(v) > 0:
                attrs['_tocall_'+ k] = v

        myLogger.info("Creating new class with name %s and attributes %s", name, repr(attrs))
        return type.__new__(cls, name, (bases[0],), attrs)


class LamportProcess(object):
    """Base class for all Lamport processes which represent EDTs,
    DBs, events, workers and memory regions (scratchpads). The actual
    classes will be dynamically created based on the actual filters
    that the user wants to use.

    Note that all variables need to be defined as public class variables.
    Derived Lamport processes will inherit from this class as usual
    but the definition of variables as class variables is required
    to inform the metaclass of what attributes need to be reserved.
    Derived classes will also get an attribute '_tocall_<method-name>'
    containing the class of Filters to call when '<method-name>' is
    called (to aggregate their result).
    """
    __metaclass__ = LamportProcessCreator

    guid = 0
    """GUID of the LamportProcess. This uniquely identifies it"""
    allMessages = 0
    """All messages sent to this LamportProcess (sorted by timestamp)"""
    myLogger = 0
    """Logger for this lamport process"""
    cachedResponders = 0
    """Caches the responders of this LamportProcess for types of messages already seen"""

    def __init__(self, guid):
        self.guid = guid
        self.allMessages = sorteddict()
        self.myLogger = logging.getLogger('base.lamport-process')
        self.cachedResponders = dict()

    def __repr__(self):
        return "LamportProcess(name=%r, guid=%r)" % (self.__class__.__name__, self.guid)

    def respondsTo(self, kind):
        cachedValue = self.cachedResponders.get(kind, None)
        if cachedValue is not None:
            self.myLogger.info("%r: using cached value of respondsTo for kind %s: %d",
                               self, kind, len(cachedValue))
            return len(cachedValue) > 0
        else:
            # Figure out if we respond to this message and cache it
            cachedValue = []
            for base in self._tocall_respondsTo:
                if base.respondsTo(self, kind):
                    cachedValue.append(base)
            self.myLogger.info("%r: built cached responders for kind %s: %d",
                               self, kind, len(cachedValue))
            self.cachedResponders[kind] = cachedValue
            return len(cachedValue) > 0

    def notify(self, message):
        val = self.allMessages.get(message.timestamp, [])
        val.append(message)
        self.myLogger.debug("%r: added message %r @ %d", self,
                            message, message.timestamp)
        self.allMessages[message.timestamp] = val

        # Now call all responders to this message
        val = self.cachedResponders.get(kind, None)
        if val is not None:
            self.myLogger.debug("%r: Going over cached bases", self)
            for base in val:
                base.notify(self, message)
        else:
            # Go over all of them, we do not update cachedResponders
            self.myLogger.debug("%r: Going over all notify bases", self)
            for base in self._tocall_notify:
                base.notify(self, message)

    def dump(self, **kwargs):
        if len(self._tocall_dump):
            for base in self._tocall_dump:
                base.dump(self, **kwargs)
        else:
            # Dump all messages
            print "--- Messages dump for %r ---" % (self)
            for msgList in self.allMessages:
                for msg in msgList:
                    print "\t%r" % (msg)

class Message(object):
    """Message class. Each line of the trace file will form a single message."""

    regExpression = re.compile("(?P<ts>[0-9]+) : T (?P<type>[0-9]+) 0x(?P<src>[a-f0-9]+)\((?P<srcType>[0-9])\) -> 0x(?P<dest>[a-f0-9]+)\((?P<destType>[0-9])\) *(?P<subtype>[\S]*) *(?P<data>.*)")
    """Regular expression used to match a message"""

    myLogger = logging.getLogger('base.message')

    kindMapping = dict()
    """Maps between a kind and the message class that encapsulates it"""

    @classmethod
    def newMessage(cls, driver, line):
        """Parse a line of the trace file and return the proper Message
        instance"""
        cls.myLogger.info("Going to parse line %s", line)
        res = cls.regExpression.match(line)
        if res is not None:
            # The following code selects a message class that
            # responds to type and also looks for one to respond to
            # 'type.subtype'. It will prefer that one if found
            kind = res['type']
            msgClass = cls.kindMapping.get(kind)
            oldMsgClass = msgClass
            if len(res['subtype']):
                kind += '.' + res['subtype']
                msgClass = cls.kindMapping.get(kind, msgClass)
                if oldMsgClass == msgClass:
                    kind = res['type'] # Go back to the proper kind

            if msgClass is not None:
                cls.myLogger.debug("Found class %s for message", msgClass.__name__)

                srcProcess = driver.getOrCreateProcessForGuid(int(res['src'], 16),
                                                              int(res['srcType']))
                destProcess = driver.getOrCreateProcessForGuid(int(res['dest'], 16),
                                                               int(res['destType']))
                assert(srcProcess is not None and destProcess is not None)

                cls.myLogger.debug("Message from %r to %r", srcProcess, destProcess)

                return msgClass(srcProcess, destProcess, int(res['ts']), kind, res['data'])
            else:
                raise InvalidMessage("Message type %s (or parent) does not have associated message" % (kind))
        else:
            raise InvalidMessage("Message '%s' could not be parsed", line)


    @classmethod
    def registerMessageType(cls, kind, derived):
        """Registers a new message class ('derived') for messages of
        kind 'kind'. For every kind of message, there can only be one
        derived class that handles it. The last call to this method for
        a given kind takes precedence
        """
        cls.myLogger.info("Registering class '%s' for messages of kind '%s'", derived.__name__, kind)
        cls.kindMapping[kind] = derived

    def __init__(self, srcProcess, destProcess, timestamp, kind, data):
        """Initialize a message and pass it to the destProcess
            - srcProcess: LamportProcess derived class indicating the source of the message
            - destProcess: LamportProcess derivied class indicating the destination
            - timestamp: Unsigned value indicating the lamport clock timestamp
            - kind: A string indicating the type of message
            - data: the rest of the message
        """
        self.srcProcess = srcProcess
        self.destProcess = destProcess
        self.timestamp = timestamp
        self.kind = kind
        self.data = data
        self.myLogger = logging.getLogger('base.message')

    def __repr__(self):
        return "Message(src=%r, dest=%r, ts=%r)" % (self.srcProcess,
                                                    self.destProcess,
                                                    self.timestamp)

    def transport(self):
        """Actually send the message from source to destination.
        Could be done in init but for now keeping separate
        """
        if destProcess.respondsTo(kind):
            self.myLogger.debug("Sending %r", self)
            destProcess.notify(self)
        else:
            self.myLogger.info("Message %r was lost because the destination \
does not respond to it")

if __name__ == "main":
    assert(False) # Should not be loaded as the main file
