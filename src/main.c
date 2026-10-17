#include "cJSON.h"
#include "log.h"
#include "util/util.h"
#include "factorio_instance.h"
#include "migration.h"
#include "flags.h"
#include "recipe_methods.h"

#include <unistd.h>

int
main( int argc, char** argv )
{
    Runtime_flags r_flags = {0};
    parse_flags( argc, argv, &r_flags );

    ENSURE_NON_NULL( r_flags.instance_path_1, "Must provide Factorio instance path with -i. See -h for help." );

    system( "rm -rf /tmp/factorio-mod-diff/" );

    LOG_VERBOSE( "Loading factorio instances" );
    Factorio_instance inst_1 = load_factorio_instance( r_flags.instance_path_1 );
    Factorio_instance inst_2 = load_factorio_instance( r_flags.instance_path_2 );

    generate_data_json( &inst_1 );
    generate_data_json( &inst_2 );

    LOG_VERBOSE( "Compute migration map" );
    Migration_map migration_map = get_full_migration( &inst_1, &inst_2 );

    LOG_VERBOSE( "Generate recipe json data" );
    cJSON* output = generate_recipe_diff( &migration_map, &inst_1, &inst_2, &r_flags );

    LOG_VERBOSE( "Writing output to: %s", r_flags.output_path );
    char* out = cJSON_Print( output );
    fprintf( r_flags.output_file, "%s\n", out );
    free( out );

    cJSON_Delete( output );

    free_factorio_instance( &inst_1 );
    free_factorio_instance( &inst_2 );

    free_migration_map( &migration_map );

    free( r_flags.instance_path_1 );
    free( r_flags.instance_path_2 );

    if( r_flags.should_close_output_file ) {
        free( r_flags.output_path );
        fclose( r_flags.output_file );
    }
    return 0;
}
