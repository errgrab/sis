#include <string.h>
#include <unistd.h>

typedef struct {
	char *name;
	void (*func)(client_t *, char *);
	bool requires_registration;
} command_t;

static void cmd_pass(client_t *client, char *args) {
	if (client->registered) {
		send_reply(client->fd, ":%s 462 :You may not reregister\r\n", SERVER_NAME);
		return;
	}

	if (!args || strlen(args) == 0) {
		send_reply(client->fd, ":%s 461 PASS :Not enough parameters\r\n", SERVER_NAME);
		return;
	}

	if (server_get()->pass && strcmp(args, server_get()->pass) != 0) {
		send_reply(client->fd, ":%s 464 :Password incorrect\r\n", SERVER_NAME);
		return;
	}

	client->authenticated = true;
}

static void cmd_nick(client_t *client, char *args) {
	if (!args || strlen(args) == 0) {
		send_reply(client->fd, ":%s 431 :No nickname given\r\n", SERVER_NAME);
		return;
	}
	
	if (client_find_nick(args)) {
		send_reply(client->fd, ":%s 433 %s :Nickname is already in use\r\n", SERVER_NAME, args);
		return;
	}
	
	strlcpy(client->nick, args, sizeof(client->nick));
	if (client->user[0] && !client->registered) {
		if (server_get()->pass && !client->authenticated) {
			send_reply(client->fd, ":%s 464 :Password incorrect\r\n", SERVER_NAME);
			return;
		}
		client->registered = true;
		send_reply(client->fd, ":%s 001 %s :%s\r\n", SERVER_NAME, client->nick, WELCOME_MSG);
	}
}

static void cmd_user(client_t *client, char *args) {
	char *user, *mode, *unused, *real;
	
	if (!args) {
		send_reply(client->fd, ":%s 461 USER :Not enough parameters\r\n", SERVER_NAME);
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
		if (server_get()->pass && !client->authenticated) {
			send_reply(client->fd, ":%s 464 :Password incorrect\r\n", SERVER_NAME);
			return;
		}
		client->registered = true;
		send_reply(client->fd, ":%s 001 %s :%s\r\n", SERVER_NAME, client->nick, WELCOME_MSG);
	}
}

static void cmd_ping(client_t *client, char *args) {
	send_reply(client->fd, ":%s PONG %s :%s\r\n", SERVER_NAME, SERVER_NAME, args ? args : "");
}

static void cmd_join(client_t *client, char *args) {
	channel_t *channel;
	client_t *curr;
	char *name;
	
	if (!args || strlen(args) == 0) {
		send_reply(client->fd, ":%s 461 JOIN :Not enough parameters\r\n", SERVER_NAME);
		return;
	}
	
	name = args;
	if (name[0] != '#') {
		send_reply(client->fd, ":%s 403 %s :No such channel\r\n", SERVER_NAME, name);
		return;
	}
	
	channel = channel_find(name);
	if (!channel) {
		channel = channel_new(name);
	}
	
	channel_add_client(channel, client);
	
	/* Send JOIN confirmation */
	send_reply(client->fd, ":%s!%s@%s JOIN %s\r\n", client->nick, client->user, client->host, name);
	
	/* Send channel join to other users */
	char join_msg[MAXMSG];
	snprintf(join_msg, sizeof(join_msg), ":%s!%s@%s JOIN %s\r\n", client->nick, client->user, client->host, name);
	channel_send_all(channel, client, join_msg);
	
	/* Send NAMES list */
	send_reply(client->fd, ":%s 353 %s = %s :", SERVER_NAME, client->nick, name);
	for (curr = channel->clients; curr; curr = curr->next) {
		if (curr->registered) {
			send_reply(client->fd, "%s ", curr->nick);
		}
	}
	send_reply(client->fd, "\r\n");
	send_reply(client->fd, ":%s 366 %s %s :End of /NAMES list\r\n", SERVER_NAME, client->nick, name);
}

static void cmd_part(client_t *client, char *args) {
	channel_t *channel;
	char *name, *message;
	char part_msg[MAXMSG];
	
	if (!args || strlen(args) == 0) {
		send_reply(client->fd, ":%s 461 PART :Not enough parameters\r\n", SERVER_NAME);
		return;
	}
	
	name = args;
	message = skip(args, ' ');
	
	if (name[0] != '#') {
		send_reply(client->fd, ":%s 403 %s :No such channel\r\n", SERVER_NAME, name);
		return;
	}
	
	channel = channel_find(name);
	if (!channel) {
		send_reply(client->fd, ":%s 403 %s :No such channel\r\n", SERVER_NAME, name);
		return;
	}
	
	/* Check if user is in channel */
	client_t *curr;
	bool in_channel = false;
	for (curr = channel->clients; curr; curr = curr->next) {
		if (curr == client) {
			in_channel = true;
			break;
		}
	}
	
	if (!in_channel) {
		send_reply(client->fd, ":%s 442 %s :You're not on that channel\r\n", SERVER_NAME, name);
		return;
	}
	
	/* Send PART message to channel */
	if (message && message[0] == ':')
		message++;
	
	if (message && strlen(message) > 0) {
		snprintf(part_msg, sizeof(part_msg), ":%s!%s@%s PART %s :%s\r\n", 
			client->nick, client->user, client->host, name, message);
	} else {
		snprintf(part_msg, sizeof(part_msg), ":%s!%s@%s PART %s\r\n", 
			client->nick, client->user, client->host, name);
	}
	
	send_reply(client->fd, "%s", part_msg);
	channel_send_all(channel, client, part_msg);
	
	/* Remove client from channel */
	channel_remove_client(channel, client);
}

static void cmd_privmsg(client_t *client, char *args) {
	char *target, *message;
	client_t *target_client;
	channel_t *target_channel;
	char msg[MAXMSG];
	
	if (!args || strlen(args) == 0) {
		send_reply(client->fd, ":%s 411 :No recipient given (PRIVMSG)\r\n", SERVER_NAME);
		return;
	}
	
	target = args;
	message = skip(args, ' ');
	
	if (!message || strlen(message) == 0) {
		send_reply(client->fd, ":%s 412 :No text to send\r\n", SERVER_NAME);
		return;
	}
	
	if (message[0] == ':')
		message++;
	
	snprintf(msg, sizeof(msg), ":%s!%s@%s PRIVMSG %s :%s\r\n", 
		client->nick, client->user, client->host, target, message);
	
	if (target[0] == '#') {
		/* Channel message */
		target_channel = channel_find(target);
		if (!target_channel) {
			send_reply(client->fd, ":%s 403 %s :No such channel\r\n", SERVER_NAME, target);
			return;
		}
		
		/* Check if user is in channel */
		client_t *curr;
		bool in_channel = false;
		for (curr = target_channel->clients; curr; curr = curr->next) {
			if (curr == client) {
				in_channel = true;
				break;
			}
		}
		
		if (!in_channel) {
			send_reply(client->fd, ":%s 404 %s :Cannot send to channel\r\n", SERVER_NAME, target);
			return;
		}
		
		channel_send_all(target_channel, client, msg);
	} else {
		/* Private message */
		target_client = client_find_nick(target);
		if (!target_client) {
			send_reply(client->fd, ":%s 401 %s :No such nick/channel\r\n", SERVER_NAME, target);
			return;
		}
		
		send_reply(target_client->fd, "%s", msg);
	}
}

static void cmd_quit(client_t *client, char *args) {
	channel_t *channel, *next_channel;
	char quit_msg[MAXMSG];
	
	/* Send quit message to all channels the client is in */
	if (client->registered) {
		snprintf(quit_msg, sizeof(quit_msg), ":%s!%s@%s QUIT :%s\r\n", 
			client->nick, client->user, client->host, args ? args : "Quit");
		
		for (channel = server_get()->channels; channel; channel = next_channel) {
			next_channel = channel->next; /* Save next before potential free */
			
			/* Check if client is in this channel */
			client_t *curr;
			for (curr = channel->clients; curr; curr = curr->next) {
				if (curr == client) {
					channel_send_all(channel, client, quit_msg);
					channel_remove_client(channel, client);
					break;
				}
			}
		}
	}
	client_remove(client);
	client_free(client);
}


static void process_message(client_t *client, char *line) {
	char *cmd, *args;
	const command_t *c;
	
	static const command_t commands[] = {
		{ "PASS",    cmd_pass,    false },
		{ "NICK",    cmd_nick,    false },
		{ "USER",    cmd_user,    false },
		{ "PING",    cmd_ping,    false },
		{ "QUIT",    cmd_quit,    false },
		{ "JOIN",    cmd_join,    true  },
		{ "PART",    cmd_part,    true  },
		{ "PRIVMSG", cmd_privmsg, true  },
		{ NULL,      NULL,        false }
	};
	
	trim(line);
	if (strlen(line) == 0)
		return;
	
	cmd = line;
	args = skip(line, ' ');
	
	for (c = commands; c->name; c++) {
		if (strcmp(cmd, c->name) == 0) {
			if (c->requires_registration && !client->registered) {
				send_reply(client->fd, ":%s 451 :You have not registered\r\n", SERVER_NAME);
				return;
			}
			c->func(client, args);
			return;
		}
	}
	
	if (client->registered)
		send_reply(client->fd, ":%s 421 %s %s :Unknown command\r\n", SERVER_NAME, client->nick, cmd);
}

static void handle_client(client_t *client) {
	char buf[MAXMSG];
	ssize_t n;
	
	n = recv(client->fd, buf, sizeof(buf) - 1, 0);
	if (n <= 0) {
		client_remove(client);
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
		
		process_message(client, line);
		line = next + 1;
	}
}