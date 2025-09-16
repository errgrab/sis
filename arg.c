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