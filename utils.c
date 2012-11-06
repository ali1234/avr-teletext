/* 
    Copyright 2010 Alistair Buxton <a.j.buxton@gmail.com>

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
#include <avr/pgmspace.h>
#include <util/delay_basic.h>
#include "globals.h"

uint8_t parity(uint8_t c) {
    uint8_t p;
    p = c ^ (c>>4);
    p ^= p >> 2;
    p ^= p >> 1;
    p ^= 1;
    return (c | (p << 7));
}

uint8_t hex(uint8_t n) {
    if (n <= 9) return '0'+n;
    else return 'A'+n-10;
}

uint8_t hamm84table[16] = {
    0x15, 0x02, 0x49, 0x5e,
    0x64, 0x73, 0x38, 0x2f,
    0xd0, 0xc7, 0x8c, 0x9b,
    0xa1, 0xb6, 0xfd, 0xea
};

//#define hamm84(n) pgm_read_byte(&(hamm84table[(n)&0xf]))

#define hamm84(n) hamm84table[(n)&0xf]

void fill_mrag(uint8_t magazine, uint8_t row, uint8_t *dest) {
    dest[0] = hamm84(magazine|((row&1)<<3));
    dest[1] = hamm84(row>>1);
}

inline void delay_ms(uint8_t ms) {
    while(ms>0) {
        _delay_loop_2(3468);
        ms--;
    }
}


inline void delay_s(uint8_t s) {
    while(s>0) {
        delay_ms(250);
        delay_ms(250);
        delay_ms(250);
        delay_ms(250);
        s--;
    }
}

