/* 
    Copyright 2012 Alistair Buxton <a.j.buxton@gmail.com>

    This file is part of avr-teletext.

    avr-teletext is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    avr-teletext is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with avr-teletext.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "globals.h"
#include "utils.h"

extern volatile uint8_t tt_buffer[NBUFFERS][42];
extern volatile uint8_t buffer_head;
extern volatile uint8_t buffer_tail;

uint8_t column;

void passthrough_setup() {
    buffer_head = 0;
    buffer_tail = 0;
    column = 0;
}

void passthrough_start() {
    column = 0;
}

void passthrough_putchar(uint8_t c) {
    tt_buffer[buffer_head][column] = c;
    column++;
    if(column == 42) {
        uint8_t tmp = (buffer_head+1)%NBUFFERS;
        column = 0;
        while(tmp == buffer_tail) { ; }
        buffer_head = tmp;
    }
}
