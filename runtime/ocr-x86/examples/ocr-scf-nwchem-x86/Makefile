CC=gcc
CFLAGS+=-g -O3 -mtune=native -Wall
OCR_INPUT_HOME=.
PROG=scf
SUFFIX=.exe
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr

OBJFILES = input.o integ.o diagonalize.o output.o twoel.o $(PROG).o

LDFLAGS += -lm

ifndef OCR_INSTALL
$(error OCR_INSTALL not set)
endif

OCR_CONFIG=${PWD}/scf-config.cfg

ifndef OCR_INPUT_HOME
$(error OCR_INPUT_HOME not set)
endif

ifdef BLAS
CFLAGS += -DBLAS
LDFLAGS += -lblas
endif

ifdef NO_PRECALC
CFLAGS += -DNO_PRECALC
endif

ifdef USE_CACHE
CFLAGS += -DUSE_CACHE
endif

ifndef NO_BOOKKEEPING
CFLAGS += -DBOOKKEEPING
endif

ifdef BAD_CACHE
CFLAGS += -DBAD_CACHE_UTILIZATION
endif

ifdef LAPACKE
CFLAGS += -DUSE_LAPACK
LDFLAGS += -llapacke -llapack -lblas
endif

ifdef DYNAMIC_SCHED
CFLAGS += -DDYNAMIC_SCHED
endif

ifdef OCR_ATOMIC
CFLAGS += -DOCR_ATOMIC
else ifdef OCR_CRITICAL
CFLAGS += -DOCR_CRITICAL
endif

ifeq (x$(MKLROOT),x)
else
CFLAGS += -I$(MKLROOT)/include -DUSE_MKL -DUSE_LAPACK -DBLAS
LDFLAGS += -L$(MKLROOT)/lib/intel64 -L$(MKLROOT)/../compiler/lib/intel64 -Wl,--start-group -lmkl_intel_lp64  -lmkl_intel_thread -lmkl_core -Wl,--end-group -liomp5
endif

OCR_RUN_FLAGS=-ocr:cfg ${OCR_CONFIG}

all: compile

run:
	./$(PROG)$(SUFFIX) $(OCR_RUN_FLAGS)

compile:  $(OBJFILES)
	gcc $(OBJFILES) $(CFLAGS) -o $(PROG)$(SUFFIX) $(LDFLAGS) $(OCR_FLAGS) -I.

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(OCR_FLAGS) -I.

clean:
	-rm -Rf *.o $(PROG)$(SUFFIX)
