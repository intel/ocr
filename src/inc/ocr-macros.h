#include <assert.h>
#include <stdlib.h>

// This is just in case we want to avoid checking sys calls.
#define OCR_CHECK_SYSCALL 1

#if OCR_CHECK_SYSCALL
#define checkedMalloc(ref, ...) (((ref = calloc(1, __VA_ARGS__))==NULL) ? (assert("error: malloc failed !\n" && 0), NULL) : ref);
#else
#define checkedMalloc(ref, ...) (ref = calloc(1, __VA_ARGS__))
#endif
