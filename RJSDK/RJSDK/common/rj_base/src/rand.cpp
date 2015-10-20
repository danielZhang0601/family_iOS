#include "util/rand.h"
#include <stdlib.h>


char* rand_char(char *dest, int len)
{
    static const char str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="; 
    static const int str_len = sizeof(str) - 1;

    for(int i = 0; i < len; i++)
    {
        dest[i] = str[ rand() % str_len ];
    }

    return dest;
}
