PROG=ocr-stat-test
CFLAGS=-O0 -g -Werror
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr
OCR_HEADER= -I${OCR_INSTALL}/../src/inc

ifndef OCR_INSTALL
$(error OCR_INSTALL not set)
endif

ifndef OCR_CONFIG
OCR_CONFIG=${OCR_INSTALL}/config/default.cfg
$(warning OCR_CONFIG not set, defaulting to ${OCR_CONFIG})
endif

OCR_RUN_FLAGS=-ocr:cfg ${OCR_CONFIG}

all-test: compile 

compile:
	~/llvm/install/bin/clang  $(OCR_HEADER)  $(OCR_FLAGS) -I. callback.c -c
	~/llvm/install/bin/clang $(PROG).c $(OCR_FLAGS) -I. -emit-llvm -c -o $(PROG).bc 
	~/llvm/install/bin/opt -load ~/llvm/Debug+Asserts/lib/LLVMMem.so -mem < $(PROG).bc > ocr-stat-test_out.bc 
	~/llvm/install/bin/llc ocr-stat-test_out.bc -o $(PROG).s
	gcc -g  $(OCR_FLAGS) $(PROG).s callback.o -o $(PROG)

run:
	./$(PROG).exe $(OCR_RUN_FLAGS)

clean:
	-rm -Rf *.o $(PROG)
