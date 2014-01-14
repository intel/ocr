/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sal.h"
#include "ocr-types.h"

// Forward declarations
static void itona(char ** buf, u32* chars, u32 size, u32 base, s64 d);
static void ftona(char** buf, u32* chars, u32 size, double fp , u32 precision);
static u32 sal_internalPrintf(char* buf, u32 size, const char* fmt, void** rest);

void sal_exit(u64 errorCode) {
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_SAL_TERMINATE
    msg.type = PD_MSG_SAL_TERMINATE | PD_MSG_REQUEST;
    PD_MSG_FIELD(errorCode) = errorCode;
    PD_MSG_FIELD(file) = NULL;
    PD_MSG_FIELD(properties) = 0;
    RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0); // Would be funny if this failed... nice loop
#undef PD_MSG
#undef PD_TYPE
}

void sal_abort(const char* file, u64 line) {
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_SAL_TERMINATE
    msg.type = PD_MSG_SAL_TERMINATE | PD_MSG_REQUEST;
    PD_MSG_FIELD(errorCode) = 0;
    PD_MSG_FIELD(file) = NULL;
    PD_MSG_FIELD(properties) = 1;
    RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0); // Would be funny if this failed... nice loop
#undef PD_MSG
#undef PD_TYPE
}

void sal_assert(bool cond, const char* file, u64 line) {
    if(!cond) {
        ocrPolicyMsg_t msg;
        ocrPolicyDomain_t *pd = NULL;
        getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_SAL_TERMINATE
        msg.type = PD_MSG_SAL_TERMINATE | PD_MSG_REQUEST;
        PD_MSG_FIELD(errorCode) = line;
        PD_MSG_FIELD(file) = file;
        PD_MSG_FIELD(properties) = 2;
        RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0); // Would be funny if this failed... nice loop
#undef PD_MSG
#undef PD_TYPE
    }
}

void sal_printf(const char* string, ...) {
    u32 len;
    static char printfBuf[PRINTF_MAX];
    __builtin_va_list t;
    __builtin_va_start(t, string);
    len = sal_internalPrintf(printfBuf, PRINTF_MAX, string, (void**)t);

    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_SAL_PRINT
    // We now have the buffer and the length of things to print. We can
    // call into the PD
    msg.type = PD_MSG_SAL_PRINT | PD_MSG_REQUEST;
    PD_MSG_FIELD(buffer) = printfBuf;
    PD_MSG_FIELD(length) = len;
    PD_MSG_FIELD(properties) = 0;
    RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
}

/********************************************/
/* SUPPORT FUNCTIONS FOR PRINTF             */
/********************************************/

static __attribute__((aligned(8))) char msgFmt[]    = "%s";
static __attribute__((aligned(8))) char msgNull[]   = "(null)";
static __attribute__((aligned(8))) char msgBadFmt[] = "(bad fmt)";

#define max(a, b) ((a) >= (b) ? (a) : (b))

static void itona(char ** buf, u32* chars, u32 size, u32 base, s64 d) {
    char *p = *buf;
    char *p1, *p2;
    u64 ud = d;
    u32 radix = 10;

    if(base == 'd' && d < 0) {
        if(*chars == (size - 1)) {
            *buf = p;
            return;
        }

        *p++ = '-';
        (*buf)++;
        (*chars)++;

        ud = -d;
    } else if (base == 'x' || base == 'X') {
        radix = 16;
    }

    do {
        u32 remainder = ud % radix;

        if(*chars == (size - 1)) {
            *buf = p;
            return;
        }

        if(remainder < 10) {
            *p++ = remainder + '0';
        } else {
            *p++ = remainder + (base == 'x' ? 'a' : 'A') - 10;
        }

        (*chars)++;
    } while (ud /= radix);

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

/* quick and dirty fp printer */
static void ftona(char** buf, u32* chars, u32 size, double fp , u32 precision) {
    u32 i, j;
    if(*chars >= (size - 1 - 4)) { //check size - must fit atleast '+inf'.
        return;
    }

    //Grab the msbs of the FP number.
    u32 v = 1;
    if(*(char *)&v == 1) //endianness check.
        v = ((u32*)&fp)[1];
    else
        v = ((u32*)&fp)[0];

    //Check if NaN, +Inf, or -Inf.
    if ((v & 0x7ff80000L) == 0x7ff80000L) {
        *(*buf)++ = 'N';
        *(*buf)++ = 'a';
        *(*buf)++ = 'N';
        *(*buf)++ = '\0';
        (*chars)+=3;
        return;
    }
    if ((v & 0xfff10000L) == 0x7ff00000L) {
        *(*buf)++ = '+';
        *(*buf)++ = 'I';
        *(*buf)++ = 'n';
        *(*buf)++ = 'f';
        *(*buf)++ = '\0';
        (*chars)+=4;
        return;
    }
    if ((v & 0xfff10000L) == 0xfff00000L) {
        *(*buf)++ = '-';
        *(*buf)++ = 'I';
        *(*buf)++ = 'n';
        *(*buf)++ = 'f';
        *(*buf)++ = '\0';
        (*chars)+=4;
        return;
    }

    //Write if negative or positive FP number.
    if (fp < 0.0) { //don't check size here.
        fp = -1*fp;
        *(*buf)++ = '-';
        (*chars)++;
    }

    //Round to the given precision - symmetrical.
    double fact = 0.5;
    for(i=0; i<precision; ++i)
        fact/=10;
    fp+=fact;

    u64 inte = (u64)fp;      //integral part of the number.
    double frac = fp - inte; //fractional part of the number.

    //Write integral part in reversal.
    i = 0;
    if(inte > 0) {
        for(; inte > 0; ++i) {
            if(*chars >= (size - 1)) { //check size.
                return;
            }
            *(*buf)++ = '0' + inte%10;
            inte = inte/10;
            (*chars)++;
        }
    } else { //don't check size here.
        *(*buf)++ = '0';
        (*chars)++;
    }

    //Reverse integral part.
    for(j=0; j<i/2; ++j) {
        char ch = (*buf)[-1*j-1];
        (*buf)[-1*j-1] = (*buf)[-1*i+j];
        (*buf)[-1*i+j] = ch;
    }

    //Write decimal point.
    *(*buf)++ = '.';
    (*chars)++;

    //Write fractional part.
    for(i=0; i < precision; ++i) {
        if(*chars >= (size - 1)) { //check size.
            return;
        }
        frac *= 10;
        inte = (u64)frac;
        frac = frac - inte;

        if(i<16) //Inaccurate after 16 bits - this is true of libc printf also.
            *(*buf)++ = '0' + inte;
        else
            *(*buf)++ = '0'; //clip.
        (*chars)++;
    }

    *(*buf) = '\0';//NULL terminate.
}

static u32 sal_internalPrintf(char* buf, u32 size, const char* fmt, void** rest) {
    char c;
    char ** arg = (char **)rest;

    u32 tmp;
    u32 chars = 0;

    // Advance to first argument
    // arg++;
    while ((c = *fmt++) != 0) {
        // Check limit
        if(chars == (size - 1)) break;
        if (c != '%') {
            // Plain character case
            *buf++ = c;
            chars++;
        } else {
            // Format specifier case
            char * p;
            char isLong = 0;
            char hasPrecision = 0;
            u32 width = 0; /* Unused */
            u32 precision = 0;
            while(true) {
                c = *fmt++;
                switch (c) {
                case 'l':
                    isLong = 1;
                    continue;

                case '.':
                    hasPrecision = 1;
                    continue;
                    
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    if(hasPrecision == 0) {
                        width = width*10 + c-0x30;
                    } else {
                        precision = precision*10 + c-0x30;
                    }
                    continue;
                    
                case 'd':
                case 'u':
                case 'x':
                case 'X':
                    if(isLong) {
                        // We make sure we are properly aligned
                        arg = (char**)(((u64)arg + 0x7)&(-0x8));
                        itona(&buf, &chars, size, c, *((s64*)arg));
                        arg = (char**)((s64*)arg + 1);
                    } else {
                        // We make sure we are properly aligned
                        arg = (char**)(((u64)arg + 0x3)&(-0x4));
                        itona(&buf, &chars, size, c, *((u32 *)arg));
                        // Note that '((int*)arg + 1) CANNOT be replaced by
                        // ((int*)arg++) (nor can it be put in the xe_itona call above)
                        // because of precendence rules: '++' takes precedence over
                        // the casting operator and therefore (int*)arg++ will perform
                        // pointer arithmetic on a char** and therefore increment by 8 bytes
                        // as opposed to 4. You also cannot write ((int*)arg)++ as the conversion
                        // makes arg an rvalue and ++ requires an lvalue
                        arg = (char**)((u32*)arg + 1);
                    }
                    break;

                case 'f':
                case 'F': /*%lf falls through to here for compatibility*/
                    if(!hasPrecision)
                        precision = 6;
                    // We are always promoted to a double.
                    // We make sure we are properly aligned
                    arg = (char**)(((u64)arg + 0x7)&(-0x8));
                    ftona(&buf, &chars, size, *((double *)arg), precision);
                    arg = (char**)((double*)arg + 1);
                    break;

                case 's':
                    if (!isLong) { /*long string not supported*/
                        p = *arg++;
                        if (! p) {
                            char ** ptrToMsg = (char**)&msgNull;
                            tmp = sal_internalPrintf(buf, max(7, size - 1 - chars), msgFmt, (void**)&ptrToMsg);
                            chars += tmp;
                            buf += tmp;
                        } else {
                            while(*p) {
                                if(chars == (size-1)) break;

                                *buf++ = *p++;
                                chars++;
                            }
                        }
                        break;
                    }
                    /* fallthrough */
                default:
                {
                    char ** ptrToMsg = (char**)&msgBadFmt;
                    tmp = sal_internalPrintf(buf, max(10, size - 1 - chars), msgFmt, (void**)&ptrToMsg);
                    chars += tmp;
                    buf += tmp;
                    break;
                }
                } /* end of switch statement */
                break; /*while(true)*/
            }
        }
    }
    return chars;
}
