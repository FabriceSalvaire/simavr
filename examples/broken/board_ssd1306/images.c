/*
  images.c

  Copyright 2014 Doug Szumski <d.s.szumski@gmail.com>

  This file is part of simavr.

  simavr is free software: you can redistribute it and/or modify it under the terms of the GNU
  General Public License as published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  simavr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
  Public License for more details.

  You should have received a copy of the GNU General Public License along with simavr.  If not, see
  <http://www.gnu.org/licenses/>.
*/

#include "images.h"

/* 128x64 run length encoded simavr logo */
const uint8_t logo[] PROGMEM =
  {
    0x0, 0x0, 0xff, 0x0, 0x0, 0x7, 0xfe, 0xfe, 0x4, 0xe, 0x6, 0x6, 0x8, 0xe, 0xfe,
    0xfe, 0x3, 0xfc, 0x0, 0x0, 0x5, 0x54, 0xfe, 0xfe, 0x2, 0x0, 0x0,
    0x3, 0xfe, 0xfe, 0x2, 0xff, 0x6, 0x1e, 0x7e, 0xfc, 0xe0, 0xc0,
    0x0, 0x80, 0xc0, 0xf0, 0x7c, 0x3e, 0xe, 0x5e, 0xff, 0xfe, 0xfe,
    0x2, 0xff, 0xfe, 0xfe, 0x2, 0x0, 0x0, 0x6, 0xfe, 0xfe, 0x3, 0x6,
    0x6, 0xa, 0x2e, 0xfe, 0xfe, 0x3, 0x0, 0x0, 0x2, 0xfe, 0xff,
    0xba, 0x0, 0x0, 0x7, 0xf4, 0xff, 0xff, 0x5, 0xfe, 0x0, 0x0, 0x2,
    0xfe, 0xff, 0xff, 0x2, 0xf, 0x7, 0x7, 0xa, 0xff, 0xff, 0x3, 0x0,
    0x0, 0xf, 0x7f, 0x7f, 0x4, 0x70, 0x60, 0x60, 0x8, 0xe0, 0xe0,
    0x2, 0xe1, 0xe1, 0x2, 0xc0, 0x0, 0x0, 0x2, 0x80, 0x80, 0x3,
    0xe5, 0xff, 0xff, 0x2, 0x0, 0x0, 0x3, 0xff, 0xff, 0x3, 0x0, 0x0,
    0x3, 0x1, 0x7, 0x1f, 0x1f, 0x2, 0xf, 0x7, 0x1, 0x0, 0x0, 0x3,
    0x55, 0xff, 0xff, 0x6, 0x0, 0x0, 0x2, 0x80, 0x80, 0x3, 0xc0,
    0xff, 0xff, 0x3, 0xf0, 0x60, 0x60, 0x8, 0xf0, 0xfd, 0xff, 0xff,
    0x3, 0x0, 0x0, 0x2, 0xff, 0xff, 0x2, 0xbb, 0x0, 0x0, 0x7, 0xf7,
    0xff, 0xff, 0x6, 0x0, 0x0, 0x2, 0xff, 0xff, 0x3, 0xf0, 0xf0,
    0x2, 0xe0, 0xe0, 0x3, 0x60, 0x60, 0x4, 0x70, 0xf0, 0xff, 0xff,
    0x3, 0xf8, 0xe0, 0xe0, 0x3, 0x0, 0x0, 0xb, 0xf0, 0xf8, 0xf8,
    0x5, 0xe0, 0x0, 0x0, 0x7, 0xff, 0xff, 0x4, 0x0, 0x0, 0x2, 0xff,
    0xff, 0x6, 0x0, 0x0, 0x3, 0xff, 0xff, 0x3, 0x0, 0x0, 0x3, 0x40,
    0xf8, 0xf8, 0x5, 0xf0, 0x0, 0x0, 0x3, 0xb5, 0xff, 0xff, 0x6,
    0x0, 0x0, 0x2, 0xff, 0xff, 0x7, 0x0, 0x0, 0xa, 0x77, 0xff, 0xff,
    0x3, 0x0, 0x0, 0x2, 0x7, 0x1f, 0x3f, 0xf8, 0xe0, 0xc0, 0x0, 0x0,
    0x4, 0xf7, 0xff, 0xff, 0x4, 0x7f, 0x3f, 0x0, 0x0, 0x2, 0xff,
    0xff, 0x7, 0x0, 0x0, 0x6, 0x1, 0xff, 0xff, 0x7, 0x0, 0x0, 0xb,
    0x7, 0xf, 0xf, 0x6, 0xe, 0xe, 0x7, 0xf, 0xf, 0x3, 0x7, 0x0, 0x0,
    0x2, 0xf, 0xf, 0x6, 0x0, 0x0, 0x3, 0xf, 0xf, 0x2, 0x7, 0x0, 0x0,
    0x3, 0x3, 0xf, 0xf, 0x6, 0x0, 0x0, 0x3, 0x2, 0xf, 0xf, 0x6, 0x0,
    0x0, 0x2, 0x7, 0xf, 0xf, 0x5, 0x7, 0x0, 0x0, 0xa, 0x3, 0xf, 0xf,
    0x3, 0x0, 0x0, 0x6, 0x3, 0x7, 0xf, 0xe, 0xe, 0x2, 0xf, 0x7, 0x3,
    0x3, 0x2, 0x1, 0x0, 0x0, 0x5, 0x7, 0xf, 0xf, 0x5, 0x7, 0x0, 0x0,
    0x7, 0x7, 0xf, 0xf, 0x5, 0x7, 0x0, 0x0, 0xff, 0x0, 0x0, 0x6
  };


