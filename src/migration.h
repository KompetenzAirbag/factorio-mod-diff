#pragma once

#include "factorio_instance.h"
#include "hashmap/hashmap.h"
#include "cJSON.h"

typedef struct Migration_data
{
    char* from;
    char* to;
} Migration_data;

typedef struct Migration_map
{
    struct hashmap* recipe_map;
    struct hashmap* fluid_map;
    struct hashmap* item_map;
} Migration_map;

/* get_full_migration returns a migration_map where from is the key of the
   map (prototype name of instance 1) and to is the name after migration
   as in instance 2
   Must be freed using free_migration_map */
Migration_map
get_full_migration( Factorio_instance* inst_1, Factorio_instance* inst_2 );

void
free_migration_map( Migration_map* migration_map );

/* get_migrated_name applies the migration to the name. The returned
   pointer must NOT be freed. */
char*
get_migrated_name( Migration_map* migration_map, cJSON* obj );
