#Sorry, ugly for now, will fix it soon, I promise

all:	tester.c machine.cfg headers.h
	gcc -g -I../inc -I../../inc -I.. -I. -Iiniparser/src tester.c -Liniparser -liniparser
	./a.out machine.cfg
