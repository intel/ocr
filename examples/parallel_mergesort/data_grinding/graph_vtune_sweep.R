#! /usr/bin/Rscript --vanilla
#usage graph_vtune_sweep.R <array size to graph> <chunk size to graph>
#<input data file> <output file>


library(ggplot2)

args = commandArgs(trailingOnly = TRUE)
inputData = read.csv(args[3])
arraySize = args[1]
chunkSize = args[2]
outFile = args[4]
inputData = data.frame(inputData)

dataSubset = subset(inputData, Array.Size == arraySize & Chunk.Size == chunkSize)

p = ggplot(dataSubset, aes(Num.Processors, Run.Time))

p = p + geom_area(aes(colour = Function, fill = Function, order = -as.numeric(Function)), position = 'stack')
p = p + theme_bw()
p = p + scale_color_brewer(palette = "Set1")
p = p + scale_fill_brewer(palette = "Set1")
plotTitle = paste("Array Size", arraySize, "Elements,", "Chunk Size", chunkSize, "Elements", sep = " ")

p = p + ggtitle(plotTitle) # Add a title
p = p+ xlab("Number of Processors") + ylab("Fraction of Execution Time")
png(outFile, width = 1600, height = 1000, res = 200)
p
garbage = dev.off()
