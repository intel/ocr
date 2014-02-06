OCR_INPUT_HOME=.
PROG=cholesky
SUFFIX=.exe
CFLAGS= -O0 -g
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include 
LIB_FLAGS=-locr -lm
OCR_INPUT=${OCR_INPUT_HOME}/cholesky

ifndef OCR_INSTALL
$(error OCR_INSTALL not set)
endif

ifndef OCR_CONFIG
OCR_CONFIG=${OCR_INSTALL}/config/default.cfg
$(warning OCR_CONFIG not set, defaulting to ${OCR_CONFIG})
endif

ifndef OCR_INPUT_HOME
$(error OCR_INPUT_HOME not set)
endif

OCR_RUN_FLAGS=-ocr:cfg ${OCR_CONFIG}

all: compile run
all-test: compile run verify

MATRIX_SIZE=100
TILE_SIZE=10

INPUT_FILE=m_${MATRIX_SIZE}.in
OUTPUT_FILE=cholesky_out_${MATRIX_SIZE}.txt

compile:
	gcc -g $(CFLAGS) $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)$(SUFFIX) $(LIB_FLAGS)

compile.omp.task:
	gcc -fopenmp $(CFLAGS) $(PROG).omp.task.c -o $(PROG).omp.task$(SUFFIX) -lm

compile_pp:
	g++ $(CFLAGS) $(OCR_FLAGS) -I. $(PROG).cpp -o $(PROG)$(SUFFIX)

run:
	./$(PROG)$(SUFFIX) $(OCR_RUN_FLAGS) ${MATRIX_SIZE} ${TILE_SIZE} ${OCR_INPUT}/${INPUT_FILE}

run.omp.task:
	./$(PROG).omp.task$(SUFFIX) ${MATRIX_SIZE} ${TILE_SIZE} ${OCR_INPUT}/${INPUT_FILE}

clean:
	-rm -Rf *.o $(PROG)$(SUFFIX) cholesky.out

verify:
	diff -b ${OCR_INPUT}/$(OUTPUT_FILE) cholesky.out
