/********
mystring.c -- utilities for manipulating strings

Eric Coutu
ID #0523365
********/

#include "mystring.h"
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

/*  Duplicate a string. The returned pointer may be passed to free()   */
char *newstr(char *oldstr) {

	if (oldstr == NULL) {
        return NULL;
    }
    else {   
        char *str = calloc(strlen(oldstr)+1, sizeof(char));
        assert(str != NULL);
        strcpy(str, oldstr);
        return str;
    }
}


/*  Convert all the characters of str to lower case using tolower() */
void str_tolower(char *str) {

    for (int i = 0; i < strlen(str); i++)
        *(str + i) = tolower(*(str + i));
}


/*  Preforms a case insensitive strcmp(str1,str2)   */
int strcmp_ic(char *str1, char *str2) {

    char tmp1[strlen(str1) + 1];
    char tmp2[strlen(str2) + 1];
    
    strcpy(tmp1, str1);
    strcpy(tmp2, str2);
    
    str_tolower(tmp1);
    str_tolower(tmp2);
    
    return strcmp(tmp1, tmp2);
}


/*  Performs a case insensitive strstr(str1,str2)   */
char *strstr_ic(char *str1, char *str2) {

    char tmp1[strlen(str1) + 1];
    char tmp2[strlen(str2) + 1];
    
    strcpy(tmp1, str1);
    strcpy(tmp2, str2);
    
    str_tolower(tmp1);
    str_tolower(tmp2);

    return strstr(tmp1, tmp2);    
}


/*  Checks if the beginning of str1 is equal to str2, ignoring case    */
_Bool strbeg_ic(char *str1, char *str2) {

    if (strlen(str1) < strlen(str2))
        return false;
    for (int i = 0; i < strlen(str2); i++) {
        if (tolower(str1[i]) != tolower(str2[i]))
            return false;
    }
    return true;
}


/*  Checks if c is any of the characters in set    */
_Bool chrset(char c, char *set) {

    if ( (set == NULL) || (strlen(set) == 0) )
        return false;
    for (int i = 0; i < strlen(set); i++) {
        if (c == *(set + i))
            return true;
    }
    return false;
}


/*  Counts the number of tokens that would be returned by strtok(str,delim) */
int str_count_toks(const char *str, char *delim) {

    int i = 0;
    char buf[strlen(str) + 1];
    strcpy(buf, str);
    char *p = strtok(buf, delim);
    for (i = 0; p != NULL; i++)
        p = strtok(NULL, delim);

    return i;
}
