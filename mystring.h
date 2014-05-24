/********
mystring.h -- header file for mystring.c

Eric Coutu
ID #0523365
********/

#ifndef COUTU_STRING_H_
#define COUTU_STRING_H_

#include <stdbool.h>

char *newstr(char *oldstr);
void str_tolower(char *str);
int strcmp_ic(char *str1, char *str2);
char *strstr_ic(char *str1, char *str2);
_Bool strbeg_ic(char *str1, char *str2);
_Bool chrset(char c, char *set);
int str_count_toks(const char *str, char *delim);

#endif

