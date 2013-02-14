PROG=smithwaterman
SUFFIX=.exe
OMP_PROG=smith_waterman_tiled_omp
CFLAGS=-O3 -g
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr
OCR_INPUT=.

all-test: compile run

compile:
	gcc $(CFLAGS) $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)$(SUFFIX)

compile_omp:
	gcc -fopenmp $(CFLAGS) $(OCR_FLAGS) -I. $(OMP_PROG).c -o $(OMP_PROG)$(SUFFIX)

SIZE=larger
#SIZE=tiny

ifeq ($(SIZE),larger)
TILE_WIDTH=290
TILE_HEIGHT=300
EXPECTED_RESULT=36472
endif

ifeq ($(SIZE),tiny)
TILE_WIDTH=2
TILE_HEIGHT=2
EXPECTED_RESULT=12
endif

run:
	./$(PROG)$(SUFFIX) ${OCR_INPUT}/string1-${SIZE}.txt ${OCR_INPUT}/string2-${SIZE}.txt ${TILE_WIDTH} ${TILE_HEIGHT}

run_omp:
	./$(OMP_PROG)$(SUFFIX) ${OCR_INPUT}/string1-${SIZE}.txt ${OCR_INPUT}/string2-${SIZE}.txt ${TILE_WIDTH} ${TILE_HEIGHT}

clean:
	-rm -Rf *.o $(PROG)$(SUFFIX) $(OMP_PROG)$(SUFFIX)
