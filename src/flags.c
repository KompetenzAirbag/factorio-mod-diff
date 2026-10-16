#include "flags.h"
#include "util/util.h"
#include "factorio_instance.h"

#include <string.h>
#include <errno.h>

char*
unescape_path( const char* path )
{
    ulong len = strlen( path );
    char* result = malloc( len + 1 );

    ENSURE_NON_NULL(result, "Failed to allocate memory for path");

    ulong j = 0;

    for( ulong i = 0; i < len; i++ ) {
        if( path[i] == '\\' && path[i + 1] == ' ' ) i++;

        result[j++] = path[i];
    }

    result[j] = '\0';
    return result;
}

void
parse_flags( int argc, char** argv, Runtime_flags* r_flags )
{
    for( int i = 0; i < argc; i++ ) {
        char* flag = argv[i];

        if( strcmp( flag, "-i") == 0 || strcmp( flag, "--instances" ) == 0 ) {
            ENSURE_ERR( (i+2) < argc, "Missing arguments for %s flag. See -h for help", flag );
            r_flags->instance_path_1 = unescape_path( argv[++i] );
            r_flags->instance_path_2 = unescape_path( argv[++i] );
        }

        if( strcmp( flag, "-h" ) == 0 || strcmp( flag, "--help" ) == 0 ) {
            printf( "factorio-mod-diff [FLAGS]\n    -i | --instances <path 1> <path 2> path should point to the base directory of Factorio (for Steam installs: <steam>/steamapps/common/Factorio). <path 1> expects a 1.x or 2.x install while <path 2> only expects a 2.x install\n    -o | --output <path> defaults to stdout, any other path will generate a file. \".json\" does not need to be mentioned.\n    --use-expensive-recipe switches default \"normal\" recipe to expensive mode\n" );
            exit(0);
        }

        if( strcmp( flag, "--use-expensive-recipe" ) == 0 ) {
            r_flags->use_expensive_recipe = 1;
        }

        if( strcmp( flag, "-o" ) == 0 || strcmp( flag, "--output" ) == 0 ) {
            ENSURE_ERR( (++i) < argc, "Missing argument for %s flag. See -h for help", flag );

            char* output_path;

            char* unescaped_path = unescape_path( argv[i] );
            char* _output_path = replace_path_variables( unescaped_path, NULL );
            free( unescaped_path );

            if( strncmp( _output_path + strlen( _output_path ) - 5, ".json", 5 ) != 0 ) {
                output_path = concat_strings( _output_path, ".json" );
                free( _output_path );
            } else output_path = _output_path;

            r_flags->output_path = output_path;
            r_flags->output_file = fopen( r_flags->output_path, "w+" );

            ENSURE_NON_NULL( r_flags->output_file, "Failed to open output file: %s", strerror( errno ) );

            r_flags->should_close_output_file = 1;
        }
    }

    if( r_flags->output_file == NULL ) {
        r_flags->output_file = stdout;
        r_flags->output_path = "stdout";
    }
}
