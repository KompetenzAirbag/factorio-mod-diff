# 1. Alignment
## 1.1 Methods
- Modifiers and return types on separate lines
- One function argument per line
- Vertically align function arguments and type names
- Opening bracket on a separate line
```c
static inline int
foo ( int  some_int
      long some_long )
{
    ...
}
```

## 1.2 General alignment
For function calls, spaces inside brackets. No space before brackets:
```c
printf( "Hello ", "World" );

/* Exception: no arguments */
abort();

/* Exception: sizeof() */
printf( "Size: %lu", sizeof(long) );

/* Exception: double bracket macros */
LOG_NOTICE(( "pass" ));
```

For single-line if statements, no braces are required:
```c
if( do_crash ) abort();

/* Braces required for line breaks */
if ( do_crash ) {
    abort();
}
```

## 1.3 Vertical alignment
Align code vertically where blocks semantically belong together
```c
#define FOO_SUCCESS    (0)
#define FOO_ERR_PROTO  (1)
#define FOO_ERR_IO     (20)

#define OTHER (0)

void
foo( void )
{
    char const* _init = some_function( &argc, &argv, "string" );
    uint        seed  = other_function( &argc, &argv );
}
```

# 2. Types
- Use the types provided in `src/gen_types.h`
- Use `u8`, `u16`, ... where size and alignment is crucial to clarify your intend
- Use `int` instead of `bool` (stdbool) where `1` is `true` and `0` is `false`
- Prefer `uint` or `ulong` over `size_t`

|   stdint    |  gen_types  |
| ----------- | ----------- |
| int8_t      | i8          |
| int16_t     | i16         |
| int32_t     | i32         |
| int64_t     | i64         |
| uint8_t     | u8          |
| uint16_t    | u16         |
| uint32_t    | u32         |
| uint64_t    | u64         |

# 3. Documentation and comments
- Use comment blocks `/*  */`
- Documentation for a function before the function prototype
- Mention the name of the function toward the beginning of the comment
- Public function declarations must be documented (implementations of these functions must not repeat the comment)
- Private functions can, but do not have to be documented
- Avoid unnecessary comments such as `if (true) /* If condition is met */`
- Add meaningful changes to `CHANGELOG.md` under `Features` or `Bugfixes`

```c
/* sign_bytes signs bytes with the provided private_key using ed25519.
   The generated signature is then written into the provided signature buffer */
int
sign_bytes( uchar        signature[64],
            const uchar  private_key[64],
            const uchar* byte_ptr,
            const ulong  byte_len );
```

# 4. AI generated code
AI generated code shall **only** be included in non-production environments for debugging purposes (e.g. pretty-printing complex structures). Such sections must be marked. If you used generative AI to generate code you must fully understand every single line of it.
```c
/* _!_ AI code <reason> _!_ */
...
/* _!_ End of AI code _!_ */
```
