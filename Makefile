CFLAGS=-Wall -Wpedantic $(shell sdl2-config --cflags) -g
LIBRARIES := $(shell sdl2-config --libs) -lm
UNAME := $(shell uname -s)

.DEFAULT_GOAL := all

main.o: main.c
	gcc $(CFLAGS) -c -o $@ $<

chip8: main.o
	gcc -o $@ $^ $(LIBRARIES)

all: chip8

clean:
	rm -f *.o chip8 || true