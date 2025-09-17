#include <stdio.h>
#include <errno.h>
#include <sys/select.h>

static void server_init(void) {
	server_t *server = server_get();
	server->port = DEFAULT_PORT;
	server->fd = -1;
	server->pass = DEFAULT_PASS;
	server->online = true;
	server->clients = NULL;
	server->channels = NULL;
}

static void server_start(void) {
	server_t *server = server_get();
	struct pollfd *pollfds;
	int nfds, maxclients = 1024;
	client_t *client, *next;
	
	server->fd = listen_on(server->port);
	printf("IRC Server listening on port %s\n", server->port);
	
	pollfds = calloc(maxclients + 1, sizeof(struct pollfd));
	if (!pollfds)
		fatal("ERR: calloc failed:");
	
	while (server->online) {
		nfds = 0;
		
		// Add server fd
		pollfds[nfds].fd = server->fd;
		pollfds[nfds].events = POLLIN;
		nfds++;
		
		// Add all client fds to poll
		client = server->clients;
		while (client && nfds < maxclients) {
			pollfds[nfds].fd = client->fd;
			pollfds[nfds].events = POLLIN;
			nfds++;
			client = client->next;
		}
		
		if (poll(pollfds, nfds, -1) == -1) {
			if (errno == EINTR) continue;
			fatal("ERR: poll failed:");
		}
		
		// Check for new connections
		if (pollfds[0].revents & POLLIN)
			client_add(client_new(accept_client(server->fd)));
		
		// Handle existing clients
		for (int i = 1; i < nfds; i++) {
			if (pollfds[i].revents & POLLIN) {
				// Find the client with this fd
				client = server->clients;
				while (client) {
					next = client->next;
					if (client->fd == pollfds[i].fd) {
						handle_client(client);
						break;
					}
					client = next;
				}
			}
		}
	}
	free(pollfds);
}

static void server_stop(void) {
	server_t *server = server_get();
	server->online = false;
}

static void server_free(void) {
	server_t *server = server_get();
	if (server->fd != -1)
		close_socket(server->fd);

	client_t *client = server->clients;
	while (client) {
		client_t *next = client->next;
		client_free(client);
		client = next;
	}

	channel_t *channel = server->channels;
	while (channel) {
		channel_t *next = channel->next;
		channel_free(channel);
		channel = next;
	}
}