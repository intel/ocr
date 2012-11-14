PROG=smithwaterman
CFLAGS=-O3 -g
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr
OCR_INPUT=.
	
all-test: compile run

compile:
	gcc $(CFLAGS) $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)

compile:
	gcc -fopenmp $(CFLAGS) $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)

SIZE=larger

ifeq ($(SIZE),larger)
TILE_WIDTH=290
TILE_HEIGHT=300
EXPECTED_RESULT=36472
endif

run:
	./$(PROG) ${OCR_INPUT}/string1-${SIZE}.txt ${OCR_INPUT}/string2-${SIZE}.txt ${TILE_WIDTH} ${TILE_HEIGHT} 
 
run_omp:
	./$(PROG) ${OCR_INPUT}/string1-${SIZE}.txt ${OCR_INPUT}/string2-${SIZE}.txt ${TILE_WIDTH} ${TILE_HEIGHT} 

clean:
	-rm -Rf *.o $(PROG)
