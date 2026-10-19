#include "recipe_methods.h"
#include "cJSON.h"
#include "migration.h"
#include "util/util.h"

#include <math.h> // NAN
#include <string.h>

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

    /* Forward iteration of all recipes of instance 1 */
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
            cJSON* ingredients_2 = get_ingredients( recipe_2 );
            if( ingredients_1 == NULL || ingredients_2 == NULL ) continue;

            cJSON* hidden_obj = cJSON_GetObjectItem( recipe_1, "hidden" );
            int was_hidden = cJSON_IsTrue( hidden_obj );

            hidden_obj = cJSON_GetObjectItem( recipe_2, "hidden" );
            int is_hidden = cJSON_IsTrue( hidden_obj );

            if( was_hidden && is_hidden ) continue;

            cJSON* obj = cJSON_AddObjectToObject( output, recipe_name_1 );
            ENSURE_NON_NULL( obj, "Failed to add object to object" );

            int has_change = 0;

            if( was_hidden ) {
                cJSON_AddBoolToObject( obj, "was-hidden", 1 );
                has_change = 1;
            }
            if( is_hidden ) {
                cJSON_AddBoolToObject( obj, "is-hidden", 1 );
                has_change = 1;
            }

            cJSON* ingredients_arr = cJSON_CreateArray();
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
                        if( amt_diff == 0 ) {
                            add_ingr_with_amount( ingredients_arr, name, amount_1, "unchanged" );
                            continue;
                        }

                        add_ingr_with_amount( ingredients_arr, name, amt_diff, "changed" );
                        has_change = 1;
                        break;
                    }
                }

                if( found == 0 ) {
                    add_ingr_with_amount( ingredients_arr, name, get_amount( ingr_1 ) * -1, "removed" );
                    has_change = 1;
                }
            }

            /* Reiterate "backwards" to backfill all newly added ingredients */
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
                    has_change = 1;
                }
            }

            if( has_change == 0 ) {
                cJSON_AddStringToObject( obj, "status", "unchanged" );
            } else {
                cJSON_AddStringToObject( obj, "status", "changed" );
            }

            cJSON_AddItemToObject( obj, "ingredients", ingredients_arr );
        }
    }

    /* Backwards iteration to fill all newly added recipes */
    cJSON_ArrayForEach( recipe_1, all_recipes_2 ) {
        recipe_1 = get_recipe( recipe_1, r_flags->use_expensive_recipe );
        char* name_1 = get_migrated_name( migration_map, recipe_1 );

        cJSON* recipe_2 = cJSON_GetObjectItem( all_recipes_1, name_1 );
        recipe_2 = get_recipe( recipe_2, r_flags->use_expensive_recipe );

        cJSON* recipe_in_map = cJSON_GetObjectItem( output, name_1 );
        if( recipe_in_map == NULL ) {
            cJSON* ingredients_1 = get_ingredients( recipe_1 );
            if( ingredients_1 == NULL ) continue;

            cJSON* hidden_obj = cJSON_GetObjectItem( recipe_1, "hidden" );
            int is_hidden = cJSON_IsTrue( hidden_obj );

            hidden_obj = cJSON_GetObjectItem( recipe_2, "hidden" );
            int was_hidden = cJSON_IsTrue( hidden_obj );

            if( was_hidden && is_hidden ) continue;

            cJSON* obj = cJSON_AddObjectToObject( output, name_1 );
            cJSON_AddStringToObject( obj, "status", "added" );

            if( was_hidden ) {
                cJSON_AddBoolToObject( obj, "was-hidden", 1 );
            }
            if( is_hidden ) {
                cJSON_AddBoolToObject( obj, "is-hidden", 1 );
            }

            cJSON* ingredients_arr = cJSON_AddArrayToObject( obj, "ingredients" );
            ENSURE_NON_NULL( ingredients_arr, "Failed to add array to object" );

            cJSON* ingr_1;
            cJSON_ArrayForEach( ingr_1, ingredients_1 ) {
                char* ingr_1_name = get_migrated_name( migration_map, ingr_1 );
                double amt = get_amount( ingr_1 );
                add_ingr_with_amount( ingredients_arr, ingr_1_name, amt, "added");
            }
        }
    }

    cJSON_Delete( json_1 );
    cJSON_Delete( json_2 );

    return output;
}

