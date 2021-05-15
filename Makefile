CFLAGS=-Wall -Werror -Wpedantic $(shell sdl2-config --cflags) -g -O2
TEST_CFLAGS=-fprofile-arcs -ftest-coverage -I/usr/local/include
LIBRARIES := $(shell sdl2-config --libs) -lSDL2_mixer -lm
UNAME := $(shell uname -s)
CC=gcc
#CC=/usr/local/Cellar/gcc/11.1.0/bin/gcc-11

.DEFAULT_GOAL := all

main.o: main.c chip8.c chip8_utils.c chip8_sound.c
	$(CC) $(CFLAGS) -c -o $@ $<

chip8: main.o chip8.o chip8_utils.o chip8_sound.o
	$(CC) -o $@ $^ $(LIBRARIES)

all: chip8

chip8_test.o: chip8_test.c
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -c -o $@ $<

chip8_test: chip8_test.o
	$(CC) -o $@ $^ $(LIBRARIES) -lcmocka --coverage

check: chip8_test
	./chip8_test

lcov:
	mkdir -p coverage && \
	cd coverage && \
	lcov --directory ../ --capture --rc lcov_branch_coverage=1 --output-file lcov.info && \
	genhtml --rc lcov_branch_coverage=1 lcov.info

clean:
	rm -f *.o chip8 || true
	rm -f *.o chip8_test || true
	rm -rf *.gcno *.gcda lcov || true

.PHONY: lcov clean
