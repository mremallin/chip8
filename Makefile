CFLAGS=-Wall -Wpedantic $(shell sdl2-config --cflags) -g
TEST_CFLAGS=-fprofile-arcs -ftest-coverage -I/usr/local/include
LIBRARIES := $(shell sdl2-config --libs) -lm
UNAME := $(shell uname -s)

.DEFAULT_GOAL := all

main.o: main.c chip8.c
	gcc $(CFLAGS) -c -o $@ $<

chip8: main.o chip8.o
	gcc -o $@ $^ $(LIBRARIES)

all: chip8

chip8_test.o: chip8_test.c
	gcc $(CFLAGS) $(TEST_CFLAGS) -c -o $@ $<

chip8_test: chip8_test.o
	gcc -v -o $@ $^ $(LIBRARIES) -lcmocka --coverage

test: chip8_test

lcov:
	mkdir -p lcov && \
	cd lcov && \
	lcov --directory ../ --capture --rc lcov_branch_coverage=1 --output-file chip8_test.info && \
	genhtml --rc lcov_branch_coverage=1 chip8_test.info

clean:
	rm -f *.o chip8 || true
	rm -f *.o chip8_test || true
	rm -rf *.gcno *.gcda lcov || true

.PHONY: test lcov clean