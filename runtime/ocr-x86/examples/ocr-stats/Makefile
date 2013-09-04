PROG=ocr-stat-test
CFLAGS=-O0 -g -Werror 
IFLAGS=-I${OCR_INSTALL}/include
LFLAGS=-L${OCR_INSTALL}/lib -locr

OCR_HEADER= -I${OCR_INSTALL}/../src/inc
# Change to match your environment. Default is that clang/opt/llc are in your path
CLANG=clang
OPT=opt
LLC=llc
STATS_PASS=../../scripts/StatsPass/OCRStatsPass.so

ifndef OCR_INSTALL
$(error OCR_INSTALL not set)
endif

ifndef OCR_CONFIG
OCR_CONFIG=${OCR_INSTALL}/config/default.cfg
$(warning OCR_CONFIG not set, defaulting to ${OCR_CONFIG})
endif

OCR_RUN_FLAGS=-ocr:cfg ${OCR_CONFIG}

all-test: compile 

%.o:%.c Makefile
	$(CLANG) $(CFLAGS) $(IFLAGS) -I. -c $*.c -emit-llvm -o $*.bc
	$(OPT) -load $(STATS_PASS) -OCRStats < $*.bc > $*_pass.bc
	$(CLANG) $(CFLAGS) -c $*_pass.bc -o $*.o

compile: $(PROG).o
	$(CLANG) $(LFLAGS) $(PROG).o -o $(PROG).exe

run:
	./$(PROG).exe $(OCR_RUN_FLAGS)

clean:
	-rm -Rf *.o *.exe *.bc
