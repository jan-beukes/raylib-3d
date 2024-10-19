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

terrain: obj/janperlin.o obj/perlin.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

run: all
	./main