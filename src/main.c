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
    // This is done so I can just call free on it later
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

int
main()
{
    cJSON* json = generate_data_json( "~/Games/factorio" );
    cJSON* recipe_json = cJSON_GetObjectItem( json, "recipe" );
    ENSURE_NON_NULL( recipe_json, "Could not load recipes from json" );

    cJSON_Delete( json );
    return 0;
}
