#include "ocr-config.h"
#include "ocr-types.h"
#include "ocr-sal.h"

void _ocrAssert(bool val, const char* file, u32 line) {
    sal_assert(val, file, line);
}

//
// Common part of the SAL for all architectures
//

#define HASH_FLAG  (1 << (sizeof(int) * 8 - 1))

static void itona (char ** buf, u32 * chars, u32 size, u32 base, void * pd) {
    char * p = *buf;
    char * p1, * p2;

    long long d;
    unsigned long long ud;

    u32 radix;
    u32 upper;

    u32 hash;

    // Demux hash from base encoding
    hash = base & HASH_FLAG;
    base ^= hash;

    switch(base) {
    case 'p':
        ud = *(unsigned long long *)pd;

        upper = 0;
        radix = 16;
        hash = HASH_FLAG;
        break;

    case 'd':
        d = *(int *)pd;
        goto decimal;

    case ('l' << 8) + 'd':
        d = *(long *)pd;
        goto decimal;

    case ('l' << 16) + ('l' << 8) + 'd':
        d = *(long long *)pd;
        // Fall-through

decimal:

        radix = 10;
        upper = 0;

        // Handle negative integer cases
        if(d < 0) {
            if(*chars == (size - 1)) {
                *buf = p;
                return;
            }

            *p++ = '-';
            (*buf)++;
            (*chars)++;

            ud = -d;
        } else {
            ud = d;
        }
        break;

    case 'u':
        ud = *(unsigned int *)pd;
        goto usigned;

    case ('l' << 8) + 'u':
        ud = *(unsigned long *)pd;
        goto usigned;

    case ('l' << 16) + ('l' << 8) + 'u':
        ud = *(unsigned long long *)pd;
        // Fall-through

usigned:

        radix = 10;
        upper = 0;
        break;

    case 'x':
        ud = *(unsigned int *)pd;
        goto lohex;

    case ('l' << 8) + 'x':
        ud = *(unsigned long *)pd;
        goto lohex;

    case ('l' << 16) + ('l' << 8) + 'x':
        ud = *(unsigned long long *)pd;
        // Fall-through

lohex:

        radix = 16;
        upper = 0;
        break;

    case 'X':
        ud = *(unsigned int *)pd;
        goto hihex;

    case ('l' << 8) + 'X':
        ud = *(unsigned long *)pd;
        goto hihex;

    case ('l' << 16) + ('l' << 8) + 'X':
        ud = *(unsigned long long *)pd;
        // Fall-through

hihex:

        radix = 16;
        upper = 1;
        break;

    default:
        // Bad format, shouldn't happen, so just give up
        return;
    }

    do {
        unsigned long long remainder = ud % radix;

        if(*chars == (size - 1)) {
            *buf = p;
            return;
        }

        if(remainder < 10) {
            *p++ = remainder + '0';
        } else {
            *p++ = remainder + (upper ? 'A' : 'a') - 10;
        }

        (*chars)++;
    } while (ud /= radix);

    // Currently, we only support xX "%#" formatting
    if(hash && radix == 16) {
        *p++ = upper ? 'X' : 'x';
        *p++ = '0';
        *chars += 2;
    }

    p1 = *buf;
    p2 = p-1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }

    *buf = p;
}

// Quick and dirty FP printer
// Adapted from Aaron Landwehr's earlier code
static void ftona(char ** buf, u32 * chars, u32 size, u32 precision, double fp)
{
    int i, j;

    // Must fit at least '+inf'
    if((*chars) >= (size - 1 - 4)) goto out;

    // Grab the MSBs of the FP number
    // NB: Little endian assumption here, but
    //     CE, XE, X86, all satisfy that
    u32 v = ((u32*)&fp)[1];

    // Check for NaN, +Inf, or -Inf.
    if ((v & 0x7ff80000L) == 0x7ff80000L) {
        *(*buf)++ = 'N';
        *(*buf)++ = 'a';
        *(*buf)++ = 'N';
        (*chars)+=3;
        goto out;
    }
    if ((v & 0xfff10000L) == 0x7ff00000L) {
        *(*buf)++ = '+';
        *(*buf)++ = 'I';
        *(*buf)++ = 'n';
        *(*buf)++ = 'f';
        (*chars)+=4;
        goto out;
    }
    if ((v & 0xfff10000L) == 0xfff00000L) {
        *(*buf)++ = '-';
        *(*buf)++ = 'I';
        *(*buf)++ = 'n';
        *(*buf)++ = 'f';
        (*chars)+=4;
        goto out;
    }

    // Check for negative FP number
    if (fp < 0.0) {
        fp = -1*fp;
        // No size check
        *(*buf)++ = '-';
        (*chars)++;
    }

    // Round to the given precision - symmetrical
    double fact = 0.5;
    for(i=0; i<precision; i++)
        fact /= 10;
    fp += fact;

    // Integral part of the number
    u64 inte = (u64)fp;
    // Fractional part of the number
    double frac = fp - inte;

    // Write integral part in reverse
    i = 0;
    if (inte > 0) {
        for (; inte > 0; i++) {
            // Size check
            if((*chars) >= (size - 1 - 1)) goto out;

            *(*buf)++ = '0' + inte%10;
            inte = inte/10;
            (*chars)++;
        }
    } else {
        // No size check
        *(*buf)++ = '0';
        (*chars)++;
    }

    // Reverse integral part
    for (j=0; j<i/2; j++) {
        char ch = (*buf)[-1*j-1];
        (*buf)[-1*j-1] = (*buf)[-1*i+j];
        (*buf)[-1*i+j] = ch;
    }

    // Size check
    if((*chars) >= (size - 1 - 1)) goto out;
    // Write decimal point
    *(*buf)++ = '.';
    (*chars)++;

    // Write fractional part
    for (i=0; i<precision; i++) {
        // Size check
        if((*chars) >= (size - 1 - 1)) goto out;

        frac *= 10;
        inte = (u64)frac;
        frac = frac - inte;

        // Inaccurate after 16 bits - this is also true of libc printf
        if(i<16)
            *(*buf)++ = '0' + inte;
        else
            *(*buf)++ = '0'; // Clip
        (*chars)++;
    }

out:

    // NULL terminate
    *(*buf) = '\0';
}

static char msg_null[]   = "(null)";
static char msg_badfmt[] = "(bad fmt)";

static u32 internal_vsnprintf(char *buf, u32 size, const char *fmt, __builtin_va_list ap) {
    char c;

    u32 chars = 0;

    u32 hash;

    unsigned int       arg_d   = 0; // 32-bit
    unsigned long long arg_lld = 0; // 64-bit
    double             arg_f   = 0; // double

    while ((c = *fmt++) != 0) {

        hash = 0;

        // Check limit
        if(chars == (size - 1)) break;

        if (c != '%') {

            // Plain character case
            *buf++ = c;
            chars++;

        } else {

            // Format specifier case
            char * p;
            char done;
            u32 precision = 0;

            do {

                done = 1;

                c = *fmt++;
                switch (c) {
                case '%':
                    // Percent escape case
                    *buf++ = c;
                    chars++;
                    break;

                case '#':
                    if(hash) {
                        goto badfmt;
                    } else {
                        hash = HASH_FLAG;

                        // Loop one more time for more formatting
                        done = 0;
                    };
                    break;

                case 'p':
                    arg_lld = (unsigned long long)__builtin_va_arg(ap, void *);
                    itona(&buf, &chars, size, hash | c, (void *)&arg_lld);
                    break;

                case 'l':

                    c = *fmt++;

                    if(c == 'l') {
                        // This must be a 'long long' -- fold into 'long'
                        // sizeof(long) == sizeof(long long) == 8 bytes
                        c = *fmt++;
                    }

                    switch(c) {
                    case 'd':
                    case 'u':
                    case 'x':
                    case 'X':
                        arg_lld = (unsigned long long)__builtin_va_arg(ap, long long);
                        itona(&buf, &chars, size, hash | ('l' << 8) | c, (void *)&arg_lld);
                        break;

                    default:
                        goto badfmt;
                    }

                    break;

                case 'd':
                case 'u':
                case 'x':
                case 'X':
                    // %d, %u, %x, %X are all 32-bit so we can pull 'unsigned int'-worth of bits
                    arg_d = __builtin_va_arg(ap, unsigned int);
                    itona(&buf, &chars, size, hash | c, (void *)&arg_d);
                    break;

                case 's':
                    // Get a string pointer arg
                    p = __builtin_va_arg(ap, char *);

                    // If NULL, substitute a null message
                    if (! p) p = msg_null;

                    // Copy into output buffer
                    while(*p) {
                        if(chars == (size-1)) break;

                        *buf++ = *p++;
                        chars++;
                    }
                    break;


                case '.':
                    // Reset precision counter
                    precision = 0;

                    // Loop one more time for more formatting
                    done = 0;
                    break;

                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    // Convert to numeric
                    precision = precision*10 + c-'0';

                    // Loop one more time for more formatting
                    done = 0;
                    break;

                case 'f':
                case 'F':
                    // Default precision
                    if(!precision) precision = 6;

                    // We are always promoted to a double
                    arg_f = (double)__builtin_va_arg(ap, double);
                    ftona(&buf, &chars, size, precision, arg_f);
                    break;

                default:

badfmt:
                    p = msg_badfmt;

                    // Copy into output buffer
                    while(*p) {
                        if(chars == (size-1)) break;

                        *buf++ = *p++;
                        chars++;
                    }
                    break;
                }
            } while(!done);
        }
    }

    // The terminating \0 is not counted
    *buf++ = 0;

    return chars;
}

u32 SNPRINTF(char * buf, u32 size, const char * fmt, ...) {
    u32 tmp;
    __builtin_va_list ap;

    __builtin_va_start(ap, fmt);

    tmp = internal_vsnprintf(buf, size, fmt, ap);

    __builtin_va_end(ap);

    return tmp;
}


//
// Currently supports the following formatting:
//   strings:         %s
//   32-bit integers: %d, %u, %x, %X
//   64-bit integers: %ld, %lu, %lx, %lX, %lld, %llu, %#llx, %llX
//   64-bit pointers: %p
//   floating point:  %f
//
// In addition, the # flag is supported for %x, %lx, %llx
// In addition, precision is supported for %f
//

#define PRINTF_MAX (1024)

u32 PRINTF(const char * fmt, ...) {
    u32 tmp;
    __builtin_va_list ap;

    char printf_buf[PRINTF_MAX] __attribute__((aligned(sizeof(u64))));


    __builtin_va_start(ap, fmt);

    tmp = internal_vsnprintf(printf_buf, PRINTF_MAX, fmt, ap);

    __builtin_va_end(ap);

    sal_print(printf_buf, tmp+1);

    return tmp;
}
