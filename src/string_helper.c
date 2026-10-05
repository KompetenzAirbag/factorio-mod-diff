#include "string_helper.h"

#include <pwd.h>
#include <unistd.h>

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

char*
replace_in_string( char* src, char* find, char* replace_with )
{
    ENSURE_NON_NULL( src, "Argument src cannot be NULL" );
    ENSURE_NON_NULL( find, "Argument find cannot be NULL" );
    ENSURE_NON_NULL( replace_with, "Argument replace_with cannot be NULL" );

    int find_len = strlen( find );
    int rep_len = strlen( replace_with );
    int src_len = strlen( src );
    int out_len = src_len - find_len + rep_len + 1;

    ENSURE_ERR( out_len > 0, "Output length of src when 'find' is replaced cannot be less than 0" );

    char* out = malloc( out_len );
    ENSURE_NON_NULL( out, "Failed to allocate memory for string replacement" );

    int found = 0;
    int find_index = 0;
    for( ulong i = 0; i < strlen( src ); i++ ) {
        // Avoid strcmp so I dont need to ensure safe size
        if( src[i] == find[find_index] ) find_index++;

        if( find_index == find_len ) {
            found = 1;
            find_index = i - find_index + 1;
            break;
        }
        else find_index = 0;
    }

    if( found ) {
        for( int i = 0; i < find_index; i++ ) {
            out[i] = src[i];
        }
        for( int i = find_index; i < rep_len; i++ ) {
            out[i] = replace_with[i-find_index];
        }
        for( int i = find_index+find_len; i < src_len; i++ ) {
            out[i + rep_len-1] = src[i];
        }
        out[out_len-1] = '\0';
    } else {
        strcpy( out, src );
    }

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
