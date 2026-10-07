#include "string_helper.h"
#include "cJSON.h"
#include "util/util.h"
#include "hashmap/hashmap.h"
#include "util/dynamic_array.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

typedef struct Factorio_instance
{
    char* base_path;
    char* bin_path;
    char* config_path;
    char* mods_path;
} Factorio_instance;

typedef struct Runtime_flags
{
    char* instance_path_1;
    char* instance_path_2;
    FILE* output_file;
    int   should_close_output_file;
} Runtime_flags;

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
generate_data_json( Factorio_instance* inst )
{
    char* factorio_flags = "--dump-data > /dev/null 2>&1";

    run_factorio_instance( inst->bin_path, factorio_flags );

    char* data_dump_file_path = concat_strings( inst->config_path, "/../script-output/data-raw-dump.json" );
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
    free( data_dump_file_path );

    free( char_buf.items );

    return out;
}

char*
replace_path_variables( char* path, Factorio_instance* inst )
{
    char* home_dir = get_home_dir();
    char* out_str = malloc( strlen( path ) + 1 );
    strcpy( out_str, path );

    if( home_dir != NULL ) {
        out_str = replace_in_string( path, out_str, "~", home_dir );
    }

    if( inst == NULL ) {
        return out_str;
    }

    char* bin_path_no_exe = malloc( strlen( inst->bin_path ) + 1 );
    strcpy( bin_path_no_exe, inst->bin_path );
    bin_path_no_exe[ strlen( inst->bin_path ) - 9 ] = '\0';

    out_str = replace_in_string( path, out_str, "__PATH__executable__", bin_path_no_exe );

    free( bin_path_no_exe );
    return out_str;
}

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

void
set_config_path( Factorio_instance* inst )
{
    char* cfg_path_file_path = concat_strings( inst->base_path, "/config-path.cfg" );
    char* cfg_path_file = load_file( cfg_path_file_path );

    String_buf cfg_path_file_lines = split_string( cfg_path_file, '\n' );
    ENSURE_ERR( cfg_path_file_lines.count > 0, "%s is empty", cfg_path_file_path );

    String_buf cfg_path_var = split_string( cfg_path_file_lines.items[0].items, '=' );
    ENSURE_ERR( cfg_path_var.count >= 2, "Could not find variable config-path in %s", cfg_path_file_path );

    inst->config_path = replace_path_variables( cfg_path_var.items[1].items, inst );

    free( cfg_path_file_path );
    free( cfg_path_file );
    free_string_buf( &cfg_path_var );
    free_string_buf( &cfg_path_file_lines );
}

void
set_mod_path( Factorio_instance* inst )
{
    char* _config_ini_file_path = concat_strings( inst->config_path, "/config.ini" );
    char* config_ini_file_path = replace_path_variables( _config_ini_file_path, inst );
    free( _config_ini_file_path );

    char* config_ini_file = load_file( config_ini_file_path );

    String_buf str_buf = split_string( config_ini_file, '\n' );

    for( ulong i = 0; i < str_buf.count; i++ ) {
        String_buf write_data_buf = split_string( str_buf.items[i].items, '=' );

        if( write_data_buf.count < 2 ) {
            free_string_buf( &write_data_buf );
            continue;
        }

        if( strcmp( write_data_buf.items[0].items, "write-data" ) == 0 ) {
            char* mods_path = replace_path_variables( write_data_buf.items[1].items, inst );
            inst->mods_path = concat_strings( mods_path, "/mods" );
            free( mods_path );
        }

        free_string_buf( &write_data_buf );
    }

    ENSURE_NON_NULL( inst->mods_path, "Failed to load mods path" );

    free_string_buf( &str_buf );
    free( config_ini_file_path );
    free( config_ini_file );
}

String_buf
get_active_mods( Factorio_instance* inst )
{
    char* mod_list_path = concat_strings( inst->mods_path, "/mod-list.json" );
    char* mod_list_file = load_file( mod_list_path );

    cJSON* mod_list = cJSON_Parse( mod_list_file );
    ENSURE_NON_NULL( mod_list, "Failed to parse mod list from %s", mod_list_path );

    cJSON* mod_list_mods = cJSON_GetObjectItem( mod_list, "mods" );

    free( mod_list_file );
    free( mod_list_path );

    String_buf active_mods = {0};

    cJSON* mod_cfg;
    cJSON_ArrayForEach( mod_cfg, mod_list_mods ) {
        cJSON* enabled = cJSON_GetObjectItem( mod_cfg, "enabled" );
        ENSURE_NON_NULL( enabled, "Could not load mod enabled from mod list" );
        if( cJSON_IsFalse( enabled ) ) continue;

        cJSON* name = cJSON_GetObjectItem( mod_cfg, "name" );
        ENSURE_NON_NULL( name, "Could not load mod name from mod list" );

        char* mod_name = cJSON_GetStringValue( name );
        if( mod_name == NULL ) continue;

        /* Unsure if this is all needed or how cJSON_GetStringValue behaves */
        da_append( &active_mods, (Char_buf){0} );
        Char_buf* active_mods_entry = &active_mods.items[active_mods.count-1];
        active_mods_entry->count = strlen( mod_name ) + 1;

        active_mods_entry->items = realloc( active_mods_entry->items, active_mods_entry->count );
        strcpy( active_mods_entry->items, mod_name );
    }

    cJSON_Delete( mod_list );

    return active_mods;
}

void
get_full_migration( struct hashmap* migration_map, Factorio_instance* inst_1, Factorio_instance* inst_2 )
{
    String_buf active_mods_1 = get_active_mods( inst_1 );
    String_buf active_mods_2 = get_active_mods( inst_2 );

    for( ulong i = 0; i < active_mods_1.count; i++ ) {
        LOG_INFO( "%s", active_mods_1.items[i].items );
    }

    free_string_buf( &active_mods_1 );
    free_string_buf( &active_mods_2 );
}

void
free_factorio_instance( Factorio_instance* inst )
{
    free( inst->base_path );
    free( inst->bin_path );
    free( inst->config_path );
    free( inst->mods_path );
}

void
print_factorio_paths( Factorio_instance* inst )
{
    LOG_INFO( "Base path: %s\nbin path: %s\nconfig_path: %s\nmods_path: %s", inst->base_path, inst->bin_path, inst->config_path, inst->mods_path );
}

Factorio_instance
load_factorio_paths( char* path )
{
    Factorio_instance inst = {0};

    inst.base_path = replace_path_variables( path, NULL );
    inst.bin_path = concat_strings( inst.base_path, "/bin/x64/factorio" );

    set_config_path( &inst );
    set_mod_path( &inst );

    return inst;
}

typedef struct Migration_data
{
    char* from;
    char* to;
} Migration_data;

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
    return hashmap_sip( migr->from, strlen( migr->from ), seed0, seed1 );
}

int
main( int argc, char** argv )
{
    Runtime_flags r_flags = {0};
    parse_flags( argc, argv, &r_flags );

    ENSURE_NON_NULL( r_flags.instance_path_1, "Must provide Factorio instance path with -i. See -h for help." );

    Factorio_instance inst_1 = load_factorio_paths( r_flags.instance_path_1 );
    Factorio_instance inst_2 = load_factorio_paths( r_flags.instance_path_2 );

    struct hashmap* migration_map = hashmap_new( sizeof(Migration_data), 0, 0, 0, migration_hash, migration_compare, NULL, NULL );
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
