all: build

build: main.c
	cc -W $< -lncurses

.PHONY: build
