#pragma once

#include "factorio_instance.h"
#include "hashmap/hashmap.h"

typedef struct Migration_data
{
    char* from;
    char* to;
} Migration_data;

/* get_full_migration returns a migration_map where from is the key of the
   map (prototype name of instance 1) and to is the name after migration
   as in instance 2
   Must be freed using free_migration_map */
struct hashmap*
get_full_migration( Factorio_instance* inst_1, Factorio_instance* inst_2 );

void
free_migration_map( struct hashmap* migration_map );
