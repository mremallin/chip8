![C/C++ CI](https://github.com/mremallin/chip8/workflows/C/C++%20CI/badge.svg?branch=master)

Chip8
=====

Yet another CHIP8 Interpreter. Written in C and SDL2.

Requirements
============
  - libsdl2-dev
  - cmocka
  - lcov

Building
========
  - make
  - make check
  - make lcov

Usage
=====
./chip8 <path/to/rom.ch8>

Key Mappings
============

```
 * The chip8 keyboard is layed out as follows:
 * -----------------
 * | 1 | 2 | 3 | C |
 * -----------------
 * | 4 | 5 | 6 | D |
 * -----------------
 * | 7 | 8 | 9 | E |
 * -----------------
 * | A | 0 | B | F |
 * -----------------
 *
 * This is mapped to a QWERTY keyboard via the following
 * layout:
 *
 * -----------------
 * | 1 | 2 | 3 | 4 |
 * -----------------
 * | Q | W | E | R |
 * -----------------
 * | A | S | D | F |
 * -----------------
 * | Z | X | C | V |
 * -----------------
```

Resources
=========
 - http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
 - https://en.wikipedia.org/wiki/CHIP-8
 - ROM Collection: https://github.com/dmatlack/chip8
