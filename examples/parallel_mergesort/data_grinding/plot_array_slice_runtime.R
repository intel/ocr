#!/usr/bin/Rscript --vanilla --slave

library(ggplot2)  # Get the plot library

a <- commandArgs(trailingOnly = TRUE)  # Get the arguments to this command

ocrFrame = read.csv(a[1]) # Read the OCR input file into a frame
pthreadFrame = read.csv(a[2]) # and the pthread input file

arraySliceSize = a[3]  #Get the array size we want to plot

ocrFrameShortened = subset(ocrFrame, Array.Size == arraySliceSize)
pthreadFrameShortened = subset(pthreadFrame, Array.Size == arraySliceSize)

ocrFrameShortened = subset(ocrFrameShortened, Serial.Chunk.Size > 64)
pthreadFrameShortened$Serial.Chunk.Size = 'Pthread Version'
pthreadFrameShortened$factored = factor(pthreadFrameShortened$Serial.Chunk.Size) # turn the array size into a factor
#print(ocrFrameShortened)
ocrFrameShortened$factored = factor(ocrFrameShortened$Serial.Chunk.Size, ordered = TRUE) # turn the array size
                                        # into a factor that will work
                                        # as a ggplot legend

#ocrFrameShortened = rbind(ocrFrameShortened, pthreadFrameShortened)
# add the pthread version data
plot = ggplot(ocrFrameShortened) # get the data into the plot-to-be

#plot run time vs. # of cores, with each array size as a separate line
plot = plot + aes_string(x = 'Num.Processors', y = 'Run.Time', color =
  'factored', group = 'factored')

plot = plot + geom_line(size = 1.25, aes(order = factored))  # Want a line graph

plot = plot + scale_color_hue(guide = guide_legend(reverse = FALSE, title = "Serial Chunk Size", nrow = 15))  #nice colors, make legend match line height
# add the pthread data

plot = plot + theme_bw()  # eliminate grey background

plotTitle = paste("Array Size", arraySliceSize, "Elements", sep = " ")

plot = plot + ggtitle(plotTitle) # Add a title

plot = plot + xlab("Number of Cores")  + ylab("Run Time (Seconds)")
plot = plot + geom_line(data = pthreadFrameShortened, linetype = 'dashed', color = 'black')

png(a[4], width = 9, height = 5, units = "in", res = 200) # open the
                                        # output file
plot

cruft <- dev.off() #close output file and ignore output
