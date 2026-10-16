#include "cJSON.h"
#include "hashmap.h"
#include "log.h"
#include "string_helper.h"
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

cJSON*
generate_recipe_diff( struct hashmap* migration_map, Factorio_instance* inst_1, Factorio_instance* inst_2 )
{
    cJSON* output = cJSON_CreateObject();

    cJSON* json_1 = get_data_json( inst_1 );
    cJSON* recipe_json_1 = cJSON_GetObjectItem( json_1, "recipe" );
    ENSURE_NON_NULL( recipe_json_1, "Could not load recipes from json for instance 1" );

    cJSON* json_2 = get_data_json( inst_2 );
    cJSON* recipe_json_2 = cJSON_GetObjectItem( json_2, "recipe" );
    ENSURE_NON_NULL( recipe_json_2, "Could not load recipes from json for instance 2" );

    cJSON* recipe_1;
    cJSON_ArrayForEach( recipe_1, recipe_json_1 ) {
        char* search_name = get_migrated_name( migration_map, recipe_1 );

        cJSON* recipe_2 = cJSON_GetObjectItem( recipe_json_2, search_name );
        if( recipe_2 == NULL ) {
            cJSON* obj = cJSON_AddObjectToObject( output, search_name );
            ENSURE_NON_NULL( obj, "Failed to create new json object" );

            cJSON_AddStringToObject( obj, "name", search_name );
            cJSON_AddStringToObject( obj, "status", "removed" );
        } else {
            /* Default to normal difficulty. This will only be active for
               1.1 instances */
            cJSON* ingredients_1 = cJSON_GetObjectItem( recipe_1, "normal" );
            int was_hidden = 0;

            if( ingredients_1 ) {
                cJSON* hidden_obj = cJSON_GetObjectItem( ingredients_1, "hidden" );
                was_hidden = cJSON_IsTrue( hidden_obj );

                ingredients_1 = cJSON_GetObjectItem( ingredients_1, "ingredients" );
            }
            else {
                cJSON* hidden_obj = cJSON_GetObjectItem( recipe_1, "hidden" );
                was_hidden = cJSON_IsTrue( hidden_obj );

                ingredients_1 = cJSON_GetObjectItem( recipe_1, "ingredients" );
            }

            char* recipe_string = cJSON_Print( recipe_1 );
            ENSURE_NON_NULL( ingredients_1, "Could not find \"ingredients\" in recipe: %s", recipe_string );
            free( recipe_string );

            cJSON* ingredients_2 = cJSON_GetObjectItem( recipe_2, "ingredients" );
            recipe_string = cJSON_Print( recipe_2 );
            ENSURE_NON_NULL( ingredients_2, "Could not find \"ingredients\" in recipe: %s", recipe_string );
            free( recipe_string );

            cJSON* obj;

            cJSON* hidden_obj = cJSON_GetObjectItem( recipe_2, "hidden" );
            int is_hidden = cJSON_IsTrue( hidden_obj );

            if( was_hidden && is_hidden ) {
                continue;
            } else {
                obj = cJSON_AddObjectToObject( output, search_name );

                if( was_hidden ) {
                    cJSON_AddBoolToObject( obj, "was-hidden", 1 );
                }
                if( is_hidden ) {
                    cJSON_AddBoolToObject( obj, "is-hidden", 1 );
                }

                ENSURE_NON_NULL( obj, "Failed to add object to object" );
            }
            cJSON* ingredients_arr = cJSON_AddArrayToObject( obj, "ingredients" );
            ENSURE_NON_NULL( ingredients_arr, "Failed to add array to object" );

            cJSON* ingr_1;
            cJSON_ArrayForEach( ingr_1, ingredients_1 ) {
                char* name = get_migrated_name( migration_map, ingr_1 );
                cJSON* ingr_2;

                int found = 0;

                cJSON_ArrayForEach( ingr_2, ingredients_2 ) {
                    cJSON* name_2_obj = cJSON_GetObjectItem( ingr_2, "name" );
                    ENSURE_NON_NULL( name_2_obj, "Could not find name in ingredients" );

                    char* name_2 = cJSON_GetStringValue( name_2_obj );
                    ENSURE_NON_NULL( name_2, "Could not convert string object to char* in ingredients" );

                    if( strcmp( name, name_2 ) == 0 ) {
                        found = 1;

                        cJSON* amount_1_obj = cJSON_GetObjectItem( ingr_1, "amount" );
                        ENSURE_NON_NULL( amount_1_obj, "Could not find amount in ingredient 1" );

                        double amount_1 = cJSON_GetNumberValue( amount_1_obj );

                        cJSON* amount_2_obj = cJSON_GetObjectItem( ingr_2, "amount" );
                        ENSURE_NON_NULL( amount_1_obj, "Could not find amount in ingredient 2" );

                        double amount_2 = cJSON_GetNumberValue( amount_2_obj );
                        double amt_diff = amount_2 - amount_1;
                        if( amt_diff == 0 ) break;

                        cJSON* ingr_obj = cJSON_CreateObject();
                        ENSURE_NON_NULL( ingr_obj, "Failed to create object" );

                        cJSON_AddStringToObject( ingr_obj, "name", name );
                        cJSON_AddNumberToObject( ingr_obj, "amount", amt_diff );
                        cJSON_AddStringToObject( ingr_obj, "status", "changed" );

                        cJSON_AddItemToArray( ingredients_arr, ingr_obj );

                        break;
                    }
                }

                if( found == 0 ) {
                    cJSON* ingr_obj = cJSON_CreateObject();
                    ENSURE_NON_NULL( ingr_obj, "Failed to create object" );

                    cJSON_AddStringToObject( ingr_obj, "name", name );
                    cJSON_AddStringToObject( ingr_obj, "status", "removed" );

                    cJSON_AddItemToArray( ingredients_arr, ingr_obj );
                }
            }

            cJSON_ArrayForEach( ingr_1, ingredients_2 ) {
                cJSON* name_1_obj = cJSON_GetObjectItem( ingr_1, "name" );
                ENSURE_NON_NULL( name_1_obj, "Could not find name in ingredients" );

                char* name_1 = cJSON_GetStringValue( name_1_obj );
                ENSURE_NON_NULL( name_1, "Could not convert string object to char* in ingredients" );

                int found = 0;

                cJSON* ingr_2;
                cJSON_ArrayForEach( ingr_2, ingredients_arr ) {
                    cJSON* name_2_obj = cJSON_GetObjectItem( ingr_2, "name" );
                    ENSURE_NON_NULL( name_2_obj, "Could not find name in ingredients array" );

                    char* name_2 = cJSON_GetStringValue( name_2_obj );
                    ENSURE_NON_NULL( name_2, "Could not convert string object to char* in ingredients array" );

                    if( strcmp( name_1, name_2 ) == 0 ) {
                        found = 1;
                        break;
                    }
                }

                if( found == 0 ) {
                    cJSON* ingr_obj = cJSON_CreateObject();
                    ENSURE_NON_NULL( ingr_obj, "Failed to create object" );

                    cJSON* amt_obj = cJSON_GetObjectItem( ingr_1, "amount" );
                    ENSURE_NON_NULL( amt_obj, "Could not get amount from ingredient" );

                    double amt = cJSON_GetNumberValue( amt_obj );

                    cJSON_AddStringToObject( ingr_obj, "name", name_1 );
                    cJSON_AddStringToObject( ingr_obj, "status", "added" );

                    cJSON_AddNumberToObject( ingr_obj, "amount", amt );
                    cJSON_AddItemToArray( ingredients_arr, ingr_obj );
                }
            }
        }
    }

    cJSON_Delete( json_1 );
    cJSON_Delete( json_2 );

    return output;
}

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
    struct hashmap* migration_map = get_full_migration( &inst_1, &inst_2 );

    LOG_VERBOSE( "Generate recipe json data" );
    cJSON* output = generate_recipe_diff( migration_map, &inst_1, &inst_2 );

    char* out = cJSON_Print( output );
    write_file( "./output.json", out );
    free( out );

    cJSON_Delete( output );

    free_factorio_instance( &inst_1 );
    free_factorio_instance( &inst_2 );

    free_migration_map( migration_map );

    if( r_flags.should_close_output_file ) fclose( r_flags.output_file );
    return 0;
}
