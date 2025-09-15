#include "arg.h"
#include "config.h"

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>

#undef strlcpy
#include "strlcpy.c"
#include "util.c"

#define MAX_CLIENTS 64
#define MAX_CHANNELS 16
#define MAX_MSG_LEN 512
#define MAX_NICK_LEN 32
#define MAX_CHAN_LEN 32

static volatile bool running = true;

static void sighandler(int sig) {
	running = false;
}

typedef struct client_s client_t;
typedef struct channel_s channel_t;

struct client_s {
	int fd;
	char nick[MAX_NICK_LEN];
	char user[MAX_NICK_LEN];
	char host[MAX_NICK_LEN];
	bool registered;
	channel_t *channels[MAX_CHANNELS];
	char buffer[MAX_MSG_LEN];
	int buffer_pos;
};

struct channel_s {
	char name[MAX_CHAN_LEN];
	client_t *clients[MAX_CLIENTS];
	int client_count;
	bool active;
};

typedef struct server_s server_t;
struct server_s {
	int fd;
	char *port;
	char *pass;
	bool online;
	client_t clients[MAX_CLIENTS];
	channel_t channels[MAX_CHANNELS];
	fd_set readfds;
	int max_fd;
};

static void client_init(client_t *client, int fd) {
	client->fd = fd;
	client->nick[0] = '\0';
	client->user[0] = '\0';
	client->host[0] = '\0';
	client->registered = false;
	client->buffer[0] = '\0';
	client->buffer_pos = 0;
	for (int i = 0; i < MAX_CHANNELS; i++)
		client->channels[i] = NULL;
}

static void client_send(client_t *client, const char *msg) {
	if (client->fd != -1) {
		send(client->fd, msg, strlen(msg), 0);
		send(client->fd, "\r\n", 2, 0);
	}
}

static void client_sendf(client_t *client, const char *fmt, ...) {
	char buffer[MAX_MSG_LEN];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);
	client_send(client, buffer);
}

static channel_t *server_find_channel(server_t *server, const char *name) {
	for (int i = 0; i < MAX_CHANNELS; i++) {
		if (server->channels[i].active && strcmp(server->channels[i].name, name) == 0)
			return &server->channels[i];
	}
	return NULL;
}

static channel_t *server_create_channel(server_t *server, const char *name) {
	for (int i = 0; i < MAX_CHANNELS; i++) {
		if (!server->channels[i].active) {
			strlcpy(server->channels[i].name, name, sizeof(server->channels[i].name));
			server->channels[i].client_count = 0;
			server->channels[i].active = true;
			return &server->channels[i];
		}
	}
	return NULL;
}

static void channel_add_client(channel_t *channel, client_t *client) {
	if (channel->client_count < MAX_CLIENTS) {
		channel->clients[channel->client_count++] = client;
		for (int i = 0; i < MAX_CHANNELS; i++) {
			if (client->channels[i] == NULL) {
				client->channels[i] = channel;
				break;
			}
		}
	}
}

static void channel_remove_client(channel_t *channel, client_t *client) {
	for (int i = 0; i < channel->client_count; i++) {
		if (channel->clients[i] == client) {
			for (int j = i; j < channel->client_count - 1; j++)
				channel->clients[j] = channel->clients[j + 1];
			channel->client_count--;
			break;
		}
	}
	for (int i = 0; i < MAX_CHANNELS; i++) {
		if (client->channels[i] == channel) {
			client->channels[i] = NULL;
			break;
		}
	}
	if (channel->client_count == 0)
		channel->active = false;
}

static void channel_broadcast(channel_t *channel, client_t *sender, const char *msg) {
	for (int i = 0; i < channel->client_count; i++) {
		if (channel->clients[i] != sender)
			client_send(channel->clients[i], msg);
	}
}

static void handle_nick(server_t *server, client_t *client, char *args) {
	char *nick = strtok(args, " ");
	if (!nick) {
		client_send(client, ":server 431 * :No nickname given");
		return;
	}
	
	// Check if nick is already in use
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (server->clients[i].fd != -1 && strcmp(server->clients[i].nick, nick) == 0) {
			client_sendf(client, ":server 433 * %s :Nickname is already in use", nick);
			return;
		}
	}
	
	strlcpy(client->nick, nick, sizeof(client->nick));
	if (client->user[0] != '\0' && !client->registered) {
		client->registered = true;
		client_sendf(client, ":server 001 %s :Welcome to the Internet Relay Network %s", 
			client->nick, client->nick);
	}
}

static void handle_user(server_t *server, client_t *client, char *args) {
	char *user = strtok(args, " ");
	if (!user) {
		client_send(client, ":server 461 USER :Not enough parameters");
		return;
	}
	
	strlcpy(client->user, user, sizeof(client->user));
	strlcpy(client->host, "localhost", sizeof(client->host));
	
	if (client->nick[0] != '\0' && !client->registered) {
		client->registered = true;
		client_sendf(client, ":server 001 %s :Welcome to the Internet Relay Network %s", 
			client->nick, client->nick);
	}
}

static void handle_join(server_t *server, client_t *client, char *args) {
	if (!client->registered) {
		client_send(client, ":server 451 * :You have not registered");
		return;
	}
	
	char *chan_name = strtok(args, " ");
	if (!chan_name || chan_name[0] != '#') {
		client_send(client, ":server 403 * :No such channel");
		return;
	}
	
	channel_t *channel = server_find_channel(server, chan_name);
	if (!channel)
		channel = server_create_channel(server, chan_name);
	
	if (!channel) {
		client_send(client, ":server 471 * :Channel is full");
		return;
	}
	
	channel_add_client(channel, client);
	
	// Send JOIN message to channel
	char join_msg[MAX_MSG_LEN];
	snprintf(join_msg, sizeof(join_msg), ":%s!%s@%s JOIN %s", 
		client->nick, client->user, client->host, chan_name);
	client_send(client, join_msg);
	channel_broadcast(channel, client, join_msg);
	
	// Send topic (none for now)
	client_sendf(client, ":server 331 %s %s :No topic is set", client->nick, chan_name);
	
	// Send names list
	char names[MAX_MSG_LEN] = "";
	for (int i = 0; i < channel->client_count; i++) {
		if (i > 0) strlcat(names, " ", sizeof(names));
		strlcat(names, channel->clients[i]->nick, sizeof(names));
	}
	client_sendf(client, ":server 353 %s = %s :%s", client->nick, chan_name, names);
	client_sendf(client, ":server 366 %s %s :End of NAMES list", client->nick, chan_name);
}

static void handle_part(server_t *server, client_t *client, char *args) {
	if (!client->registered) {
		client_send(client, ":server 451 * :You have not registered");
		return;
	}
	
	char *chan_name = strtok(args, " ");
	if (!chan_name) {
		client_send(client, ":server 461 PART :Not enough parameters");
		return;
	}
	
	channel_t *channel = server_find_channel(server, chan_name);
	if (!channel) {
		client_sendf(client, ":server 403 %s :No such channel", chan_name);
		return;
	}
	
	char part_msg[MAX_MSG_LEN];
	snprintf(part_msg, sizeof(part_msg), ":%s!%s@%s PART %s", 
		client->nick, client->user, client->host, chan_name);
	client_send(client, part_msg);
	channel_broadcast(channel, client, part_msg);
	
	channel_remove_client(channel, client);
}

static void handle_privmsg(server_t *server, client_t *client, char *args) {
	if (!client->registered) {
		client_send(client, ":server 451 * :You have not registered");
		return;
	}
	
	char *target = strtok(args, " ");
	char *message = strtok(NULL, "");
	
	if (!target || !message) {
		client_send(client, ":server 461 PRIVMSG :Not enough parameters");
		return;
	}
	
	if (message[0] == ':') message++; // Remove leading colon
	
	if (target[0] == '#') {
		// Channel message
		channel_t *channel = server_find_channel(server, target);
		if (!channel) {
			client_sendf(client, ":server 403 %s :No such channel", target);
			return;
		}
		
		char msg[MAX_MSG_LEN];
		snprintf(msg, sizeof(msg), ":%s!%s@%s PRIVMSG %s :%s", 
			client->nick, client->user, client->host, target, message);
		channel_broadcast(channel, client, msg);
	} else {
		// Private message
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (server->clients[i].fd != -1 && strcmp(server->clients[i].nick, target) == 0) {
				char msg[MAX_MSG_LEN];
				snprintf(msg, sizeof(msg), ":%s!%s@%s PRIVMSG %s :%s", 
					client->nick, client->user, client->host, target, message);
				client_send(&server->clients[i], msg);
				return;
			}
		}
		client_sendf(client, ":server 401 %s :No such nick", target);
	}
}

static void handle_quit(server_t *server, client_t *client, char *args) {
	char *reason = args;
	if (!reason || strlen(reason) == 0)
		reason = "Client Quit";
	
	if (reason[0] == ':') reason++; // Remove leading colon
	
	char quit_msg[MAX_MSG_LEN];
	snprintf(quit_msg, sizeof(quit_msg), ":%s!%s@%s QUIT :%s", 
		client->nick, client->user, client->host, reason);
	
	// Send QUIT to all channels the client is in
	for (int i = 0; i < MAX_CHANNELS; i++) {
		if (client->channels[i]) {
			channel_broadcast(client->channels[i], client, quit_msg);
			channel_remove_client(client->channels[i], client);
		}
	}
	
	close_socket(client->fd);
	client->fd = -1;
}

static void process_message(server_t *server, client_t *client, char *line) {
	trim(line);
	if (strlen(line) == 0) return;
	
	char *cmd = strtok(line, " ");
	char *args = strtok(NULL, "");
	
	if (!cmd) return;
	
	if (strcmp(cmd, "NICK") == 0) {
		handle_nick(server, client, args);
	} else if (strcmp(cmd, "USER") == 0) {
		handle_user(server, client, args);
	} else if (strcmp(cmd, "JOIN") == 0) {
		handle_join(server, client, args);
	} else if (strcmp(cmd, "PART") == 0) {
		handle_part(server, client, args);
	} else if (strcmp(cmd, "PRIVMSG") == 0) {
		handle_privmsg(server, client, args);
	} else if (strcmp(cmd, "QUIT") == 0) {
		handle_quit(server, client, args);
	} else if (strcmp(cmd, "PING") == 0) {
		client_sendf(client, ":server PONG server :%s", args ? args : "");
	} else {
		client_sendf(client, ":server 421 %s :Unknown command", cmd);
	}
}

static void handle_client_data(server_t *server, client_t *client) {
	char buffer[MAX_MSG_LEN];
	ssize_t bytes = recv(client->fd, buffer, sizeof(buffer) - 1, 0);
	
	if (bytes <= 0) {
		handle_quit(server, client, "Connection lost");
		return;
	}
	
	buffer[bytes] = '\0';
	
	for (int i = 0; i < bytes; i++) {
		char c = buffer[i];
		if (c == '\r' || c == '\n') {
			if (client->buffer_pos > 0) {
				client->buffer[client->buffer_pos] = '\0';
				process_message(server, client, client->buffer);
				client->buffer_pos = 0;
			}
		} else if (client->buffer_pos < MAX_MSG_LEN - 1) {
			client->buffer[client->buffer_pos++] = c;
		}
	}
}

void server_init(server_t *server) {
	server->port = "6667";
	server->fd = -1;
	server->pass = NULL;
	server->online = true;
	server->max_fd = 0;
	
	for (int i = 0; i < MAX_CLIENTS; i++) {
		server->clients[i].fd = -1;
	}
	
	for (int i = 0; i < MAX_CHANNELS; i++) {
		server->channels[i].active = false;
		server->channels[i].client_count = 0;
	}
	
	FD_ZERO(&server->readfds);
}

void server_start(server_t *server) {
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	
	server->fd = listen_on(server->port);
	printf("IRC server listening on port %s\n", server->port);
	
	while (server->online && running) {
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(server->fd, &readfds);
		int max_fd = server->fd;
		
		// Add client sockets to fd_set
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (server->clients[i].fd != -1) {
				FD_SET(server->clients[i].fd, &readfds);
				if (server->clients[i].fd > max_fd)
					max_fd = server->clients[i].fd;
			}
		}
		
		struct timeval timeout = {1, 0}; // 1 second timeout
		int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
		
		if (activity < 0) {
			perror("select error");
			break;
		}
		
		// Check for new connections
		if (FD_ISSET(server->fd, &readfds)) {
			int client_fd = accept_client(server->fd);
			
			// Find empty client slot
			bool added = false;
			for (int i = 0; i < MAX_CLIENTS; i++) {
				if (server->clients[i].fd == -1) {
					client_init(&server->clients[i], client_fd);
					printf("New client connected (fd=%d, slot=%d)\n", client_fd, i);
					added = true;
					break;
				}
			}
			
			if (!added) {
				printf("Server full, rejecting connection\n");
				close_socket(client_fd);
			}
		}
		
		// Check for client data
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (server->clients[i].fd != -1 && FD_ISSET(server->clients[i].fd, &readfds)) {
				handle_client_data(server, &server->clients[i]);
			}
		}
	}
}

void server_deinit(server_t *server) {
	printf("Shutting down server...\n");
	
	// Disconnect all clients
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (server->clients[i].fd != -1) {
			client_send(&server->clients[i], ":server NOTICE * :Server shutting down");
			close_socket(server->clients[i].fd);
			server->clients[i].fd = -1;
		}
	}
	
	// Close server socket
	if (server->fd != -1) {
		close_socket(server->fd);
		server->fd = -1;
	}
	
	printf("Server shutdown complete.\n");
}

int main(int ac, char **av) {
	arg_t args;
	server_t server;
	int opt;

	server_init(&server);
	arg_init(&args, ac, av);
	while ((opt = arg_next(&args, ac, av)) != -1) {
		switch (opt) {
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
				break;
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
