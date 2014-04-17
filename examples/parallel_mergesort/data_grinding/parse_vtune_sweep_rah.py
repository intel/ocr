#! /usr/bin/env python

import sys, csv, string, os, glob
#from glob import glob, iglob

if len(sys.argv) != 3:
    print "Usage: parse_vtune_sweep.py <directory of result files> <output filename>"
    exit()

vtune_result_directory = sys.argv[1]

#iterate over all the csv files in the result directory
for vtune_file in glob.iglob(os.path.join(vtune_result_directory, '*.csv')):
    print vtune_file

