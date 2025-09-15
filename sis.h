/* sis - simple IRC server */
#ifndef SIS_H
#define SIS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <time.h>

#include "config.h"

typedef struct Client {
	int fd;
	char nick[MAX_NICK_LEN + 1];
	char user[MAX_NICK_LEN + 1];
	char realname[MAX_MSG_LEN + 1];
	char hostname[INET_ADDRSTRLEN];
	int registered;
	struct Client *next;
} Client;

typedef struct Channel {
	char name[MAX_CHAN_LEN + 1];
	Client *clients;
	struct Channel *next;
} Channel;

/* function prototypes */
void die(const char *msg);
void setup_server(int port);
void handle_client(Client *client, char *msg);
void parse_message(Client *client, char *line);
Client *add_client(int fd, struct sockaddr_in *addr);
void remove_client(Client *client);
void send_to_client(Client *client, const char *msg);
void send_to_channel(Channel *chan, Client *sender, const char *msg);
Channel *find_or_create_channel(const char *name);
void join_channel(Client *client, const char *channame);
void part_channel(Client *client, const char *channame);
void handle_privmsg(Client *client, const char *target, const char *msg);
void handle_nick(Client *client, const char *nick);
void handle_user(Client *client, const char *user, const char *realname);
void handle_join(Client *client, const char *channame);
void handle_part(Client *client, const char *channame);
void handle_quit(Client *client, const char *msg);

/* global variables */
extern Client *clients;
extern Channel *channels;
extern int server_fd;

#endif