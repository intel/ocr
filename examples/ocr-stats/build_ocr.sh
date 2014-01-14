clang -c callback.c
clang -c ocr-stat-test.c -emit-llvm -o ocr-stat-test.bc
~opt -load ../../scripts/StatsPass/LLVMMem.so -mem < ocr-stat-test.bc > ocr-stat-test_out.bc
llc ocr-stat-test_out.bc -o ocr-stat-test_out.o
clang ocr-stat-test_out.o callback.o -o ocr-stat-test
