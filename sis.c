#include "arg.h"
#include "config.h"

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/select.h>

#undef strlcpy
#include "util.c"

typedef struct client_s client_t;
struct client_s {
	int fd;
	char nick[MAXNICK];
	char user[MAXNICK];
	char realname[MAXMSG];
	char host[128];
	bool registered;
	client_t *next;
};

typedef struct channel_s channel_t;
struct channel_s {
	char name[MAXCHAN];
	client_t *clients;
	channel_t *next;
};

typedef struct server_s server_t;
struct server_s {
	int fd;
	char *port;
	char *pass;
	bool online;
	client_t *clients;
	channel_t *channels;
};

static void irc_send(int fd, const char *fmt, ...) {
	char buf[MAXMSG];
	va_list ap;
	
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	
	if (write(fd, buf, strlen(buf)) == -1)
		perror("ERR: write failed");
}

static client_t *client_new(int fd) {
	client_t *client = malloc(sizeof(client_t));
	if (!client)
		fatal("ERR: malloc failed:");
	
	client->fd = fd;
	client->nick[0] = '\0';
	client->user[0] = '\0';
	client->realname[0] = '\0';
	client->host[0] = '\0';
	client->registered = false;
	client->next = NULL;
	
	return client;
}

static void client_free(client_t *client) {
	if (client) {
		close_socket(client->fd);
		free(client);
	}
}

static void client_add(server_t *server, client_t *client) {
	client->next = server->clients;
	server->clients = client;
}

static void client_remove(server_t *server, client_t *client) {
	client_t **c = &server->clients;
	while (*c) {
		if (*c == client) {
			*c = client->next;
			break;
		}
		c = &(*c)->next;
	}
}

static client_t *client_find_nick(server_t *server, const char *nick) {
	client_t *c = server->clients;
	while (c) {
		if (strcmp(c->nick, nick) == 0)
			return c;
		c = c->next;
	}
	return NULL;
}

static void handle_nick(server_t *server, client_t *client, char *args) {
	if (!args || strlen(args) == 0) {
		irc_send(client->fd, ":server 431 :No nickname given\r\n");
		return;
	}
	
	if (client_find_nick(server, args)) {
		irc_send(client->fd, ":server 433 %s :Nickname is already in use\r\n", args);
		return;
	}
	
	strlcpy(client->nick, args, sizeof(client->nick));
	if (client->user[0] && !client->registered) {
		client->registered = true;
		irc_send(client->fd, ":server 001 %s :Welcome to the IRC Network\r\n", client->nick);
	}
}

static void handle_user(server_t *server, client_t *client, char *args) {
	char *user, *mode, *unused, *realname;
	
	if (!args) {
		irc_send(client->fd, ":server 461 USER :Not enough parameters\r\n");
		return;
	}
	
	user = args;
	args = skip(args, ' ');
	mode = args;
	args = skip(args, ' ');
	unused = args;
	args = skip(args, ' ');
	realname = args;
	
	if (realname && realname[0] == ':')
		realname++;
	
	strlcpy(client->user, user, sizeof(client->user));
	if (realname)
		strlcpy(client->realname, realname, sizeof(client->realname));
	
	if (client->nick[0] && !client->registered) {
		client->registered = true;
		irc_send(client->fd, ":server 001 %s :Welcome to the IRC Network\r\n", client->nick);
	}
}

static void handle_ping(server_t *server, client_t *client, char *args) {
	irc_send(client->fd, ":server PONG server :%s\r\n", args ? args : "");
}

static void handle_quit(server_t *server, client_t *client, char *args) {
	client_remove(server, client);
	client_free(client);
}

static void handle_message(server_t *server, client_t *client, char *line) {
	char *cmd, *args;
	
	trim(line);
	if (strlen(line) == 0)
		return;
	
	cmd = line;
	args = skip(line, ' ');
	
	if (strcmp(cmd, "NICK") == 0)
		handle_nick(server, client, args);
	else if (strcmp(cmd, "USER") == 0)
		handle_user(server, client, args);
	else if (strcmp(cmd, "PING") == 0)
		handle_ping(server, client, args);
	else if (strcmp(cmd, "QUIT") == 0)
		handle_quit(server, client, args);
	else if (client->registered)
		irc_send(client->fd, ":server 421 %s %s :Unknown command\r\n", client->nick, cmd);
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
		
		handle_message(server, client, line);
		line = next + 1;
	}
}

void server_init(server_t *server) {
	server->port = DEFAULT_PORT;
	server->fd = -1;
	server->pass = DEFAULT_PASS;
	server->online = true;
	server->clients = NULL;
	server->channels = NULL;
}

void server_start(server_t *server) {
	fd_set readfds;
	int maxfd;
	client_t *client, *next;
	
	server->fd = listen_on(server->port);
	printf("IRC Server listening on port %s\n", server->port);
	
	while (server->online) {
		FD_ZERO(&readfds);
		FD_SET(server->fd, &readfds);
		maxfd = server->fd;
		
		// Add all client fds to select
		client = server->clients;
		while (client) {
			FD_SET(client->fd, &readfds);
			if (client->fd > maxfd)
				maxfd = client->fd;
			client = client->next;
		}
		
		if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			fatal("ERR: select failed:");
		}
		
		// Check for new connections
		if (FD_ISSET(server->fd, &readfds)) {
			int client_fd = accept_client(server->fd);
			client_add(server, client_new(client_fd));
			printf("New client connected (fd: %d)\n", client_fd);
		}
		
		// Handle existing clients
		client = server->clients;
		while (client) {
			next = client->next;
			if (FD_ISSET(client->fd, &readfds)) {
				handle_client(server, client);
			}
			client = next;
		}
	}
}

void server_deinit(server_t *server) {
	if (server->fd != -1)
		close_socket(server->fd);
}

int main(int ac, char **av) {
	arg_t args;
	server_t server;
	int opt;

	server_init(&server);
	arg_init(&args, ac, av);
	while ((opt = arg_next(&args, ac, av)) != -1) {
		switch (opt) {
			case 'h':
				printf("Usage: %s [-p port] [-k password] [-v]\n", args.av0);
				return 0;
			case 'k':
				server.pass = arg_value(&args, ac, av);
				if (!server.pass)
					return fprintf(stderr, "%s: password is empty!\n", args.av0), 1;
				break;
			case 'p':
				server.port = arg_value(&args, ac, av);
				if (!server.port)
					return fprintf(stderr, "%s: port is empty!", args.av0), 1;
				break;
			case 'v':
				printf("%s: version %s\n", args.av0, VERSION);
				return 0;
			default:
				fprintf(stderr, "%s: unknown option -%c\n", args.av0, opt);
				return 1;
		}
	}
	for (int i = args.i; i < ac; i++) {
		printf("UNUSED ARGUMENT: %s\n", av[i]);
	}

	server_start(&server);
	server_deinit(&server);
	return 0;
}
