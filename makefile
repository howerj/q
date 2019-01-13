CFLAGS=-std=c99 -Wall -Wextra -O2 -pedantic -fwrapv
CC=gcc
TARGET=q
RM=rm -fv
AR=ar
RANLIB=ranlib

.PHONY: all run clean

all: ${TARGET} cpp

run: test ${TARGET} t.q
	./${TARGET} t.q

test: ${TARGET}
	./${TARGET} -t


q.o: q.c q.h

c.o: c.cpp q.hpp q.h

lib${TARGET}.a: q.o
	${AR} rcs $@ $<
	${RANLIB} $@

check: *.c *.h
	cppcheck --enable=all *.c *.h

${TARGET}: lib${TARGET}.a t.o

cpp: c.o lib${TARGET}.a
	${CXX} ${CXFLAGS} $^ -o $@

clean:
	${RM} ${TARGET} cpp *.a *.o
