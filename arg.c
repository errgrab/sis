#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct arg_s arg_t;
struct arg_s {
	char *av0;
	int i;
	char *opt;
};

static void arg_init(arg_t *arg, int ac, char **av) {
	(void)ac;
	arg->av0 = av[0];
	arg->i = 1;
	arg->opt = NULL;
}

static int arg_next(arg_t *arg, int ac, char **av) {
	if (arg->opt == NULL || *arg->opt == '\0') {
		if (arg->i >= ac) return -1;
		if (av[arg->i][0] != '-') return -1;
		if (strcmp(av[arg->i], "--") == 0) {
			arg->i++;
			return -1;
		}
		arg->opt = av[arg->i++] + 1;
	}
	return *arg->opt++;
}

static char *arg_value(arg_t *arg, int ac, char **av) {
	if (arg->opt && *arg->opt != '\0') {
		char *val = arg->opt;
		arg->opt = NULL;
		return val;
	}
	if (arg->i < ac)
		return av[arg->i++];
	return NULL;
}

int get_args(int ac, char **av) {
	arg_t args;
	int opt;

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
	/*
	for (int i = args.i; i < ac; i++) {
		printf("UNUSED ARGUMENT: %s\n", av[i]);
	}*/
	return 0;
}
