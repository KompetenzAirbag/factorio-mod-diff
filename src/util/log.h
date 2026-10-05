#pragma once

#include "util.h"

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef ANSI_COLOR_RED
#ifdef NO_COLOR
#define ANSI_COLOR_RED
#define ANSI_COLOR_GREEN
#define ANSI_COLOR_YELLOW
#define ANSI_COLOR_BLUE
#define ANSI_COLOR_MAGENTA
#define ANSI_COLOR_CYAN
#define ANSI_COLOR_RESET
#else
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#endif
#endif

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* Logging library
   Provides useful logging tools such as DEBUG, WARN, VERBOSE, ERROR
   Defaults to stdout but can be redirected by #define "filename" BEFORE
   including this file
   Useful Makefile defines:
     DEBUG    enables  LOG_DEBUG
     VERBOSE  enables  LOG_VERBOSE
     NO_WARN  disables LOG_WARN
     NO_COLOR disables colorful logging */

#ifndef LOG_FILE
#define LOG_FILE stdout
#endif

#define AN_LOG_WALLCLOCK_CSTR_BUF_SZ (40ul)

#define CLOSE_STREAM( x ) \
    __builtin_choose_expr( \
        __builtin_types_compatible_p( typeof(x), FILE* ), \
        0, \
        1 \
    )


#define LOG_STREAM(x) \
    __builtin_choose_expr( \
        __builtin_types_compatible_p( typeof(x), FILE* ), \
        (FILE*)(x), \
        fopen( (char*)(x), "a+" ) \
    )

typedef enum {
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_COUNT
} LOG_LEVEL;

/* AN_WARN("%d is the loneliest number", 1) will print something like:
     [WARN] 09-02-2026 22:15:23.26 src/file.c(102): 1 is the loneliest number
*/
#define LOG_INFO( fmt, ... ) do { log_no_err( LOG_STREAM( LOG_FILE ), CLOSE_STREAM( LOG_FILE ), LOG_LEVEL_INFO, NULL, 0, fmt, ##__VA_ARGS__ ); } while( 0 )

#ifdef DEBUG
#define LOG_DEBUG( fmt, ... ) do { log_no_err( LOG_STREAM( LOG_FILE ), CLOSE_STREAM( LOG_FILE ), LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__ ); } while( 0 )
#else
#define LOG_DEBUG( fmt, ... ) do {} while( 0 )
#endif /* DEBUG */

#ifdef VERBOSE
#define LOG_VERBOSE( fmt, ... ) do { log_no_err( LOG_STREAM( LOG_FILE ), CLOSE_STREAM( LOG_FILE ), LOG_LEVEL_VERBOSE, "", 0, fmt, ##__VA_ARGS__ ); } while( 0 )
#else
#define LOG_VERBOSE( fmt, ... ) do {} while( 0 )
#endif /* VERBOSE */

#ifdef NO_WARN
#define LOG_WARN( fmt, ... ) do {} while( 0 )
#else
#ifdef DEBUG
#define LOG_WARN( fmt, ... ) do { log_no_err( LOG_STREAM( LOG_FILE ), CLOSE_STREAM( LOG_FILE ), LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__ ); fflush( stdout ); } while( 0 )
#else
#define LOG_WARN( fmt, ... ) do { log_no_err( LOG_STREAM( LOG_FILE ), CLOSE_STREAM( LOG_FILE ), LOG_LEVEL_WARN, NULL, 0, fmt, ##__VA_ARGS__ ); fflush( stdout ); } while( 0 )
#endif
#endif /* NO_WARN */

#define LOG_ERROR( fmt, ... ) do { log_err( LOG_STREAM( LOG_FILE ), CLOSE_STREAM( LOG_FILE ), __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__ ); } while( 0 )

/* log_wallclock_cstr will fill buf with the provided timestamp in the
   logging format */
void
log_wallclock_cstr( long now, char* buf );

/* log_no_err will log with any level without terminating the program */
void
log_no_err( FILE*       file_ptr,
            int         close_file,
            LOG_LEVEL   level,
            const char* file,
            int         line,
            const char* message_fmt,
            ... );

/* log_err will log an error and terminate the program */
void
log_err( FILE*       file_ptr,
         int         close_file,
         const char* file,
         int         line,
         const char* func,
         const char* message_fmt,
         ...
) __attribute__((noreturn)); /* Let compiler know this will not be returning */
