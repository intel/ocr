#!/usr/bin/python

import argparse
import multiprocessing
import os.path
import sys

# README First
# This script currently generates only OCR-HC configs
# WIP to extend it to:
# 1. Target FSim on X86
# 2. Read FSim machine config to spit out OCR-FSim configs

parser = argparse.ArgumentParser(description='Generate an OCR config file.')
parser.add_argument('--guid', dest='guid', default='PTR',
                   help='guid type to use (default: PTR)')
parser.add_argument('--platform', dest='platform', default='X86',
                   help='platform type to use (default: X86)')
parser.add_argument('--target', dest='target', default='X86',
                   help='target type to use (default: X86)')
parser.add_argument('--threads', dest='threads', default='4',
                   help='number of threads available to OCR (default: 4)')
parser.add_argument('--alloc', dest='alloc', default='32',
                   help='size (in MB) of memory available for app use (default: 32)')
parser.add_argument('--output', dest='output', default='default.cfg',
                   help='config output filename (default: default.cfg)')

args = parser.parse_args()
guid = args.guid
platform = args.platform
target = args.target
threads = int(args.threads)
alloc = args.alloc
outputfilename = args.output

def GenerateGuid(output, guid):
	output.write("[GuidType0]\n\tname\t=\t%s\n\n" % (guid))
	output.write("[GuidInst0]\n\tid\t=\t%d\n\ttype\t=\t%s\n" % (0, guid))
	output.write("\n#======================================================\n")

def GeneratePd(output, pdtype, threads):
	output.write("[PolicyDomainType0]\n\tname\t=\t%s\n\n" % (pdtype))
	output.write("[PolicyDomainInst0]\n")
	output.write("\tid\t\t\t=\t0\n")
	output.write("\ttype\t\t\t=\t%s\n" % (pdtype))
	output.write("\tworker\t\t\t=\t0-%d\n" % (threads-1))
	output.write("\tscheduler\t\t=\t0\n")
	output.write("\tallocator\t\t=\t0\n")
	output.write("\tcommapi\t\t\t=\t0\n")
	output.write("\tguid\t\t\t=\t0\n")
	output.write("\tparent\t\t\t=\t0\n")
	output.write("\tlocation\t\t=\t0\n")
	pdtype = "HC"
	output.write("\ttaskfactory\t\t=\t%s\n" % (pdtype))
	output.write("\ttasktemplatefactory\t=\t%s\n" % (pdtype))
	output.write("\tdatablockfactory\t=\tRegular\n")
	output.write("\teventfactory\t\t=\t%s\n\n" % (pdtype))

def GenerateCommon(output, pdtype):
	output.write("[TaskType0]\n\tname=\t%s\n\n" % (pdtype))
	output.write("[TaskTemplateType0]\n\tname=\t%s\n\n" % (pdtype))
	output.write("[DataBlockType0]\n\tname=\tRegular\n\n")
	output.write("[EventType0]\n\tname=\t%s\n\n" % (pdtype))
	output.write("\n#======================================================\n")

def GenerateMem(output, size, count):
	output.write("[MemPlatformType0]\n\tname\t=\t%s\n" % ("malloc"))
	output.write("[MemPlatformInst0]\n")
	output.write("\tid\t=\t0\n")
	output.write("\ttype\t=\t%s\n" % ("malloc"))
	output.write("\tsize\t=\t%d\n" % (int(size*1.05)))
	output.write("\n#======================================================\n")
	output.write("[MemTargetType0]\n\tname\t=\t%s\n" % ("shared"))
	output.write("[MemTargetInst0]\n")
	output.write("\tid\t=\t0\n")
	output.write("\ttype\t=\t%s\n" % ("shared"))
	output.write("\tsize\t=\t%d\n" % (int(size*1.05)))
	output.write("\tmemplatform\t=\t0\n")
	output.write("\n#======================================================\n")
	output.write("[AllocatorType0]\n\tname\t=\t%s\n" % ("tlsf"))
	output.write("[AllocatorInst0]\n")
	output.write("\tid\t=\t0\n")
	output.write("\ttype\t=\t%s\n" % ("tlsf"))
	output.write("\tsize\t=\t%d\n" % (size))
	output.write("\tmemtarget\t=\t0\n")
	output.write("\n#======================================================\n")

def GenerateComm(output, comms):
	output.write("[CommApiType0]\n\tname\t=\t%s\n" % ("Handleless"))
	output.write("[CommApiInst0]\n")
	output.write("\tid\t=\t0\n")
	output.write("\ttype\t=\t%s\n" % ("Handleless"))
	output.write("\tcommplatform\t=\t0\n")
	output.write("\n#======================================================\n")
	output.write("[CommPlatformType0]\n\tname\t=\t%s\n" % ("None"))
	output.write("[CommPlatformInst0]\n")
	output.write("\tid\t=\t0\n")
	output.write("\ttype\t=\t%s\n" % ("None"))
	output.write("\n#======================================================\n")

def GenerateComp(output, threads):
	output.write("[CompPlatformType0]\n\tname\t=\t%s\n" % ("pthread"))
	output.write("\tstacksize\t=\t0\n")
	output.write("[CompPlatformInst0]\n")
	output.write("\tid\t=\t0-%d\n" % (threads-1))
	output.write("\ttype\t=\t%s\n" % ("pthread"))
	output.write("\tstacksize\t=\t0\n")
	output.write("\n#======================================================\n")
	output.write("[CompTargetType0]\n\tname\t=\t%s\n" % ("PASSTHROUGH"))
	output.write("[CompTargetInst0]\n")
	output.write("\tid\t=\t0-%d\n" % (threads-1))
	output.write("\ttype\t=\t%s\n" % ("PASSTHROUGH"))
	output.write("\tcompplatform\t=\t0-%d\n" % (threads-1))
	output.write("\n#======================================================\n")
	output.write("[WorkerType0]\n\tname\t=\t%s\n" % ("HC"))
	output.write("[WorkerInst0]\n")
	output.write("\tid\t=\t0\n")
	output.write("\ttype\t=\tHC\n")
	output.write("\tworkertype\t=\tmaster\n")
	output.write("\tcomptarget\t=\t0\n")
	output.write("[WorkerInst1]\n")
	output.write("\tid\t=\t1-%d\n" % (threads-1))
	output.write("\ttype\t=\tHC\n")
	output.write("\tworkertype\t=\tslave\n")
	output.write("\tcomptarget\t=\t1-%d\n" % (threads-1))
	output.write("\n#======================================================\n")
	output.write("[WorkPileType0]\n\tname\t=\t%s\n" % ("HC"))
	output.write("[WorkPileInst0]\n")
	output.write("\tid\t=\t0-%d\n" % (threads-1))
	output.write("\ttype\t=\tHC\n")
	output.write("\n#======================================================\n")
	output.write("[SchedulerType0]\n\tname\t=\t%s\n" % ("HC"))
	output.write("[SchedulerInst0]\n")
	output.write("\tid\t=\t0\n")
	output.write("\ttype\t=\tHC\n")
	output.write("\tworkpile\t=\t0-%d\n" % (threads-1))
	output.write("\tworkeridfirst\t=\t0\n")
	output.write("\n#======================================================\n")

def GenerateConfig(filehandle, guid, platform, target, threads, alloc):
	filehandle.write("# Generated by python script, modify as you see fit\n\n")
	GenerateGuid(filehandle, guid)
	if target=='X86':
		GeneratePd(filehandle, "HC", threads)
		GenerateCommon(filehandle, "HC")
		GenerateMem(filehandle, alloc, 1)
		GenerateComm(filehandle, "null")
		GenerateComp(filehandle, threads)
	elif target=='FSIM':
		GeneratePd(filehandle, "CE", 1)
		GeneratePd(filehandle, "XE", threads)
		GenerateCommon(filehandle, "HC")
		GenerateMem(filehandle, alloc, 1)


def GetString(prompt, default):
	prompt = "%s [%s] " % (prompt, default)
	value = raw_input(prompt)
	if value == '':
		value = default
	if value == str(value):
		return value
	return int(value)


def CheckValue(value, valid):
	if str(value) not in str(valid):
		print 'Unsupported ', value, '; supported ones are ', valid
		raise

if target=='X86' and platform=='FSIM':
	print 'Incorrect values for platform & target (possibly switched?)'
	raise

if platform=='X86' or platform=='FSIM':
	max = multiprocessing.cpu_count()
	CheckValue(threads, range(1,max+1))

	alloc = int(alloc)*1048576

	if os.path.isfile(outputfilename):
		print 'Output file ', outputfilename, ' already exists'
		sys.exit(0)
	else:
		with open(outputfilename, "w") as filehandle:
			GenerateConfig(filehandle, guid, platform, target, threads, alloc)
else:
	print 'Platform ', platform, ' and target ', target, ' unsupported'
	raise
