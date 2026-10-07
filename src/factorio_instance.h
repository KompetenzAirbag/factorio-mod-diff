#pragma once

#include "cJSON.h"
#include "string_helper.h"

typedef struct Factorio_instance
{
    char* base_path;
    char* bin_path;
    char* config_path;
    char* mods_path;
} Factorio_instance;

void
run_factorio_instance( char* path, char* args );

cJSON*
generate_data_json( Factorio_instance* inst );

char*
replace_path_variables( char* path, Factorio_instance* inst );

String_buf
get_active_mods( Factorio_instance* inst );

void
free_factorio_instance( Factorio_instance* inst );

void
print_factorio_paths( Factorio_instance* inst );

Factorio_instance
load_factorio_instance( char* path );
