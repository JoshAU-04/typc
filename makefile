all: build

install: all
	./INSTALL

build: main.c
	cc -pedantic -std=c99 -Wall -Wextra $< -lncurses -o typc

format: main.c
	/usr/bin/clang-format -i $< --style=Mozilla

.PHONY: all install build
