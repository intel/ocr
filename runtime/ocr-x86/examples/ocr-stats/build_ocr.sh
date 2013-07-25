~/llvm/install/bin/clang callback.c -c
~/llvm/install/bin/clang helloworld.c -emit-llvm -c -o hello.bc
~/llvm/install/bin/opt -load ../llvm/Debug+Asserts/lib/LLVMMem.so -mem < hello.bc >hello_out.bc
~/llvm/install/bin/llc hello_out.bc -o hello_out.s
gcc hello_out.s callback.o -o hello_out
