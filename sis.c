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
#include <signal.h>
#include <sys/select.h>
#include <poll.h>

typedef struct client_s client_t;
struct client_s {
	int fd;
	char nick[MAXNICK];
	char user[MAXNICK];
	char real[MAXMSG];
	char host[128];
	bool registered;
	bool authenticated;
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

static server_t *server_get(void) {
	static server_t server = {0};
	return &server;
}

#undef strlcpy
#include "arg.c"
#include "util.c"
#include "client.c"
#include "channel.c"
#include "commands.c"
#include "server.c"

static void signal_handler(int sig) {
	switch (sig) {
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
			printf("\nReceived signal %d, shutting down server...\n", sig);
			server_stop();
			break;
		default:
			break;
	}
}

static void setup_signals(void) {
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGPIPE, SIG_IGN); // Ignore broken pipe signals
}

int main(int ac, char **av) {
	arg_t args;
	int opt;

	server_init();
	arg_init(&args, ac, av);
	while ((opt = arg_next(&args, ac, av)) != -1) {
		switch (opt) {
			case 'k':
				{
					char *pass = arg_value(&args, ac, av);
					if (!pass)
						return fprintf(stderr, "%s: password is empty!\n", args.av0), 1;
					server_get()->pass = pass;
				}
				break;
			case 'p':
				{
					char *port = arg_value(&args, ac, av);
					if (!port)
						return fprintf(stderr, "%s: port is empty!", args.av0), 1;
					server_get()->port = port;
				}
				break;
			case 'v':
				printf("%s: version %s\n", args.av0, SERVER_VERSION);
				return 0;
			default:
				fprintf(stderr, "%s: unknown option -%c\n", args.av0, opt);
				return 1;
		}
	}
	for (int i = args.i; i < ac; i++) {
		printf("UNUSED ARGUMENT: %s\n", av[i]);
	}

	setup_signals();
	server_start();
	server_deinit();
	return 0;
}
