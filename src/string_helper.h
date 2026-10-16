#pragma once

#include "util/gen_types.h"

typedef struct String_buf
{
    char** items;
    ulong count;
    ulong capacity;
} String_buf;

/* free_string_buf will free String_buf type variables */
void
free_string_buf( String_buf* str_buf );

void
log_string_buf( String_buf* str_buf );

void
string_buf_sort( String_buf* buf );

/* replace_in_string will search for 'find' in src and replace it with
   'replace_with'. This function may (re)allocate memory and must be freed.
   This is a naive implementation */
char*
replace_in_string( char* src, char* out, char* find, char* replace_with );

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

void
write_file( char* path, char* content);

/* load_file loads an entire file and returns its content. This function
   allocates memory which must be freed */
char*
load_file( char* path );

/* split_string splits a string by a delimiter. String_buf contains allocated memory which must be freed using free_string_buf */
String_buf
split_string( const char* str, char delimiter );
