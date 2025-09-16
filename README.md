# sis
Simple IRC Server (inspired by sic suckless IRC Client)

A minimal IRC server implementation following suckless principles with full core IRC functionality.

## Quick Start

```bash
# 1. Build the server
make config.h
make

# 2. Start server
./sis

# 3. Connect with sic client (in another terminal)
sic -h localhost -p 6667 -n alice

# 4. Join a channel and chat
/j #test
/p #test Hello everyone!
```

## Features

- **RFC 1459 IRC Protocol Support**: Complete basic IRC command handling
- **Multi-client Support**: Handles multiple concurrent connections using poll()
- **Channel Management**: Dynamic channel creation and management
- **Password Protection**: Server-wide password authentication
- **Core IRC Commands**:
  - `PASS` - Server password authentication
  - `NICK` - Set nickname with collision detection
  - `USER` - User registration
  - `JOIN` - Join/create channels with automatic cleanup
  - `PRIVMSG` - Private messages and channel communication
  - `PING/PONG` - Keep-alive functionality  
  - `QUIT` - Clean disconnection with channel cleanup
- **Proper Registration Flow**: Requires PASS (if set), NICK and USER before allowing IRC usage
- **Channel Features**:
  - Automatic channel creation (channels starting with #)
  - User lists and NAMES command support
  - Message broadcasting to all channel members
  - Automatic channel cleanup when empty
- **Standard Error Responses**: IRC-compliant error codes and messages

## Compilation

### Requirements
- C99 compatible compiler (`c99` or `gcc`)
- POSIX-compliant system
- Standard C library

### 1. Generate Configuration
```bash
make config.h
```
This copies `config.def.h` to `config.h` where you can customize server settings like:
- `SERVER_NAME` - Server identity (default: "sis")
- `SERVER_VERSION` - Version string (default: "0.42")  
- `MAXCLIENTS` - Max concurrent connections (default: 1024)
- `DEFAULT_PORT` - Default listening port (default: "6667")

### 2. Build
```bash
make
```

### 3. Clean Build (if needed)
```bash
make clean && make
```

## Usage

```bash
make
./sis [-p port] [-k password] [-v]
```

### Options

- `-p <port>` - Listen port (default: 6667)
- `-k <password>` - Server password (optional)
- `-v` - Show version and exit

### Examples

```bash
# Start server on default port 6667
./sis

# Start server on port 8080
./sis -p 8080

# Start server with password protection
./sis -k mypassword

# Show version
./sis -v
```

## Testing

You can test the server with any IRC client or using netcat:

### Using sic (suckless IRC client)

First, install and configure sic:
```bash
# Clone and build sic
git clone https://git.suckless.org/sic
cd sic
make config.h
make
sudo make install
```

Connect to sis server:
```bash
# Connect without password
sic -h localhost -p 6667 -n yournick

# Connect with password
sic -h localhost -p 6667 -n yournick -k mypassword
```

### sic Commands
Once connected with sic, you can use these commands:
```
/j #channel          - Join a channel
/p nick message      - Send private message
/p #channel message  - Send channel message
/l                   - Leave current channel
/q                   - Quit
```

### Using netcat for Testing

### Basic Connection (No Password)
```bash
echo -e "NICK testuser\r\nUSER test 0 * :Test User\r\nPING test\r\n" | nc localhost 6667
```

### With Password Protection
```bash
echo -e "PASS mypassword\r\nNICK testuser\r\nUSER test 0 * :Test User\r\n" | nc localhost 6667
```

### Channel Operations
```bash
echo -e "NICK alice\r\nUSER alice 0 * :Alice\r\nJOIN #test\r\nPRIVMSG #test :Hello everyone!\r\n" | nc localhost 6667
```

### Private Messaging
```bash
echo -e "NICK bob\r\nUSER bob 0 * :Bob\r\nPRIVMSG alice :Hello Alice!\r\n" | nc localhost 6667
```

### Using Other IRC Clients

The server works with any standard IRC client:

```bash
# irssi
irssi -c localhost -p 6667 -w mypassword -n yournick

# weechat
/server add sis localhost/6667 -notls -password=mypassword -username=yournick

# hexchat
# Use GUI to connect to localhost:6667 with password if needed
```

## Expected Responses

### Welcome Message
```
:sis 001 testuser :Welcome to the IRC Network
```

### Channel Join
```
:alice!alice@localhost JOIN #test
:sis 353 alice = #test :alice 
:sis 366 alice #test :End of /NAMES list
```

### Password Error
```
:sis 464 :Password incorrect
```

## Architecture

The server follows suckless design principles:
- **Minimal dependencies**: Standard C library only
- **Header-free design**: Direct .c file inclusion following suckless philosophy
- **Single-threaded design**: poll() multiplexing for efficient I/O
- **Simple data structures**: Linked lists for clients and channels
- **Singleton pattern**: Centralized server state management
- **Static configuration**: Compile-time configuration via config.h
- **Modular structure**:
  - `sis.c` - Main entry point and signal handling
  - `client.c` - Client connection management
  - `channel.c` - Channel operations and membership
  - `commands.c` - IRC protocol command processing
  - `server.c` - Network server lifecycle
  - `util.c` - Utility functions and network helpers
  - `arg.c` - Command-line argument parsing

### Network Design
- Uses `poll()` instead of `select()` for better scalability
- Constrained to basic socket functions: `socket()`, `bind()`, `listen()`, `accept()`, `send()`, `recv()`, `close()`
- Memory allocation via `calloc()` for zero-initialization
- Graceful shutdown on SIGINT/SIGTERM/SIGQUIT signals

## Configuration

Edit `config.h` to customize:
- `SERVER_NAME` - Server identity in IRC responses
- `SERVER_VERSION` - Version string
- `MAXCLIENTS` - Maximum concurrent connections
- `MAXMSG` - Message buffer size
- `MAXNICK` - Maximum nickname length
- `MAXCHAN` - Maximum channel name length

## IRC Protocol Compliance

Implements core IRC functionality:
- User registration and authentication
- Channel management with automatic creation/cleanup
- Private and channel messaging
- Standard IRC error codes and responses
- Proper nick!user@host formatting
- NAMES list support for channels

## License

See LICENSE file.
