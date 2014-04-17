OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr
CFLAGS = -g

EXES=sequential_mergesort_test parallel_mergesort_test

all: ${EXES}

parallel_mergesort_test.exe: sequential_mergesort.cpp parallel_mergesort.cpp parallel_mergesort.cpp sequential_mergesort.h Makefile
	g++ ${CFLAGS} ${OCR_FLAGS} -o parallel_mergesort_test.exe sequential_mergesort.cpp parallel_mergesort.cpp -pthread


sequential_mergesort_test.exe: sequential_mergesort.cpp sequential_mergesort_test.cpp sequential_mergesort.h Makefile
	g++ ${CFLAGS} ${OCR_FLAGS} -o sequential_mergesort_test.exe sequential_mergesort_test.cpp sequential_mergesort.cpp

clean:
	rm -rf ${EXES}
