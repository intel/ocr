#include "ocr-config.h"
#include "ocr-types.h"
#include "ocr-sal.h"

//
// Common part of the SAL for all architectures
//

#define HASH_FLAG  (1 << (sizeof(int) * 8 - 1))

static void itona (char ** buf, u32 * chars, u32 size, u32 base, void * pd)
{
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

static char msg_null[]   = "(null)";
static char msg_badfmt[] = "(bad fmt)";

static u32 internal_vsnprintf(char *buf, u32 size, const char *fmt, __builtin_va_list ap)
{
    char c;

    u32 chars = 0;
    
    u32 hash;

    unsigned int       arg_d   = 0; // 32-bit
    unsigned long long arg_lld = 0; // 64-bit

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

u32 SNPRINTF(char * buf, u32 size, char * fmt, ...)
{
    u32 tmp;
    __builtin_va_list ap;

    __builtin_va_start(ap, fmt); 

    tmp = internal_vsnprintf(buf, size, fmt, ap);

    __builtin_va_end(ap);

    return tmp;
}


#define PRINTF_MAX (1024)
static char printf_buf[PRINTF_MAX] __attribute__((aligned(sizeof(u64))));

//
// Currently supports the following formatting:
//   strings:         %s
//   32-bit integers: %d, %u, %x, %X
//   64-bit integers: %ld, %lu, %lx, %lX, %lld, %llu, %#llx, %llX
//   64-bit pointers: %p
//
// In addition, the # flag is supported for %x, %lx, %llx
//
u32 PRINTF(char * fmt, ...)
{
    u32 tmp;
    __builtin_va_list ap;

    __builtin_va_start(ap, fmt); 

    tmp = internal_vsnprintf(printf_buf, PRINTF_MAX, fmt, ap);

    __builtin_va_end(ap);

    sal_print(printf_buf, tmp+1);

    return tmp;
}
