CC = gcc
CFLAGS = -Wall -Wextra
LFLAGS = -lm -lraylib -lpthread -ldl

all: main

main: main.c
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS) 