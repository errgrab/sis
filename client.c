#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static client_t *client_new(int fd) {
	client_t *client = calloc(1, sizeof(client_t));
	if (!client)
		fatal("ERR: calloc failed:");
	
	client->fd = fd;
	client->nick[0] = '\0';
	client->user[0] = '\0';
	client->real[0] = '\0';
    strlcpy(client->host, SERVER_NAME, sizeof(client->host));
	client->registered = false;
	client->authenticated = false;
	client->next = NULL;
	
	return client;
}

static void client_free(client_t *client) {
	if (client) {
		close_socket(client->fd);
		free(client);
	}
}

static void client_add(client_t *client) {
	server_t *server = server_get();
	client->next = server->clients;
	server->clients = client;
	printf("Client connected (fd: %d)\n", client->fd);
}

static void client_remove(client_t *client) {
	server_t *server = server_get();
	client_t **c = &server->clients;
	while (*c) {
		if (*c == client) {
			*c = client->next;
			break;
		}
		c = &(*c)->next;
	}
	printf("Client disconnected (fd: %d)\n", client->fd);
}

static client_t *client_find_nick(const char *nick) {
	server_t *server = server_get();
	client_t *c = server->clients;
	while (c) {
		if (strcmp(c->nick, nick) == 0)
			return c;
		c = c->next;
	}
	return NULL;
}