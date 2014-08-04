#ifndef RMD_LIB_H
#define RMD_LIB_H

s64 atoi(char *str)
{
    s64 retval = 0;
    s64 sign = 1;
    u64 index = 0;

    if(str[index]=='-') {
        sign = -1;
        index++;
    }

    while(str[index] >= '0' && str[index] <= '9') {
        retval = retval*10;
        retval += (str[index]-'0');
        index++;
    }

    return retval * sign;

}

#endif //RMD_LIB_H
