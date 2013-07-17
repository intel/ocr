#
# This file is subject to the license agreement located in the file LICENSE
# and cannot be distributed without it. This notice cannot be
# removed or modified.
#

import objects

# This configuration line lists the types of
# LamportProcess that should be created for each type of
# LamportProcess in the trace file (ID)

# See ocr-guid.h for the up-to-date file detailing what IDs correspond to
# what processes
gProcessesToCreate = { 1 """allocator""": SimpleLamportProcess,
                      2 """DB""": SimpleLamportProcess,
                      3 """EDT""": SimpleLamportProcess,
                      4 """Event""": SimpleLamportProcess,
                      6 """Worker""": SimpleLamportProcess }
