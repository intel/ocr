CFLAGS=-O3 -I../common
LDFLAGS=-lm
CC := gcc

all: fft.out

%.out: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f *.out
