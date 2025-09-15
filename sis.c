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

#undef strlcpy
#include "strlcpy.c"
#include "util.c"

typedef struct server_s server_t;
struct server_s {
	int fd;
	char *port;
	char *pass;
	bool online;
};

void server_init(server_t *server) {
	server->port = "6667";
	server->fd = -1;
	server->pass = NULL;
	server->online = true;
}

void server_start(server_t *server) {
	server->fd = listen_on(server->port);

	while (server->online) {
		int client_fd = accept_client(server->fd);
		printf("Connected!");
		close_socket(client_fd);
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
