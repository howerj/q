CFLAGS=-std=c99 -Wall -Wextra -O2 -pedantic -fwrapv
CC=gcc
TARGET=q
RM=rm -fv
AR=ar
RANLIB=ranlib

.PHONY: all run clean

all: ${TARGET}

run: test ${TARGET} t.q
	./${TARGET} t.q

test: ${TARGET}
	./${TARGET} -t


q.o: q.c q.h

lib${TARGET}.a: q.o
	${AR} rcs $@ $<
	${RANLIB} $@

check: *.c *.h
	cppcheck --enable=all *.c *.h

${TARGET}: lib${TARGET}.a t.o

clean:
	${RM} ${TARGET} *.a *.o
