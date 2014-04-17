#! /usr/bin/env python

import sys, csv, string

if len(sys.argv) != 4:
    print "Usage: slice_runtime_by_chunk_size.py <input csv file> <output
    csv file> <chunk size to extract>"
