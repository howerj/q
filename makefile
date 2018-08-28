CFLAGS=-std=c99 -Wall -Wextra -O2 -pedantic -fwrapv
TARGET=q
RM=rm -fv
AR=ar
RANLIB=ranlib

.PHONY: all run clean

all: ${TARGET}

run: ${TARGET} t.q
	./${TARGET} t.q

test: ${TARGET}
	./${TARGET} -t

lib${TARGET}.a: q.o
	${AR} rcs $@ $<
	${RANLIB} $@

${TARGET}: lib${TARGET}.a t.o

cpp: cpp.cpp lib${TARGET}.a

clean:
	${RM} ${TARGET} cpp *.a *.o
