#include "cJSON.h"
#include "hashmap.h"
#include "util/util.h"
#include "factorio_instance.h"
#include "migration.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

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

            char* output_file_path = replace_path_variables( argv[i], NULL );
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

    Factorio_instance inst_1 = load_factorio_instance( r_flags.instance_path_1 );
    Factorio_instance inst_2 = load_factorio_instance( r_flags.instance_path_2 );

    struct hashmap* migration_map = new_migration_map();
    get_full_migration( migration_map, &inst_1, &inst_2 );

    cJSON* json_1 = generate_data_json( &inst_1 );
    cJSON* recipe_json_1 = cJSON_GetObjectItem( json_1, "recipe" );
    ENSURE_NON_NULL( recipe_json_1, "Could not load recipes from json for instance 1" );

    cJSON* json_2 = generate_data_json( &inst_2 );
    cJSON* recipe_json_2 = cJSON_GetObjectItem( json_2, "recipe" );
    ENSURE_NON_NULL( recipe_json_2, "Could not load recipes from json for instance 2" );

    cJSON_Delete( json_1 );
    cJSON_Delete( json_2 );

    free_factorio_instance( &inst_1 );
    free_factorio_instance( &inst_2 );

    ulong iter = 0;
    void* item;
    while( hashmap_iter( migration_map, &iter, &item ) ) {
        const struct Migration_data* migr_data = item;

        free( migr_data->from );
        free( migr_data->to );
    }

    hashmap_free( migration_map );

    if( r_flags.should_close_output_file ) fclose( r_flags.output_file );
    return 0;
}
