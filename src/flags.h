#pragma once

#include <stdio.h>

typedef struct Runtime_flags
{
    char* instance_path_1;
    char* instance_path_2;

    char* output_path;
    FILE* output_file;
    int   should_close_output_file;

    int   use_expensive_recipe;
} Runtime_flags;

void
parse_flags( int argc, char** argv, Runtime_flags* r_flags );
