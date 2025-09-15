# sis
Simple IRC Server (inspired by sic suckless IRC Client)

A minimal IRC server implementation following suckless principles.

## Features

- **RFC 1459 IRC Protocol Support**: Basic IRC command handling
- **Multi-client Support**: Handles multiple concurrent connections using select()
- **Core IRC Commands**:
  - `NICK` - Set nickname with collision detection
  - `USER` - User registration
  - `PING/PONG` - Keep-alive functionality  
  - `QUIT` - Clean disconnection
- **Proper Registration Flow**: Requires both NICK and USER before allowing IRC usage
- **Standard Error Responses**: IRC-compliant error codes

## Usage

```bash
make
./sis [-p port] [-k password] [-v]
```

### Options

- `-p <port>` - Listen port (default: 6667)
- `-k <password>` - Server password (not yet implemented)
- `-v` - Show version and exit

### Examples

```bash
# Start server on default port 6667
./sis

# Start server on port 8080
./sis -p 8080

# Show version
./sis -v
```

## Testing

You can test the server with any IRC client or using netcat:

```bash
# Using netcat
echo -e "NICK testuser\r\nUSER test 0 * :Test User\r\nPING test\r\nQUIT\r\n" | nc localhost 6667

# Expected response:
# :server 001 testuser :Welcome to the IRC Network
# :server PONG server :test
```

## Architecture

The server follows suckless design principles:
- Minimal dependencies (standard C library only)
- Single-threaded design with select() multiplexing
- Simple data structures (linked lists)
- Clear, readable code structure
- Static buffers to avoid malloc complexity

## License

See LICENSE file.
