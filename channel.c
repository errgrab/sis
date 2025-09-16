#include <string.h>
#include <stdlib.h>

static channel_t *channel_new(char *name) {
	channel_t *channel;
	
	channel = calloc(1, sizeof(channel_t));
	if (!channel)
		fatal("calloc");
	
	strlcpy(channel->name, name, sizeof(channel->name));
	channel->next = server_get()->channels;
	server_get()->channels = channel;
	
	return channel;
}

static void channel_free(channel_t *channel) {
	channel_t **curr;
	
	for (curr = &server_get()->channels; *curr; curr = &(*curr)->next) {
		if (*curr == channel) {
			*curr = channel->next;
			break;
		}
	}
	
	free(channel);
}

static channel_t *channel_find(char *name) {
	channel_t *channel;
	
	for (channel = server_get()->channels; channel; channel = channel->next) {
		if (strcmp(channel->name, name) == 0)
			return channel;
	}
	
	return NULL;
}

static void channel_add_client(channel_t *channel, client_t *client) {
	client_t *curr;
	
	for (curr = channel->clients; curr; curr = curr->next) {
		if (curr == client)
			return;
	}
	
	client->next = channel->clients;
	channel->clients = client;
}

static void channel_remove_client(channel_t *channel, client_t *client) {
	client_t **curr;
	
	for (curr = &channel->clients; *curr; curr = &(*curr)->next) {
		if (*curr == client) {
			*curr = client->next;
			client->next = NULL;
			break;
		}
	}
	
	if (!channel->clients) {
		channel_free(channel);
	}
}

static void channel_send_all(channel_t *channel, client_t *sender, char *message) {
	client_t *client;
	
	for (client = channel->clients; client; client = client->next) {
		if (client != sender && client->registered) {
			send_reply(client->fd, "%s", message);
		}
	}
}