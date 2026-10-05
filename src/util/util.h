#pragma once

#include "log.h"

#include <stddef.h>

/* USE WITH CAUTION */
#define LIKELY( cond )   __builtin_expect( !!(cond), 1L )
#define UNLIKELY( cond ) __builtin_expect( !!(cond), 0L )

/* ENSURE works similar to assert but uses the logging system. You can also
   evaluate ENSURE which returns 0 if the condition is not met */
/* ENSURE_ERR works the same as assert and uses the logging system */
/* ENSURE_NON_NULL is a wrapper for ENSURE_ERR for checking nullptr */
#define ENSURE(cond, message, ...)                \
    ({                                            \
        int _result = !!(cond);                   \
        if( UNLIKELY( !_result ) ) {              \
            LOG_WARN( (message), ##__VA_ARGS__ ); \
        }                                         \
        _result;                                  \
    })
#define ENSURE_ERR( cond, message, ... ) if( UNLIKELY( !(cond) ) ) { LOG_ERROR( (message), ##__VA_ARGS__ ); }
#define ENSURE_NON_NULL( ptr, message, ... ) ENSURE_ERR( ptr, message, ##__VA_ARGS__ )

/* ATTRIBUTES */
#define __check_return __attribute__((__warn_unused_result__))

/* get_millis gets the time in milliseconds */
long
get_millis();
