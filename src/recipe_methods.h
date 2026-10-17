#pragma once

#include "cJSON.h"
#include "migration.h"
#include "flags.h"

/* generate_recipe_diff generates a JSON with all the recipe differences
   found between the two instances */
cJSON*
generate_recipe_diff( Migration_map* migration_map, Factorio_instance* inst_1, Factorio_instance* inst_2, Runtime_flags* r_flags );
