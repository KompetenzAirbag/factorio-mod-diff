#pragma once

#define DA_INIT_CAP (256UL)

/* da_reserve will reserve space for a dynamic array */
#define da_reserve( da, expected_capacity )                                              \
    do {                                                                                 \
        if( (expected_capacity) > (da)->capacity ) {                                     \
            if( (da)->capacity == 0 ) {                                                  \
                (da)->capacity = DA_INIT_CAP;                                            \
            }                                                                            \
            while( (expected_capacity) > (da)->capacity ) {                              \
                (da)->capacity *= 2;                                                     \
            }                                                                            \
            (da)->items = realloc( (da)->items, (da)->capacity * sizeof(*(da)->items) ); \
            assert( (da)->items != NULL && "Dynamic array out of memory" );              \
        }                                                                                \
    } while( 0 )

/* da_append appends an item to a dynamic array of structure:
     <type>* items;
     ulong   count;
     ulong   capacity; */
#define da_append( da, item )                \
    do {                                     \
        da_reserve( (da), (da)->count + 1 ); \
        (da)->items[(da)->count++] = (item); \
    } while( 0 )

/* da_insert will insert an item into a dynamic array at given index */
#define da_insert( da, item, index )                                                                \
    do {                                                                                            \
        assert( index <= (da)->count && "Insertion index cannot be greater than amount of items" ); \
        ulong _idx = (index);                                                                       \
                                                                                                    \
        da_reserve( (da), (da)->count + 1);                                                         \
        for( ulong _i = (da)->count; _i > _idx; _i-- ) {                                            \
            (da)->items[_i] = (da)->items[_i-1];                                                    \
        }                                                                                           \
                                                                                                    \
        (da)->items[_idx] = (item);                                                                 \
        (da)->count++;                                                                              \
    } while( 0 )
