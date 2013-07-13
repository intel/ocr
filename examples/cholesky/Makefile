OCR_INPUT_HOME=.
PROG=cholesky
SUFFIX=.exe
CFLAGS= -g
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr -lm
OCR_INPUT=${OCR_INPUT_HOME}

all: compile run
all-test: compile run verify

MATRIX_SIZE=2000
TILE_SIZE=80

INPUT_FILE=m_${MATRIX_SIZE}.in
OUTPUT_FILE=cholesky_out_${MATRIX_SIZE}.txt

compile:
	gcc -g $(CFLAGS) $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)$(SUFFIX)

compile.omp.task:
	gcc -fopenmp $(CFLAGS) $(PROG).omp.task.c -o $(PROG).omp.task$(SUFFIX) -lm

compile_pp:
	g++ $(CFLAGS) $(OCR_FLAGS) -I. $(PROG).cpp -o $(PROG)$(SUFFIX)

run:
	./$(PROG)$(SUFFIX) ${MATRIX_SIZE} ${TILE_SIZE} ${OCR_INPUT}/${INPUT_FILE}

clean:
	-rm -Rf *.o $(PROG)$(SUFFIX) cholesky.out

verify:
	diff -b ${OCR_INPUT}/$(OUTPUT_FILE) cholesky.out
