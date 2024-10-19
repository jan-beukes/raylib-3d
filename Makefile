CC = gcc
CFLAGS = -Wall -Wextra
LFLAGS = -lm -lraylib -lpthread -ldl

# Directories
SRCS = $(wildcard src/*.c) # wildcard function read all mathing the expression
OBJS = $(patsubst src/%.c, obj/%.o,$(SRCS)) # $(patsubst <pattern>, <replacement>, <text>)

all: main

main: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LFLAGS)

obj/%.o: src/%.c 
	$(CC) $(CFLAGS) -c $< -o $@ 

terrain: obj/terrain/janperlin.o obj/terrain/perlin.o
	$(CC) $(CFLAGS) -o $@ $^ ~/Cloned/raylib/src/libraylib.a -lm

run: all
	./main