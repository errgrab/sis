#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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