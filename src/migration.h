#pragma once

#include "factorio_instance.h"
#include "hashmap/hashmap.h"

typedef struct Migration_data
{
    char* from;
    char* to;
} Migration_data;

void
get_full_migration( struct hashmap* migration_map, Factorio_instance* inst_1, Factorio_instance* inst_2 );

struct hashmap*
new_migration_map();

void
free_migration_map( struct hashmap* migration_map );
