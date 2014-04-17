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

output_line = ["Array Size", "Serial Chunk Size", "# of processors", "Run Time (s)"]
output = ", ".join(output_line)

if write_file:
    outFile.writerow(output_line)
    print output
else:
    print output


for i in range(15,30):  # i = log2 of array size
    for k in range (0, 7):  # k = log2 of number of processors to use
        for j in range(0, min((i-k)+1, i)):   # j = log2 of serial chunk size
            size = pow(2, i)
            chunk = pow(2, j)
            cores = pow(2, k)
            mdfile = ["mach_", str(cores), ".xml"]
            mdfile = "".join(mdfile)
            # Create the command to execute 
            args = ("./parallel_mergesort_tcmalloc.exe", "-md", mdfile, str(size), str(chunk))
            
            # Execute that command and wait for it to complete
            sort_task = subprocess.Popen(args, stdout=subprocess.PIPE)
            sort_task.wait()
            
            #get the output and parse it
            output = sort_task.stdout.read()
            output_lines = output.split("\n")
#            print output_lines
            #second line of output should tell us that the sorted
            #array is ok 
            if len(output_lines) == 3 and output_lines[1] == "Sorted array checks out":

                
            #ok, parse first line of output, which should be:
            #array size: <array_size> chunk size: <chunk_size> run time
            #(microseconds): <run_time>
                output_words = output_lines[0].split(" ")

            #check format of first line
                if (output_words[0] != 'array') or (output_words[1] !=\
                                                'size:') or (output_words[3] != 'chunk') or (output_words[4]\
                                                                                             != 'size:') or (output_words[6] != 'run') or (output_words[7] !=\
                                                                                                                                           'time') or (output_words[8] != '(microseconds):'):
                    print "Detected an error in output formatting\n"
                    exit()
                
            #ok, format looks good, extract the data
                array_size = output_words[2]
                chunk_size = output_words[5]
                run_time = float(output_words[9])/1000000
                run_time = str(run_time)
                    
                output_line = [array_size, chunk_size, str(cores), run_time];
                output = ", ".join(output_line)
                if write_file:
                    outFile.writerow(output_line)
                    print output        
                else:
                    print output        
 
            else:  
                print "Test seems to have not run correctly\n"
