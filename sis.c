/* sis - simple IRC server */
#include "sis.h"

/* global variables */
Client *clients = NULL;
Channel *channels = NULL;
int server_fd = -1;

void
die(const char *msg)
{
	perror(msg);
	exit(1);
}

void
send_to_client(Client *client, const char *msg)
{
	if (client && client->fd > 0) {
		send(client->fd, msg, strlen(msg), 0);
		send(client->fd, "\r\n", 2, 0);
	}
}

void
send_to_channel(Channel *chan, Client *sender, const char *msg)
{
	Client *c;
	
	if (!chan)
		return;
		
	for (c = chan->clients; c; c = c->next) {
		if (c != sender)
			send_to_client(c, msg);
	}
}

Client *
find_client_by_nick(const char *nick)
{
	Client *c;
	
	for (c = clients; c; c = c->next) {
		if (strcmp(c->nick, nick) == 0)
			return c;
	}
	return NULL;
}

Channel *
find_channel(const char *name)
{
	Channel *chan;
	
	for (chan = channels; chan; chan = chan->next) {
		if (strcmp(chan->name, name) == 0)
			return chan;
	}
	return NULL;
}

Channel *
find_or_create_channel(const char *name)
{
	Channel *chan = find_channel(name);
	
	if (!chan) {
		chan = malloc(sizeof(Channel));
		if (!chan)
			return NULL;
		strncpy(chan->name, name, MAX_CHAN_LEN);
		chan->name[MAX_CHAN_LEN] = '\0';
		chan->clients = NULL;
		chan->next = channels;
		channels = chan;
	}
	return chan;
}

void
join_channel(Client *client, const char *channame)
{
	Channel *chan = find_or_create_channel(channame);
	Client *c;
	char msg[MAX_MSG_LEN];
	
	if (!chan)
		return;
		
	/* check if already in channel */
	for (c = chan->clients; c; c = c->next) {
		if (c == client)
			return;
	}
	
	/* add client to channel */
	client->next = chan->clients;
	chan->clients = client;
	
	/* notify channel including the joining client */
	snprintf(msg, sizeof(msg), ":%s!%s@%s JOIN %s", 
		client->nick, client->user, client->hostname, channame);
	send_to_channel(chan, NULL, msg);
	
	/* send names list */
	snprintf(msg, sizeof(msg), ":%s 353 %s = %s :", 
		SERVER_NAME, client->nick, channame);
	for (c = chan->clients; c; c = c->next) {
		if (strlen(msg) + strlen(c->nick) + 1 < sizeof(msg) - 1) {
			strcat(msg, c->nick);
			if (c->next)
				strcat(msg, " ");
		}
	}
	send_to_client(client, msg);
	
	snprintf(msg, sizeof(msg), ":%s 366 %s %s :End of /NAMES list", 
		SERVER_NAME, client->nick, channame);
	send_to_client(client, msg);
}

void
part_channel(Client *client, const char *channame)
{
	Channel *chan = find_channel(channame);
	Client **c;
	char msg[MAX_MSG_LEN];
	
	if (!chan)
		return;
		
	/* remove client from channel */
	for (c = &chan->clients; *c; c = &(*c)->next) {
		if (*c == client) {
			*c = client->next;
			break;
		}
	}
	
	/* notify channel */
	snprintf(msg, sizeof(msg), ":%s!%s@%s PART %s", 
		client->nick, client->user, client->hostname, channame);
	send_to_channel(chan, client, msg);
	send_to_client(client, msg);
}

void
handle_privmsg(Client *client, const char *target, const char *msg)
{
	char response[MAX_MSG_LEN];
	
	if (target[0] == '#') {
		/* channel message */
		Channel *chan = find_channel(target);
		if (chan) {
			snprintf(response, sizeof(response), ":%s!%s@%s PRIVMSG %s :%s",
				client->nick, client->user, client->hostname, target, msg);
			send_to_channel(chan, client, response);
		}
	} else {
		/* private message */
		Client *recipient = find_client_by_nick(target);
		if (recipient) {
			snprintf(response, sizeof(response), ":%s!%s@%s PRIVMSG %s :%s",
				client->nick, client->user, client->hostname, target, msg);
			send_to_client(recipient, response);
		}
	}
}

void
handle_nick(Client *client, const char *nick)
{
	char msg[MAX_MSG_LEN];
	
	if (find_client_by_nick(nick)) {
		snprintf(msg, sizeof(msg), ":%s 433 * %s :Nickname is already in use", 
			SERVER_NAME, nick);
		send_to_client(client, msg);
		return;
	}
	
	strncpy(client->nick, nick, MAX_NICK_LEN);
	client->nick[MAX_NICK_LEN] = '\0';
	
	if (client->registered) {
		snprintf(msg, sizeof(msg), ":%s!%s@%s NICK %s",
			client->nick, client->user, client->hostname, nick);
		send_to_client(client, msg);
	}
}

void
handle_user(Client *client, const char *user, const char *realname)
{
	char msg[MAX_MSG_LEN];
	
	strncpy(client->user, user, MAX_NICK_LEN);
	client->user[MAX_NICK_LEN] = '\0';
	strncpy(client->realname, realname, MAX_MSG_LEN);
	client->realname[MAX_MSG_LEN] = '\0';
	
	if (strlen(client->nick) > 0 && !client->registered) {
		client->registered = 1;
		
		/* send welcome messages */
		snprintf(msg, sizeof(msg), ":%s 001 %s :Welcome to %s", 
			SERVER_NAME, client->nick, SERVER_NAME);
		send_to_client(client, msg);
		
		snprintf(msg, sizeof(msg), ":%s 002 %s :Your host is %s, running %s", 
			SERVER_NAME, client->nick, SERVER_NAME, SERVER_VERSION);
		send_to_client(client, msg);
		
		snprintf(msg, sizeof(msg), ":%s 003 %s :This server was created today", 
			SERVER_NAME, client->nick);
		send_to_client(client, msg);
		
		snprintf(msg, sizeof(msg), ":%s 004 %s %s %s oiwsvcr bkloveqjfI", 
			SERVER_NAME, client->nick, SERVER_NAME, SERVER_VERSION);
		send_to_client(client, msg);
	}
}

void
handle_join(Client *client, const char *channame)
{
	if (channame[0] != '#') {
		char msg[MAX_MSG_LEN];
		snprintf(msg, sizeof(msg), ":%s 403 %s %s :No such channel", 
			SERVER_NAME, client->nick, channame);
		send_to_client(client, msg);
		return;
	}
	join_channel(client, channame);
}

void
handle_part(Client *client, const char *channame)
{
	part_channel(client, channame);
}

void
handle_quit(Client *client, const char *msg)
{
	Channel *chan;
	Client **c;
	char response[MAX_MSG_LEN];
	
	snprintf(response, sizeof(response), ":%s!%s@%s QUIT :%s",
		client->nick, client->user, client->hostname, msg ? msg : "Quit");
	
	/* remove from all channels and notify */
	for (chan = channels; chan; chan = chan->next) {
		for (c = &chan->clients; *c; c = &(*c)->next) {
			if (*c == client) {
				*c = client->next;
				send_to_channel(chan, client, response);
				break;
			}
		}
	}
}

void
parse_message(Client *client, char *line)
{
	char *cmd, *arg1, *rest;
	
	/* remove trailing \r\n */
	line[strcspn(line, "\r\n")] = '\0';
	
	if (strlen(line) == 0)
		return;
		
	cmd = strtok(line, " ");
	if (!cmd)
		return;
		
	if (strcmp(cmd, "NICK") == 0) {
		arg1 = strtok(NULL, " ");
		if (arg1)
			handle_nick(client, arg1);
	} else if (strcmp(cmd, "USER") == 0) {
		arg1 = strtok(NULL, " ");
		strtok(NULL, " "); /* skip hostname */
		strtok(NULL, " "); /* skip servername */
		rest = strtok(NULL, "");
		if (arg1 && rest && rest[0] == ':')
			handle_user(client, arg1, rest + 1);
	} else if (strcmp(cmd, "JOIN") == 0) {
		arg1 = strtok(NULL, " ");
		if (arg1 && client->registered)
			handle_join(client, arg1);
	} else if (strcmp(cmd, "PART") == 0) {
		arg1 = strtok(NULL, " ");
		if (arg1 && client->registered)
			handle_part(client, arg1);
	} else if (strcmp(cmd, "PRIVMSG") == 0) {
		arg1 = strtok(NULL, " ");
		rest = strtok(NULL, "");
		if (arg1 && rest && rest[0] == ':' && client->registered)
			handle_privmsg(client, arg1, rest + 1);
	} else if (strcmp(cmd, "QUIT") == 0) {
		rest = strtok(NULL, "");
		handle_quit(client, rest ? rest + 1 : NULL);
	} else if (strcmp(cmd, "PING") == 0) {
		arg1 = strtok(NULL, " ");
		if (arg1) {
			char response[MAX_MSG_LEN];
			snprintf(response, sizeof(response), ":%s PONG %s :%s",
				SERVER_NAME, SERVER_NAME, arg1);
			send_to_client(client, response);
		}
	}
}

void
handle_client(Client *client, char *buffer)
{
	char *line, *next;
	
	line = buffer;
	while ((next = strstr(line, "\r\n")) || (next = strstr(line, "\n"))) {
		*next = '\0';
		parse_message(client, line);
		line = next + (next[1] == '\n' ? 2 : 1);
	}
	
	/* handle incomplete line at end */
	if (strlen(line) > 0) {
		parse_message(client, line);
	}
}

Client *
add_client(int fd, struct sockaddr_in *addr)
{
	Client *client = malloc(sizeof(Client));
	
	if (!client)
		return NULL;
		
	client->fd = fd;
	client->nick[0] = '\0';
	client->user[0] = '\0';
	client->realname[0] = '\0';
	client->registered = 0;
	inet_ntop(AF_INET, &addr->sin_addr, client->hostname, INET_ADDRSTRLEN);
	client->next = clients;
	clients = client;
	
	return client;
}

void
remove_client(Client *client)
{
	Client **c;
	
	/* remove from client list */
	for (c = &clients; *c; c = &(*c)->next) {
		if (*c == client) {
			*c = client->next;
			break;
		}
	}
	
	/* handle quit and cleanup */
	handle_quit(client, "Connection closed");
	
	if (client->fd > 0)
		close(client->fd);
	free(client);
}

void
setup_server(int port)
{
	struct sockaddr_in addr;
	int opt = 1;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		die("socket");
		
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		die("setsockopt");
		
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	
	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		die("bind");
		
	if (listen(server_fd, 10) < 0)
		die("listen");
		
	printf("IRC server listening on port %d\n", port);
}

int
main(int argc, char *argv[])
{
	fd_set readfds;
	int max_fd, activity, new_fd;
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	Client *client, *next_client;
	char buffer[MAX_MSG_LEN + 1];
	int bytes_read;
	int port = DEFAULT_PORT;
	
	if (argc > 1)
		port = atoi(argv[1]);
		
	setup_server(port);
	
	while (1) {
		FD_ZERO(&readfds);
		FD_SET(server_fd, &readfds);
		max_fd = server_fd;
		
		/* add client sockets to set */
		for (client = clients; client; client = client->next) {
			FD_SET(client->fd, &readfds);
			if (client->fd > max_fd)
				max_fd = client->fd;
		}
		
		activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
		if (activity < 0 && errno != EINTR)
			die("select");
			
		/* new connection */
		if (FD_ISSET(server_fd, &readfds)) {
			new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
			if (new_fd >= 0) {
				printf("New connection from %s\n", 
					inet_ntoa(client_addr.sin_addr));
				add_client(new_fd, &client_addr);
			}
		}
		
		/* handle client data */
		client = clients;
		while (client) {
			next_client = client->next;
			
			if (FD_ISSET(client->fd, &readfds)) {
				bytes_read = recv(client->fd, buffer, MAX_MSG_LEN, 0);
				if (bytes_read <= 0) {
					printf("Client %s disconnected\n", client->hostname);
					remove_client(client);
				} else {
					buffer[bytes_read] = '\0';
					handle_client(client, buffer);
				}
			}
			
			client = next_client;
		}
	}
	
	return 0;
}