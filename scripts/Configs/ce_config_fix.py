#!/usr/bin/python

import argparse
import multiprocessing
import os.path
import sys
import ConfigParser
import io
import itertools
import tempfile

# README First
# This script modifies a few fields in the OCR config from an FSim config
# Currently it only changes core counts, future changes will involve memory

parser = argparse.ArgumentParser(description='Generate a modified OCR config \
        file for CE from original config file & FSim config file.')
parser.add_argument('--fsimcfg', dest='fsimcfg', default='config.cfg',
                   help='FSim config file to use (default: config.cfg)')
parser.add_argument('--ocrcfg', dest='ocrcfg', default='default.cfg',
                   help='OCR config file to use (will be overwritten)')

args = parser.parse_args()
fsimcfg = args.fsimcfg
ocrcfg = args.ocrcfg
neighbors = 0
xe_count = 0

def ExtractValues(infilename):
    config = ConfigParser.SafeConfigParser(allow_no_value=True)
    config.readfp(infilename)
    uc = config.get('ChipGlobal', 'unit_count').strip(' ').split(' ')[0]
    uc = int(''.join(itertools.takewhile(lambda s: s.isdigit(), uc)))
    bc = config.get('UnitGlobal', 'block_count').strip(' ').split(' ')[0]
    bc = int(''.join(itertools.takewhile(lambda s: s.isdigit(), bc)))
    xc = config.get('BlockGlobal', 'xe_count').strip(' ').split(' ')[0]
    xc = int(''.join(itertools.takewhile(lambda s: s.isdigit(), xc)))

    global neighbors, xe_count
    neighbors = (bc-1)+(uc-1)
    xe_count = xc

def RewriteConfig(cfg):
    global neighbors, xe_count
    with open(cfg, 'r+') as fp:
        lines = fp.readlines()
        fp.seek(0)
        fp.truncate()
        for line in lines:
            if 'neighborcount' in line:
                line = '   neighborcount\t\t=\t'+ \
                    str(neighbors) + '\n'
            if 'neighbors' in line:
                if neighbors > 0:
                    line = '   neighbors\t\t=\t0-' \
                    + str(neighbors-1) + '\n'
                else:
                    line = '#neighbors\t\t=\n'
            if 'xecount' in line:
                line = '   xecount\t\t=\t' + \
                    str(xe_count) + '\n'
            fp.write(line)


def StripLeadingWhitespace(infile, outfile):
    with open(infile, "r") as inhandle:
        for line in inhandle:
            outfile.write(line.lstrip())

if os.path.isfile(ocrcfg):
    # Because python config parsing can't handle leading tabs
    with tempfile.TemporaryFile() as temphandle:
        StripLeadingWhitespace(fsimcfg, temphandle)
        temphandle.seek(0)
        ExtractValues(temphandle)
    RewriteConfig(ocrcfg)
else:
    print 'Unable to find OCR config file ', ocrcfg
    sys.exit(0)
