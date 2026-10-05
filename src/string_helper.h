#pragma once

#include "util/dynamic_array.h"
#include "util/util.h"

#include <string.h>

typedef struct Char_buf
{
    char* items;
    ulong count;
    ulong capacity;
} Char_buf;

typedef struct String_buf
{
    Char_buf* items;
    ulong count;
    ulong capacity;
} String_buf;

/* replace_in_string will search for 'find' in src and replace it with
   'replace_with'. This function may allocate memory and must be freed.
   This is a naive implementation */
char*
replace_in_string( char* src, char* find, char* replace_with );

/* concat_strings will concatenate two strings, this function will allocate
   memory which must be freed after use */
char*
concat_strings( char* str_1, char* str_2 );

/* concat_strings_with_delimiter will concatenate two strings, this function will allocate
   memory which must be freed after use */
char*
concat_strings_with_delimiter( char* str_1, char* str_2, char delimiter );

/* get_home_dir returns the home directory or NULL for windows. */
char*
get_home_dir();
