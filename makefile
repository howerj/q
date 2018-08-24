CFLAGS=-std=c99 -Wall -Wextra -O2 -DTEST
TARGET=q

.PHONY: all run clean

all: ${TARGET}

run: ${TARGET}
	./${TARGET}

${TARGET}: *.c

clean:
	rm -f ${TARGET}
