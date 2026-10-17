#include "migration.h"
#include "cJSON.h"
#include "hashmap.h"
#include "log.h"
#include "util.h"
#include "util/gen_types.h"
#include "string_helper.h"
#include "util/dynamic_array.h"

#include <malloc.h>
#include <dirent.h>
#include <string.h>

int
migration_compare( const void* a, const void* b, void* migration_data )
{
    const Migration_data* ua = a;
    const Migration_data* ub = b;

    (void)(migration_data);

    return strcmp( ua->from, ub->from );
}

u64
migration_hash( const void* item, u64 seed0, u64 seed1 ) {
    const Migration_data* migr = item;
    ENSURE_NON_NULL( migr->from, "\"From\" cannot be null" );
    return hashmap_sip( migr->from, strlen( migr->from ), seed0, seed1 );
}

/* unzip_mods will unzip all active mods from the given mods_path to output_path */
void
unzip_mods( char* mods_path, String_buf* active_mods, char* output_path )
{
    DIR* dir = opendir( mods_path );
    ENSURE_NON_NULL( dir, "Could not open directory %s", mods_path );

    String_buf paths_to_unzip = {0};
    ulong mods_path_strlen = strlen( mods_path );

    const struct dirent* ent;
    while( (ent = readdir( dir )) != NULL ) {
        String_buf mod_names = split_string( ent->d_name, '_' );

        for( uint i = 0; i < active_mods->count; i++ ) {
            ulong active_mod_strlen = strlen( active_mods->items[i] );
            ulong file_name_strlen = strlen( ent->d_name );

            if( file_name_strlen < active_mod_strlen ) continue;
            if( strncmp( ent->d_name + file_name_strlen - 4, ".zip", 4 ) != 0 ) continue;

            if( mod_names.count < 1 ) continue;

            if( strcmp( mod_names.items[0], active_mods->items[i] ) == 0 ) {
                da_append( &paths_to_unzip, (char*){0} );
                paths_to_unzip.items[paths_to_unzip.count - 1] = malloc( file_name_strlen + mods_path_strlen + 2 );

                strcpy( paths_to_unzip.items[paths_to_unzip.count - 1], mods_path );
                paths_to_unzip.items[paths_to_unzip.count - 1][mods_path_strlen] = '/';
                strcpy( paths_to_unzip.items[paths_to_unzip.count - 1] + mods_path_strlen + 1, ent->d_name );
            }
        }

        free_string_buf( &mod_names );
    }

    char* mkdir_command = concat_strings_with_delimiter( "mkdir -p", output_path, ' ');
    system( mkdir_command );
    free( mkdir_command );

    for( uint i = 0; i < paths_to_unzip.count; i++ ) {
        char* unzip_path = replace_in_string( paths_to_unzip.items[i], NULL, " ", "\\ " );

        char* unzip_cmd_part_1 = concat_strings_with_delimiter( "unzip", unzip_path, ' ');
        char* unzip_cmd_part_2 = concat_strings_with_delimiter( unzip_cmd_part_1, "'*/migrations/*' -d", ' ');
        char* unzip_cmd_part_3 = concat_strings_with_delimiter( unzip_cmd_part_2, output_path, ' ');
        char* unzip_cmd = concat_strings_with_delimiter( unzip_cmd_part_3, "> /dev/null 2>&1", ' ');

        /* TODO currently we do not care about the status code of unzip as
           it fails often due to some mods not having a migrations folder */
        int status = system( unzip_cmd );
        if( status == 0 ) LOG_DEBUG( "Unzipping %s", paths_to_unzip.items[i] );

        free( unzip_cmd );
        free( unzip_path );
        free( unzip_cmd_part_1 );
        free( unzip_cmd_part_2 );
        free( unzip_cmd_part_3 );
    }

    closedir (dir);

    free_string_buf( &paths_to_unzip );
}

typedef struct Migration_file_list
{
    String_buf* items; /* Reprecent paths to each migration file per mod */
    ulong       count;
    ulong       capacity;
} Migration_file_list;

String_buf
get_dir_entries( char* path )
{
    DIR* dir = opendir( path );
    ENSURE_NON_NULL( dir, "Could not open directory %s", path );

    String_buf files_dir = {0};

    const struct dirent* ent;
    while( (ent = readdir( dir )) != NULL ) {
        if( strcmp( ent->d_name, ".." ) == 0 || strcmp( ent->d_name, "." ) == 0 ) continue;

        char* f_name = malloc( strlen( ent->d_name ) + 1 );
        strcpy( f_name, ent->d_name );
        da_append( &files_dir, f_name );
    }

    closedir( dir );

    return files_dir;
}

String_buf
get_file_list_per_mod( char* path_1, char* path_2 )
{
    String_buf m_files_1 = get_dir_entries( path_1 );
    String_buf m_files_2 = get_dir_entries( path_2 );

    String_buf out = {0};

    for( uint i2 = 0; i2 < m_files_2.count; i2++ ) {
        ulong file_name_strlen = strlen( m_files_2.items[i2] );

        if( strncmp( m_files_2.items[i2] + file_name_strlen - 5, ".json", 5 ) != 0 ) continue;

        int found = 0;

        for( uint i1 = 0; i1 < m_files_1.count; i1++ ) {
            if( strcmp( m_files_2.items[i2], m_files_1.items[i1] ) == 0 ) {
                found = 1;
                break;
            }
        }

        if( found == 0 ) {
            da_append( &out, concat_strings_with_delimiter( path_2, m_files_2.items[i2], '/' ) );
        }
    }

    free_string_buf( &m_files_1 );
    free_string_buf( &m_files_2 );

    return out;
}

Migration_file_list
get_needed_migration_file_list( char* unzip_dir_1, char* unzip_dir_2 )
{
    Migration_file_list m_file_list = {0};

    String_buf files_dir_1 = get_dir_entries( unzip_dir_1 );
    String_buf files_dir_2 = get_dir_entries( unzip_dir_2 );
    String_buf files_dir_1_no_version = {0};
    String_buf files_dir_2_no_version = {0};

    for( uint i = 0; i < files_dir_1.count; i++ ) {
        String_buf f_name_cleaned = split_string( files_dir_1.items[i], '_' );

        if( f_name_cleaned.count >= 1 ) {
            char* f_name = malloc( strlen( f_name_cleaned.items[0] ) + 1 );
            strcpy( f_name, f_name_cleaned.items[0] );
            da_append( &files_dir_1_no_version, f_name );
        }

        free_string_buf( &f_name_cleaned );
    }

    for( uint i = 0; i < files_dir_2.count; i++ ) {
        String_buf f_name_cleaned = split_string( files_dir_2.items[i], '_' );

        if( f_name_cleaned.count >= 1 ) {
        char* f_name = malloc( strlen( f_name_cleaned.items[0] ) + 1 );
            strcpy( f_name, f_name_cleaned.items[0] );
            da_append( &files_dir_2_no_version, f_name );
        }

        free_string_buf( &f_name_cleaned );
    }

    ENSURE_ERR( files_dir_1.count == files_dir_1_no_version.count, "Count of files for directory 1 do not match" );
    ENSURE_ERR( files_dir_2.count == files_dir_2_no_version.count, "Count of files for directory 2 do not match" );

    for( uint f_index_1 = 0; f_index_1 < files_dir_1.count; f_index_1++ ) {
        char* _base_path_1 = concat_strings_with_delimiter( unzip_dir_1, files_dir_1.items[f_index_1], '/' );
        char* base_path_1 = concat_strings_with_delimiter( _base_path_1, "migrations", '/' );

        for( ulong f_index_2 = 0; f_index_2 < files_dir_2.count; f_index_2++ ) {
            char* _base_path_2 = concat_strings_with_delimiter( unzip_dir_2, files_dir_2.items[f_index_2], '/' );
            char* base_path_2 = concat_strings_with_delimiter( _base_path_2, "migrations", '/' );

            if( strcmp( files_dir_1_no_version.items[f_index_1], files_dir_2_no_version.items[f_index_2] ) == 0 ) {
                String_buf file_list = get_file_list_per_mod( base_path_1, base_path_2 );
                string_buf_sort( &file_list );

                da_append( &m_file_list, file_list );
            }

            free( _base_path_2 );
            free( base_path_2 );
        }

        free( _base_path_1 );
        free( base_path_1 );
    }

    free_string_buf( &files_dir_1 );
    free_string_buf( &files_dir_2 );

    free_string_buf( &files_dir_1_no_version );
    free_string_buf( &files_dir_2_no_version );

    return m_file_list;
}

struct hashmap*
new_migration_map()
{
    return hashmap_new( sizeof(Migration_data), 0, 0, 0, migration_hash, migration_compare, NULL, NULL );
}

void
get_partial_migration( struct hashmap* migration_map, cJSON* json )
{
    cJSON* obj;
    cJSON_ArrayForEach( obj, json ) {
        cJSON* from = cJSON_GetArrayItem( obj, 0 );
        cJSON* to = cJSON_GetArrayItem( obj, 1 );
        char* from_str = cJSON_GetStringValue( from );
        char* to_str = cJSON_GetStringValue( to );
        if( from_str == NULL || to_str == NULL ) continue;

        const Migration_data* migr_data = hashmap_get( migration_map, &(Migration_data){ .from = from_str });
        if( migr_data == NULL ) {
            Migration_data new_migr_data = {0};

            new_migr_data.from = malloc( strlen( from_str ) + 1 );
            new_migr_data.to = malloc( strlen( to_str ) + 1 );

            strcpy( new_migr_data.from, from_str );
            strcpy( new_migr_data.to, to_str );
            hashmap_set( migration_map, &new_migr_data );
        } else {
            Migration_data new_migr_data = {0};

            new_migr_data.from = migr_data->from;
            new_migr_data.to = realloc( migr_data->to, strlen( to_str ) + 1 );
            strcpy( new_migr_data.to, to_str );

            hashmap_set( migration_map, &new_migr_data );
        }
    }

}

Migration_map
get_full_migration( Factorio_instance* inst_1, Factorio_instance* inst_2 )
{
    String_buf active_mods_1 = get_active_mods( inst_1 );
    String_buf active_mods_2 = get_active_mods( inst_2 );
    String_buf difference_in_mods = {0};

    /* TODO This is a lazy implementation and doesn't account for the fact
       that active_mods_2 could have more mods active
       The double iteration is needed because of mod dependency changes
       (like angelspetrochem now has a graphics mod which changes the order) */
    for( uint i = 0; i < active_mods_1.count; i++ ) {
        uint found = 0;
        for( uint j = 0; j < active_mods_2.count; j++ ) {
            if( strcmp( active_mods_1.items[i], active_mods_2.items[j] ) == 0 ) {
                found = 1;
                break;
            }
        }

        if( found == 0 ) {
            char* diff = malloc( strlen( active_mods_1.items[i] ) + 1 );
            strcpy( diff, active_mods_1.items[i] );
            da_append( &difference_in_mods, diff );
        }
    }

    if( difference_in_mods.count > 0 ) {
        LOG_WARN( "Found difference in active mods (active in instance 1 but not instance 2): " );

        for( uint i = 0; i < difference_in_mods.count; i++ ) {
            LOG_WARN( "%s", difference_in_mods.items[i] );
        }
    }

    free_string_buf( &difference_in_mods );

    char* unzip_dir_1 = "/tmp/factorio-mod-diff/mods1";
    char* unzip_dir_2 = "/tmp/factorio-mod-diff/mods2";

    LOG_VERBOSE( "Unzipping mods from instance 1" );
    unzip_mods( inst_1->mods_path, &active_mods_1, unzip_dir_1 );

    LOG_VERBOSE( "Unzipping mods from instance 2" );
    unzip_mods( inst_2->mods_path, &active_mods_2, unzip_dir_2 );

    free_string_buf( &active_mods_1 );
    free_string_buf( &active_mods_2 );

    Migration_map m_map = {0};
    m_map.recipe_map = new_migration_map();
    m_map.fluid_map  = new_migration_map();
    m_map.item_map   = new_migration_map();

    Migration_file_list m_file_list = get_needed_migration_file_list( unzip_dir_1, unzip_dir_2 );
    for( uint i = 0; i < m_file_list.count; i++ ) {
        String_buf* files = &m_file_list.items[i];

        for( uint j = 0; j < files->count; j++ ) {
            LOG_DEBUG( "Migrating: %s", files->items[j] );

            char* read_file = load_file( files->items[j] );
            cJSON* json_migration = cJSON_Parse( read_file );
            ENSURE_NON_NULL( json_migration, "Could not load json from migration" );
            free( read_file );

            cJSON* recipe_migration = cJSON_GetObjectItem( json_migration, "recipe" );
            cJSON* item_migration = cJSON_GetObjectItem( json_migration, "item" );
            cJSON* fluid_migration = cJSON_GetObjectItem( json_migration, "fluid" );

            if( recipe_migration == NULL || item_migration == NULL || fluid_migration == NULL ) {
                cJSON_Delete( json_migration );
                continue;
            }

            get_partial_migration( m_map.recipe_map, recipe_migration );
            get_partial_migration( m_map.item_map, item_migration );
            get_partial_migration( m_map.item_map, fluid_migration );

            cJSON_Delete( json_migration );
        }
    }

    for( uint i = 0; i < m_file_list.count; i++ ) {
        free_string_buf( &m_file_list.items[i] );
    }

    free( m_file_list.items );

    return m_map;
}

void
free_single_migration_map( struct hashmap* migration_map )
{
    ulong iter = 0;
    void* item;
    while( hashmap_iter( migration_map, &iter, &item ) ) {
        const struct Migration_data* migr_data = item;

        free( migr_data->from );
        free( migr_data->to );
    }

    hashmap_free( migration_map );
}

void
free_migration_map( Migration_map* migration_map )
{
    free_single_migration_map( migration_map->recipe_map );
    free_single_migration_map( migration_map->item_map );
    free_single_migration_map( migration_map->fluid_map );
};

char*
get_migrated_name( Migration_map* migration_map, cJSON* obj )
{
    cJSON* recipe_name_obj = cJSON_GetObjectItem( obj, "name" );
    char* recipe_name = cJSON_GetStringValue( recipe_name_obj );

    char* recipe_string = cJSON_Print( obj );
    ENSURE_NON_NULL( recipe_name, "Could not find recipe name in recipe: %s", recipe_string );
    free( recipe_string );

    char* type_name;
    cJSON* type_obj = cJSON_GetObjectItem( obj, "type" );

    if( type_obj == NULL ) {
        type_name = "recipe";
    } else {
        type_name = cJSON_GetStringValue( type_obj );
    }

    char* search_name;

    if( strcmp( type_name, "recipe" ) == 0 ) {
        const Migration_data* migr_data = hashmap_get( migration_map->recipe_map, &(Migration_data){ .from = recipe_name } );
        if( migr_data ) {
            search_name = migr_data->to;
        } else {
            search_name = recipe_name;
        }
    }
    else if( strcmp( type_name, "item" ) == 0 ) {
        const Migration_data* migr_data = hashmap_get( migration_map->item_map, &(Migration_data){ .from = recipe_name } );
        if( migr_data ) {
            search_name = migr_data->to;
        } else {
            search_name = recipe_name;
        }
    }
    else if( strcmp( type_name, "fluid" ) == 0 ) {
        const Migration_data* migr_data = hashmap_get( migration_map->fluid_map, &(Migration_data){ .from = recipe_name } );
        if( migr_data ) {
            search_name = migr_data->to;
        } else {
            search_name = recipe_name;
        }
    }

    return search_name;
}
