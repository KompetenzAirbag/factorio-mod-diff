#pragma once

#include "factorio_instance.h"
#include "hashmap/hashmap.h"

typedef struct Migration_data
{
    char* from;
    char* to;
} Migration_data;

/* get_full_migration fills the migration_map where from is the key of the
   map (prototype name of instance 1) and to is the name after migration
   as in instance 2 */
void
get_full_migration( struct hashmap* migration_map, Factorio_instance* inst_1, Factorio_instance* inst_2 );

/* new_migration_map generates an empty migration map which must be freed
   using free_migration_map */
struct hashmap*
new_migration_map();

void
free_migration_map( struct hashmap* migration_map );
