CFLAGS=-std=c99 -Wall -Wextra -O2 -pedantic
TARGET=q
RM=rm -fv

.PHONY: all run clean

all: ${TARGET}

run: ${TARGET} t.q
	./${TARGET} t.q

${TARGET}: *.c

clean:
	${RM} ${TARGET}
