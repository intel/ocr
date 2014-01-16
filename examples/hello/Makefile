PROG=helloworld
CFLAGS=-O0 -g -Werror -I. -I../../inc -c
#OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr
OCR_FLAGS= ../../build/x86-linux/libocr.a -lpthread -lelf

ifndef OCR_INSTALL
$(error OCR_INSTALL not set)
endif

ifndef OCR_CONFIG
OCR_CONFIG=${OCR_INSTALL}/config/default.cfg
$(warning OCR_CONFIG not set, defaulting to ${OCR_CONFIG})
endif

OCR_RUN_FLAGS=-ocr:cfg hw.cfg

all-test: compile run

compile:
	gcc $(CFLAGS) $(PROG).c
	gcc -g -o $(PROG).exe $(PROG).o $(OCR_FLAGS)

run:
	./$(PROG).exe $(OCR_RUN_FLAGS)

clean:
	-rm -Rf *.o $(PROG).exe
