#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

static size_t strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	if (n != 0)
		while (--n != 0) if ((*d++ = *s++) == '\0') break;
	if (n == 0) {
		if (siz != 0) *d = '\0';
		while (*s++);
	}
	return(s - src - 1);
}

static void fatal(const char *fmt, ...) {
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s", buf);
	if(fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s\n", strerror(errno));
	exit(1);
}

static int listen_on(char *port) {
	struct addrinfo hints, *res, *r;
	int sockfd;
	int yes = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, port, &hints, &res) != 0)
		fatal("ERR: getaddrinfo failed:");
	for (r = res; r; r = r->ai_next) {
		if ((sockfd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1)
			continue;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
			fatal("ERR: setsockopt failed:");
		if (bind(sockfd, r->ai_addr, r->ai_addrlen) == 0)
			break;
		close(sockfd);
	}
	freeaddrinfo(res);
	if (!r)
		fatal("ERR: cannot bind socket:");
	if (listen(sockfd, 10) == -1)
		fatal("ERR: listen failed:");
	return sockfd;
};

static int accept_client(int server_fd) {
	struct sockaddr_storage client_addr;
	socklen_t addr_size = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
	if (client_fd == -1)
		fatal("ERR: accept failed:");
	return client_fd;
}

static void close_socket(int fd) {
	if (close(fd) == -1)
		perror("ERR: close failed");
}


/*
static int dial(char *host, char *port) {
	struct addrinfo hints;
	struct addrinfo *res, *r;
	int fd;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(host, port, &hints, &res) != 0)
		eprint("error: cannot resolve hostname '%s':", host);
	for(r = res; r; r = r->ai_next) {
		if((fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1)
			continue;
		if(connect(fd, r->ai_addr, r->ai_addrlen) == 0)
			break;
		close(fd);
	}
	freeaddrinfo(res);
	if(!r)
		eprint("error: cannot connect to host '%s'\n", host);
	return fd;
}
*/

static char *eat(char *s, int (*p)(int), int r) {
	while(*s != '\0' && p((unsigned char)*s) == r)
		s++;
	return s;
}

static char *skip(char *s, char c) {
	while(*s != c && *s != '\0')
		s++;
	if(*s != '\0')
		*s++ = '\0';
	return s;
}

static void trim(char *s) {
	char *e;

	for (e = s + strlen(s); e > s && isspace((unsigned char)*(e - 1)); e--);
	*e = '\0';
}
