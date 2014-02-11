PROG=helloworld
CFLAGS=-O0 -g -Werror -I. -I../../inc -c
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr -lpthread

ifndef OCR_INSTALL
$(error OCR_INSTALL not set)
endif

MYCONFIG=fsim_x86.cfg

ifeq ($(wildcard ${MYCONFIG}),)
ifndef OCR_CONFIG
OCR_CONFIG=${OCR_INSTALL}/config/default.cfg
$(warning OCR_CONFIG not set, defaulting to ${OCR_CONFIG})
endif
else
OCR_CONFIG=${MYCONFIG}
endif

OCR_RUN_FLAGS=-ocr:cfg ${OCR_CONFIG}

all-test: compile run

compile:
	gcc $(CFLAGS) $(PROG).c
	gcc -g -o $(PROG).exe $(PROG).o $(OCR_FLAGS)

run:
	./$(PROG).exe -ocr:cfg $(OCR_CONFIG)

clean:
	-rm -Rf *.o $(PROG).exe core*
