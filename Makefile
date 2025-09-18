.POSIX:

NAME = sis
VERSION = 0.42

SIS_CFLAGS = ${CFLAGS} -Wall -Wextra -Wpedantic -Werror -std=c99
SIS_LDFLAGS = ${LDFLAGS}
SIS_CPPFLAGS = ${LDFLAGS} -DVERSION=\"${VERSION}\" -D_GNU_SOURCE

BIN = sis
SRC = ${BIN:=.c}
OBJ = ${SRC:.c=.o}

all: ${BIN}

${BIN}: ${@:=.o}

${OBJ}:

.o:
	${CC} -o $@ $< ${SIS_LDFLAGS}

.c.o:
	${CC} -c ${SIS_CFLAGS} ${SIS_CPPFLAGS} -o $@ -c $<

config.h:
	cp config.def.h $@

clean:
	rm -f ${BIN} ${OBJ}

re: clean all

.PHONY: all clean re dist
