#! /usr/bin/env python

# Script to run a bunch of parallel_mergesort_test instances with
# different parameters and output the results in a CSV table for easy
# graphing

import subprocess, csv, sys

if (len(sys.argv) == 2):
    write_file = True;
    outFileName = sys.argv[1]
    outFile = csv.writer(open(outFileName, 'wb'))
else:
    write_file = False;
    
output_line = ["Array Size", "# of processors", "Run Time (s)"]
output = ", ".join(output_line)

if write_file:
    outFile.writerow(output_line)
    print output
else:
    print output


for i in range(15,30):  # i = log2 of array size
    for k in range (0, 7):  # k = log2 of number of processors to use
        size = pow(2, i)
        cores = pow(2, k)
        # Create the command to execute 
        args = ("./parallel_mergesort_test", str(size), str(cores))
        
        # Execute that command and wait for it to complete
        sort_task = subprocess.Popen(args, stdout=subprocess.PIPE)
        sort_task.wait()
        
        #get the output and parse it
        output = sort_task.stdout.read()
        output_lines = output.split("\n")
     
        #second line of output should tell us that the sorted
        #array is ok 
        if output_lines[1] != "Sorted array checks out":
            print "Detected an error in the sort\n"
            exit()
            
        #ok, parse first line of output, which should be:
        #array size: <array_size> chunk size: <chunk_size> run time
        #(microseconds): <run_time>
        output_words = output_lines[0].split(" ")
            
        #check format of first line
        if (output_words[0] != 'array') or (output_words[1] !=\
                                            'size:') \
        or (output_words[3] != 'threads:') \
        or (output_words[5] != 'run') or (output_words[6] !=\
                                          'time')\
        or (output_words[7] != '(microseconds):'):
            print "Detected an error in output formatting\n"
            exit()
                
        #ok, format looks good, extract the data
        array_size = output_words[2]
        threads = output_words[4]
        run_time = float(output_words[8])/1000000
        run_time = str(run_time)
        
        output_line = [array_size, threads, run_time];
        output = ", ".join(output_line)
        if write_file:
            outFile.writerow(output_line)
            print output        
        else:
            print output        
                   
