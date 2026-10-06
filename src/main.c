#include "string_helper.h"
#include "cJSON.h"
#include "util/util.h"

#include <errno.h>

void
run_factorio_instance( char* path, char* args )
{
    char* exe_path = concat_strings( path, args );

    FILE* f_instance_1 = popen( exe_path, "r" );
    ENSURE_NON_NULL( f_instance_1, "Failed not execute %s", path );
    free( exe_path );
    pclose( f_instance_1 );
}

cJSON*
generate_data_json( char* f_base_path )
{
    // This is done to be able to just call free on it later
    char* factorio_base_path = malloc( strlen( f_base_path ) + 1 );
    strcpy( factorio_base_path, f_base_path );

    char* home_dir = get_home_dir();

    if( home_dir != NULL ) {
        free( factorio_base_path ); // This must be freed as replace_in_string allocates its own memory
        factorio_base_path = replace_in_string( f_base_path, "~", home_dir );
    }

    char* f_bin_path = concat_strings( factorio_base_path, "/bin/x64/factorio" );
    char* factorio_flags = "--dump-data > /dev/null 2>&1";

    run_factorio_instance( f_bin_path, factorio_flags );

    char* data_dump_file_path = concat_strings( factorio_base_path, "/script-output/data-raw-dump.json" );
    FILE* data_dump_file = fopen( data_dump_file_path, "r" );
    ENSURE_NON_NULL( data_dump_file, "Could not open data dump file at %s: %s", data_dump_file_path, strerror( errno ) );

    char ch;
    Char_buf char_buf = {0};

    /* Reading file */
    while( (ch = fgetc( data_dump_file )) != EOF ) {
        da_append( &char_buf, ch );
    }
    da_append( &char_buf, '\0' );

    cJSON* out = cJSON_Parse( char_buf.items );
    ENSURE_NON_NULL( out, "Could not parse json file at: ", data_dump_file_path );

    fclose( data_dump_file );
    free( f_bin_path );
    free( factorio_base_path );
    free( data_dump_file_path );

    free( char_buf.items );

    return out;
}

typedef struct Runtime_flags
{
    char* instance_path_1;
    char* instance_path_2;
    FILE* output_file;
    int   should_close_output_file;
} Runtime_flags;

void
parse_flags( int argc, char** argv, Runtime_flags* r_flags )
{
    for( int i = 0; i < argc; i++ ) {
        char* flag = argv[i];

        if( strcmp( flag, "-i") == 0 || strcmp( flag, "--instances" ) == 0 ) {
            ENSURE_ERR( (i+2) < argc, "Missing arguments for %s flag. See -h for help", flag );
            r_flags->instance_path_1 = argv[++i];
            r_flags->instance_path_2 = argv[++i];
        }

        if( strcmp( flag, "-h" ) == 0 || strcmp( flag, "--help" ) == 0 ) {
            printf( "factorio-mod-diff [FLAGS]\n    -i | --instances <path> <path> path should point to the base directory of Factorio (for Steam installs: <steam>/steamapps/common/Factorio)\n    -o | --output <path> defaults to stdout, any other path will generate a file. \".json\" does not need to be mentioned.\n" );
            exit(0);
        }

        if( strcmp( flag, "-o" ) == 0 || strcmp( flag, "--output" ) == 0 ) {
            ENSURE_ERR( (i++) < argc, "Missing argument for %s flag. See -h for help", flag );

            char* output_file_path = malloc( strlen(argv[i])  + 1 );
            strcpy( output_file_path, argv[i] );

            char* home_dir = get_home_dir();

            if( home_dir != NULL ) {
                free( output_file_path ); // This must be freed as replace_in_string allocates its own memory
                output_file_path = replace_in_string( argv[i], "~", home_dir );
            }
            r_flags->output_file = fopen( output_file_path, "w+" );

            ENSURE_NON_NULL( r_flags->output_file, "Failed to open output file: %s", strerror( errno ) );

            r_flags->should_close_output_file = 1;
            free( output_file_path );
        }
    }
}

int
main( int argc, char** argv )
{
    Runtime_flags r_flags = {0};
    parse_flags( argc, argv, &r_flags );

    ENSURE_NON_NULL( r_flags.instance_path_1, "Must provide Factorio instance path with -i. See -h for help." );
    cJSON* json = generate_data_json( r_flags.instance_path_1 );
    cJSON* recipe_json = cJSON_GetObjectItem( json, "recipe" );
    ENSURE_NON_NULL( recipe_json, "Could not load recipes from json" );

    cJSON_Delete( json );

    if( r_flags.should_close_output_file ) fclose( r_flags.output_file );
    return 0;
}
