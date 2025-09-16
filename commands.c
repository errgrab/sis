#include <string.h>
#include <unistd.h>

static void cmd_nick(server_t *server, client_t *client, char *args) {
	if (!args || strlen(args) == 0) {
		send_reply(client->fd, ":server 431 :No nickname given\r\n");
		return;
	}
	
	if (client_find_nick(server, args)) {
		send_reply(client->fd, ":server 433 %s :Nickname is already in use\r\n", args);
		return;
	}
	
	strlcpy(client->nick, args, sizeof(client->nick));
	if (client->user[0] && !client->registered) {
		client->registered = true;
		send_reply(client->fd, ":server 001 %s :Welcome to the IRC Network\r\n", client->nick);
	}
}

static void cmd_user(server_t *server, client_t *client, char *args) {
	char *user, *mode, *unused, *real;
	
	if (!args) {
		send_reply(client->fd, ":server 461 USER :Not enough parameters\r\n");
		return;
	}
	
	user = args;
	args = skip(args, ' ');
	mode = args;
	args = skip(args, ' ');
	unused = args;
	args = skip(args, ' ');
	real = args;
	
	if (real && real[0] == ':')
		real++;
	
	strlcpy(client->user, user, sizeof(client->user));
	if (real)
		strlcpy(client->real, real, sizeof(client->real));
	
	if (client->nick[0] && !client->registered) {
		client->registered = true;
		send_reply(client->fd, ":server 001 %s :Welcome to the IRC Network\r\n", client->nick);
	}
}

static void cmd_ping(server_t *server, client_t *client, char *args) {
	send_reply(client->fd, ":server PONG server :%s\r\n", args ? args : "");
}

static void cmd_quit(server_t *server, client_t *client, char *args) {
	client_remove(server, client);
	client_free(client);
}

static void process_message(server_t *server, client_t *client, char *line) {
	char *cmd, *args;
	
	trim(line);
	if (strlen(line) == 0)
		return;
	
	cmd = line;
	args = skip(line, ' ');
	
	if (strcmp(cmd, "NICK") == 0)
		cmd_nick(server, client, args);
	else if (strcmp(cmd, "USER") == 0)
		cmd_user(server, client, args);
	else if (strcmp(cmd, "PING") == 0)
		cmd_ping(server, client, args);
	else if (strcmp(cmd, "QUIT") == 0)
		cmd_quit(server, client, args);
	else if (client->registered)
		send_reply(client->fd, ":server 421 %s %s :Unknown command\r\n", client->nick, cmd);
}

static void handle_client(server_t *server, client_t *client) {
	char buf[MAXMSG];
	ssize_t n;
	
	n = read(client->fd, buf, sizeof(buf) - 1);
	if (n <= 0) {
		client_remove(server, client);
		client_free(client);
		return;
	}
	
	buf[n] = '\0';
	char *line = buf;
	char *next;
	
	while ((next = strchr(line, '\n')) != NULL) {
		*next = '\0';
		if (next > line && *(next - 1) == '\r')
			*(next - 1) = '\0';
		
		process_message(server, client, line);
		line = next + 1;
	}
}