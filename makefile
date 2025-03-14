all: build

install: all
	./INSTALL

build: main.c
	cc -W $< -lncurses -o typc

.PHONY: all install build
