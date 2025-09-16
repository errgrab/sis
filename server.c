#include <stdio.h>
#include <errno.h>
#include <sys/select.h>

static void server_init(server_t *server) {
	server->port = DEFAULT_PORT;
	server->fd = -1;
	server->pass = DEFAULT_PASS;
	server->online = true;
	server->clients = NULL;
	server->channels = NULL;
}

static void server_start(server_t *server) {
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

static void server_stop(server_t *server) {
    server->online = false;
}

static void server_deinit(server_t *server) {
	if (server->fd != -1)
		close_socket(server->fd);
}