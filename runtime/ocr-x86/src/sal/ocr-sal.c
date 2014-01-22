/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sal.h"
#include "ocr-sysboot.h"
#include "ocr-types.h"

#include <stdarg.h>

// TODO: Modify exit to properly handle bootup
void sal_exit(u64 errorCode) {
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_SAL_TERMINATE
    msg.type = PD_MSG_SAL_TERMINATE | PD_MSG_REQUEST;
    PD_MSG_FIELD(errorCode) = errorCode;
    PD_MSG_FIELD(properties) = 0;
    RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0); // Would be funny if this failed... nice loop
#undef PD_MSG
#undef PD_TYPE
}

void sal_abort(const char* file, u64 line) {
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    if(getCurrentEnv) {
        getCurrentEnv(&pd, NULL, NULL, &msg);
    }
    if(!pd) {
        bootUpAbort();
        return;
    }
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_SAL_TERMINATE
    msg.type = PD_MSG_SAL_TERMINATE | PD_MSG_REQUEST;
    PD_MSG_FIELD(errorCode) = 0;
    PD_MSG_FIELD(properties) = 1;
    RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0); // Would be funny if this failed... nice loop
#undef PD_MSG
#undef PD_TYPE
}

void sal_assert(bool cond, const char* file, u64 line) {
    if(!cond) {
        ocrPolicyMsg_t msg;
        ocrPolicyDomain_t *pd = NULL;
        if(getCurrentEnv)
            getCurrentEnv(&pd, NULL, NULL, &msg);
        if(!pd) {
            bootUpAbort();
            return;
        }
        sal_printf("ASSERT failure in file '%s' line %lu\n", file, line);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_SAL_TERMINATE
        msg.type = PD_MSG_SAL_TERMINATE | PD_MSG_REQUEST;
        PD_MSG_FIELD(errorCode) = 0;
        PD_MSG_FIELD(properties) = 2;
        RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0); // Would be funny if this failed... nice loop
#undef PD_MSG
#undef PD_TYPE
    }
}

/********************************************/
/* SUPPORT FUNCTIONS FOR PRINTF             */
/********************************************/

#define HASH_FLAG     (1 << (sizeof(int) * 8 - 1))

static void itona (char ** buf, int * chars, int size, int base, void * pd)
{
    char * p = *buf;
    char * p1, * p2;

    long long d;
    unsigned long long ud;

    int radix;
    int upper;

    int hash;

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

static __attribute__((aligned(sizeof(u64)))) char msg_null[]   = "(null)";
static __attribute__((aligned(sizeof(u64)))) char msg_badfmt[] = "(bad fmt)";

static int internal_vsnprintf(char *buf, int size, const char *fmt, va_list ap)
{
    char c;

    int chars = 0;

    int hash;

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
                    arg_lld = (unsigned long long)va_arg(ap, void *);
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
                        arg_lld = (unsigned long long)va_arg(ap, long long);
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
                    arg_d = va_arg(ap, unsigned int);
                    itona(&buf, &chars, size, hash | c, (void *)&arg_d);
                    break;

                case 's':
                    // Get a string pointer arg
                    p = va_arg(ap, char *);

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


int sal_snprintf(char * buf, int size, const char * fmt, ...)
{
    int tmp;
    va_list ap;

    va_start(ap, fmt);

    tmp = internal_vsnprintf(buf, size, fmt, ap);

    va_end(ap);

    return tmp;
}

static char printf_buf[PRINTF_MAX] __attribute__((aligned(sizeof(u64))));

int sal_printf(const char * fmt, ...) {

    int len;
    va_list ap;

    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;

    va_start(ap, fmt);

    len = internal_vsnprintf(printf_buf, PRINTF_MAX, fmt, ap);

    va_end(ap);


    if(getCurrentEnv)
        getCurrentEnv(&pd, NULL, NULL, &msg);
    if(!pd) {
        bootUpPrint(printf_buf, len);
        return len;
    }
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_SAL_PRINT
    // We now have the buffer and the length of things to print. We can
    // call into the PD
    msg.type = PD_MSG_SAL_PRINT | PD_MSG_REQUEST;
    PD_MSG_FIELD(buffer) = printf_buf;
    PD_MSG_FIELD(length) = len;
    PD_MSG_FIELD(properties) = 0;
    RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE

    return len;
}
