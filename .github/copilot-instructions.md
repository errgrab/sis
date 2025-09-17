# GitHub Copilot Instructions for SIS (Simple IRC Server)

## Project Overview

This is a "suckless-styled" IRC server implemented in C following the principle of doing the simplest thing possible while saving lines of code. The project emphasizes minimal, clean, and readable code following the suckless philosophy.

## Architecture and Code Style

### Project Structure
- `sis.c` - Main IRC server implementation
- `util.c` - Utility functions for network operations (socket handling, address resolution, string parsing)
- `commands.c` - IRC command implementations (NICK, USER, JOIN, PART, PRIVMSG, etc.)
- `client.c` - Client connection management
- `channel.c` - Channel management functions
- `server.c` - Server lifecycle and main loop
- `arg.c` - Command-line argument parsing implementation
- `config.def.h` - Default configuration template
- `Makefile` - POSIX-compliant build system

### Coding Standards
- **Minimalism**: Write the least amount of code necessary to achieve functionality
- **Readability**: Code should be self-documenting and easy to understand
- **POSIX Compliance**: Use standard C99 and POSIX functions
- **No Dependencies**: Avoid external libraries; implement necessary functionality locally
- **Static Functions**: Use `static` for internal functions to limit scope
- **Error Handling**: Use simple, direct error handling with `perror()` and `fatal()`

### Code Conventions
- Use `snake_case` for function names and variables
- Use `typedef struct name_s name_t;` pattern for structures
- Include system headers first, then local headers
- Use `#include "file.c"` pattern for incorporating utility code directly
- Keep functions small and focused on a single responsibility
- Use descriptive variable names even if they're longer

### Build System
- Uses POSIX-compliant Makefile
- Supports standard targets: `all`, `clean`, `dist`
- Configuration through `config.h` (copied from `config.def.h`)
- Uses `c99` compiler with minimal flags: `-O1 -DVERSION="..." -D_GNU_SOURCE`

## Development Guidelines

### When Adding Features
1. Keep it minimal - implement only what's necessary
2. Follow existing code patterns and style
3. Avoid adding new dependencies or complex abstractions
4. Use existing utility functions (`eat()`, `token()`, `skip()`) for string parsing
5. Test with simple manual verification
6. Document any configuration options in `config.def.h`

### When Fixing Bugs
1. Make surgical changes - modify as little code as possible
2. Maintain compatibility with existing configuration
3. Use simple, direct solutions over complex ones
4. Preserve the existing function signatures and interfaces

### Network Programming Notes
- The server uses standard Berkeley sockets API
- IPv4/IPv6 dual-stack support via `getaddrinfo()`
- Simple single-threaded blocking I/O model
- Error handling uses `fatal()` for unrecoverable errors

### Testing and Validation
- Build with `make` to check compilation
- Test basic server startup and socket binding
- Verify configuration parsing with command-line options
- Use manual testing with IRC clients for functionality verification

## Common Patterns

### Error Handling
```c
static void fatal(const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    fprintf(stderr, "%s", buf);
    if(fmt[0] && fmt[strlen(fmt) - 1] == ':')
        fprintf(stderr, " %s\n", strerror(errno));
    exit(1);
}
```

### String Parsing Utilities
```c
/* Advance through characters matching predicate */
static char *eat(char *s, int (*p)(int), int r) {
    while(*s != '\0' && p((unsigned char)*s) == r)
        s++;
    return s;
}

/* Extract whitespace-delimited token and advance pointer */
static char *token(char **ps) {
    char *start = eat(*ps, isspace, 1);  /* skip leading spaces */
    char *end = eat(start, isspace, 0);  /* find end of token */
    if (*end) *end++ = '\0';
    *ps = eat(end, isspace, 1);          /* skip to next token */
    return start;
}

/* Skip to character and null-terminate */
static char *skip(char *s, char c) {
    while(*s != c && *s != '\0') s++;
    if(*s != '\0') *s++ = '\0';
    return s;
}
```

### IRC Command Parameter Parsing
```c
/* Parse IRC USER command: USER <user> <mode> <unused> :<real> */
user = token(&args);
mode = token(&args);
unused = token(&args);
real = token(&args);
real = (real[0] == ':') ? real + 1 : real;  /* strip leading ':' from real name */
```

### Structure Definition
```c
typedef struct server_s server_t;
struct server_s {
    int fd;
    char *pass;
    bool online;
};
```

### Socket Operations
```c
static int listen_on(char *port) {
    // Use getaddrinfo() for address resolution
    // Bind to all available interfaces
    // Return socket file descriptor
}
```

### Argument Parsing
```c
while ((opt = arg_next(&args, ac, av)) != -1) {
    switch (opt) {
        case 'p': /* handle port option */
        case 'k': /* handle password option */
        case 'v': /* handle version option */
    }
}
```

Remember: This is a suckless project - simplicity and clarity are more important than feature completeness or performance optimization.
