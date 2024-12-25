CC = gcc
CFLAGS = -Wall -Wextra
LFLAGS = -lm -lraylib -lpthread -ldl

# Directories
SRCS = $(wildcard src/*.c) # wildcard function read all mathing the expression
OBJS = $(patsubst src/%.c, obj/%.o,$(SRCS)) # $(patsubst <pattern>, <replacement>, <text>)

all: terrain

main: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LFLAGS)

obj/%.o: src/%.c 
	$(CC) $(CFLAGS) -c $< -o $@ 

terrain: terrain/main.c
	$(CC) $(CFLAGS) terrain/*.c -lraylib -lm

run: all
	./main
