#include "string_helper.h"
#include "util.h"
#include "util/dynamic_array.h"
#include "util/util.h"

#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>

void
free_string_buf( String_buf* str_buf )
{
    for( ulong i = 0; i < str_buf->count; i++ ) {
        free( str_buf->items[i] );
    }

    free( str_buf->items );
}

void
log_string_buf( String_buf* str_buf )
{
    for( uint i = 0; i < str_buf->count; i++ ) {
        LOG_INFO( "%s", str_buf->items[i] );
    }
}

int
compare_strings( const void* a, const void* b )
{
    const char* str_a = *(const char**)a;
    const char* str_b = *(const char**)b;

    return strcmp( str_a, str_b );
}

void
string_buf_sort( String_buf* buf )
{
    if( buf == NULL || buf->items == NULL || buf->count <= 1 ) {
        return;
    }

    qsort( buf->items, buf->count, sizeof(char*), compare_strings );
}

char*
concat_strings_with_delimiter( char* str_1, char* str_2, char delimiter )
{
    char* out = malloc( strlen( str_1 ) + strlen( str_2 ) + 2 );
    ENSURE_NON_NULL( out, "Could not allocate memory for string concatenation")

    strcpy( out, str_1 );
    out[strlen( str_1 )] = delimiter;
    out[strlen( str_1 ) + 1] = '\0';

    strcat( out, str_2 );

    return out;
}

char*
concat_strings( char* str_1, char* str_2 )
{
    char* out = malloc( strlen( str_1 ) + strlen( str_2 ) + 1 );
    ENSURE_NON_NULL( out, "Could not allocate memory for string concatenation")

    strcpy( out, str_1 );
    strcat( out, str_2 );

    return out;
}

/* find_in_string will find the first occurance of 'find' inside 'src'.
   Returns -1 if nothing is found */
long
find_in_string( char* src, char* find )
{
    ENSURE_NON_NULL( src, "Argument src cannot be NULL" );
    ENSURE_NON_NULL( find, "Argument src cannot be NULL" );
    ENSURE_ERR( find[0] != '\0', "Argument find cannot be empty" );

    char *match = strstr( src, find );

    if( !match ) return -1;

    return match - src;
}

char*
replace_in_string( char* src, char* out, char* find, char* replace_with )
{
    ENSURE_NON_NULL( src, "Argument src cannot be NULL" );
    ENSURE_NON_NULL( find, "Argument find cannot be NULL" );
    ENSURE_NON_NULL( replace_with, "Argument replace_with cannot be NULL" );
    ENSURE_ERR( find[0] != '\0', "Argument find cannot be empty" );

    ulong find_len = strlen( find );
    ulong rep_len  = strlen( replace_with );
    ulong src_len  = strlen( src );

    long find_index = find_in_string( src, find );
    if( find_index == -1 ) {
        out = realloc( out, src_len + 1 );
        ENSURE_NON_NULL( out, "Failed to reallocate memory" );

        memcpy( out, src, src_len + 1 );
        return out;
    }

    ulong out_len = src_len - find_len + rep_len + 1;

    out = realloc( out, out_len );
    ENSURE_NON_NULL( out, "Failed to allocate memory" );

    memcpy( out, src, find_index );
    memcpy( out + find_index, replace_with, rep_len );
    memcpy( out + find_index + rep_len,
            src + find_index + find_len,
            src_len - find_index - find_len );

    out[out_len - 1] = '\0';

    return out;
}

char*
get_home_dir()
{
#ifdef __WINDOWS__
    return NULL;
#else
    struct passwd* pw = getpwuid( getuid() );
    return pw->pw_dir;
#endif
}

char*
load_file( char* path )
{
    FILE* file = fopen( path, "r" );
    ENSURE_NON_NULL( file, "Could not open file for reading" );

    char ch;
    typedef struct Char_buf
    {
        char* items;
        ulong count;
        ulong capacity;
    } Char_buf;

    Char_buf char_buf = {0};

    while( (ch = fgetc( file )) != EOF ) {
        da_append( &char_buf, ch );
    }
    da_append( &char_buf, '\0' );

    char* out = malloc( char_buf.count );
    strcpy( out, char_buf.items );

    free( char_buf.items );
    fclose( file );

    return out;
}

String_buf
split_string( const char* str, char delimiter )
{
    String_buf out_buf = {0};
    ulong last_i = 0;
    ulong str_strlen = strlen( str );

    for( ulong i = 0; i < str_strlen; i++ ) {
        if( str[i] == delimiter ) {
            ulong token_len = i - last_i;

            char* token = malloc( token_len + 1 );
            memcpy( token, str + last_i, token_len );
            token[token_len] = '\0';
            last_i = ++i;

            da_append( &out_buf, token );
        }
    }

    if( last_i + 1 < str_strlen ) {
        ulong token_len = str_strlen - last_i;

        char* token = malloc( token_len + 1 );
        memcpy( token, str + last_i, token_len );
        token[token_len] = '\0';

        da_append( &out_buf, token );
    }

    return out_buf;
}
