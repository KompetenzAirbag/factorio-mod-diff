#include "migration.h"
#include "util/gen_types.h"
#include "string_helper.h"
#include "util/log.h"

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
    return hashmap_sip( migr->from, strlen( migr->from ), seed0, seed1 );
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

struct hashmap*
new_migration_map()
{
    return hashmap_new( sizeof(Migration_data), 0, 0, 0, migration_hash, migration_compare, NULL, NULL );
}
