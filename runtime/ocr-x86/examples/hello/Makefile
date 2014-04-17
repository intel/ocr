PROG=helloworld
CFLAGS=-O0 -g -Werror
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr

ifndef OCR_INSTALL
$(error OCR_INSTALL not set)
endif

ifndef OCR_CONFIG
OCR_CONFIG=${OCR_INSTALL}/config/default.cfg
$(warning OCR_CONFIG not set, defaulting to ${OCR_CONFIG})
endif

OCR_RUN_FLAGS=-ocr:cfg ${OCR_CONFIG}

all-test: compile run

compile:
	gcc -g $(OCR_FLAGS) -I. $(PROG).c -o $(PROG).exe

run:
	./$(PROG).exe $(OCR_RUN_FLAGS)

clean:
	-rm -Rf *.o $(PROG).exe
