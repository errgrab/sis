# sis
Simple IRC Server (inspired by sic suckless IRC Client)

A minimal, suckless-styled IRC server implementation.

## Features

- **Simple**: Minimal codebase (~300 lines of C)
- **Standards compliant**: Implements core IRC protocol (RFC 2812)
- **Multi-client support**: Handles multiple concurrent connections
- **Channel support**: Create and join channels for group communication
- **Private messaging**: Direct messages between users
- **No dependencies**: Pure C with standard library only

## Supported IRC Commands

- `NICK` - Set nickname
- `USER` - Set user information
- `JOIN` - Join a channel
- `PART` - Leave a channel
- `PRIVMSG` - Send messages to channels or users
- `QUIT` - Disconnect from server
- `PING/PONG` - Keep-alive mechanism

## Building

```bash
make
```

## Usage

Start the server:
```bash
./sis [port]
```

Default port is 6667 if not specified.

## Connecting

Connect with any IRC client:
```bash
# Using telnet for testing
telnet localhost 6667

# Using standard IRC clients
irssi -c localhost -p 6667
weechat
```

## Example Session

```
NICK myuser
USER myuser 0 * :My Real Name
JOIN #general
PRIVMSG #general :Hello everyone!
QUIT :Goodbye
```

## Configuration

Edit `config.h` and rebuild to customize:
- Maximum clients
- Maximum channels
- Message length limits
- Server name and version

## License

MIT License - see LICENSE file
