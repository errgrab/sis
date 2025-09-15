.POSIX:

NAME = sis
VERSION = 0.42

SIS_CFLAGS = ${CFLAGS}
SIS_LDFLAGS = ${LDFLAGS}
SIS_CPPFLAGS = ${LDFLAGS} -DVERSION=\"${VERSION}\" -D_GNU_SOURCE

BIN = sis
SRC = ${BIN:=.c}
OBJ = ${SRC:.c=.o}

all: ${BIN}

${BIN}: ${@:=.o}

${OBJ}: config.h strlcpy.c util.c

.o:
	${CC} -o $@ $< ${SIS_LDFLAGS}

.c.o:
	${CC} -c ${SIS_CFLAGS} ${SIS_CPPFLAGS} -o $@ -c $<

config.h:
	cp config.def.h $@

clean:
	rm -f ${BIN} ${OBJ} "${NAME}-${VERSION}.tar.gz"

dist:
	mkdir -p "${NAME}-${VERSION}"
	cp -fR LICENSE Makefile README arg.h config.def.h \
		${SRC} util.c strlcpy.c "${NAME}-${VERSION}"
	tar -cf - "${NAME}-${VERSION}" | \
		gzip -c > "${NAME}-${VERSION}.tar.gz"
	rm -rf "${NAME}-${VERSION}"

.PHONY: all clean dist
