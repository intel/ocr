#!/usr/bin/Rscript --vanilla --slave

library(ggplot2)  # Get the plot library
library(scales)

a <- commandArgs(trailingOnly = TRUE)  # Get the arguments to this command

dataFrame = read.csv(a[1]) # Read the input file into a frame

dataFrame$factored = as.factor(dataFrame$chunk_size) # turn the array size
                                        # into a factor that will work
                                        # as a ggplot legend

plot = ggplot(dataFrame) # get the data into the plot-to-be

#plot run time vs. # of cores, with each chunk size as a separate line
plot = plot + aes_string(x = 'num_cores', y = 'speedup', color =
  'factored', group = 'factored')

plot = plot + geom_line(size = 1.25)  # Want a line graph

plot = plot + scale_color_hue(guide =
  guide_legend(reverse = TRUE, title = "Chunk Size (# Integers)", nrow = 15))  #nice colors, make legend match line height

plot = plot + theme_bw()

png(a[2], width = 9, height = 5, units = "in", res = 200) # open the
                                        # output file

plot

cruft <- dev.off() #close output file and ignore output
