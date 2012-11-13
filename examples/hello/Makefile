PROG=helloworld
CFLAGS=-O0 -g
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr -lrt

all-test: compile run

compile:
	gcc -g $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)

run:
	./$(PROG)

clean:
	-rm -Rf *.o $(PROG)
