#include "util.h"

#include <time.h>

long
get_millis()
{
    struct timespec now;
    timespec_get( &now, TIME_UTC );
    return ((long) now.tv_sec) * 1000 + ((long) now.tv_nsec) / 1.0e6;
}

