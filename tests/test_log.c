#include "../src/util/log.h"
#include "../src/util/dynamic_array.h"
#include "../src/util/gen_types.h"

#include <string.h>

typedef struct Char_buf
{
    char* items;
    ulong capacity;
    ulong count;
} Char_buf;

typedef struct String_buf
{
    Char_buf* items;
    ulong capacity;
    ulong count;
} String_buf;

void
print_log()
{
    #undef LOG_FILE
    #define LOG_FILE "/tmp/unit_test_log"

    remove("/tmp/unit_test_log");

    int counter = 0;
    LOG_INFO(    "INFO, %i",    counter++ );
    LOG_DEBUG(   "DEBUG, %i",   counter++ );
    LOG_VERBOSE( "VERBOSE, %i", counter++ );
    LOG_WARN(    "WARN, %i",    counter++ );

    #undef LOG_FILE
    #define LOG_FILE stdout
}

void
compare_log_file( long timestamp )
{
    FILE* file = fopen("/tmp/unit_test_log", "r");
    ENSURE_NON_NULL( file, "Could not open log file");

    char ch;
    String_buf str_buf = {0};
    uint line_count = 0;

    /* Reading file */
    while( (ch = fgetc(file)) != EOF ) {
        if( line_count == str_buf.count ) da_append( &str_buf, (Char_buf){0});

        if( ch == '\n' ) {
            da_append( &str_buf.items[line_count], '\0' );
            line_count++;
            continue;
        }

        da_append( &str_buf.items[line_count], ch );
    }

    fclose( file );

    char now_cstr[AN_LOG_WALLCLOCK_CSTR_BUF_SZ];
    log_wallclock_cstr( timestamp, now_cstr );

    static const char* const prefixes[] = {
        ANSI_COLOR_GREEN  "[INFO]   " ANSI_COLOR_RESET,
        ANSI_COLOR_CYAN   "[DEBUG]  " ANSI_COLOR_RESET,
        ANSI_COLOR_BLUE   "[VERBOSE]" ANSI_COLOR_RESET,
        ANSI_COLOR_YELLOW "[WARN]   " ANSI_COLOR_RESET,
        ANSI_COLOR_RED    "[ERROR]  " ANSI_COLOR_RESET
    };

    char expected_line[128];

    /* INFO */
    sprintf( expected_line,
             "%s %s: INFO, %i",
             prefixes[LOG_LEVEL_INFO],
             now_cstr,
             LOG_LEVEL_INFO );
    ENSURE_ERR( strcmp( str_buf.items[0].items, expected_line ) == 0,
                "LOG_INFO does not produce the expected output:\nExpected: %s\nFound: %s",
                expected_line,
                str_buf.items[0].items );

    /* DEBUG */
    sprintf( expected_line,
             "%s %s "ANSI_COLOR_GREEN"test_log.c(31): "ANSI_COLOR_RESET"DEBUG, %i",
             prefixes[LOG_LEVEL_DEBUG],
             now_cstr,
             LOG_LEVEL_DEBUG );
    ENSURE_ERR( strcmp( str_buf.items[1].items, expected_line ) == 0,
                "LOG_DEBUG does not produce the expected output:\nExpected: %s\nFound: %s",
                expected_line,
                str_buf.items[1].items );

    /* VERBOSE */
    sprintf( expected_line, "%s %s: VERBOSE, %i",
             prefixes[LOG_LEVEL_VERBOSE],
             now_cstr,
             LOG_LEVEL_VERBOSE );
    ENSURE_ERR( strcmp( str_buf.items[2].items, expected_line ) == 0,
                "LOG_VERBOSE does not produce the expected output:\nExpected: %s\nFound: %s",
                expected_line,
                str_buf.items[2].items );

    /* WARN */
    sprintf( expected_line,
             "%s %s "ANSI_COLOR_GREEN"test_log.c(33): "ANSI_COLOR_RESET"WARN, %i",
             prefixes[LOG_LEVEL_WARN],
             now_cstr,
             LOG_LEVEL_WARN );
    ENSURE_ERR( strcmp( str_buf.items[3].items, expected_line ) == 0,
                "LOG_WARN does not produce the expected output:\nExpected: %s\nFound: %s",
                expected_line,
                str_buf.items[3].items );

    /* Freeing all the dynamic array allocations */
    for( uint i = 0; i < str_buf.count; i++ ) {
        free( str_buf.items[i].items );
    }

    free( str_buf.items );
}

int
main()
{
    LOG_INFO( "Starting unit test for logger" );

    print_log();
    compare_log_file( get_millis() );

    LOG_INFO( "Finished unit test for logger (tmp file generated at /tmp/unit_test_log)" );

    return 0;
}
