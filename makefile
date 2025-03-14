all: build

install: all
	./INSTALL

build: main.c
	cc -pedantic -std=c99 -Wall -Wextra $< -lncurses -o typc

.PHONY: all install build
