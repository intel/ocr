PROG=cholesky
CFLAGS= -g
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr -lm
OCR_INPUT=${OCR_INPUT_HOME}/cholesky
	
all: compile run 
all-test: compile run verify

MATRIX_SIZE=2000
TILE_SIZE=80

INPUT_FILE=m_${MATRIX_SIZE}.in
OUTPUT_FILE=cholesky_out_${MATRIX_SIZE}.txt

compile:
	g++ $(CFLAGS) $(OCR_FLAGS) -I. $(PROG).cpp -o $(PROG)

run:
	./$(PROG)$(SUFFIX) ${MATRIX_SIZE} ${TILE_SIZE} ${OCR_INPUT}/${INPUT_FILE}

clean:
	-rm -Rf *.o $(PROG)$(SUFFIX) cholesky.out

verify:
	diff -b ${OCR_INPUT}/$(OUTPUT_FILE) cholesky.out
