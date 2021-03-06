![C/C++ CI](https://github.com/mremallin/chip8/workflows/C/C++%20CI/badge.svg?branch=master)

Chip8
=====

Yet another CHIP8 Interpreter. Written in C and SDL2.

![Screenshot](https://github.com/mremallin/chip8/raw/master/images/space_invaders.png)

Requirements
============
  - libsdl2-dev
  - libsdl2-mixer-dev
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


Additional Software Notices
===========================

kiss_sdl widget toolkit
  Copyright (c) 2016 Tarvo Korrovits <tkorrovi@mail.com>

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would
     be appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not
     be misrepresented as being the original software.
  3. This notice may not be removed or altered from any source
     distribution.



Bitstream Vera Fonts Copyright

Copyright (c) 2003 by Bitstream, Inc. All Rights Reserved. Bitstream
Vera is a trademark of Bitstream, Inc.

Permission is hereby granted, free of charge, to any person obtaining
a copy of the fonts accompanying this license ("Fonts") and associated
documentation files (the "Font Software"), to reproduce and distribute
the Font Software, including without limitation the rights to use, copy,
merge, publish, distribute, and/or sell copies of the Font Software,
and to permit persons to whom the Font Software is furnished to do so,
subject to the following conditions:

The above copyright and trademark notices and this permission notice shall
be included in all copies of one or more of the Font Software typefaces.

The Font Software may be modified, altered, or added to, and in particular
the designs of glyphs or characters in the Fonts may be modified and
additional glyphs or characters may be added to the Fonts, only if the
fonts are renamed to names not containing either the words "Bitstream"
or the word "Vera".

This License becomes null and void to the extent applicable to Fonts
or Font Software that has been modified and is distributed under the
"Bitstream Vera" names.

The Font Software may be sold as part of a larger software package but no
copy of one or more of the Font Software typefaces may be sold by itself.

THE FONT SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF COPYRIGHT, PATENT, TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL
BITSTREAM OR THE GNOME FOUNDATION BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, INCLUDING ANY GENERAL, SPECIAL, INDIRECT, INCIDENTAL,
OR CONSEQUENTIAL DAMAGES, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF THE USE OR INABILITY TO USE THE FONT
SOFTWARE OR FROM OTHER DEALINGS IN THE FONT SOFTWARE.

Except as contained in this notice, the names of Gnome, the Gnome
Foundation, and Bitstream Inc., shall not be used in advertising or
otherwise to promote the sale, use or other dealings in this Font
Software without prior written authorization from the Gnome Foundation
or Bitstream Inc., respectively. For further information, contact:
fonts at gnome dot org.
