PROG=treesum_2level
SUFFIX=.exe
CFLAGS=-O0 -g
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr

all-test: compile run

compile:
	gcc -g $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)$(SUFFIX)

run:
	./$(PROG)$(SUFFIX)

clean:
	-rm -Rf *.o $(PROG)$(SUFFIX)
