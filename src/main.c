#include "cJSON.h"
#include "log.h"
#include "util/util.h"
#include "factorio_instance.h"
#include "migration.h"
#include "flags.h"


#include <math.h> // NAN
#include <string.h>
#include <unistd.h>

void
add_removed_recipe( cJSON* output, char* name )
{
    cJSON* obj = cJSON_AddObjectToObject( output, name );
    ENSURE_NON_NULL( obj, "Failed to create new json object" );

    cJSON_AddStringToObject( obj, "name", name );
    cJSON_AddStringToObject( obj, "status", "removed" );
}

cJSON*
get_recipe( cJSON* recipe, int use_expensive_recipe )
{
    if( recipe == NULL ) return NULL;

    char* recipe_mode = "normal";
    if( use_expensive_recipe ) recipe_mode = "expensive";

    cJSON* out_recipe = recipe;

    cJSON* out_obj = cJSON_GetObjectItem( recipe, recipe_mode );
    cJSON* out_ingredients = cJSON_GetObjectItem( out_obj, "ingredients" );
    out_ingredients = cJSON_Duplicate( out_ingredients, 1 );

    if( out_ingredients ) {
        cJSON_AddItemToObject( out_recipe, "ingredients", out_ingredients );
        cJSON* hidden_obj = cJSON_GetObjectItem( out_recipe, "hidden" );

        int is_hidden = cJSON_IsTrue( cJSON_GetObjectItem( out_obj, "hidden" ) );
        if( is_hidden ) {
            if( hidden_obj == NULL ) {
                cJSON_AddBoolToObject( out_recipe, "hidden", is_hidden );
            } else {
                cJSON_SetBoolValue( hidden_obj, is_hidden );
            }
        }
    }

    char* recipe_string = cJSON_Print( recipe );
    ENSURE_NON_NULL( out_recipe, "Could not convert recipe %s", recipe_string );
    free( recipe_string );

    return out_recipe;
}

cJSON*
get_ingredients( cJSON* recipe )
{
    cJSON* ingredients = cJSON_GetObjectItem( recipe, "ingredients" );

    char* recipe_string = cJSON_Print( recipe );
    ENSURE_NON_NULL( ingredients, "Could not find \"ingredients\" in recipe: %s", recipe_string );
    free( recipe_string );

    return ingredients;
}

double
get_amount( cJSON* obj )
{
    char* obj_string = cJSON_Print( obj );

    cJSON* amt = cJSON_GetObjectItem( obj, "amount" );
    ENSURE_NON_NULL( amt, "Could not find amount in obj %s", obj_string );

    double amt_val = cJSON_GetNumberValue( amt );
    ENSURE_ERR( amt_val != ((double)NAN), "Failed to get amount in obj %s", obj_string );

    free( obj_string );

    return amt_val;
}

void
add_ingr_with_amount( cJSON* ingredients_arr, char* name, double amt, char* status )
{

    cJSON* ingr_obj = cJSON_CreateObject();
    ENSURE_NON_NULL( ingr_obj, "Failed to create object" );

    cJSON_AddStringToObject( ingr_obj, "name", name );
    cJSON_AddStringToObject( ingr_obj, "status", status );
    if( amt != 0.0 ) cJSON_AddNumberToObject( ingr_obj, "amount", amt );

    cJSON_AddItemToArray( ingredients_arr, ingr_obj );
}

cJSON*
generate_recipe_diff( Migration_map* migration_map, Factorio_instance* inst_1, Factorio_instance* inst_2, Runtime_flags* r_flags )
{
    cJSON* output = cJSON_CreateObject();

    cJSON* json_1 = get_data_json( inst_1 );
    cJSON* all_recipes_1 = cJSON_GetObjectItem( json_1, "recipe" );
    ENSURE_NON_NULL( all_recipes_1, "Could not load recipes from json for instance 1" );

    cJSON* json_2 = get_data_json( inst_2 );
    cJSON* all_recipes_2 = cJSON_GetObjectItem( json_2, "recipe" );
    ENSURE_NON_NULL( all_recipes_2, "Could not load recipes from json for instance 2" );

    cJSON* recipe_1;
    cJSON_ArrayForEach( recipe_1, all_recipes_1 ) {
        recipe_1 = get_recipe( recipe_1, r_flags->use_expensive_recipe );
        char* recipe_name_1 = get_migrated_name( migration_map, recipe_1 );

        cJSON* recipe_2 = cJSON_GetObjectItem( all_recipes_2, recipe_name_1 );
        recipe_2 = get_recipe( recipe_2, r_flags->use_expensive_recipe );

        if( recipe_2 == NULL ) {
            add_removed_recipe( output, recipe_name_1 );
        } else {
            cJSON* ingredients_1 = get_ingredients( recipe_1 );

            cJSON* hidden_obj = cJSON_GetObjectItem( recipe_1, "hidden" );
            int was_hidden = cJSON_IsTrue( hidden_obj );

            cJSON* ingredients_2 = get_ingredients( recipe_2 );

            hidden_obj = cJSON_GetObjectItem( recipe_2, "hidden" );
            int is_hidden = cJSON_IsTrue( hidden_obj );

            cJSON* obj;

            if( was_hidden && is_hidden ) {
                continue;
            } else {
                obj = cJSON_AddObjectToObject( output, recipe_name_1 );

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

            /* Iterate "forwards" from ingredients 1 */
            cJSON* ingr_1;
            cJSON_ArrayForEach( ingr_1, ingredients_1 ) {
                char* name = get_migrated_name( migration_map, ingr_1 );
                cJSON* ingr_2;
                int found = 0;

                cJSON_ArrayForEach( ingr_2, ingredients_2 ) {
                    char* name_2 = get_migrated_name( migration_map, ingr_2 );

                    if( strcmp( name, name_2 ) == 0 ) {
                        found = 1;

                        double amount_1 = get_amount( ingr_1 );
                        double amount_2 = get_amount( ingr_2 );

                        double amt_diff = amount_2 - amount_1;
                        if( amt_diff == 0 ) break;

                        add_ingr_with_amount( ingredients_arr, name, amt_diff, "changed" );
                        break;
                    }
                }

                if( found == 0 ) {
                    add_ingr_with_amount( ingredients_arr, name, 0, "removed" );
                }
            }

            /* Reiterate "backwards" to backfill all newly added recipes */
            cJSON_ArrayForEach( ingr_1, ingredients_2 ) {
                char* name_1 = get_migrated_name( migration_map, ingr_1 );

                int found = 0;

                cJSON* ingr_2;
                cJSON_ArrayForEach( ingr_2, ingredients_arr ) {
                    char* name_2 = get_migrated_name( migration_map, ingr_2 );

                    if( strcmp( name_1, name_2 ) == 0 ) {
                        found = 1;
                        break;
                    }
                }

                if( found == 0 ) {
                    add_ingr_with_amount( ingredients_arr, name_1, get_amount( ingr_1 ), "added" );
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
