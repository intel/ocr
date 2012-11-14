PROG=treesum_1level
CFLAGS=-O0 -g
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr

all-test: compile run

compile:
	gcc -g $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)

run:
	./$(PROG) 2012 1

clean:
	-rm -Rf *.o $(PROG)
