CFLAGS=-std=c99 -Wall -Wextra -O2 -pedantic -fwrapv
TARGET=q
RM=rm -fv

.PHONY: all run clean

all: ${TARGET}

run: ${TARGET} t.q
	./${TARGET} t.q

test: ${TARGET}
	./${TARGET} -t

${TARGET}: *.c

clean:
	${RM} ${TARGET}
