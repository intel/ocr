#! /usr/bin/env python

# Script to run a bunch of parallel_mergesort_tcmalloc instances with
# different parameters through vtune to generate execution-time breakdowns

import subprocess, csv, sys
for i in range(18,30):  # i = log2 of array size
    for k in range (0, 7):  # k = log2 of number of processors to use
        for j in range(0, min((i-k)+1, i)):   # j = log2 of serial chunk size
            size = pow(2, i)
            chunk = pow(2, j)
            cores = pow(2, k)
            mdfile = ["mach_", str(cores), ".xml"]
            mdfile = "".join(mdfile)

            # create the command to run parallel_mergesort_tcmalloc through vtune
            command = ("amplxe-cl",  "-collect", "nehalem-general-exploration", "-knob", \
                           "enable-stack-collection=true", "-result-dir", "vtune_temp", "--", \
                           "/home/npcarter/migratory/microbenchmarks/parallel-mergesort/parallel_mergesort_tcmalloc", \
                           str(size), str(chunk), "-md", mdfile)
            print command
            # Execute that command and wait for it to complete
            sort_task = subprocess.Popen(command, stdout=subprocess.PIPE)
            sort_task.wait()
            
            outfile = ["vtune_sweep/parallel_mergesort_tcmalloc_", str(size), "_", str(chunk), "_", str(cores)]
            outfile = "".join(outfile)
            #create the command to generate the report
            command = ("amplxe-cl", "-R", "hotspots", "-result-dir", "vtune_temp", "-report-output",\
                           outfile, "-format", "csv", "-csv-delimiter", "comma")
            # Execute that command and wait for it to complete

            print command
            sort_task = subprocess.Popen(command, stdout=subprocess.PIPE)
            sort_task.wait()

            #create the command to clean up the vtune results directory
            command = ("rm", "-r", "vtune_temp")
            # Execute that command and wait for it to complete
            sort_task = subprocess.Popen(command, stdout=subprocess.PIPE)
            sort_task.wait()
