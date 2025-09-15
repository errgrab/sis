# GitHub Copilot Instructions for SIS (Simple IRC Server)

## Project Overview

This is a "suckless-styled" IRC server implemented in C following the principle of doing the simplest thing possible while saving lines of code. The project emphasizes minimal, clean, and readable code following the suckless philosophy.

## Architecture and Code Style

### Project Structure
- `sis.c` - Main IRC server implementation
- `util.c` - Utility functions for network operations (socket handling, address resolution)
- `strlcpy.c` - Safe string copying implementation
- `arg.h` - Command-line argument parsing library
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
4. Test with simple manual verification
5. Document any configuration options in `config.def.h`

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
static void fatal(const char *msg) {
    perror(msg);
    exit(-1);
}
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
