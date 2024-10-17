CC = gcc
CFLAGS = -Wall -Wextra
LFLAGS = -lm -lraylib -lpthread -ldl

all: main

main: src/*.c
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

run: all
	./main