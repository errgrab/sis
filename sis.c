#include "config.h"

#include <time.h>
#include <poll.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/select.h>

#define MODE_INVITE_ONLY 0x01
#define MODE_KEY		 0x02
#define MODE_TOPIC_OP    0x04

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
	char topic[MAXMSG];
	char topic_who[MAXNICK];
	time_t topic_time;
	unsigned int mode;
	char key[MAXNICK];
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
#include "channel.c"
#include "client.c"
#include "commands.c"
#include "server.c"
#include "signal.c"

int main(int ac, char **av) {

	server_init();
	if (get_args(ac, av)) return 1;
	setup_signals();
	server_start();
	server_free();
	return 0;
}
